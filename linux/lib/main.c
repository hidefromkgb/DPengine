#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <gdk/x11/gdkglx.h>

#include "../../core/core.h"
#include "../../core/ogl/core.h"



UFRM UpdateFrame;

cairo_surface_t *draw;
GtkWidget *gwnd;
ulong fram;



char *LoadFile(void *name) {
    struct {
        char *name;
        long size;
    } *data = name;
    char *retn = 0;
    long file, flen;

    if ((file = open(data->name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        read(file, retn, flen);
        close(file);
        data->size = flen;
    }
    return retn;
}



void MakeThread(THRD *thrd) {
    pthread_t pthr;
    pthread_create(&pthr, 0, ThrdFunc, thrd);
}



void FreeSemaphore(SEMD *retn, long nthr) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    pthread_cond_init(&retn->cvar, 0);
    pthread_mutex_init(&retn->cmtx, 0);
    pthread_mutex_lock(&retn->cmtx);
    retn->full = (1 << nthr) - 1;
    retn->list = retn->full & mask;
    pthread_mutex_unlock(&retn->cmtx);
}



long PickSemaphore(TMRD *tmrd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &tmrd->osem : &tmrd->isem,
         *pick = (open)? &tmrd->isem : &tmrd->osem;
    long retn;

    pthread_mutex_lock(&drop->cmtx);
    retn = (drop->list & mask)? TRUE : FALSE;
    drop->list &= ~(drop->full & mask);
    pthread_mutex_unlock(&drop->cmtx);

    pthread_mutex_lock(&pick->cmtx);
    pick->list |= pick->full & mask;
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);

    return retn;
}



SEM_TYPE WaitSemaphore(TMRD *tmrd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &tmrd->osem : &tmrd->isem;
    SEM_TYPE retn;

    pthread_mutex_lock(&wait->cmtx);
    if (mask != SEM_NULL)
        while ((wait->list & mask) != (wait->full & mask))
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    retn = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return retn;
}



GdkGLDrawable *gtk_widget_gl_begin(GtkWidget *gwnd) {
    GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(gwnd);
    if (!gdk_gl_drawable_gl_begin(pGLD, gtk_widget_get_gl_context(gwnd)))
        pGLD = 0;
    return pGLD;
}



uint64_t TimeFunc() {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}



gboolean TimeFuncWrapper(gpointer user) {
    *(uint64_t*)user = TimeFunc();
    return TRUE;
}



gboolean FPSFunc(gpointer user) {
    char fsec[64];

    sprintf(fsec, TXT_FFPS, fram);
    fram = 0;
    printf("%s\n", fsec);
    gtk_window_set_title(GTK_WINDOW(gwnd), fsec);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    TMRD *tmrd = user;
    GdkRectangle rect = {};
    GdkModifierType gmod;
    GdkRegion *creg;
    GdkWindow *hwnd;
    gint xptr, yptr;
    long pick, flgs;

    if (!tmrd->draw || !(hwnd = gtk_widget_get_window(gwnd)))
        return TRUE;
    gdk_window_get_pointer(hwnd, &xptr, &yptr, &gmod);

    flgs = (gmod & GDK_BUTTON1_MASK)? 1 : 0
         | (gmod & GDK_BUTTON2_MASK)? 2 : 0
         | (gmod & GDK_BUTTON3_MASK)? 4 : 0;
    pick = SelectUnit(tmrd->uarr, tmrd->data, tmrd->size, xptr, yptr);
    tmrd->size = UpdateFrame(tmrd->data, &tmrd->time, flgs, xptr, yptr, pick);
    if (pick >= 0) {
        rect.width  = tmrd->pict.xdim;
        rect.height = tmrd->pict.ydim;
    }
    creg = gdk_region_rectangle(&rect);
    gdk_window_input_shape_combine_region(hwnd, creg, 0, 0);
    gdk_region_destroy(creg);
    switch (tmrd->rndr) {
        case BRT_RSTD: {
            cairo_t *temp = cairo_create(draw);
            cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(temp, 0, 0, 0, 0);
            cairo_paint(temp);
            cairo_destroy(temp);
            PickSemaphore(tmrd, 1, SEM_FULL);
            WaitSemaphore(tmrd, 1, SEM_FULL);
            break;
        }
        case BRT_ROGL: {
            GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
            DrawRendererOGL(tmrd->data, tmrd->size);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
    gdk_window_invalidate_rect(hwnd, 0, FALSE);
    gdk_window_process_updates(hwnd, FALSE);
    fram++;
    return TRUE;
}



gboolean OnDestroy(GtkWidget *gwnd, gpointer user) {
    gtk_main_quit();
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
    else {
        printf("Transparent windows not supported! Emergency exit...\n");
        exit(0);
    }
    return TRUE;
}



gboolean OnRedraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
    TMRD *tmrd = user;

    if (!tmrd->draw)
        return TRUE;

    switch (tmrd->rndr) {
        case BRT_RSTD: {
            cairo_t *temp = gdk_cairo_create(gtk_widget_get_window(gwnd));
            cairo_surface_mark_dirty(draw);
            cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(temp, draw, 0, 0);
            cairo_paint(temp);
            cairo_destroy(temp);
            break;
        }
        case BRT_ROGL: {
            GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
            gdk_gl_drawable_swap_buffers(pGLD);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
    return TRUE;
}



GdkGLConfig *GetGDKGL(GdkScreen *scrn) {
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
                    if (pfmt && pfmt->direct.alphaMask > 0)
                        pGGL =
                        gdk_x11_gl_config_new_from_visualid(pvis->visualid);
                    XFree(pvis);
                }
            }
            XFree(pFBC);
        }
    }
    if (!pGGL)
        pGGL = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA
                                       | GDK_GL_MODE_ALPHA
                                       | GDK_GL_MODE_DEPTH
                                       | GDK_GL_MODE_DOUBLE);
    return pGGL;
}



void InitRenderer(TMRD *tmrd) {
    switch (tmrd->rndr) {
        case BRT_RSTD:
            SwitchThreads(tmrd, 1);
            break;

        case BRT_ROGL: {
            GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
            MakeRendererOGL(tmrd->uarr, tmrd->uniq, tmrd->size, 0);
            SizeRendererOGL(tmrd->pict.xdim, tmrd->pict.ydim);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
}



long EngineInitialize(uint32_t rndr,
                      uint32_t *xdim, uint32_t *ydim, uint32_t flgs) {
    GdkGLDrawable *pGLD;
    GdkScreen *gscr;
    long iter;

    draw = 0;
    tmrd.flgs = flgs;
    tmrd.rndr = rndr;
    tmrd.draw = FALSE;

    gtk_init(0, 0);
    gtk_gl_init(0, 0);
    gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gscr = gtk_window_get_screen(gwnd);
    OnChange(gwnd, 0, 0);

    tmrd.pict.xdim = *xdim = (*xdim)? *xdim : gdk_screen_get_width(gscr);
    tmrd.pict.ydim = *ydim = (*ydim)? *ydim : gdk_screen_get_height(gscr);

    gtk_window_set_default_size(GTK_WINDOW(gwnd),
                                tmrd.pict.xdim, tmrd.pict.ydim);

    gtk_widget_set_app_paintable(gwnd, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(gwnd), FALSE);
    gtk_window_set_position(GTK_WINDOW(gwnd), GTK_WIN_POS_CENTER);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(gwnd), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(gwnd), TRUE);
    gtk_window_stick(GTK_WINDOW(gwnd));

    g_signal_connect(G_OBJECT(gwnd), "expose-event",
                     G_CALLBACK(OnRedraw), &tmrd);
    g_signal_connect(G_OBJECT(gwnd), "delete-event",
                     G_CALLBACK(OnDestroy), 0);
    g_signal_connect(G_OBJECT(gwnd), "screen-changed",
                     G_CALLBACK(OnChange), 0);
    g_signal_connect(G_OBJECT(gwnd), "key-press-event",
                     G_CALLBACK(OnKeyDown), 0);
    gtk_widget_set_events(gwnd, gtk_widget_get_events(gwnd)
                              | GDK_KEY_PRESS_MASK);
    switch (tmrd.rndr) {
        case BRT_RSTD:
            draw = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                              tmrd.pict.xdim, tmrd.pict.ydim);
            tmrd.pict.bptr = cairo_image_surface_get_data(draw);
            break;

        case BRT_ROGL:
            gtk_widget_unrealize(gwnd);
            gtk_widget_set_gl_capability(gwnd, GetGDKGL(gscr),
                                         0, TRUE, GDK_GL_RGBA_TYPE);
            gtk_widget_realize(gwnd);

            if (!(pGLD = gtk_widget_gl_begin(gwnd)) || !InitRendererOGL()) {
                printf("Unsupported OpenGL version! Emergency exit...\n");
                gtk_widget_destroy(gwnd);
                gtk_main_quit();
                return 0;
            }
            gdk_gl_drawable_gl_end(pGLD);
            break;
    }
    tmrd.ncpu = min(sizeof(SEM_TYPE) * 8,
                    max(1, sysconf(_SC_NPROCESSORS_ONLN)));
    tmrd.thrd = malloc(tmrd.ncpu * sizeof(*tmrd.thrd));
    MakeSemaphore(&tmrd.isem, tmrd.ncpu, SEM_NULL);
    MakeSemaphore(&tmrd.osem, tmrd.ncpu, SEM_FULL);
    SwitchThreads(&tmrd, 0);
    tmrd.time = TimeFunc();
    return ~0;
}



void EngineRunMainLoop(UFRM func, uint32_t msec, uint32_t size) {
    if (tmrd.uarr) {
        UpdateFrame = func;
        tmrd.data = calloc(((size >> 12) + 2) * 4096, sizeof(*tmrd.data));
        tmrd.size = size;
        InitRenderer(&tmrd);

        gtk_widget_show(gwnd);
        gdk_window_set_cursor(gtk_widget_get_window(gwnd),
                              gdk_cursor_new(GDK_HAND1));

        g_timeout_add(   1, TimeFuncWrapper, &tmrd.time);
        g_timeout_add(msec, DrawFunc, &tmrd);
        g_timeout_add(1000, FPSFunc,  0);

        gtk_main();

        switch (tmrd.rndr) {
            case BRT_RSTD:
                StopThreads(&tmrd);
                break;

            case BRT_ROGL: {
                GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
                FreeRendererOGL();
                gdk_gl_drawable_gl_end(pGLD);
                break;
            }
        }
        FreeUnitArray(&tmrd.uarr);
        FreeHashTrees();
        free(tmrd.data);
    }
    else
        printf("No animation base found! Exiting...\n");

    if (draw)
        cairo_surface_destroy(draw);
    FreeSemaphore(&tmrd.isem, tmrd.ncpu);
    FreeSemaphore(&tmrd.osem, tmrd.ncpu);
    free(tmrd.thrd);
}
