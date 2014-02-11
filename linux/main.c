#include <dirent.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../core/core.h"



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
    union {
        FILL fill;
        DRAW draw;
    } fprm;
    long loop;
    SEM_TYPE uuid;
    SEMD *isem, *osem;
    void (*func)(void*);
    pthread_t ithr;
} THRD;

/// timer data
typedef struct _TMRD {
    SEMD isem, osem;
    ulong *time, ncpu, fram;
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    THRD *thrd;
    UNIT *tail;
    PICT *pict;
} TMRD;



GdkDevice *gptr;
UNIT *pick = NULL;
VEC2 cptr = {};



void FreeSemaphore(SEMD *retn) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long lstl, SEM_TYPE mask) {
    pthread_cond_init(&retn->cvar, NULL);
    pthread_mutex_init(&retn->cmtx, NULL);
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
        data->func(&data->fprm);
        if (!PickSemaphore(data->isem, data->osem, data->uuid))
            return;
    }
    printf("Thread exited\n");
    PickSemaphore(data->isem, data->osem, data->uuid);
}



gboolean TimeFunc(gpointer user) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    *(ulong*)user = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    return TRUE;
}



gboolean FPSFunc(gpointer user) {
    TMRD *tmrd = user;
    char fsec[24];

    sprintf(fsec, "%u FPS", tmrd->fram);
    tmrd->fram = 0;
    printf("%s\n", fsec);
    gtk_window_set_title(GTK_WINDOW(tmrd->gwnd), fsec);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    TMRD *tmrd = user;
    GdkModifierType gmod;
    GdkWindow *gwnd;
    cairo_t *surf;
    UNIT *tail;
    long iter;

    if ((gwnd = gtk_widget_get_window(tmrd->gwnd)) &&
       (tail = UpdateFrameStd(&tmrd->tail, &pick, tmrd->time, cptr))) {
        surf = cairo_create(tmrd->surf);
        cairo_set_operator(surf, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(surf, 0, 0, 0, 0);
        cairo_paint(surf);
        cairo_destroy(surf);

        if (pick)
//            gdk_window_get_pointer(gwnd, &cptr.x, &cptr.y, &gmod);
            gdk_window_get_device_position(gwnd, gptr,
                                           &cptr.x, &cptr.y, &gmod);
        for (iter = 0; iter < tmrd->ncpu; iter++)
            tmrd->thrd[iter].fprm.draw.tail = tail;
        PickSemaphore(&tmrd->osem, &tmrd->isem, SEM_FULL);
        WaitSemaphore(&tmrd->osem, SEM_FULL);

        gdk_window_invalidate_rect(gwnd, NULL, FALSE);
        gdk_window_process_updates(gwnd, FALSE);

        tmrd->fram++;
        return TRUE;
    }
    return FALSE;
}



void ScreenChange(GtkWidget *gwnd, GdkScreen *scrn, gpointer user) {
    GdkVisual *gvis;
    if ((gvis = gdk_screen_get_rgba_visual(gtk_widget_get_screen(gwnd))))
        gtk_widget_set_visual(gwnd, gvis);
//    GdkColormap *cmap;
//    if ((cmap = gdk_screen_get_rgba_colormap(gtk_widget_get_screen(gwnd))))
//        gtk_widget_set_colormap(gwnd, cmap);
    else {
        printf("Transparent windows not supported! Emergency exit...\n");
        exit(0);
    }
}



gboolean Redraw(GtkWidget *gwnd, cairo_t *draw, gpointer user) {
//gboolean Redraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
//    cairo_t *draw = gdk_cairo_create(gtk_widget_get_window(gwnd));
    cairo_surface_t *surf = user;

    cairo_surface_mark_dirty(surf);
    cairo_set_operator(draw, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(draw, surf, 0, 0);
    cairo_paint(draw);
//    cairo_destroy(draw);
    return TRUE;
}



gboolean KeyDown(GtkWidget *gwnd, GdkEventKey *ekey, gpointer user) {
    if (ekey->keyval == GDK_KEY_Escape) {
        gtk_widget_destroy(gwnd);
        gtk_main_quit();
    }
    return TRUE;
}



gboolean MouseButton(GtkWidget* gwnd,
                     GdkEventButton *embt, GdkWindowEdge edge) {
    if (embt->button == 1)
        switch (embt->type) {
            case GDK_BUTTON_PRESS:
                gptr = embt->device;
                cptr.x = embt->x;
                cptr.y = embt->y;
                pick = EMP_PICK;
                break;

            case GDK_BUTTON_RELEASE:
                pick = NULL;
                break;
        }
    return TRUE;
}



int main(int argc, char *argv[]) {
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    GdkScreen *gscr;

    ulong msec, mtmp;
    long ncpu, iter, curr;
    struct dirent **dirs;
    char *anim;

    ULIB *ulib;
    UNIT *tail;
    THRD *thrd;
    TMRD tmrd;
    PICT pict;

    anim = "anim";
    gtk_init(&argc, &argv);
    gscr = gtk_window_get_screen(gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL));
    ScreenChange(gwnd, NULL, NULL);

    pict.size.x = gdk_screen_get_width(gscr);
    pict.size.y = gdk_screen_get_height(gscr);
    surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                      pict.size.x, pict.size.y);
    pict.bptr = cairo_image_surface_get_data(surf);

    gtk_widget_set_app_paintable(gwnd, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(gwnd), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(gwnd), pict.size.x, pict.size.y);
    gtk_window_set_position(GTK_WINDOW(gwnd), GTK_WIN_POS_CENTER);

    g_signal_connect(G_OBJECT(gwnd), "draw",
//    g_signal_connect(G_OBJECT(gwnd), "expose-event",
                     G_CALLBACK(Redraw), surf);
    g_signal_connect(G_OBJECT(gwnd), "delete-event",
                     G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(gwnd), "screen-changed",
                     G_CALLBACK(ScreenChange), NULL);
    g_signal_connect(G_OBJECT(gwnd), "key-press-event",
                     G_CALLBACK(KeyDown), NULL);
    g_signal_connect(G_OBJECT(gwnd), "button-press-event",
                     G_CALLBACK(MouseButton), NULL);
    g_signal_connect(G_OBJECT(gwnd), "button-release-event",
                     G_CALLBACK(MouseButton), NULL);
    gtk_widget_set_events(gwnd, gtk_widget_get_events(gwnd)
                              | GDK_KEY_PRESS_MASK
                              | GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK);
    tail = NULL;
    ulib = NULL;

    ncpu = min(sizeof(SEM_TYPE) * 8, max(1, sysconf(_SC_NPROCESSORS_ONLN)));
    thrd = malloc(ncpu * sizeof(*thrd));
    MakeSemaphore(&tmrd.isem, ncpu, SEM_NULL);
    MakeSemaphore(&tmrd.osem, ncpu, SEM_FULL);
    for (iter = 0; iter < ncpu; iter++) {
        thrd[iter] = (THRD){{(FILL){NULL, pict.size, 0}}, TRUE, 1 << iter,
                            &tmrd.isem, &tmrd.osem, FillLibStdThrd};
        pthread_create(&thrd[iter].ithr, NULL, ThrdFunc, &thrd[iter]);
    }
    TimeFunc(&mtmp);
    seed = mtmp;
    if ((iter = scandir(anim, &dirs, NULL, alphasort)) >= 0) {
        while (iter--) {
            if ((dirs[iter]->d_type == DT_DIR)
            &&  strcmp(dirs[iter]->d_name, ".")
            &&  strcmp(dirs[iter]->d_name, "..")) {
                curr = WaitSemaphore(&tmrd.osem, SEM_NULL);
                curr = log2l(curr & (curr ^ (curr - 1)));
                MakeEmptyLib(&ulib, anim, dirs[iter]->d_name);
                thrd[curr].fprm.fill.ulib = ulib;
                PickSemaphore(&tmrd.osem, &tmrd.isem, 1 << curr);
            }
            free(dirs[iter]);
        }
        free(dirs);
    }
    WaitSemaphore(&tmrd.osem, SEM_FULL);
    for (iter = 0; iter < ncpu; iter++)
        thrd[iter].loop = FALSE;
    PickSemaphore(&tmrd.osem, &tmrd.isem, SEM_FULL);
    WaitSemaphore(&tmrd.osem, SEM_FULL);

    if (ulib) {
        TimeFunc(&msec);
        mtmp = msec - mtmp;
        for (curr = iter = 0; iter < ncpu; iter++)
            curr += thrd[iter].fprm.fill.load;
        printf("\nLoading complete: %ld objects, %u ms [%0.3f ms/obj]\n\n",
               curr, mtmp, (float)mtmp / (float)curr);

        for (iter = 0; iter < ncpu; iter++) {
            thrd[iter].loop = TRUE;
            thrd[iter].func = DrawPixStdThrd;
            thrd[iter].fprm.draw = (DRAW){NULL, &pict,
                                   ((pict.size.y + 1) / ncpu)* iter,
                                   ((pict.size.y + 1) / ncpu)*(iter + 1)};
            pthread_create(&thrd[iter].ithr, NULL, ThrdFunc, &thrd[iter]);
        }
        thrd[ncpu - 1].fprm.draw.ymax = pict.size.y;

        UnitListFromLib(ulib, &tail);
        tmrd = (TMRD){tmrd.isem, tmrd.osem, &msec, ncpu,
                      0, surf, gwnd, thrd, tail, &pict};
        g_timeout_add(MIN_WAIT, TimeFunc, &msec);
        g_timeout_add(FRM_WAIT, DrawFunc, &tmrd);
        g_timeout_add(1000, FPSFunc, &tmrd);

        gtk_widget_show_all(gwnd);
        gdk_window_set_cursor(gtk_widget_get_window(gwnd),
                              gdk_cursor_new(GDK_HAND1));
        gtk_main();

        WaitSemaphore(&tmrd.osem, SEM_FULL);
        for (iter = 0; iter < ncpu; iter++)
            thrd[iter].loop = FALSE;
        PickSemaphore(&tmrd.osem, &tmrd.isem, SEM_FULL);
        WaitSemaphore(&tmrd.osem, SEM_FULL);

        FreeLibList(&ulib, FreeAnimStd);
        FreeUnitList(&tail, NULL);
    }
    else
        printf("No animation base found! Exiting...\n");
    cairo_surface_destroy(surf);
    FreeSemaphore(&tmrd.isem);
    FreeSemaphore(&tmrd.osem);
    free(thrd);
    return 0;
}
