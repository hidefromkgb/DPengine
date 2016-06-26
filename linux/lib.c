#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/x11/gdkglx.h>
#include <X11/extensions/Xrender.h>

#include <gdk/gdkkeysyms.h>

#include <core.h>

struct SEMD {
    pthread_mutex_t cmtx;
    pthread_cond_t cvar;
    SEM_TYPE list, full;
};



GdkGLDrawable *gtk_widget_gl_begin(GtkWidget *gwnd) {
    GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(gwnd);
    if (!gdk_gl_drawable_gl_begin(pGLD, gtk_widget_get_gl_context(gwnd)))
        pGLD = 0;
    return pGLD;
}



gboolean FPSFunc(gpointer user) {
    ENGD *engd = user;
    intptr_t *data;
    char fout[64];

    cOutputFPS(engd, fout);
    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    gtk_window_set_title(GTK_WINDOW(data[0]), fout);
    printf("%s\n", fout);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    ENGD *engd = user;
    GdkGLDrawable *pGLD = 0;
    GdkRectangle rect = {};
    GdkModifierType gmod;
    gint xptr, yptr;
    GdkRegion *creg;
    GdkWindow *hwnd;
    intptr_t *data;
    uint32_t attr;

    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    if (!(hwnd = gtk_widget_get_window(GTK_WIDGET(data[0]))))
        return TRUE;

    gdk_window_get_pointer(hwnd, &xptr, &yptr, &gmod);
    attr = ((gmod & GDK_BUTTON1_MASK)? UFR_LBTN : 0)
         | ((gmod & GDK_BUTTON2_MASK)? UFR_MBTN : 0)
         | ((gmod & GDK_BUTTON3_MASK)? UFR_RBTN : 0)
         | ((gtk_window_is_active(GTK_WINDOW(data[0])))? UFR_MOUS : 0);
    attr = cPrepareFrame(engd, xptr, yptr, attr);
    if (attr & PFR_SKIP)
        usleep(1000);
    if (attr & PFR_HALT)
        return TRUE;
    if (attr & PFR_PICK) {
        rect.width  = (int16_t)(data[2]);
        rect.height = (int16_t)(data[2] >> 16);
    }
    creg = gdk_region_rectangle(&rect);
    gdk_window_input_shape_combine_region(hwnd, creg, 0, 0);
    gdk_region_destroy(creg);

    cEngineCallback(engd, ECB_GFLG, (intptr_t)&attr);
    if (attr & COM_RGPU)
        pGLD = gtk_widget_gl_begin(GTK_WIDGET(data[0]));
    else {
        /// comment the lines below to enable manual zeroing
        //*
        cairo_t *temp = cairo_create((cairo_surface_t*)data[1]);
        cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(temp, 0, 0, 0, 0);
        cairo_paint(temp);
        cairo_destroy(temp);
        //*/
    }
    cOutputFrame(engd, 0);
    if (attr & COM_RGPU)
        gdk_gl_drawable_gl_end(pGLD);

    gdk_window_invalidate_rect(hwnd, 0, FALSE);
    gdk_window_process_updates(hwnd, FALSE);

    return TRUE;
}



gboolean OnDestroy(GtkWidget *gwnd, gpointer user) {
    cEngineCallback((ENGD*)user, ECB_QUIT, ~0);
    return TRUE;
}



gboolean OnKeyDown(GtkWidget *gwnd, GdkEventKey *ekey, gpointer user) {
    if (ekey->keyval == GDK_KEY_Escape)
        OnDestroy(gwnd, user);
    return TRUE;
}



gboolean OnChange(GtkWidget *gwnd, GdkScreen *scrn, gpointer user) {
    GdkColormap *cmap;

    if ((cmap = gdk_screen_get_rgba_colormap(gtk_widget_get_screen(gwnd))))
        gtk_widget_set_colormap(gwnd, cmap);
    else
        cEngineCallback((ENGD*)user, ECB_QUIT, 0);
    return TRUE;
}



gboolean OnRedraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
    ENGD *engd = user;
    intptr_t *data;
    uint32_t flgs;

    cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
    if (~flgs & COM_DRAW)
        return TRUE;

    if (~flgs & COM_RGPU) {
        cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
        cairo_t *temp = gdk_cairo_create(gtk_widget_get_window(gwnd));
        cairo_surface_mark_dirty((cairo_surface_t*)data[1]);
        cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
        if (flgs & COM_OPAQ) {
            cairo_set_source_rgba(temp, 0, 0, 0, 1);
            cairo_paint(temp);
            cairo_set_operator(temp, CAIRO_OPERATOR_OVER);
        }
        cairo_set_source_surface(temp, (cairo_surface_t*)data[1], 0, 0);
        cairo_paint(temp);
        cairo_destroy(temp);
    }
    else {
        GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
        gdk_gl_drawable_swap_buffers(pGLD);
        gdk_gl_drawable_gl_end(pGLD);
    }
    return TRUE;
}



GdkGLConfig *GetGDKGL(GtkWidget *gwnd) {
    GdkScreen *scrn = gtk_window_get_screen(GTK_WINDOW(gwnd));
    GdkGLConfig *pGGL = 0;

    Display *disp = GDK_SCREEN_XDISPLAY(scrn);
    int iter, numc,
        attr[] = {
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE,   GLX_RGBA_BIT,
            GLX_DOUBLEBUFFER,  True,
            GLX_RED_SIZE,      1,
            GLX_GREEN_SIZE,    1,
            GLX_BLUE_SIZE,     1,
            GLX_ALPHA_SIZE,    1,
            None
        };

    if (XRenderQueryExtension(disp, &iter, &iter)) {
        GLXFBConfig *pFBC =
            glXChooseFBConfig(disp, GDK_SCREEN_XNUMBER(scrn), attr, &numc);
        if (pFBC) {
            for (iter = 0; !pGGL && iter < numc; iter++) {
                XVisualInfo *pvis = glXGetVisualFromFBConfig(disp, pFBC[iter]);
                if (pvis) {
                    XRenderPictFormat *pfmt =
                        XRenderFindVisualFormat(disp, pvis->visual);
                    if (pfmt && pfmt->direct.alphaMask > 0) {
                        VisualID viid = pvis->visualid;
                        pGGL = gdk_x11_gl_config_new_from_visualid(viid);
                    }
                    XFree(pvis);
                }
            }
            XFree(pFBC);
        }
    }
    if (!pGGL)
        pGGL = gdk_gl_config_new_by_mode(GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE
                                       | GDK_GL_MODE_ALPHA | GDK_GL_MODE_RGBA);
    return pGGL;
}



uint64_t lTimeFunc() {
    struct timespec spec = {};

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}



void lRestartEngine(ENGD *engd) {
    gtk_main_quit();
}



void lShowMainWindow(ENGD *engd, long show) {
    intptr_t *data;

    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    ((show)? gtk_widget_show : gtk_widget_hide)(GTK_WIDGET(data[0]));
}



char *lLoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        if (read(file, retn, flen) == flen) {
            if (size)
                *size = flen;
        }
        else {
            free(retn);
            retn = 0;
        }
        close(file);
    }
    return retn;
}



long lCountCPUs() {
    return min(sizeof(SEM_TYPE) * CHAR_BIT,
               max(1, sysconf(_SC_NPROCESSORS_ONLN)));
}



void lMakeThread(THRD *thrd) {
    pthread_t pthr;

    pthread_create(&pthr, 0, (void *(*)(void*))cThrdFunc, thrd);
}



void lFreeSemaphore(SEMD **retn, long nthr) {
    if (retn && *retn) {
        pthread_cond_destroy(&(*retn)->cvar);
        pthread_mutex_destroy(&(*retn)->cmtx);
        free(*retn);
        *retn = 0;
    }
}



void lMakeSemaphore(SEMD **retn, long nthr, SEM_TYPE mask) {
    if (retn) {
        *retn = malloc(sizeof(**retn));
        pthread_mutex_init(&(*retn)->cmtx, 0);
        pthread_cond_init(&(*retn)->cvar, 0);
        (*retn)->full = (1 << nthr) - 1;
        (*retn)->list = (*retn)->full & mask;
    }
}



long lPickSemaphore(SEMD *drop, SEMD *pick, SEM_TYPE mask) {
    long retn;

    retn = (__sync_fetch_and_and(&drop->list, ~(drop->full & mask)) & mask)?
            TRUE : FALSE;
    __sync_or_and_fetch(&pick->list, pick->full & mask);

    pthread_mutex_lock(&pick->cmtx);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return retn;
}



SEM_TYPE lWaitSemaphore(SEMD *wait, SEM_TYPE mask) {
    pthread_mutex_lock(&wait->cmtx);
    if (mask)
        while ((wait->list ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



void lRunMainLoop(ENGD *engd, long xpos, long ypos, long xdim, long ydim,
                  BGRA **bptr, intptr_t *data, uint32_t flgs) {
    GdkGLDrawable *pGLD = 0;
    GtkWidget *gwnd;
    guint tmrf, tmrd;

    xdim -= xpos;
    ydim -= ypos;
    gtk_init(0, 0);
    gtk_gl_init(0, 0);
    data[0] = (intptr_t)(gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL));
    data[2] = (int16_t)xdim | (int32_t)(ydim << 16);
    OnChange(gwnd, 0, (gpointer)engd);

    g_signal_connect(G_OBJECT(gwnd), "expose-event",
                     G_CALLBACK(OnRedraw), engd);
    g_signal_connect(G_OBJECT(gwnd), "delete-event",
                     G_CALLBACK(OnDestroy), engd);
    g_signal_connect(G_OBJECT(gwnd), "screen-changed",
                     G_CALLBACK(OnChange), engd);
    g_signal_connect(G_OBJECT(gwnd), "key-press-event",
                     G_CALLBACK(OnKeyDown), engd);
    gtk_widget_set_events(gwnd, gtk_widget_get_events(gwnd) |
                                GDK_KEY_PRESS_MASK);
    if (~flgs & COM_RGPU) {
        data[1] = (intptr_t)
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, xdim, ydim);
        *bptr = (BGRA*)cairo_image_surface_get_data((cairo_surface_t*)data[1]);
    }
    else {
        gtk_widget_set_gl_capability(gwnd, GetGDKGL(gwnd), 0,
                                     TRUE, GDK_GL_RGBA_TYPE);
        gtk_widget_realize(gwnd);
    }
    gtk_widget_set_app_paintable(gwnd, TRUE);
    gtk_widget_set_size_request(gwnd, xdim, ydim);
    gtk_window_set_type_hint(GTK_WINDOW(gwnd), GDK_WINDOW_TYPE_HINT_TOOLBAR);
    gtk_window_set_position(GTK_WINDOW(gwnd), GTK_WIN_POS_CENTER);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gwnd), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(gwnd), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(gwnd), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(gwnd), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(gwnd), TRUE);
    gtk_window_stick(GTK_WINDOW(gwnd));
    gtk_widget_show(gwnd);
    gdk_window_set_cursor(gtk_widget_get_window(gwnd),
                          gdk_cursor_new(GDK_HAND1));
    lShowMainWindow(engd, flgs & COM_SHOW);

    tmrf = g_timeout_add(1000, FPSFunc, engd);
    tmrd = g_idle_add(DrawFunc, engd);

    gtk_main();

    if (flgs & COM_RGPU)
        pGLD = gtk_widget_gl_begin(gwnd);
    cDeallocFrame(engd, 0);
    if (flgs & COM_RGPU)
        gdk_gl_drawable_gl_end(pGLD);
    else
        cairo_surface_destroy((cairo_surface_t*)data[1]);

    g_source_remove(tmrd);
    g_source_remove(tmrf);
    gtk_widget_destroy(gwnd);
}
