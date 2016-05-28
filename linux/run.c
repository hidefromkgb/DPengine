#include <dirent.h>
#include <gtk/gtk.h>
#include "../exec/exec.h"



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



void OpenContextMenu(MENU *menu) {
    GtkMenu *mwnd = (GtkMenu*)Submenu(menu, 0);

    if (mwnd)
        gtk_menu_popup(mwnd, NULL, NULL, NULL, NULL,
                       0, gtk_get_current_event_time());
}



void MainMenu(GtkStatusIcon *icon, guint mbtn, guint32 time, gpointer user) {
    OpenContextMenu(((ENGC*)user)->mctx);
}



inline MENU *OSSpecificMenu(ENGC *engc) {
    return NULL;
}



char *ConvertUTF8(char *utf8) {
    return strdup(utf8);
}



void SetTrayIconText(uintptr_t icon, char *text) {
    gtk_status_icon_set_tooltip_text((GtkStatusIcon*)icon, (gchar*)text);
}



char *LoadFileZ(char *name, long *size) {
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
    GtkStatusIcon *icon;
    GdkScreen *gscr;
    GdkPixbuf *pbuf;
    gint xdim, ydim;

    struct dirent **dirs;
    uint32_t *bptr;
    long uses;

    ENGC engc = {};

    if ((uses = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
        while (uses--) {
            if ((dirs[uses]->d_type == DT_DIR)
            &&  strcmp(dirs[uses]->d_name, ".")
            &&  strcmp(dirs[uses]->d_name, ".."))
                AppendLib(&engc, DEF_CONF, DEF_FLDR, dirs[uses]->d_name);
            free(dirs[uses]);
        }
        free(dirs);
    }


    /// [TODO:] substitute this by GUI selection
    uses = (argc >= 2)? atol(argv[1]) : 0;
    uses = (uses != 0)? uses : 1;
    __DEL_ME__SetLibUses(&engc, uses);


    gtk_init(&argc, &argv);
    gscr = gdk_screen_get_default();
    gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &xdim, &ydim);

    bptr = InitEngine(&engc, 0, xdim, ydim);
    pbuf = gdk_pixbuf_new_from_data((guchar*)bptr, GDK_COLORSPACE_RGB,
                                    TRUE, CHAR_BIT, xdim, ydim,
                                    xdim * sizeof(*bptr),
                                   (GdkPixbufDestroyNotify)free, bptr);

    icon = gtk_status_icon_new_from_pixbuf(pbuf);
    gtk_status_icon_set_visible(icon, TRUE);
    g_signal_connect(G_OBJECT(icon), "popup-menu",
                     G_CALLBACK(MainMenu), &engc);

    ExecEngine(&engc, (uintptr_t)icon,
               0, 0, gdk_screen_get_width(gscr), gdk_screen_get_height(gscr),
              ((uses < 0)? 0 : COM_RGPU) | COM_SHOW | COM_DRAW);

    g_object_unref(G_OBJECT(icon));
    g_object_unref(G_OBJECT(pbuf));
    return 0;
}
