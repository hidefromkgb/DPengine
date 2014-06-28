#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <gdk/x11/gdkglx.h>

#include "../../core/core.h"
#include "../../core/ogl/core.h"



#define SEM_NULL 0
#define SEM_FULL ~SEM_NULL
#define SEM_TYPE uint64_t



/// semaphore data
typedef struct _SEMD {
    pthread_mutex_t cmtx;
    pthread_cond_t cvar;
    SEM_TYPE list, full;
} SEMD;

/// thread data
typedef struct _THRD {
    SEM_TYPE uuid;
    SEMD *isem, *osem;
    ulong loop;
    void (*func)(struct _THRD*);
    union {
        struct _TMRD *orig;
        TREE *elem;
    };
    union {
        ulong ymin;
        char *path;
    };
    ulong ymax;
} THRD;

/// timer data
typedef struct _TMRD {
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    SEMD isem, osem;

    uint64_t time;
    ulong ncpu, fram, rndr, uniq, size;
    PICT  pict;
    UNIT *uarr;
    T2UV *data;
    THRD *thrd;
} TMRD;



UFRM UpdateFrame;
TMRD tmrd;



void FreeSemaphore(SEMD *retn) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long lstl, SEM_TYPE mask) {
    pthread_cond_init(&retn->cvar, 0);
    pthread_mutex_init(&retn->cmtx, 0);
    pthread_mutex_lock(&retn->cmtx);
    retn->full = (1 << lstl) - 1;
    retn->list = retn->full & mask;
    pthread_mutex_unlock(&retn->cmtx);
}



long PickSemaphore(SEMD *drop, SEMD *pick, SEM_TYPE mask) {
    long retn;
    pthread_mutex_lock(&drop->cmtx);
    retn = (drop->list & mask)? TRUE : FALSE;
    drop->list &= ~(drop->full & mask);
    pthread_mutex_unlock(&drop->cmtx);

    pthread_mutex_lock(&pick->cmtx);
    pick->list |=  (pick->full & mask);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return retn;
}



SEM_TYPE WaitSemaphore(SEMD *wait, SEM_TYPE mask) {
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



void ThrdFunc(THRD *data) {
    while (TRUE) {
        WaitSemaphore(data->isem, data->uuid);
        if (!data->loop)
            break;
        data->func(data);
        if (!PickSemaphore(data->isem, data->osem, data->uuid))
            return;
    }
    printf(TXT_EXIT"\n");
    PickSemaphore(data->isem, data->osem, data->uuid);
}



gboolean TimeFunc(gpointer user) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    *(uint64_t*)user = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    return TRUE;
}



gboolean FPSFunc(gpointer user) {
    TMRD *tmrd = user;
    char fsec[24];

    sprintf(fsec, TXT_FFPS, tmrd->fram);
    tmrd->fram = 0;
    printf("%s\n", fsec);
    gtk_window_set_title(GTK_WINDOW(tmrd->gwnd), fsec);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    TMRD *tmrd = user;
    GdkRectangle rect = {};
    GdkModifierType gmod;
    GdkRegion *creg;
    GdkWindow *gwnd;
    gint xptr, yptr;
    cairo_t *surf;
    long pick, flgs;

    if (!(gwnd = gtk_widget_get_window(tmrd->gwnd)))
        return FALSE;
    gdk_window_get_pointer(gwnd, &xptr, &yptr, &gmod);

    flgs = (gmod & GDK_BUTTON1_MASK)? 1 : 0
         | (gmod & GDK_BUTTON2_MASK)? 2 : 0
         | (gmod & GDK_BUTTON3_MASK)? 4 : 0;
    pick = SelectUnit(tmrd->uarr, tmrd->data, tmrd->size, xptr, yptr);
    UpdateFrame(tmrd->data, &tmrd->time, flgs, xptr, yptr, pick);
    if (pick >= 0) {
        rect.width  = tmrd->pict.xdim;
        rect.height = tmrd->pict.ydim;
    }
    creg = gdk_region_rectangle(&rect);
    gdk_window_input_shape_combine_region(gwnd, creg, 0, 0);
    gdk_region_destroy(creg);
    switch (tmrd->rndr) {
        case BRT_RSTD:
            surf = cairo_create(tmrd->surf);
            cairo_set_operator(surf, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_rgba(surf, 0, 0, 0, 0);
            cairo_paint(surf);
            cairo_destroy(surf);
            PickSemaphore(&tmrd->osem, &tmrd->isem, SEM_FULL);
            WaitSemaphore(&tmrd->osem, SEM_FULL);
            break;

        case BRT_ROGL:
            break;
    }
    gdk_window_invalidate_rect(gwnd, 0, FALSE);
    gdk_window_process_updates(gwnd, FALSE);
    tmrd->fram++;
    return TRUE;
}



void ScreenChange(GtkWidget *gwnd, GdkScreen *scrn, gpointer user) {
    GdkColormap *cmap;
    if ((cmap = gdk_screen_get_rgba_colormap(gtk_widget_get_screen(gwnd))))
        gtk_widget_set_colormap(gwnd, cmap);
    else {
        printf("Transparent windows not supported! Emergency exit...\n");
        exit(0);
    }
}



gboolean Redraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
    TMRD *tmrd = user;

    switch (tmrd->rndr) {
        case BRT_RSTD: {
            cairo_t *draw = gdk_cairo_create(gtk_widget_get_window(gwnd));

            cairo_surface_mark_dirty(tmrd->surf);
            cairo_set_operator(draw, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(draw, tmrd->surf, 0, 0);
            cairo_paint(draw);
            cairo_destroy(draw);
            break;
        }
        case BRT_ROGL: {
            GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(gwnd);
            GdkGLContext  *pGLC = gtk_widget_get_gl_context(gwnd);

            if (!gdk_gl_drawable_gl_begin(pGLD, pGLC))
                return FALSE;

            DrawRendererOGL(tmrd->data, tmrd->size);

            gdk_gl_drawable_swap_buffers(pGLD);
            gdk_gl_drawable_gl_end(pGLD);
            break;
        }
    }
    return TRUE;
}



gboolean KeyDown(GtkWidget *gwnd, GdkEventKey *ekey, gpointer user) {
    if (ekey->keyval == GDK_KEY_Escape)
        gtk_main_quit();
    return TRUE;
}



gboolean InitGL(TMRD *tmrd) {
    GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(tmrd->gwnd);

    if (!gdk_gl_drawable_gl_begin(pGLD, gtk_widget_get_gl_context(tmrd->gwnd))
    ||  !InitRendererOGL()) {
        printf("Unsupported OpenGL version! Emergency exit...\n");
        return FALSE;
    }
    gdk_gl_drawable_gl_end(pGLD);
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
        GLXFBConfig *pFBC = glXChooseFBConfig(disp, GDK_SCREEN_XNUMBER(scrn),
                                              attr, &numc);
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



SEM_TYPE FindBit(SEM_TYPE inpt) {
    SEM_TYPE retn;
    retn  = (inpt & 0xFFFFFFFF00000000)? 32 : 0;
    retn += (inpt & 0xFFFF0000FFFF0000)? 16 : 0;
    retn += (inpt & 0xFF00FF00FF00FF00)?  8 : 0;
    retn += (inpt & 0xF0F0F0F0F0F0F0F0)?  4 : 0;
    retn += (inpt & 0xCCCCCCCCCCCCCCCC)?  2 : 0;
    retn += (inpt & 0xAAAAAAAAAAAAAAAA)?  1 : 0;
    return retn;
}



void LTHR(THRD *data) {
    char *path, *apal, *file;
    long  indx;
    TREE *elem;
    ASTD *retn;

    retn = 0;
    path = data->path;
    elem = data->elem;
    if (path) {
        indx = strlen(path);
        if (((path[indx - 3] == 'g') || (path[indx - 3] == 'G'))
        &&  ((path[indx - 2] == 'i') || (path[indx - 2] == 'I'))
        &&  ((path[indx - 1] == 'f') || (path[indx - 1] == 'F'))
        &&  (retn = MakeAnimStd(path))) {
            *elem->epix->xdim = retn->xdim;
            *elem->epix->ydim = retn->ydim;
            *elem->epix->fcnt = retn->fcnt;
            *elem->epix->time = retn->time;
             elem->epix->scal = DownsampleAnimStd(retn, &elem->epix->xoff,
                                                        &elem->epix->yoff);
            apal = strdup(path);
            apal[indx - 3] = 'a';
            apal[indx - 2] = 'r';
            apal[indx - 1] = 't';
            file = LoadFile(apal, &indx);
            RecolorPalette(retn->bpal, file, indx);
            free(apal);
            free(file);
        }
        elem->epix->anim = retn;
    }
}



void PTHR(THRD *data) {
    DrawPixStdThrd(&data->orig->pict, data->orig->uarr, data->orig->data,
                    data->orig->size, data->ymin, data->ymax);
}



TREE *LoadUST(TREE *elem, char *path) {
    SEM_TYPE curr;
    TREE *retn;

    curr = WaitSemaphore(&tmrd.osem, SEM_NULL);
    curr = FindBit(curr & (curr ^ (curr - 1)));
    retn = tmrd.thrd[curr].elem;
    tmrd.thrd[curr].elem = elem;
    tmrd.thrd[curr].path = path;
    PickSemaphore(&tmrd.osem, &tmrd.isem, 1 << curr);
    return retn;
}



LIB_OPEN long EngineInitialize(uint32_t rndr,
                               uint32_t *xdim, uint32_t *ydim, uint32_t flgs) {
    GdkScreen *gscr;
    pthread_t  thrd;
    long iter;

    tmrd.rndr = rndr;
    tmrd.surf = 0;
    LoadUnitStdThrd = LoadUST;

    gtk_init(0, 0);
    gtk_gl_init(0, 0);
    tmrd.gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gscr = gtk_window_get_screen(tmrd.gwnd);
    ScreenChange(tmrd.gwnd, 0, 0);

    tmrd.pict.xdim = *xdim = (*xdim)? *xdim : gdk_screen_get_width(gscr);
    tmrd.pict.ydim = *ydim = (*ydim)? *ydim : gdk_screen_get_height(gscr);

    gtk_window_set_default_size(GTK_WINDOW(tmrd.gwnd),
                                tmrd.pict.xdim, tmrd.pict.ydim);

    gtk_widget_set_app_paintable(tmrd.gwnd, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(tmrd.gwnd), FALSE);
    gtk_window_set_position(GTK_WINDOW(tmrd.gwnd), GTK_WIN_POS_CENTER);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(tmrd.gwnd), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(tmrd.gwnd), TRUE);
    gtk_window_stick(GTK_WINDOW(tmrd.gwnd));

    g_signal_connect(G_OBJECT(tmrd.gwnd), "expose-event",
                     G_CALLBACK(Redraw), &tmrd);
    g_signal_connect(G_OBJECT(tmrd.gwnd), "delete-event",
                     G_CALLBACK(gtk_main_quit), 0);
    g_signal_connect(G_OBJECT(tmrd.gwnd), "screen-changed",
                     G_CALLBACK(ScreenChange), 0);
    g_signal_connect(G_OBJECT(tmrd.gwnd), "key-press-event",
                     G_CALLBACK(KeyDown), 0);
    gtk_widget_set_events(tmrd.gwnd, gtk_widget_get_events(tmrd.gwnd)
                                   | GDK_KEY_PRESS_MASK);
    switch (tmrd.rndr) {
        case BRT_RSTD:
            tmrd.surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                   tmrd.pict.xdim,
                                                   tmrd.pict.ydim);
            tmrd.pict.bptr = cairo_image_surface_get_data(tmrd.surf);
            break;

        case BRT_ROGL:
            gtk_widget_unrealize(tmrd.gwnd);
            gtk_widget_set_gl_capability(tmrd.gwnd, GetGDKGL(gscr),
                                         0, TRUE, GDK_GL_RGBA_TYPE);
            gtk_widget_realize(tmrd.gwnd);
            if (!InitGL(&tmrd)) {
                gtk_widget_destroy(tmrd.gwnd);
                gtk_main_quit();
                return 0;
            }
            break;
    }
    tmrd.ncpu = min(sizeof(SEM_TYPE) * 8,
                    max(1, sysconf(_SC_NPROCESSORS_ONLN)));
    tmrd.thrd = malloc(tmrd.ncpu * sizeof(*tmrd.thrd));
    MakeSemaphore(&tmrd.isem, tmrd.ncpu, SEM_NULL);
    MakeSemaphore(&tmrd.osem, tmrd.ncpu, SEM_FULL);

    for (iter = 0; iter < tmrd.ncpu; iter++) {
        tmrd.thrd[iter] = (THRD){1 << iter, &tmrd.isem, &tmrd.osem,
                                 TRUE, LTHR};
        pthread_create(&thrd, 0, ThrdFunc, &tmrd.thrd[iter]);
    }
    TimeFunc(&tmrd.time);
    return ~0;
}



void EngineLoadAnimAsync(uint8_t *path, uint32_t *uuid,
                         uint32_t *xdim, uint32_t *ydim,
                         uint32_t *fcnt, uint32_t **time) {
    TryLoadUnit(path, uuid, xdim, ydim, fcnt, time);
}



void EngineFinishLoading() {
    long iter;

    WaitSemaphore(&tmrd.osem, SEM_FULL);
    for (iter = 0; iter < tmrd.ncpu; iter++) {
        TryUpdatePixTree(tmrd.thrd[iter].elem);
        tmrd.thrd[iter].loop = FALSE;
    }
    PickSemaphore(&tmrd.osem, &tmrd.isem, SEM_FULL);
    WaitSemaphore(&tmrd.osem, SEM_FULL);
    MakeUnitArray(&tmrd.uarr);
}



void EngineRunMainLoop(UFRM func, uint32_t msec, uint32_t size) {
    pthread_t thrd;
    uint64_t mtmp;
    long iter;

    if (tmrd.uarr) {
        TimeFunc(&mtmp);
        mtmp -= tmrd.time;
        UpdateFrame = func;
        tmrd.uniq = tmrd.uarr[0].scal - 1;
        tmrd.data = calloc(((size >> 12) + 2) * 4096, sizeof(*tmrd.data));
        tmrd.size = size;
        printf(TXT_AEND"\n", tmrd.uniq, mtmp, (float)mtmp / tmrd.uniq,
              (tmrd.rndr == BRT_ROGL)? TXT_ROGL : TXT_RSTD);

        switch (tmrd.rndr) {
            case BRT_RSTD:
                for (iter = 0; iter < tmrd.ncpu; iter++) {
                    mtmp = (tmrd.pict.ydim / tmrd.ncpu) + 1;
                    tmrd.thrd[iter] =
                        (THRD){tmrd.thrd[iter].uuid, tmrd.thrd[iter].isem,
                               tmrd.thrd[iter].osem, TRUE, PTHR, {&tmrd},
                               {mtmp * iter}, mtmp * (iter + 1)};
                    pthread_create(&thrd, 0, ThrdFunc, &tmrd.thrd[iter]);
                }
                tmrd.thrd[tmrd.ncpu - 1].ymax = tmrd.pict.ydim;
                break;

            case BRT_ROGL: {
                GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(tmrd.gwnd);
                GdkGLContext  *pGLC = gtk_widget_get_gl_context(tmrd.gwnd);

                if (gdk_gl_drawable_gl_begin(pGLD, pGLC)) {
                    MakeRendererOGL(tmrd.uarr, tmrd.uniq, tmrd.data, tmrd.size,
                                    FALSE);
                    SizeRendererOGL(tmrd.pict.xdim, tmrd.pict.ydim);
                }
                gdk_gl_drawable_gl_end(pGLD);
                break;
            }
        }
        gtk_widget_show(tmrd.gwnd);
        gdk_window_set_cursor(gtk_widget_get_window(tmrd.gwnd),
                              gdk_cursor_new(GDK_HAND1));

        g_timeout_add(msec, DrawFunc, &tmrd);
        g_timeout_add(1000, FPSFunc,  &tmrd);
        g_timeout_add(   1, TimeFunc, &tmrd.time);
        gtk_main();

        switch (tmrd.rndr) {
            case BRT_RSTD:
                WaitSemaphore(&tmrd.osem, SEM_FULL);
                for (iter = 0; iter < tmrd.ncpu; iter++)
                    tmrd.thrd[iter].loop = FALSE;
                PickSemaphore(&tmrd.osem, &tmrd.isem, SEM_FULL);
                WaitSemaphore(&tmrd.osem, SEM_FULL);
                break;

            case BRT_ROGL: {
                GdkGLDrawable *pGLD = gtk_widget_get_gl_drawable(tmrd.gwnd);
                GdkGLContext  *pGLC = gtk_widget_get_gl_context(tmrd.gwnd);

                if (gdk_gl_drawable_gl_begin(pGLD, pGLC))
                    FreeRendererOGL();

                gdk_gl_drawable_gl_end(pGLD);
                break;
            }
        }
        FreeUnitArray(&tmrd.uarr);
        free(tmrd.data);
    }
    else
        printf("No animation base found! Exiting...\n");

    if (tmrd.surf)
        cairo_surface_destroy(tmrd.surf);
    FreeSemaphore(&tmrd.isem);
    FreeSemaphore(&tmrd.osem);
    free(tmrd.thrd);
}
