//#include <stdio.h>
//#include <dirent.h>
//#include <pthread.h>
//#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../core/core.h"



#define LST_TYPE uint64_t

#define FRM_SKIP 40
#define DEF_SKIP 1



/// semaphore list
typedef struct _SEML {
    pthread_cond_t cvar;
    pthread_mutex_t cmtx;
    LST_TYPE list, full;
} SEML;

typedef struct _THRD {
    union {
        FILL fill;
        DRAW draw;
    } fprm;
    long loop;
    LST_TYPE uuid;
    pthread_t ithr;
    SEML *isem, *osem;
    void (*func)(void*);
} THRD;

typedef struct _TMRF {
    SEML *isem, *osem;
    ulong *time, ncpu, fram;
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    THRD *thrd;
    UNIT *tail;
    PICT *pict;
} TMRF;

pthread_cond_t cond;

UNIT *pick = NULL;
VEC2 cptr = {};



void InitSemList(SEML *retn, long lstl, LST_TYPE mask) {
    pthread_mutex_init(&retn->cmtx, NULL);
    pthread_cond_init(&retn->cvar, NULL);
    retn->full = (1 << lstl) - 1;
    retn->list = retn->full & mask;
}



void ResetSem(SEML *list, LST_TYPE what, long drop) {
    pthread_mutex_lock(&list->cmtx);
    if (drop)
        list->list &= ~what;
    else
        list->list |=  what;
    pthread_cond_signal(&list->cvar);
    pthread_mutex_unlock(&list->cmtx);
}



void WaitForSem(SEML *list, LST_TYPE what) {
    pthread_mutex_lock(&list->cmtx);
    while (!(list->list & what))
        pthread_cond_wait(&list->cvar, &list->cmtx);
    pthread_mutex_unlock(&list->cmtx);
}



LST_TYPE WaitForSemList(SEML *list, long full) {
    LST_TYPE retn;

    pthread_mutex_lock(&list->cmtx);
    while (( full && list->list != list->full)
       ||  (!full && list->list == 0)) {
        pthread_cond_wait(&list->cvar, &list->cmtx);
    }
    retn = list->list;
    pthread_mutex_unlock(&list->cmtx);
    return retn;
}



void ThrdFunc(void *data) {
    #define data ((THRD*)data)
    do {
        WaitForSem(data->isem, data->uuid);
        ResetSem(data->isem, data->uuid, TRUE);
        if (data->loop)
            data->func(&data->fprm);
        ResetSem(data->osem, data->uuid, FALSE);
    } while (data->loop);
    printf("Thread exited\n");
    #undef data
}



gboolean TimeFunc(gpointer user) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    *(ulong*)user = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    return TRUE;
}



gboolean FPSFunc(gpointer user) {
    TMRF *tmrf = user;
    char fsec[24];

    sprintf(fsec, "%u FPS", tmrf->fram);
    tmrf->fram = 0;
    printf("%s\n", fsec);
    gtk_window_set_title(GTK_WINDOW(tmrf->gwnd), fsec);
    return TRUE;
}



gboolean DrawFunc(gpointer user) {
    TMRF *tmrf = user;
    cairo_t *surf;
    UNIT *tail;
    long iter;

    if (tmrf->gwnd->window &&
       (tail = UpdateFrameStd(&tmrf->tail, &pick, tmrf->time, cptr))) {
        surf = cairo_create(tmrf->surf);
        cairo_set_operator(surf, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(surf, 0, 0, 0, 0);
        cairo_paint(surf);
        cairo_destroy(surf);

        for (iter = 0; iter < tmrf->ncpu; iter++)
            tmrf->thrd[iter].fprm.draw.tail = tail;
        ResetSem(tmrf->isem, tmrf->isem->full, FALSE);
        WaitForSemList(tmrf->osem, TRUE);
        ResetSem(tmrf->osem, tmrf->osem->full, TRUE);

        gdk_window_invalidate_rect(tmrf->gwnd->window, NULL, FALSE);
        tmrf->fram++;
        return TRUE;
    }
    return FALSE;
}



void ScreenChange(GtkWidget *gwnd, GdkScreen *scrn, gpointer user) {
    GdkColormap *cmap = gdk_screen_get_rgba_colormap(
                        gtk_widget_get_screen(gwnd));
    if (cmap)
        gtk_widget_set_colormap(gwnd, cmap);
    else {
        printf("Transparent windows not supported! Emergency exit...\n");
        exit(0);
    }
}



gboolean Redraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer user) {
    cairo_t *draw = gdk_cairo_create(gwnd->window);
    cairo_surface_t *surf = user;

    cairo_surface_mark_dirty(surf);
    cairo_set_operator(draw, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(draw, surf, 0, 0);
    cairo_paint(draw);
    cairo_destroy(draw);
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
                     GdkEventButton *evmb, GdkWindowEdge edge) {
    if (evmb->button == 1)
        switch (evmb->type) {
            case GDK_BUTTON_PRESS:
                pick = EMP_PICK;
                cptr.x = evmb->x;
                cptr.y = evmb->y;
                break;

            case GDK_BUTTON_RELEASE:
                pick = NULL;
                break;
        }
    return TRUE;
}



gboolean MouseMove(GtkWidget *gwnd, GdkEventMotion *emov, gpointer user) {
    GdkModifierType gmod;

    if (pick)
        gdk_window_get_pointer(emov->window, &cptr.x, &cptr.y, &gmod);
    return TRUE;
}



int main(int argc, char *argv[]) {
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    GdkScreen *gscr;

    struct dirent **dirs;
    long ncpu, iter, curr;
    char *anim = "anim";

    UNIT *tail;
    ULIB *ulib;
    PICT pict;

    ulong msec, mtmp;
    SEML isem, osem;
    THRD *thrd;
    TMRF tmrf;

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

    g_signal_connect(G_OBJECT(gwnd), "delete-event",
                     gtk_main_quit, NULL);
    g_signal_connect(G_OBJECT(gwnd), "expose-event",
                     G_CALLBACK(Redraw), surf);
    g_signal_connect(G_OBJECT(gwnd), "screen-changed",
                     G_CALLBACK(ScreenChange), NULL);
    g_signal_connect(G_OBJECT(gwnd), "key-press-event",
                     G_CALLBACK(KeyDown), NULL);
    g_signal_connect(G_OBJECT(gwnd), "button-press-event",
                     G_CALLBACK(MouseButton), NULL);
    g_signal_connect(G_OBJECT(gwnd), "button-release-event",
                     G_CALLBACK(MouseButton), NULL);
    g_signal_connect(G_OBJECT(gwnd), "motion-notify-event",
                     G_CALLBACK(MouseMove), NULL);
    gtk_widget_set_events(gwnd, gtk_widget_get_events(gwnd) |
                                GDK_KEY_PRESS_MASK          |
                                GDK_BUTTON_PRESS_MASK       |
                                GDK_BUTTON_RELEASE_MASK     |
                                GDK_POINTER_MOTION_MASK     |
                                GDK_POINTER_MOTION_HINT_MASK);
    tail = NULL;
    ulib = NULL;

    ncpu = min(sizeof(LST_TYPE) * 8, max(1, sysconf(_SC_NPROCESSORS_ONLN)));

    ncpu = 1;

    thrd = malloc(ncpu * sizeof(*thrd));
    InitSemList(&isem, ncpu, 0);
    InitSemList(&osem, ncpu, ~0);
    for (iter = 0; iter < ncpu; iter++) {
        thrd[iter].isem = &isem;
        thrd[iter].osem = &osem;
        thrd[iter].func = (void (*)(void*))FillLibStdThrd;
        thrd[iter].loop = TRUE;
        thrd[iter].uuid = 1 << iter;
        thrd[iter].fprm.fill.load = 0;
        thrd[iter].fprm.fill.scrn = pict.size;
        pthread_create(&thrd[iter].ithr, NULL, ThrdFunc, &thrd[iter]);
    }

    TimeFunc(&mtmp);
    seed = mtmp;
    if ((iter = scandir(anim, &dirs, NULL, alphasort)) >= 0) {
        while (iter--) {
            if ((dirs[iter]->d_type == DT_DIR)
            &&  strcmp(dirs[iter]->d_name, ".")
            &&  strcmp(dirs[iter]->d_name, "..")) {
                curr = WaitForSemList(&osem, FALSE);
                curr = log2l(curr & (curr ^ (curr - 1)));
                MakeEmptyLib(&ulib, anim, dirs[iter]->d_name);
                thrd[curr].fprm.fill.ulib = ulib;
                ResetSem(&osem, 1 << curr, TRUE);
                ResetSem(&isem, 1 << curr, FALSE);
            }
            free(dirs[iter]);
        }
        free(dirs);
    }
    WaitForSemList(&osem, TRUE);
    printf("\n");
    for (iter = 0; iter < ncpu; iter++)
        thrd[iter].loop = FALSE;
    ResetSem(&osem, osem.full, TRUE);
    ResetSem(&isem, isem.full, FALSE);
    WaitForSemList(&osem, TRUE);

    if (ulib) {
        TimeFunc(&msec);
        mtmp = msec - mtmp;
        for (curr = iter = 0; iter < ncpu; iter++)
            curr += thrd[iter].fprm.fill.load;
        printf("\nLoading complete: %ld objects, %u ms [%0.3f ms/obj]\n\n",
               curr, mtmp, (float)mtmp / (float)curr);

        for (iter = 0; iter < ncpu; iter++) {
            thrd[iter].loop = TRUE;
            thrd[iter].func = (void (*)(void*))DrawPixStdThrd;
            thrd[iter].fprm.draw = (DRAW){NULL, &pict,
                                   ((pict.size.y + 1) / ncpu)* iter,
                                   ((pict.size.y + 1) / ncpu)*(iter + 1)};
            pthread_create(&thrd[iter].ithr, NULL, ThrdFunc, &thrd[iter]);
        }
        thrd[ncpu - 1].fprm.draw.ymax = pict.size.y;

        UnitListFromLib(ulib, &tail);
        tmrf = (TMRF){&isem, &osem, &msec, ncpu, 0,
                       surf,  gwnd,  thrd, tail, &pict};
        g_timeout_add(DEF_SKIP, TimeFunc, &msec);
        g_timeout_add(FRM_SKIP, DrawFunc, &tmrf);
        g_timeout_add(1000, FPSFunc, &tmrf);

        gtk_widget_show_all(gwnd);
        gtk_main();

        for (iter = 0; iter < ncpu; iter++)
            thrd[iter].loop = FALSE;
        ResetSem(&isem, isem.full, FALSE);
        WaitForSemList(&osem, TRUE);

        FreeLibList(&ulib, (void (*)(void**))FreeAnimStd);
        FreeUnitList(&tail, NULL);
    }
    else
        printf("No animation base found! Exiting...\n");
    cairo_surface_destroy(surf);
    free(thrd);
    return 0;
}
