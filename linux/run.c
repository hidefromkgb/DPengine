#include <pwd.h>
#include <errno.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include "../exec/exec.h"



void GTKMenuHandler(GtkWidget *item, gpointer user) {
    eProcessMenuItem((MENU*)user);
}



void GTKMenuDestroy(GtkWidget *item, gpointer user) {
    GtkWidget *chld = (user)? (GtkWidget*)user
                            :  gtk_menu_item_get_submenu((GtkMenuItem*)item);
    if (chld) {
        gtk_container_foreach((GtkContainer*)chld, GTKMenuDestroy, 0);
        gtk_widget_destroy(chld);
    }
    if (!user)
        gtk_widget_destroy(item);
}



GtkWidget *Submenu(MENU *menu, ulong chld) {
    if (!menu)
        return 0;

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



void MainMenu(GtkStatusIcon *icon, guint mbtn, guint32 time, gpointer user) {
    rOpenContextMenu((MENU*)user);
}



void rOpenContextMenu(MENU *menu) {
    GtkMenu *mwnd = (GtkMenu*)Submenu(menu, 0);

    if (mwnd)
        gtk_menu_popup(mwnd, 0, 0, 0, 0, 0, gtk_get_current_event_time());
}



inline MENU *rOSSpecificMenu(ENGC *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long rMessage(char *text, char *head, uint32_t flgs) {
    GtkWidget *hdlg;
    long retn;

    hdlg = gtk_message_dialog_new(0, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER,
                                  GTK_BUTTONS_OK, text);
    if (head)
        gtk_window_set_title(GTK_WINDOW(hdlg), head);

    retn = gtk_dialog_run(GTK_DIALOG(hdlg));
    gtk_widget_destroy(hdlg);
    return retn;
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                      uint32_t *data, long xdim, long ydim) {
    GtkStatusIcon *icon = 0;
    GdkPixbuf *pbuf;
    uint32_t *temp;
    long iter;

    temp = malloc(xdim * ydim * sizeof(*temp));
    for (iter = xdim * ydim - 1; iter >= 0; iter--)
        temp[iter] =  (data[iter] & 0xFF00FF00)
                   | ((data[iter] & 0xFF) << 16)
                   | ((data[iter] >> 16) & 0xFF);
    pbuf = gdk_pixbuf_new_from_data((guchar*)temp, GDK_COLORSPACE_RGB,
                                    TRUE, CHAR_BIT, xdim, ydim,
                                    xdim * sizeof(*temp),
                                   (GdkPixbufDestroyNotify)free, temp);

    icon = gtk_status_icon_new_from_pixbuf(pbuf);
    gtk_status_icon_set_visible(icon, TRUE);
    gtk_status_icon_set_tooltip_text(icon, (gchar*)text);
    g_signal_connect(G_OBJECT(icon), "popup-menu",
                     G_CALLBACK(MainMenu), mctx);
    g_object_unref(G_OBJECT(pbuf));
    return (intptr_t)icon;
}



void rFreeTrayIcon(intptr_t icon) {
    g_object_unref(G_OBJECT(icon));
}



char *rLoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen + 1);
        read(file, retn, flen);
        retn[flen] = '\0';
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



long rSaveFile(char *name, char *data, long size) {
    long file;

    if ((file = open(name, O_CREAT | O_WRONLY, 0644)) > 0) {
        size = write(file, data, size);
        close(file);
        return size;
    }
    return 0;
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                       ENGC *engc, intptr_t data) {
    gtk_main();
}



void ButtonSwitch(GtkToggleButton *butn, gpointer data) {
    CTRL *ctrl = (typeof(ctrl))data;

    if (ctrl->fc2e)
        ctrl->fc2e(ctrl, MSG_BCLK, gtk_toggle_button_get_active(butn));
}



intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_WSZC: {
            /// [TODO]
            return 0;
        }
    }
    return 0;
}



intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PLIM:
            ctrl->priv[1] = (data > 0)? data : 1;
            return 0;

        case MSG_PPOS:
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctrl->priv[0]),
                                          data / ctrl->priv[1]);
            return 0;

        case MSG_PTXT:
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctrl->priv[0]),
                                     (char*)data);
            return 0;
    }
    return 0;
}



intptr_t FE2CC(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            return 0;
    }
    return 0;
}



intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            return 0;
    }
    return 0;
}



intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            return 0;

        case MSG_NGET:
            return gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctrl->priv[0]));

        case MSG_NSET:
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctrl->priv[0]), data);
            return 0;
    }
    return 0;
}



intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_SMAX:
            /// [TODO]
            return 0;
    }
    return 0;
}



gboolean ButtonSize(GtkWidget *gwnd, GdkRectangle *rect, gpointer user) {
    PangoLayout *play = gtk_label_get_layout(GTK_LABEL(user));
    PangoRectangle prec;

    pango_layout_set_width(play, rect->width * PANGO_SCALE);
    pango_layout_set_height(play, rect->height * PANGO_SCALE);
    pango_layout_get_pixel_extents(play, 0, &prec);
    gtk_widget_set_size_request(GTK_WIDGET(user), prec.width, prec.height);
    gtk_widget_queue_draw(GTK_WIDGET(user));
    return TRUE;
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    static GSList *lgrp = 0;
    GtkWidget *gwnd;
    CTRL *root;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        ctrl->fe2c = FE2CW;
        gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_app_paintable(gwnd, TRUE);
        /// [TODO] fix 750x450
        gtk_widget_set_size_request(gwnd, 750, 450);
        gtk_window_set_position(GTK_WINDOW(gwnd), GTK_WIN_POS_CENTER);
        gtk_window_set_resizable(GTK_WINDOW(gwnd), TRUE);
        gtk_window_set_decorated(GTK_WINDOW(gwnd), TRUE);
        gtk_window_set_title(GTK_WINDOW(gwnd), text);
        gtk_container_set_border_width(GTK_CONTAINER(gwnd), 0);
        gtk_widget_show(gwnd);

        /// [TODO] DEL ME
        g_signal_connect(G_OBJECT(gwnd), "destroy",
                         G_CALLBACK(gtk_main_quit), 0);

        ctrl->priv[1] = (intptr_t)gtk_table_new(1, 1, TRUE);
        gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(ctrl->priv[1]));
        gtk_widget_show(GTK_WIDGET(ctrl->priv[1]));
    }
    else if (root) {
        GtkTable *gtbl = (typeof(gtbl))ctrl->prev->priv[1];
        guint xdim, ydim;
        long xpos, ypos;

        while (root->prev)
            root = root->prev;

        xpos = ctrl->xpos + ((xoff && (ctrl->flgs & FCP_HORZ))?
                             *xoff : root->xpos);
        ypos = ctrl->ypos + ((yoff && (ctrl->flgs & FCP_VERT))?
                             *yoff : root->ypos);

        gtk_table_get_size(gtbl, &xdim, &ydim);
        /// [TODO] properly handle absolute (negative) values of ctrl->*dim
        #define max(a, b) (((a) > (b))? (a) : (b))
        gtk_table_resize(gtbl, max(ydim, ypos + abs(ctrl->ydim) + root->ypos),
                               max(xdim, xpos + abs(ctrl->xdim) + root->xpos));
        #undef max
        xdim = xpos + abs(ctrl->xdim);
        ydim = ypos + abs(ctrl->ydim);

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT: {
                GtkLabel *capt = GTK_LABEL(gtk_label_new(text));

                gwnd = gtk_frame_new(0);
                gtk_widget_show(GTK_WIDGET(capt));
                gtk_misc_set_alignment(GTK_MISC(capt), 0.0, 0.5);
                gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(capt));
                gtk_frame_set_shadow_type(GTK_FRAME(gwnd),
                                         (ctrl->flgs & FST_SUNK)?
                                          GTK_SHADOW_ETCHED_IN :
                                          GTK_SHADOW_NONE);
                break;
            }
            case FCT_BUTN: {
                GtkLabel *capt = GTK_LABEL(gtk_label_new(text));

                gwnd = gtk_button_new();
                gtk_widget_show(GTK_WIDGET(capt));
                gtk_label_set_line_wrap(capt, TRUE);
                gtk_label_set_justify(capt, GTK_JUSTIFY_CENTER);
                gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(capt));
                g_signal_connect(G_OBJECT(gwnd), "size-allocate",
                                 G_CALLBACK(ButtonSize), capt);
                if (ctrl->flgs & FSB_DFLT) {
                    gtk_widget_set_can_default(gwnd, TRUE);
                    gtk_window_set_default(GTK_WINDOW(ctrl->prev->priv[0]),
                                           gwnd);
                }
                break;
            }
            case FCT_CBOX: {
                GtkLabel *capt = GTK_LABEL(gtk_label_new(text));

                gwnd = gtk_check_button_new();
                gtk_widget_show(GTK_WIDGET(capt));
                gtk_widget_set_size_request(GTK_WIDGET(capt), INT16_MAX, -1);
                gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(capt));

                GtkTextDirection gdir = gtk_widget_get_direction(gwnd);

                gtk_misc_set_alignment(GTK_MISC(capt),
                                    !!(gdir == GTK_TEXT_DIR_RTL), 0.5);
                if (ctrl->flgs & FSX_LEFT)
                    gdir = (gdir == GTK_TEXT_DIR_RTL)?
                            GTK_TEXT_DIR_LTR : GTK_TEXT_DIR_RTL;
                gtk_widget_set_direction(gwnd, gdir);
                ctrl->fe2c = FE2CC;
                g_signal_connect(G_OBJECT(gwnd), "toggled",
                                 G_CALLBACK(ButtonSwitch), ctrl);
                break;
            }
            case FCT_RBOX:
                if (ctrl->flgs & FSR_NGRP)
                    lgrp = 0;
                gwnd = gtk_radio_button_new_with_label(lgrp, text);
                lgrp = gtk_radio_button_get_group(GTK_RADIO_BUTTON(gwnd));
                g_signal_connect(G_OBJECT(gwnd), "toggled",
                                 G_CALLBACK(ButtonSwitch), ctrl);
                break;

            case FCT_SPIN:
                gwnd = gtk_spin_button_new_with_range(0, 100, 1);
                ctrl->priv[0] = (intptr_t)gwnd;
                ctrl->fe2c = FE2CN;
                break;

            case FCT_LIST: {
                char *fgrp[] = {
                    "Main ponies",
                    "Supporting ponies",
                    "Alternate art",
                    "Fillies",
                    "Colts",
                    "Pets",
                    "Stallions",
                    "Mares",
                    "Alicorns",
                    "Unicorns",
                    "Pegasi",
                    "Earth ponies",
                    "Non-ponies",
                };
                GtkTreeViewColumn *column;
                GtkCellRenderer *renderer;
                GtkListStore *store;
                GtkTreeView *tree;
                GtkTreeIter iter;
                long indx;

                gwnd = gtk_scrolled_window_new(0, 0);
                gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gwnd),
                                               GTK_POLICY_NEVER,
                                               GTK_POLICY_ALWAYS);

                store = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
                for (indx = 0; indx < sizeof(fgrp) / sizeof(*fgrp); indx++) {
                    gtk_list_store_append(store, &iter);
                    gtk_list_store_set(store, &iter, 0, FALSE, 1, fgrp[indx], -1);
                }
                tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store)));
                g_object_unref(G_OBJECT(store));

                renderer = gtk_cell_renderer_toggle_new();
                column = gtk_tree_view_column_new_with_attributes("", renderer,
                                                                  NULL);
                gtk_tree_view_append_column(tree, column);

                renderer = gtk_cell_renderer_text_new();
                column = gtk_tree_view_column_new_with_attributes("[At least one:]", renderer,
                                                                  "text", 1, NULL);
                gtk_tree_view_append_column(tree, column);

                gtk_widget_show(GTK_WIDGET(tree));
                gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(tree));
                ctrl->fe2c = FE2CL;
                break;
            }
            case FCT_SBOX:
                gwnd = gtk_scrolled_window_new(0, 0);
                gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gwnd),
                                               GTK_POLICY_NEVER,
                                               GTK_POLICY_ALWAYS);
                ctrl->fe2c = FE2CS;
                break;

            case FCT_IBOX:
                gwnd = gtk_frame_new(0);
                break;

            case FCT_PBAR:
                ctrl->fe2c = FE2CP;
                ctrl->priv[0] = (intptr_t)(gwnd = gtk_progress_bar_new());
                break;
        }
        gtk_table_attach_defaults(gtbl, gwnd, xpos, xdim, ypos, ydim);
        gtk_widget_show(gwnd);
        if (xoff)
            *xoff = xdim;
        if (yoff)
            *yoff = ydim;
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}

void rFreeControl(CTRL *ctrl) {
}



int main(int argc, char *argv[]) {
    GdkScreen *gscr;
    gint xdim, ydim;

    long uses;
    char *home, *conf;
    struct dirent **dirs;
    ENGC *engc;

    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    conf = calloc(32 + strlen(home), sizeof(*conf));
    strcat(conf, home);
    strcat(conf, "/.config");
    mkdir(conf, 0700);
    strcat(conf, DEF_OPTS);
    if (!(home = (mkdir(conf, 0755))? (errno != EEXIST)? 0 : conf : conf))
        printf("WARNING: cannot create '%s'!", conf);

    engc = eInitializeEngine(conf);
    free(conf);
    if ((uses = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
        while (uses--) {
            if ((dirs[uses]->d_type == DT_DIR)
            &&  strcmp(dirs[uses]->d_name, ".")
            &&  strcmp(dirs[uses]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, DEF_FLDR, dirs[uses]->d_name);
            free(dirs[uses]);
        }
        free(dirs);
    }
    gtk_init(&argc, &argv);
    gscr = gdk_screen_get_default();
    gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &xdim, &ydim);
    eExecuteEngine(engc, xdim, ydim, 0, 0,
                   gdk_screen_get_width(gscr), gdk_screen_get_height(gscr));
    return 0;
}
