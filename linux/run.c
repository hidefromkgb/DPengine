#include <pwd.h>
#include <errno.h>
#include <dirent.h>
#include <semaphore.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <openssl/err.h>
#include "../exec/exec.h"



typedef struct {
    long size;
    UPRE func;
    sem_t isem;
} DTHR;

typedef struct {
    struct dirent **dirs;
    long iter;
} FIND;

pthread_mutex_t *pmtx; /// the necessary evil: global mutex table for OpenSSL



void GTKMenuHandler(GtkWidget *item, gpointer user) {
    eProcessMenuItem((MENU*)user);
}



void GTKMenuDestroy(GtkWidget *item, gpointer user) {
    GtkWidget *chld = (user)? GTK_WIDGET(user)
                            : gtk_menu_item_get_submenu(GTK_MENU_ITEM(item));
    if (chld) {
        gtk_container_foreach(GTK_CONTAINER(chld), GTKMenuDestroy, 0);
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

    retn = GTK_MENU(gtk_menu_new());
    if (!chld)
        g_signal_connect(G_OBJECT(retn), "selection-done",
                         G_CALLBACK(GTKMenuDestroy), retn);
    indx = 0;
    while (menu->text) {
        if (!*menu->text)
            item = gtk_separator_menu_item_new();
        else {
            if (menu->flgs & MFL_CCHK) {
                item = gtk_check_menu_item_new_with_mnemonic(menu->text);
                gtk_check_menu_item_set_active
                    (GTK_CHECK_MENU_ITEM(item), menu->flgs & MFL_VCHK);
                gtk_check_menu_item_set_draw_as_radio
                    (GTK_CHECK_MENU_ITEM(item), menu->flgs &  MFL_RCHK
                                                           & ~MFL_CCHK);
            }
            else
                item = gtk_menu_item_new_with_mnemonic((char*)menu->text);
            gtk_widget_set_sensitive(item, !(menu->flgs & MFL_GRAY));
            g_signal_connect(G_OBJECT(item), "activate",
                             G_CALLBACK(GTKMenuHandler), menu);
            if (menu->chld)
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),
                                          Submenu(menu->chld, ~0));
        }
        gtk_menu_attach(retn, item, 0, 1, indx, indx + 1);
        indx++;
        menu++;
    }
    gtk_widget_show_all(GTK_WIDGET(retn));
    return GTK_WIDGET(retn);
}

void OpenMenu(MENU *menu, guint32 time) {
    GtkMenu *mwnd = GTK_MENU(Submenu(menu, 0));

    if (mwnd)
        gtk_menu_popup(mwnd, 0, 0, 0, 0, 0, time);
}

void MainMenu(GtkStatusIcon *icon, guint mbtn, guint32 time, gpointer user) {
    OpenMenu((MENU*)user, gtk_get_current_event_time());
}

void rOpenContextMenu(MENU *menu) {
    OpenMenu(menu, GDK_CURRENT_TIME);
}



inline MENU *rOSSpecificMenu(void *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long rMessage(char *text, char *head, char *byes, char *bnay) {
    GtkWidget *hdlg;
    long retn;

    hdlg = gtk_message_dialog_new(0, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER,
                                     GTK_BUTTONS_NONE, text);
    gtk_dialog_add_buttons(GTK_DIALOG(hdlg), byes,
                          (char*)GTK_RESPONSE_OK, (char*)0);
    if (bnay)
        gtk_dialog_add_buttons(GTK_DIALOG(hdlg), bnay,
                              (char*)GTK_RESPONSE_CANCEL, (char*)0);
    if (head)
        gtk_window_set_title(GTK_WINDOW(hdlg), head);

    retn = gtk_dialog_run(GTK_DIALOG(hdlg));
    gtk_widget_destroy(hdlg);
    return !!(retn == GTK_RESPONSE_OK);
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



intptr_t rFindMake(char *base) {
    FIND *find;

    find = calloc(1, sizeof(*find));
    find->iter = scandir(base, &find->dirs, 0, alphasort);
    find->iter = (find->iter > 0)? find->iter : 0;
    return (intptr_t)find;
}



char *rFindFile(intptr_t data) {
    FIND *find = (FIND*)data;
    char *retn = 0;

    if (find->iter <= 0)
        return 0;
    while ((--find->iter + 1) && ((find->dirs[find->iter]->d_type != DT_DIR)
                              || !strcmp(find->dirs[find->iter]->d_name, "..")
                              || !strcmp(find->dirs[find->iter]->d_name, ".")))
        free(find->dirs[find->iter]);
    if (find->iter >= 0) {
        retn = strdup(find->dirs[find->iter]->d_name);
        free(find->dirs[find->iter]);
    }
    if (find->iter <= 0)
        free(find->dirs);
    return retn;
}



char *rLoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen + 1);
        if (read(file, retn, flen) == flen) {
            retn[flen] = '\0';
            if (size)
                *size = flen;
        }
        else {
            free(retn);
            retn = 0;
        }
        close(file);
    }
    return retn;
}



long rSaveFile(char *name, char *data, long size) {
    long file;

    if ((file = open(name, O_CREAT | O_WRONLY, 0644)) > 0) {
        size = write(file, data, size);
        ftruncate(file, size);
        close(file);
        return size;
    }
    return 0;
}



long rMakeDir(char *name, long dupl) {
    return !mkdir(name, 0755) || (!dupl && (errno == EEXIST));
}



long rMoveDir(char *dsrc, char *ddst) {
    __attribute__((aligned(32))) volatile long retn, iter;
    char *pcmd, *ptmp, *dtmp, *dddd[] = {(ddst)? ddst : dsrc, dsrc};

    ptmp = pcmd = calloc(1, 128 + 4 * (strlen(dddd[0]) + strlen(dddd[1])));
    memcpy(ptmp, (ddst)? "mv -f '" : "rm -r '", retn = sizeof("mv -f '") - 1);
    ptmp += retn;
    for (iter = (ddst)? 1 : 0; iter >= 0; iter--) {
        while ((dtmp = strchr(dddd[iter], '\''))) {
            strncpy(ptmp, dddd[iter], retn = dtmp - dddd[iter]);
            ptmp += retn;
            memcpy(ptmp, "'\\''", retn = sizeof("'\\''") - 1);
            ptmp += retn;
            dddd[iter] = dtmp + 1;
        }
        strncpy(ptmp, dddd[iter], retn = strlen(dddd[iter]));
        ptmp += retn;
        dtmp = (iter)? "' '" : "'";
        memcpy(ptmp, dtmp, retn = strlen(dtmp));
        ptmp += retn;
    }
    retn = !system(pcmd);
    free(pcmd);
    return retn;
}



char *ChooseFileDir(CTRL *root, char *file, char *fext) {
    GtkFileFilter *fltr;
    GtkWidget *wdlg;
    char *temp;

    while (root->prev)
        root = root->prev;
    wdlg = gtk_file_chooser_dialog_new
               ("", GTK_WINDOW(root->priv[0]),
               (fext)? GTK_FILE_CHOOSER_ACTION_OPEN
                     : GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, (char*)0);
    if (fext) {
        fltr = gtk_file_filter_new();
        temp = realloc(strdup("*."), sizeof("*.") + strlen(fext));
        gtk_file_filter_add_pattern(fltr, strcat(temp, fext));
        gtk_file_filter_set_name(fltr, temp);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wdlg), fltr);
        free(temp);
    }
    if (file)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(wdlg), file);
    if (gtk_dialog_run(GTK_DIALOG(wdlg)) != GTK_RESPONSE_ACCEPT)
        temp = 0;
    else {
        fext = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wdlg));
        temp = strdup(fext);
        g_free(fext);
    }
    gtk_widget_destroy(wdlg);
    return temp;
}

char *rChooseDir(CTRL *root, char *base) {
    return ChooseFileDir(root, base, 0);
}

char *rChooseFile(CTRL *root, char *fext, char *file) {
    return ChooseFileDir(root, file, fext);
}



void *ParallelFunc(void *data) {
    DTHR *dthr = (DTHR*)((intptr_t*)data)[0];
    intptr_t user = ((intptr_t*)data)[1];

    free(data);
    dthr->func(user, 0);
    sem_post(&dthr->isem);
    return 0;
}



intptr_t rMakeParallel(UPRE func, long size) {
    DTHR *dthr;

    size *= sysconf(_SC_NPROCESSORS_ONLN);
    *(dthr = calloc(1, sizeof(*dthr))) = (DTHR){(size > 1)? size : 1, func};
    sem_init(&dthr->isem, 0, size);
    return (intptr_t)dthr;
}



void rLoadParallel(intptr_t user, intptr_t data) {
    DTHR *dthr = (DTHR*)user;
    pthread_t pthr;
    intptr_t *pass;

    pass = calloc(2, sizeof(intptr_t));
    pass[0] = user;
    pass[1] = data;
    sem_wait(&dthr->isem);
    pthread_create(&pthr, 0, ParallelFunc, pass);
}



void rFreeParallel(intptr_t user) {
    DTHR *dthr = (DTHR*)user;

    for (; dthr->size; dthr->size--)
        sem_wait(&dthr->isem);
    sem_destroy(&dthr->isem);
    free(dthr);
}



size_t WriteHTTPS(char *cptr, size_t size, size_t memb, void *user) {
    intptr_t *data = (intptr_t*)user;

    data[1] = (intptr_t)realloc((char*)data[1], 1 + data[0] + (size *= memb));
    memcpy((char*)data[1] + data[0], cptr, size);
    ((char*)data[1])[data[0] += size] = 0;
    if (data[2])
        ((void (*)(long, intptr_t))data[2])(data[0], data[3]);
    return size;
}



void rFreeHTTPS(intptr_t user) {
    if (user) {
        free(((char**)user)[0]);
        free(((char**)user)[1]);
        free((char**)user);
    }
}



intptr_t rMakeHTTPS(char *user, char *serv) {
    char **retn;

    if (!user || !serv)
        return 0;
    retn = calloc(2, sizeof(*retn));
    retn[0] = strdup(user);
    retn[1] = calloc(1, 10 + strlen(serv));
    strcat(strcat(strcat(retn[1], "https://"), serv), "/");
    return (intptr_t)retn;
}



long rLoadHTTPS(intptr_t user, char *page, char **dest,
                void (*load)(long, intptr_t), intptr_t lprm) {
    intptr_t data[4] = {0, 0, (intptr_t)load, lprm};
    char *hreq, **ctxt = (char**)user;
    CURL *curl;

    if (!page || !dest) {
        *dest = 0;
        return 0;
    }
    curl = curl_easy_init();
    hreq = calloc(1, 1 + strlen(ctxt[1]) + strlen(page));
    curl_easy_setopt(curl, CURLOPT_URL, strcat(strcat(hreq, ctxt[1]), page));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteHTTPS);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ctxt[0]);
    if (curl_easy_perform(curl)) {
        data[1] = (intptr_t)realloc((char*)data[1], 0);
        data[1] = data[0] = 0;
    }
    curl_easy_cleanup(curl);
    free(hreq);
    *dest = (char*)data[1];
    return data[0];
}



gboolean TmrFunc(gpointer data) {
    intptr_t *user = (intptr_t*)data;
    struct timespec spec = {};
    uint64_t time;

    clock_gettime(CLOCK_REALTIME, &spec);
    time = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    ((UPRE)user[0])(user[1], time);
    return TRUE;
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre, intptr_t data) {
    intptr_t user[2] = {(intptr_t)upre, data};
    guint tmrp;

    tmrp = g_timeout_add(fram, TmrFunc, user);
    gtk_main();
    g_source_remove(tmrp);
}



void ProcessSpin(GtkSpinButton *spin, gpointer data) {
    long spos = gtk_spin_button_get_value(spin);
    CTRL *ctrl = (CTRL*)data;

    spos = (spos >= ctrl->priv[1])? spos : ctrl->priv[1];
    spos = (spos <= ctrl->priv[2])? spos : ctrl->priv[2];
    ctrl->fc2e(ctrl, MSG_NSET, spos);
}



void MoveControl(CTRL *ctrl, intptr_t data) {
    long xpos = ((long*)data)[0], ypos = ((long*)data)[1];
    CTRL *root = ctrl;

    while (root->prev)
        root = root->prev;
    xpos = (xpos < 0)? -xpos : xpos * (uint16_t)(root->priv[2]      );
    ypos = (ypos < 0)? -ypos : ypos * (uint16_t)(root->priv[2] >> 16);
    gtk_fixed_move(GTK_FIXED(ctrl->prev->priv[7]),
                   GTK_WIDGET(ctrl->priv[0]), xpos, ypos);
}



uint32_t GetWidgetSize(intptr_t what) {
    GtkRequisition requ;

    gtk_widget_size_request(GTK_WIDGET(what), &requ);
    return (uint16_t)requ.width | ((uint32_t)requ.height << 16);
}



void LabelSize(GtkWidget *gwnd, GdkRectangle *rect, gpointer data) {
    CTRL *ctrl = (CTRL*)data;
    uint32_t size;

    if (((size = GetWidgetSize(ctrl->priv[7])) != ctrl->priv[2])
    ||  ((uint16_t)(ctrl->priv[1] >> 16) != rect->height)
    ||  ((uint16_t)(ctrl->priv[1]      ) != rect->width)) {
        ctrl->priv[2] = size;
        ctrl->priv[1] = (uint16_t)rect->width | ((uint32_t)rect->height << 16);
        gtk_layout_move(GTK_LAYOUT(ctrl->priv[6]), GTK_WIDGET(ctrl->priv[7]),
                       ((ctrl->flgs & FST_CNTR)?
                       (rect->width  - (int16_t)(size      )) / 2 : 0),
                       (rect->height - (int16_t)(size >> 16)) / 2 - 1);
    }
}



void ButtonSize(GtkWidget *gwnd, GdkRectangle *rect, gpointer data) {
    PangoLayout *play = gtk_label_get_layout(GTK_LABEL(data));
    PangoRectangle prec;

    pango_layout_set_width(play, (rect->width - 8) * PANGO_SCALE);
    pango_layout_set_height(play, rect->height * PANGO_SCALE);
    pango_layout_get_pixel_extents(play, 0, &prec);
    gtk_widget_set_size_request(GTK_WIDGET(data), prec.width, prec.height);
    gtk_widget_queue_draw(GTK_WIDGET(data));
}



void ButtonSwitch(GtkToggleButton *butn, gpointer data) {
    CTRL *ctrl = (CTRL*)data;

    ctrl->fc2e(ctrl, MSG_BCLK, ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
                                 gtk_toggle_button_get_active(butn) : 0);
}



void ListItemSwitch(GtkCellRendererToggle *cell, gchar *path, gpointer data) {
    CTRL *ctrl = (CTRL*)data;
    GtkTreeIter iter;
    gboolean bbbb;

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ctrl->priv[2]),
                                       &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(ctrl->priv[2]), &iter, 0, &bbbb, -1);
    ctrl->fc2e(ctrl, MSG_LSST, (strtol(path, 0, 10) << 1) | ((bbbb)? 0 : 1));
    gtk_list_store_set(GTK_LIST_STORE(ctrl->priv[2]), &iter, 0, !bbbb, -1);
}



gboolean ListItemUpdate(GtkTreeModel *tree, GtkTreePath *path,
                        GtkTreeIter *iter, gpointer data) {
    CTRL *ctrl = (CTRL*)data;
    char *pstr = gtk_tree_path_to_string(path);
    gboolean bbbb = ctrl->fc2e(ctrl, MSG_LGST, strtol(pstr, 0, 10));

    gtk_list_store_set(GTK_LIST_STORE(ctrl->priv[2]), iter, 0, bbbb, -1);
    g_free(pstr);
    return FALSE;
}



void WndScroll(GtkAdjustment *vadj, gpointer data) {
    CTRL *ctrl = (CTRL*)data;

    ctrl->fc2e(ctrl, MSG_SGIP, ctrl->fe2c(ctrl, MSG_SGIP, 0));
}



gboolean IBoxDraw(GtkWidget *gwnd, GdkEventExpose *eexp, gpointer data) {
    CTRL *ctrl = (CTRL*)data;
    cairo_surface_t *surf;
    cairo_t *temp;

    AINF anim = {(ctrl->priv[7] >> 10) & 0x3FFFFF,
                 (int16_t)ctrl->priv[3], (int32_t)ctrl->priv[3] >> 16,
                  ctrl->priv[7] & 0x3FF, (uint32_t*)ctrl->priv[2]};

    if (anim.uuid) {
        temp = cairo_create(surf = (cairo_surface_t*)ctrl->priv[1]);
        cairo_set_operator(temp, CAIRO_OPERATOR_CLEAR);
        cairo_paint(temp);
        cairo_destroy(temp);
        ctrl->fc2e(ctrl, MSG_IFRM, (intptr_t)&anim);
        cairo_surface_mark_dirty(surf);
        temp = gdk_cairo_create(gtk_widget_get_window(gwnd));
        cairo_set_operator(temp, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface(temp, surf, 0, 0);
        cairo_paint(temp);
        cairo_destroy(temp);
    }
    return TRUE;
}



gboolean OptQuit(GtkWidget *gwnd, GdkEvent *ewcl, gpointer data) {
    if (((CTRL*)data)->fc2e((CTRL*)data, MSG_WEND, 0)) {
        gtk_main_quit();
        return FALSE;
    }
    return TRUE;
}



gboolean OptResize(GtkWidget *gwnd, GdkEventConfigure *ecnf, gpointer data) {
    CTRL *ctrl = (CTRL*)data;

    if ((ecnf->width  != (uint16_t)(ctrl->priv[1]      ))
    ||  (ecnf->height != (uint16_t)(ctrl->priv[1] >> 16))) {
        ctrl->priv[1] = (uint16_t)ecnf->width | ((uint32_t)ecnf->height << 16);
        ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
    }
    return FALSE;
}



/// PRIV:
///  0: GtkWidget
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (fontmul.x) | (fontmul.y << 16)
///  3: PangoFontDescription
///  4:
///  5:
///  6:
///  7: GtkFixed
intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW:
            ((data)? gtk_widget_show
                   : gtk_widget_hide)(GTK_WIDGET(ctrl->priv[0]));
            break;

        case MSG__TXT:
            gtk_window_set_title(GTK_WINDOW(ctrl->priv[0]), (char*)data);
            break;

        case MSG_WSZC: {
            GtkWidget *gwnd = GTK_WIDGET(ctrl->priv[0]);
            long xdim, ydim;

            xdim = (((uint16_t)(data      )) + ctrl->xdim)
                 *   (uint16_t)(ctrl->priv[2]      );
            ydim = (((uint16_t)(data >> 16)) + ctrl->ydim)
                 *   (uint16_t)(ctrl->priv[2] >> 16);
            ctrl->priv[1] = (uint16_t)xdim | ((uint32_t)ydim << 16);
            gtk_widget_set_size_request(gwnd, xdim, ydim);
            gtk_window_set_position(GTK_WINDOW(gwnd), GTK_WIN_POS_CENTER);
            break;
        }
        case MSG_WTDA: {
            AINF *anim = (AINF*)data;
            uint8_t cfnt[4] = {anim->fcnt >> 16, anim->fcnt >> 8, anim->fcnt};
            PangoRectangle rect =
                {(uint8_t)(anim->xdim >> 16), (uint8_t)(anim->ydim >> 16)};
            PangoFontDescription *pfnt = (PangoFontDescription*)ctrl->priv[3];
            PangoLayout *play;
            PangoAttrList *attr;
            cairo_surface_t *pict, *surf;
//            cairo_font_options_t *opts;
            cairo_t *temp;

            rect.width  = (uint16_t)anim->xdim - rect.x
                        - (uint8_t)(anim->xdim >> 24);
            rect.height = (uint16_t)anim->ydim - rect.y
                        - (uint8_t)(anim->ydim >> 24);

            /// [TODO:] get rid of this disgusting width hack
            rect.width += 6;
            rect.x -= 3;

            pict = cairo_image_surface_create_for_data
                       ((uint8_t*)anim->uuid, CAIRO_FORMAT_ARGB32,
                        (uint16_t)anim->xdim, (uint16_t)anim->ydim,
                         sizeof(uint32_t) * (uint16_t)anim->xdim);
            surf = cairo_surface_create_for_rectangle
                       (pict, rect.x, rect.y, rect.width, rect.height);
            play = pango_cairo_create_layout(temp = cairo_create(surf));
//            opts = cairo_font_options_create();
//            cairo_font_options_set_antialias(opts, CAIRO_ANTIALIAS_SUBPIXEL);
//            cairo_set_font_options(temp, opts);
            pango_attr_list_insert(attr = pango_attr_list_new(),
                                   pango_attr_foreground_new(cfnt[0] << 8,
                                                             cfnt[1] << 8,
                                                             cfnt[2] << 8));
            pango_layout_set_attributes(play, attr);
            pango_layout_set_font_description(play, pfnt);
            pango_layout_set_wrap(play, PANGO_WRAP_WORD_CHAR);
            pango_layout_set_alignment(play, PANGO_ALIGN_CENTER);
            pango_layout_set_width (play, rect.width  * PANGO_SCALE);
            pango_layout_set_height(play, rect.height * PANGO_SCALE);
            pango_layout_set_text(play, (char*)anim->time, -1);
            pango_cairo_update_layout(temp, play);
            pango_cairo_show_layout(temp, play);
            cairo_destroy(temp);
            g_object_unref(play);
            cairo_surface_destroy(surf);
            cairo_surface_destroy(pict);
//            cairo_font_options_destroy(opts);
            pango_attr_list_unref(attr);
            break;
        }
        case MSG_WTGD: {
            AINF *anim = (AINF*)data;
            int xdim = (anim->xdim > 0)? anim->xdim * PANGO_SCALE : -1,
                ydim = (anim->ydim > 0)? anim->ydim * PANGO_SCALE : -1;
            PangoFontDescription *pfnt = (PangoFontDescription*)ctrl->priv[3];
            PangoLayout *play;
            cairo_surface_t *surf;
            cairo_t *temp;

            surf = cairo_recording_surface_create(CAIRO_CONTENT_ALPHA, 0);
            play = pango_cairo_create_layout(temp = cairo_create(surf));
            pango_layout_set_width(play, xdim);
            pango_layout_set_height(play, ydim);
            pango_layout_set_font_description(play, pfnt);
            pango_layout_set_wrap(play, PANGO_WRAP_WORD_CHAR);
            pango_layout_set_alignment(play, PANGO_ALIGN_CENTER);
            pango_layout_set_text(play, (char*)anim->time, -1);
            pango_layout_get_pixel_size(play, &xdim, &ydim);
            cairo_destroy(temp);
            g_object_unref(play);
            cairo_surface_destroy(surf);
            return (uint16_t)xdim | ((uint32_t)ydim << 16);
        }
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget
///  1: progress position
///  2: progress maximum
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PPOS: {
            double dcur = ctrl->priv[1] = data,
                   dmax = (ctrl->priv[2] > 0)? ctrl->priv[2] : 1;

            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctrl->priv[0]),
                                          dcur / dmax);
            gtk_widget_queue_draw(GTK_WIDGET(ctrl->priv[0]));
            while (gtk_events_pending())
                gtk_main_iteration_do(0);
            break;
        }
        case MSG__TXT:
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctrl->priv[0]),
                                     (char*)data);
            break;

        case MSG_PLIM:
            ctrl->priv[2] = data;
            break;

        case MSG_PGET:
            return (data)? ctrl->priv[2] : ctrl->priv[1];
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget (container)
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7: GtkWidget (text)
intptr_t FE2CX(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            break;

        case MSG__TXT:
            gtk_label_set_text(GTK_LABEL(ctrl->priv[7]), (char*)data);
            break;

        case MSG_BGST:
            data = ctrl->priv[0];
            cmsg = (((ctrl->flgs & FCT_TTTT) == FCT_CBOX)
                   && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data)))?
                      FCS_MARK : 0;
            cmsg |= (gtk_widget_get_sensitive(GTK_WIDGET(data)))? FCS_ENBL : 0;
            return cmsg;

        case MSG_BCLK:
            if ((ctrl->flgs & FCT_TTTT) != FCT_CBOX)
                return 0;
            cmsg =
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctrl->priv[0]));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl->priv[0]),
                                         !!data);
            return !!cmsg;
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget
///  1: GtkTreeView
///  2: GtkListStore
///  3: GtkTreeViewColumn
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            break;

        case MSG__TXT:
            gtk_tree_view_column_set_title
                (GTK_TREE_VIEW_COLUMN(ctrl->priv[3]), (char*)data);
            gtk_tree_model_foreach(GTK_TREE_MODEL(ctrl->priv[2]),
                                   ListItemUpdate, ctrl);
            break;

        case MSG_LADD: {
            GtkTreeIter iter;

            gtk_list_store_append(GTK_LIST_STORE(ctrl->priv[2]), &iter);
            gtk_list_store_set(GTK_LIST_STORE(ctrl->priv[2]), &iter,
                               0, FALSE, 1, (char*)data, -1);
            break;
        }
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget
///  1: minimum
///  2: maximum
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ:
            return GetWidgetSize(ctrl->priv[0]);

        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            ((data)? gtk_widget_show
                   : gtk_widget_hide)(GTK_WIDGET(ctrl->priv[0]));
            break;

        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            break;

        case MSG_NGET:
            return gtk_spin_button_get_value(GTK_SPIN_BUTTON(ctrl->priv[0]));

        case MSG_NSET:
            data = (data > ctrl->priv[1])? data : ctrl->priv[1];
            data = (data < ctrl->priv[2])? data : ctrl->priv[2];
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctrl->priv[0]), data);
            break;

        case MSG_NDIM:
            ctrl->priv[1] = (int16_t)data;
            ctrl->priv[2] = (int16_t)(data >> 16);
            gtk_spin_button_set_range(GTK_SPIN_BUTTON(ctrl->priv[0]),
                                      ctrl->priv[1], ctrl->priv[2]);
            break;
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget (outer container)
///  1: (inner.width) | (inner.height << 16)
///  2: (label.width) | (label.height << 16)
///  3:
///  4:
///  5:
///  6: GtkWidget (inner container)
///  7: GtkWidget (text label itself)
intptr_t FE2CT(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ:
            return GetWidgetSize(ctrl->priv[0]);

        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__TXT:
            gtk_label_set_text(GTK_LABEL(ctrl->priv[7]), (char*)data);
            break;

        case MSG__ENB:
            gtk_widget_set_sensitive(GTK_WIDGET(ctrl->priv[0]), !!data);
            break;

        case MSG__SHW:
            ((data)? gtk_widget_show
                   : gtk_widget_hide)(GTK_WIDGET(ctrl->priv[0]));
            break;
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (wndmove.x) | (wndmove.y << 16)
///  3: total height
///  4:
///  5:
///  6: GtkViewport
///  7: GtkFixed
intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ:
            return ctrl->priv[1];

        case MSG_WSZC: {
            GtkAllocation area;
            long xdim, ydim;
            double temp;

            if ((ctrl->prev->flgs & FCT_TTTT) != FCT_WNDW)
                return -1;

            if (!data) {
                xdim = (uint16_t)(ctrl->priv[1]      );
                ydim = (uint16_t)(ctrl->priv[1] >> 16);
                data = ctrl->priv[1];
            }
            else {
                xdim = (uint16_t)(ctrl->priv[2]      );
                ydim = (uint16_t)(ctrl->priv[2] >> 16);
                xdim = (uint16_t)(data      ) - xdim - ctrl->prev->xdim
                     * (uint16_t)(ctrl->prev->priv[2]      );
                ydim = (uint16_t)(data >> 16) - ydim - ctrl->prev->ydim
                     * (uint16_t)(ctrl->prev->priv[2] >> 16);
                data = (uint16_t)xdim | ((uint32_t)ydim << 16);
            }
            gtk_widget_set_size_request(GTK_WIDGET(ctrl->priv[0]), xdim, ydim);
            if (ctrl->fc2e) {
                gtk_widget_get_allocation
                    (gtk_scrolled_window_get_vscrollbar
                         (GTK_SCROLLED_WINDOW(ctrl->priv[0])), &area);
                temp = (ctrl->priv[3])?
                    (double)ctrl->fe2c(ctrl, MSG_SGIP, 0) / ctrl->priv[3] : 0;
                ctrl->priv[3] = ctrl->fc2e(ctrl, MSG_SSID, data - area.width);
                gtk_widget_set_size_request(GTK_WIDGET(ctrl->priv[7]),
                                            xdim, ctrl->priv[3]);
                ctrl->priv[1] = data;
                ctrl->fc2e(ctrl, MSG_SGIP, temp *= ctrl->priv[3]);
                gtk_adjustment_set_value
                    (gtk_scrolled_window_get_vadjustment
                         (GTK_SCROLLED_WINDOW(ctrl->priv[0])), temp);
            }
            ctrl->priv[1] = data;
            gtk_widget_show(GTK_WIDGET(ctrl->priv[0]));
            break;
        }
        case MSG_SGIP:
            return gtk_adjustment_get_value
                       (gtk_scrolled_window_get_vadjustment
                            (GTK_SCROLLED_WINDOW(ctrl->priv[0])));

        case MSG_SGTH:
            return ctrl->priv[3];

        case MSG__SHW:
            ((data)? gtk_widget_show
                   : gtk_widget_hide)(GTK_WIDGET(ctrl->priv[0]));
            break;
    }
    return 0;
}



/// PRIV:
///  0: GtkWidget
///  1: cairo_surface_t
///  2: data array
///  3: (xdim) | (ydim << 16)
///  4:
///  5:
///  6:
///  7: (animation ID << 10) | (current frame)
intptr_t FE2CI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            ((data)? gtk_widget_show
                   : gtk_widget_hide)(GTK_WIDGET(ctrl->priv[0]));
            break;

        case MSG_IFRM:
            ctrl->priv[7] = data;
            gdk_window_invalidate_rect
                (gtk_widget_get_window(GTK_WIDGET(ctrl->priv[0])), 0, FALSE);
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW:
            break;

        case FCT_TEXT:
            break;

        case FCT_BUTN:
            break;

        case FCT_CBOX:
            break;

        case FCT_SPIN:
            break;

        case FCT_LIST:
            g_object_unref(G_OBJECT(ctrl->priv[2])); /// the list data
            break;

        case FCT_SBOX:
            break;

        case FCT_IBOX:
            cairo_surface_destroy((cairo_surface_t*)ctrl->priv[1]);
            break;

        case FCT_PBAR:
            break;
    }
    /// no need to destroy widgets explicitly:
    /// this all happens after gtk_main_quit()
}



void rMakeControl(CTRL *ctrl, long *xoff, long *yoff) {
    GtkWidget *gwnd;
    CTRL *root;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        PangoFontMetrics *fmet;
        PangoContext *pctx;
        long xfon, yfon;

        ctrl->fe2c = FE2CW;
        gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_app_paintable(gwnd, TRUE);
        gtk_window_set_resizable(GTK_WINDOW(gwnd), TRUE);
        gtk_window_set_decorated(GTK_WINDOW(gwnd), TRUE);
        if (~ctrl->flgs & FSW_SIZE) {
            gtk_window_set_skip_taskbar_hint(GTK_WINDOW(gwnd), TRUE);
            gtk_window_set_resizable(GTK_WINDOW(gwnd), FALSE);
            gtk_window_set_type_hint(GTK_WINDOW(gwnd),
                                     GDK_WINDOW_TYPE_HINT_DIALOG);
        }
        gtk_container_set_border_width(GTK_CONTAINER(gwnd), 0);
        pctx = gtk_widget_get_pango_context(gwnd);
        ctrl->priv[3] = (intptr_t)gwnd->style->font_desc;
        fmet = pango_context_get_metrics(pctx, gwnd->style->font_desc, 0);
        xfon = pango_font_metrics_get_approximate_char_width(fmet);
        yfon = pango_font_metrics_get_ascent(fmet);
        ctrl->priv[2] =  (uint16_t)round(1.500 * xfon / PANGO_SCALE)
                      | ((uint32_t)round(0.675 * yfon / PANGO_SCALE) << 16);

        g_signal_connect(G_OBJECT(gwnd), "configure-event",
                         G_CALLBACK(OptResize), ctrl);
        g_signal_connect(G_OBJECT(gwnd), "delete-event",
                         G_CALLBACK(OptQuit), ctrl);

        ctrl->priv[7] = (intptr_t)gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(ctrl->priv[7]));
        gtk_widget_show(GTK_WIDGET(ctrl->priv[7]));
    }
    else if (root) {
        GtkFixed *gtbl = (typeof(gtbl))ctrl->prev->priv[7];
        long xspc, yspc, xpos, ypos, xdim, ydim;

        while (root->prev)
            root = root->prev;
        xspc = (uint16_t)(root->priv[2]);
        yspc = (uint16_t)(root->priv[2] >> 16);
        xpos =  ctrl->xpos + ((xoff && (ctrl->flgs & FCP_HORZ))?
                              *xoff : root->xpos);
        ypos =  ctrl->ypos + ((yoff && (ctrl->flgs & FCP_VERT))?
                              *yoff : root->ypos);
        xdim = (ctrl->xdim < 0)? -ctrl->xdim : ctrl->xdim * xspc;
        ydim = (ctrl->ydim < 0)? -ctrl->ydim : ctrl->ydim * yspc;
        if (xoff)
            *xoff = xpos
                  + ((ctrl->xdim < 0)? 1 - ctrl->xdim / xspc : ctrl->xdim);
        if (yoff)
            *yoff = ypos
                  + ((ctrl->ydim < 0)? 1 - ctrl->ydim / yspc : ctrl->ydim);

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT:
                gwnd = gtk_frame_new(0);
                ctrl->fe2c = FE2CT;
                ctrl->priv[1] = ctrl->priv[2] = 0;
                ctrl->priv[7] = (intptr_t)gtk_label_new(0);
                ctrl->priv[6] = (intptr_t)gtk_layout_new(0, 0);
                g_signal_connect(G_OBJECT(ctrl->priv[6]), "size-allocate",
                                 G_CALLBACK(LabelSize), ctrl);
                gtk_container_add(GTK_CONTAINER(ctrl->priv[6]),
                                  GTK_WIDGET(ctrl->priv[7]));
                gtk_container_add(GTK_CONTAINER(gwnd),
                                  GTK_WIDGET(ctrl->priv[6]));
                gtk_frame_set_shadow_type(GTK_FRAME(gwnd),
                                         (ctrl->flgs & FST_SUNK)?
                                          GTK_SHADOW_ETCHED_IN :
                                          GTK_SHADOW_NONE);
                gtk_widget_show(GTK_WIDGET(ctrl->priv[6]));
                gtk_widget_show(GTK_WIDGET(ctrl->priv[7]));
                break;

            case FCT_BUTN:
                gwnd = gtk_button_new();
                ctrl->fe2c = FE2CX;
                ctrl->priv[7] = (intptr_t)gtk_label_new(0);
                gtk_widget_show(GTK_WIDGET(ctrl->priv[7]));
                gtk_label_set_line_wrap(GTK_LABEL(ctrl->priv[7]), TRUE);
                gtk_label_set_justify(GTK_LABEL(ctrl->priv[7]),
                                      GTK_JUSTIFY_CENTER);
                gtk_container_add(GTK_CONTAINER(gwnd),
                                  GTK_WIDGET(ctrl->priv[7]));
                g_signal_connect(G_OBJECT(gwnd), "size-allocate",
                                 G_CALLBACK(ButtonSize), (void*)ctrl->priv[7]);
                g_signal_connect(G_OBJECT(gwnd), "clicked",
                                 G_CALLBACK(ButtonSwitch), ctrl);
                if (ctrl->flgs & FSB_DFLT) {
                    gtk_widget_set_can_default(gwnd, TRUE);
                    gtk_window_set_default(GTK_WINDOW(ctrl->prev->priv[0]),
                                           gwnd);
                }
                break;

            case FCT_CBOX: {
                GtkTextDirection gdir;

                gwnd = gtk_check_button_new();
                ctrl->fe2c = FE2CX;
                ctrl->priv[7] = (intptr_t)gtk_label_new(0);
                gtk_widget_show(GTK_WIDGET(ctrl->priv[7]));
                gtk_widget_set_size_request(GTK_WIDGET(ctrl->priv[7]),
                                            INT16_MAX, -1);
                gtk_container_add(GTK_CONTAINER(gwnd),
                                  GTK_WIDGET(ctrl->priv[7]));
                gdir = gtk_widget_get_direction(gwnd);
                gtk_misc_set_alignment(GTK_MISC(ctrl->priv[7]),
                                    !!(gdir == GTK_TEXT_DIR_RTL), 0.5);
                if (ctrl->flgs & FSX_LEFT)
                    gdir = (gdir == GTK_TEXT_DIR_RTL)?
                            GTK_TEXT_DIR_LTR : GTK_TEXT_DIR_RTL;
                gtk_widget_set_direction(gwnd, gdir);
                g_signal_connect(G_OBJECT(gwnd), "toggled",
                                 G_CALLBACK(ButtonSwitch), ctrl);
                break;
            }
            case FCT_SPIN:
                ctrl->fe2c = FE2CN;
                ctrl->priv[1] = 0;
                ctrl->priv[2] = 1;
                gwnd = gtk_spin_button_new_with_range(ctrl->priv[1],
                                                      ctrl->priv[2], 1);
                g_signal_connect(G_OBJECT(gwnd), "value-changed",
                                 G_CALLBACK(ProcessSpin), ctrl);
                break;

            case FCT_LIST: {
                GtkTreeView *tree = GTK_TREE_VIEW(gtk_tree_view_new());
                GtkCellRenderer *cell = gtk_cell_renderer_toggle_new();

                ctrl->fe2c = FE2CL;
                ctrl->priv[1] = (intptr_t)tree;
                ctrl->priv[2] = (intptr_t)gtk_list_store_new(2, G_TYPE_BOOLEAN,
                                                                G_TYPE_STRING);
                gwnd = gtk_scrolled_window_new(0, 0);
                gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gwnd),
                                               GTK_POLICY_NEVER,
                                               GTK_POLICY_ALWAYS);
                g_signal_connect(G_OBJECT(cell), "toggled",
                                 G_CALLBACK(ListItemSwitch), ctrl);
                gtk_tree_view_append_column
                    (tree, gtk_tree_view_column_new_with_attributes
                               ("", cell, "active", 0, NULL));
                ctrl->priv[3] = (intptr_t)
                    gtk_tree_view_column_new_with_attributes
                        ("", gtk_cell_renderer_text_new(), "text", 1, NULL);
                gtk_tree_view_append_column
                    (tree, GTK_TREE_VIEW_COLUMN(ctrl->priv[3]));
                gtk_tree_view_set_model(tree, GTK_TREE_MODEL(ctrl->priv[2]));
                gtk_widget_show(GTK_WIDGET(tree));
                gtk_container_add(GTK_CONTAINER(gwnd), GTK_WIDGET(tree));
                break;
            }
            case FCT_SBOX:
                ctrl->fe2c = FE2CS;
                ctrl->priv[1] =  (uint16_t)xdim | ((uint32_t)ydim << 16);
                ctrl->priv[2] =  (uint16_t)(xpos * xspc)
                              | ((uint32_t)(ypos * yspc) << 16);
                gwnd = gtk_scrolled_window_new(0, 0);
                gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gwnd),
                                               GTK_POLICY_NEVER,
                                               GTK_POLICY_ALWAYS);
                ctrl->priv[6] = (intptr_t)gtk_viewport_new(0, 0);
                ctrl->priv[7] = (intptr_t)gtk_fixed_new();
                gtk_viewport_set_shadow_type(GTK_VIEWPORT(ctrl->priv[6]),
                                             GTK_SHADOW_NONE);
                gtk_container_add(GTK_CONTAINER(ctrl->priv[6]),
                                  GTK_WIDGET(ctrl->priv[7]));
                gtk_container_add(GTK_CONTAINER(gwnd),
                                  GTK_WIDGET(ctrl->priv[6]));
                gtk_widget_show(GTK_WIDGET(ctrl->priv[6]));
                gtk_widget_show(GTK_WIDGET(ctrl->priv[7]));
                g_signal_connect(G_OBJECT(gtk_scrolled_window_get_vadjustment
                                              (GTK_SCROLLED_WINDOW(gwnd))),
                                 "value-changed", G_CALLBACK(WndScroll), ctrl);
                break;

            case FCT_IBOX:
                ctrl->fe2c = FE2CI;
                gwnd = gtk_drawing_area_new();

                ctrl->priv[1] = (intptr_t)
                    cairo_image_surface_create
                        (CAIRO_FORMAT_ARGB32, xdim, ydim);
                ctrl->priv[2] = (intptr_t)
                    cairo_image_surface_get_data
                        ((cairo_surface_t*)ctrl->priv[1]);
                ctrl->priv[3] = (uint16_t)xdim | ((uint32_t)ydim << 16);

                g_signal_connect(G_OBJECT(gwnd), "expose-event",
                                 G_CALLBACK(IBoxDraw), ctrl);
                break;

            case FCT_PBAR:
                ctrl->fe2c = FE2CP;
                ctrl->priv[0] = (intptr_t)(gwnd = gtk_progress_bar_new());
                break;
        }
        gtk_fixed_put(gtbl, gwnd, xpos * xspc, ypos * yspc);
        gtk_widget_set_size_request(gwnd, xdim, ydim);
        gtk_widget_show(gwnd);
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}



void lockfunc(int mode, int indx, const char *file, int line) {
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&pmtx[indx]);
    else
        pthread_mutex_unlock(&pmtx[indx]);
}



void _start() {
    /** the stack MUST be aligned to a 256-bit (32-byte) boundary: **/
    __attribute__((aligned(32))) volatile uint32_t size = 32;
    char *home, *conf;
    GdkScreen *gscr;
    gint xdim, ydim, cmtx;

    pmtx = calloc(cmtx = CRYPTO_num_locks(), sizeof(*pmtx));
    for (xdim = 0; xdim < cmtx; xdim++)
        pthread_mutex_init(&pmtx[xdim], 0);
    CRYPTO_set_id_callback(pthread_self);
    CRYPTO_set_locking_callback(lockfunc);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (!(home = getenv("HOME")))
        home = getpwuid(getuid())->pw_dir;
    conf = calloc(strlen(home) + size, sizeof(*conf));
    strcat(conf, home);
    strcat(conf, "/.config");
    rMakeDir(conf, 0);
    strcat(conf, DEF_OPTS);
    if (!rMakeDir(conf, 0))
        printf("WARNING: cannot create '%s'!", conf);

    gtk_init(0, 0);
    gscr = gdk_screen_get_default();
    gtk_icon_size_lookup(GTK_ICON_SIZE_DIALOG, &xdim, &ydim);
    eExecuteEngine(conf, conf, xdim, ydim, 0, 0,
                   gdk_screen_get_width(gscr), gdk_screen_get_height(gscr));
    free(conf);
    curl_global_cleanup();
    CRYPTO_set_locking_callback(0);
    CRYPTO_set_id_callback(0);
    for (xdim = 0; xdim < cmtx; xdim++)
        pthread_mutex_destroy(&pmtx[xdim]);
    free(pmtx);
    exit(0);
}
