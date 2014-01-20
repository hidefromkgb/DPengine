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

#define DEF_NLIM 0



typedef struct _TMRF {
    unsigned *time, fram;
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    INST *tail;
    PICT *pict;
    VEC2 ytmp;
} TMRF;



INST *pick = NULL;
VEC2 cptr = {};



gboolean TimeFunc(gpointer user) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    *(unsigned*)user = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
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
    INST *head;
    cairo_t *draw;

    if (tmrf->gwnd->window &&
       (head = UpdateFrame(&tmrf->tail, &pick, tmrf->time, cptr))) {
        draw = cairo_create(tmrf->surf);
        cairo_set_operator(draw, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(draw, 0, 0, 0, 0);
        cairo_paint(draw);
        cairo_destroy(draw);
        BlendPixStdThrd(head, tmrf->pict, &tmrf->ytmp);
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



/** List of sprites to remain unscaled. Just a stub, since the scale has to be
    read from configs. The reason for not downscaling is given in parentheses.
 **/
char *del_me_plz[] = {
    "/Angel/",                           /// (downscale is his natural size)
    "/Parasprite/",                      /// [same as above]
    "/random-pony.gif",                  /// (question mark)
    "/tree.gif",                         /// Applejack`s apple tree (appples)
    "/apple_drop.gif",                   /// [same as above]
    "/lemon_hearts_sweeping_right.gif",  /// (broom)
    "/Ruby Pinch/",                      /// (cutie mark)
    "/scoot_right.gif",                  /// Skating Scootaloo (wheels)
    "/Sparkler/",                        /// (cutie mark again)
    "/stage.gif",                        /// Trixie`s stage (many details)
    "/balloon_right.gif",                /// Twilight`s balloon [same as above]
    NULL
};

int main(int argc, char *argv[]) {
    cairo_surface_t *surf;
    GtkWidget *gwnd;
    GdkScreen *gscr;

    struct dirent **dirs, **gifs;
    char *path, *temp;
    int x, y, lenp;
    INST *tail;
    PICT pict;
    VEC2 loop;

    unsigned msec, mtmp, nlim;
    TMRF tmrf;

    nlim = (DEF_NLIM)? DEF_NLIM : (unsigned)-1;

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
    loop = (VEC2){0, TRUE};
    tail = malloc(sizeof(*tail));
    tail->prev = NULL;

    TimeFunc(&mtmp);
    if ((y = scandir("anim", &dirs, NULL, alphasort)) >= 0) {
        while (y--) {
            if ((dirs[y]->d_type == DT_DIR)
            &&  strcmp(dirs[y]->d_name, ".")
            &&  strcmp(dirs[y]->d_name, "..")) {
                path = malloc(lenp = strlen(dirs[y]->d_name) + 6);
                sprintf(path, "anim/%s", dirs[y]->d_name);

                if ((x = scandir(path, &gifs, NULL, alphasort)) >= 0) {
                    while (x--) {
                        temp = gifs[x]->d_name;
                        if (nlim &&
                            !strcasecmp(temp - 4 + strlen(temp), ".gif")) {
                            tail->path = malloc(lenp + 1 + strlen(temp));
                            sprintf(tail->path, "%s/%s", path, temp);

                            tail->scal = 0;
                            tail->cpos = pict.size;
                            while (del_me_plz[tail->scal]) {
                                if (strstr(tail->path, del_me_plz[tail->scal]))
                                    break;
                                tail->scal++;
                            }
                            tail->scal = (del_me_plz[tail->scal])? 0 : 1;
                            MakeAnimStdThrd(tail, NULL, &loop);

                            tail->next = malloc(sizeof(*tail));
                            tail->next->prev = tail;
                            tail = tail->next;
                            nlim--;
                        }
                        free(gifs[x]);
                    }
                    free(gifs);
                }
                free(path);
            }
            free(dirs[y]);
        }
        free(dirs);
    }
    if (tail->prev) {
        tail = tail->prev;
        free(tail->next);
        tail->next = NULL;
        SortByY(&tail);

        TimeFunc(&msec);
        mtmp = msec - mtmp;
        printf("\nLoading complete: %ld objects, %u ms [%0.3f ms/obj]\n\n",
               loop.x, mtmp, (float)mtmp / (float)loop.x);

        tmrf = (TMRF){&msec, 0, surf, gwnd, tail,
                      &pict, (VEC2){0, pict.size.y}};
        g_timeout_add(DEF_SKIP, TimeFunc, &msec);
        g_timeout_add(FRM_SKIP, DrawFunc, &tmrf);
        g_timeout_add(1000, FPSFunc, &tmrf);

        gtk_widget_show_all(gwnd);
        gtk_main();

        while (!0) {
            FreeAnimStd((ASTD**)&tail->anim);
            free(tail->path);
            if (tail->prev) {
                tail = tail->prev;
                free(tail->next);
            }
            else {
                free(tail);
                break;
            }
        }
    }
    cairo_surface_destroy(surf);
    return 0;
}
