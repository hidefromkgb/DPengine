#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include <gdk/x11/gdkglx.h>
#include <X11/extensions/Xrender.h>

#include <core.h>
#include <ogl/oglstd.h>



void RestartEngine(ENGD *engd, ulong rscm) {
    engd->draw = rscm;
    gtk_main_quit();
}



void ShowMainWindow(ENGD *engd, ulong show) {
    if (show)
        gtk_widget_show((GtkWidget*)engd->user[0]);
    else
        gtk_widget_hide((GtkWidget*)engd->user[0]);
}



inline MENU *OSSpecificMenu(ENGD *engd) {
    return NULL;
}



void GTKMenuHandler(GtkWidget *item, gpointer user) {
    ProcessMenuItem((MENU*)user);
}



void GTKMenuDestroy(GtkWidget *item, gpointer user) {
    GtkWidget *chld = (user)? (GtkWidget*)user
                            :  gtk_menu_item_get_submenu((GtkMenuItem*)item);
    if (chld) {
        gtk_container_foreach((GtkContainer*)chld, GTKMenuDestroy, NULL);
        gtk_widget_destroy(chld);
    }
    if (!user)
        gtk_widget_destroy(item);
}



GtkWidget *Submenu(MENU *menu, ulong chld) {
    if (!menu)
        return NULL;

    GtkWidget *item;
    GtkMenu *retn;
    ulong indx;

    retn = (GtkMenu*)gtk_menu_new();
    if (!chld)
        g_signal_connect(G_OBJECT(retn), "selection-done",
                         G_CALLBACK(GTKMenuDestroy), retn);
    indx = 0;
    while (menu->text) {
        if (!*menu->text)
            item = gtk_separator_menu_item_new();
        else {
            if (menu->flgs & MFL_CCHK) {
                item =
                    gtk_check_menu_item_new_with_mnemonic((char*)menu->text);
                gtk_check_menu_item_set_active((GtkCheckMenuItem*)item,
                                                menu->flgs & MFL_VCHK);
                gtk_check_menu_item_set_draw_as_radio((GtkCheckMenuItem*)item,
                                                       menu->flgs &  MFL_RCHK
                                                                  & ~MFL_CCHK);
            }
            else
                item = gtk_menu_item_new_with_mnemonic((char*)menu->text);
            gtk_widget_set_sensitive(item, !(menu->flgs & MFL_GRAY));
            g_signal_connect(G_OBJECT(item), "activate",
                             G_CALLBACK(GTKMenuHandler), menu);
            if (menu->chld)
                gtk_menu_item_set_submenu((GtkMenuItem*)item,
                                           Submenu(menu->chld, ~0));
        }
        gtk_menu_attach(retn, item, 0, 1, indx, indx + 1);
        indx++;
        menu++;
    }
    gtk_widget_show_all((GtkWidget*)retn);
    return (GtkWidget*)retn;
}



void PopupMenu(GtkStatusIcon *icon, guint mbtn, guint32 time, gpointer user) {
    EngineOpenContextMenu(((ENGD*)user)->menu);
}



void EngineOpenContextMenu(MENU *menu) {
    GtkMenu *mwnd = (GtkMenu*)Submenu(menu, 0);

    if (mwnd)
        gtk_menu_popup(mwnd, NULL, NULL, NULL, NULL,
                       0, gtk_get_current_event_time());
}



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        read(file, retn, flen);
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



char *ConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long CountCPUs() {
    return min(sizeof(SEM_TYPE) * 8, max(1, sysconf(_SC_NPROCESSORS_ONLN)));
}



void MakeThread(THRD *thrd) {
    pthread_t pthr;
    pthread_create(&pthr, 0, (void *(*)(void*))ThrdFunc, thrd);
}



void FreeSemaphore(SEMD *retn, long nthr) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    pthread_mutex_init(&retn->cmtx, 0);
    pthread_cond_init(&retn->cvar, 0);
    retn->full = (1 << nthr) - 1;
    retn->list = retn->full & mask;
}



long PickSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &engd->osem : &engd->isem,
         *pick = (open)? &engd->isem : &engd->osem;

    open = (__sync_fetch_and_and(&drop->list, ~(drop->full & mask)) & mask)?
            TRUE : FALSE;
    __sync_or_and_fetch(&pick->list, pick->full & mask);

    pthread_mutex_lock(&pick->cmtx);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return open;
}



SEM_TYPE WaitSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &engd->osem : &engd->isem;

    pthread_mutex_lock(&wait->cmtx);
    if (mask != SEM_NULL)
        while ((wait->list ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



GdkGLDrawable *gtk_widget_gl_begin(GtkWidget *gwnd) {
    GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(gwnd);
    if (!gdk_gl_drawable_gl_begin(pGLD, gtk_widget_get_gl_context(gwnd)))
        pGLD = 0;
    return pGLD;
}



uint64_t TimeFunc() {
    struct timespec spec = {};

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}



gboolean TimeFuncWrapper(gpointer user) {
    *(uint64_t*)user = TimeFunc();
    return TRUE;
}



gboolean FPSFunc(gpointer user) {
    ENGD *engd = user;
    char fout[64];

    OutputFPS(engd, fout);
    gtk_window_set_title(GTK_WINDOW(engd->user[0]), fout);
    printf("%s\n", fout);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    ENGD *engd = user;
    GdkRectangle rect = {};
    GdkModifierType gmod;
    GdkRegion *creg;
    GdkWindow *hwnd;
    gint xptr, yptr;
    long pick, flgs;

    if (engd->time - engd->tfrm < engd->msec) {
        usleep(1000);
        return TRUE;
    }
    engd->tfrm = engd->time;
    if (!engd->draw
    || !(hwnd = gtk_widget_get_window((GtkWidget*)engd->user[0])))
        return TRUE;

    gdk_window_get_pointer(hwnd, &xptr, &yptr, &gmod);
    flgs = ((gmod & GDK_BUTTON1_MASK)? UFR_LBTN : 0)
         | ((gmod & GDK_BUTTON2_MASK)? UFR_MBTN : 0)
         | ((gmod & GDK_BUTTON3_MASK)? UFR_RBTN : 0)
         | ((gtk_window_is_active((GtkWindow*)engd->user[0]))? UFR_MOUS : 0);
    pick = SelectUnit(engd->uarr, engd->data, engd->size, xptr, yptr);
    engd->size = engd->ufrm((uintptr_t)engd, engd->udat, engd->data,
                            &engd->time, flgs, xptr, yptr, pick);
    if (!engd->size) {
        RestartEngine(engd, SCM_QUIT);
        return TRUE;
    }
    if ((pick >= 0) || (engd->flgs & COM_IOPQ)) {
        rect.width  = engd->pict.xdim;
        rect.height = engd->pict.ydim;
    }
    creg = gdk_region_rectangle(&rect);
    gdk_window_input_shape_combine_region(hwnd, creg, 0, 0);
    gdk_region_destroy(creg);
    switch (engd->rscm) {
        case SCM_RSTD: {
            cairo_t *temp = cairo_create((cairo_surface_t*)engd->user[1]);
            cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(temp, 0, 0, 0, 0);
            cairo_paint(temp);
            cairo_destroy(temp);
            PickSemaphore(engd, 1, SEM_FULL);
            WaitSemaphore(engd, 1, SEM_FULL);
            break;
        }
        case SCM_ROGL: {
            GdkGLDrawable *pGLD =
                gtk_widget_gl_begin((GtkWidget*)engd->user[0]);
            DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                            engd->size, engd->flgs & COM_IOPQ);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
    gdk_window_invalidate_rect(hwnd, 0, FALSE);
    gdk_window_process_updates(hwnd, FALSE);
    engd->fram++;
    return TRUE;
}



gboolean OnDestroy(GtkWidget *gwnd, gpointer user) {
    RestartEngine((ENGD*)user, SCM_QUIT);
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
        printf(TXL_FAIL" Transparent windows not supported! Exiting...\n");
        exit(-1);
    }
    return TRUE;
}



gboolean OnRedraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
    ENGD *engd = user;

    if (!engd->draw)
        return TRUE;

    switch (engd->rscm) {
        case SCM_RSTD: {
            cairo_t *temp = gdk_cairo_create(gtk_widget_get_window(gwnd));
            cairo_surface_mark_dirty((cairo_surface_t*)engd->user[1]);
            cairo_set_operator(temp, CAIRO_OPERATOR_SOURCE);
            if (engd->flgs & COM_IOPQ) {
                cairo_set_source_rgba(temp, 0, 0, 0, 1);
                cairo_paint(temp);
                cairo_set_operator(temp, CAIRO_OPERATOR_OVER);
            }
            cairo_set_source_surface(temp,
                                    (cairo_surface_t*)engd->user[1], 0, 0);
            cairo_paint(temp);
            cairo_destroy(temp);
            break;
        }
        case SCM_ROGL: {
            GdkGLDrawable *pGLD = gtk_widget_gl_begin(gwnd);
            gdk_gl_drawable_swap_buffers(pGLD);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
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



void InitRenderer(ENGD *engd) {
    switch (engd->rscm) {
        case SCM_RSTD:
            SwitchThreads(engd, 1);
            break;

        case SCM_ROGL: {
            GdkGLDrawable *pGLD =
                gtk_widget_gl_begin((GtkWidget*)engd->user[0]);
            engd->rndr = MakeRendererOGL(engd->uarr, engd->uniq,
                                         engd->size, 0);
            SizeRendererOGL(engd->rndr, engd->pict.xdim, engd->pict.ydim);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
}



void RunMainLoop(ENGD *engd) {
    INCBIN("../core/icon.gif", MainIcon);

    guint tmrf, tmrt, tmrd;
    GdkGLDrawable *pGLD;
    GtkWidget *gwnd;

    gtk_init(0, 0);
    gtk_gl_init(0, 0);
    engd->user[0] = (uintptr_t)(gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL));
    OnChange(gwnd, 0, 0);

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
    switch (engd->rscm) {
        case SCM_RSTD:
            engd->user[1] = (uintptr_t)
                cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                           engd->pict.xdim, engd->pict.ydim);
            engd->pict.bptr = (BGRA*)
                cairo_image_surface_get_data((cairo_surface_t*)engd->user[1]);
            break;

        case SCM_ROGL:
            gtk_widget_set_gl_capability(gwnd, GetGDKGL(gwnd), 0,
                                         TRUE, GDK_GL_RGBA_TYPE);
            gtk_widget_realize(gwnd);

            if (!(pGLD = gtk_widget_gl_begin(gwnd))
            ||  !LoadOpenGLFunctions()) {
                printf(TXL_FAIL" %s\n", engd->tran[TXT_NOGL]);
                gtk_widget_destroy(gwnd);
                RestartEngine(engd, SCM_RSTD);
                return;
            }
            gdk_gl_drawable_gl_end(pGLD);
            break;
    }
    InitRenderer(engd);

    gtk_widget_set_app_paintable(gwnd, TRUE);
    gtk_widget_set_size_request(gwnd, engd->pict.xdim, engd->pict.ydim);
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

    long xdim = 128,
         ydim = 128;
    GdkPixbuf *pbuf;
    GtkStatusIcon *icon;

    /// the size is wrong, but let it be: MainIcon does have a GIF ending
    ASTD *igif = MakeDataAnimStd(MainIcon, 1024 * 1024);
    BGRA *bptr = ExtractRescaleSwizzleAlign(igif, 0xC6, 0, xdim, ydim);

    pbuf = gdk_pixbuf_new_from_data((guchar*)bptr, GDK_COLORSPACE_RGB, TRUE,
                                    8, xdim, ydim, xdim * sizeof(*bptr),
                                    (GdkPixbufDestroyNotify)free, bptr);
    FreeAnimStd(&igif);

    icon = gtk_status_icon_new_from_pixbuf(pbuf);
    gtk_status_icon_set_tooltip_text(icon, (gchar*)engd->tran[TXT_HEAD]);
    gtk_status_icon_set_visible(icon, TRUE);
    g_signal_connect(G_OBJECT(icon), "popup-menu",
                     G_CALLBACK(PopupMenu), engd);

    tmrt = g_timeout_add(   1, TimeFuncWrapper, &engd->time);
    tmrf = g_timeout_add(1000, FPSFunc, engd);
    tmrd = g_idle_add(DrawFunc, engd);

    gtk_main();

    switch (engd->rscm) {
        case SCM_RSTD:
            StopThreads(engd);
            cairo_surface_destroy((cairo_surface_t*)engd->user[1]);
            break;

        case SCM_ROGL: {
            pGLD = gtk_widget_gl_begin(gwnd);
            FreeRendererOGL(engd->rndr);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
    g_source_remove(tmrd);
    g_source_remove(tmrf);
    g_source_remove(tmrt);
    gtk_widget_destroy(gwnd);
    g_object_unref(G_OBJECT(icon));
    g_object_unref(G_OBJECT(pbuf));
}
