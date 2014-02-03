#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "../core/core.h"

#define FRM_SKIP 40
#define DEF_SKIP 1



typedef struct _TMRF {
    ulong *time, fram;
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    UNIT *tail;
    PICT *pict;
    VEC2 ytmp;
} TMRF;



UNIT *pick = NULL;
VEC2 cptr = {};



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
    DRAW draw;

    if (tmrf->gwnd->window &&
       (draw.head = UpdateFrameStd(&tmrf->tail, &pick, tmrf->time, cptr))) {
        surf = cairo_create(tmrf->surf);
        cairo_set_operator(surf, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(surf, 0, 0, 0, 0);
        cairo_paint(surf);
        cairo_destroy(surf);

        draw.pict = tmrf->pict;
        draw.ymin = 0;
        draw.ymax = tmrf->pict->size.y;
        DrawPixStdThrd(&draw, FALSE);

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
    long y;

    UNIT *tail;
    ULIB *ulib;
    PICT pict;

    ulong msec, mtmp;
    TMRF tmrf;
    FILL fill;

    seed = time(NULL);
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

    TimeFunc(&mtmp);
    if ((y = scandir("anim", &dirs, NULL, alphasort)) >= 0) {
        while (y--) {
            if ((dirs[y]->d_type == DT_DIR)
            &&  strcmp(dirs[y]->d_name, ".")
            &&  strcmp(dirs[y]->d_name, "..")) {
                MakeEmptyLib(&ulib, "anim", dirs[y]->d_name);
                fill = (FILL){ulib, pict.size, 0};
                FillLibStdThrd(&fill, FALSE);
                FillLibStdThrd(&fill, TRUE);
            }
            free(dirs[y]);
        }
        free(dirs);
    }
    if (ulib) {
        UnitListFromLib(ulib, &tail);

        TimeFunc(&msec);
        mtmp = msec - mtmp;
        printf("\nLoading complete: %ld objects, %u ms [%0.3f ms/obj]\n\n",
               fill.load, mtmp, (float)mtmp / (float)fill.load);

        tmrf = (TMRF){&msec, 0, surf, gwnd, tail,
                      &pict, (VEC2){0, pict.size.y}};
        g_timeout_add(DEF_SKIP, TimeFunc, &msec);
        g_timeout_add(FRM_SKIP, DrawFunc, &tmrf);
        g_timeout_add(1000, FPSFunc, &tmrf);

        gtk_widget_show_all(gwnd);
        gtk_main();

        FreeLibList(&ulib, FreeAnimStd);
        FreeUnitList(&tail, NULL);
    }
    else
        printf("No animation base found! Exiting...\n");
    cairo_surface_destroy(surf);
    return 0;
}
