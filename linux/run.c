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
    long iter;

    for (iter = xdim * ydim - 1; iter >= 0; iter--)
        data[iter] =  (data[iter] & 0xFF00FF00)
                   | ((data[iter] & 0xFF) << 16)
                   | ((data[iter] >> 16) & 0xFF);
    pbuf = gdk_pixbuf_new_from_data((guchar*)data, GDK_COLORSPACE_RGB,
                                    TRUE, CHAR_BIT, xdim, ydim,
                                    xdim * sizeof(*data),
                                   (GdkPixbufDestroyNotify)free, data);

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
    strcat(conf, "/DesktopPonies");
    if (!(home = (mkdir(conf, 0755))? (errno != EEXIST)? 0 : conf : conf))
        printf("WARNING: cannot create '%s'!", conf);

    engc = eInitializeEngine(home);
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


    /// [TODO:] substitute this by GUI selection
    uses = (argc >= 2)? atol(argv[1]) : 0;
    uses = (uses != 0)? uses : 1;
    __DEL_ME__SetLibUses(engc, uses);


    gtk_init(&argc, &argv);
    gscr = gdk_screen_get_default();
    gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &xdim, &ydim);
    eExecuteEngine(engc, xdim, ydim, 0, 0,
                   gdk_screen_get_width(gscr), gdk_screen_get_height(gscr));
    free(conf);
    return 0;
}
