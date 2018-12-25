#include <errno.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <curl/curl.h>

#include "../exec/exec.h"
#include "load/mac_load.h"



#define CLS_CELL "lNSX" /** class name for list box cell **/
#define CLS_TRAY "lNSR" /** class name for tray icon **/
#define CLS_MENU "lNSM" /** class name for menu responder **/
#define CLS_WNDW "lNSW" /** class name for window **/
#define CLS_TEXT "lNST" /** class name for text box **/
#define CLS_BUTN "lNSB" /** class name for button + checkbox + radiobox **/
#define CLS_SPIN "lNSN" /** class name for spin box **/
#define CLS_LIST "lNSL" /** class name for list box **/
#define CLS_PBAR "lNSP" /** class name for progress bar **/
#define CLS_SBOX "lNSS" /** class name for size box **/
#define CLS_IBOX "lNSI" /** class name for image box **/
#define CLS_FRMT "lNSF" /** class name for number formatter **/

#define VAR_DATA "data" /** instance variable for the associated data **/



/// subclass storage
typedef struct {
    Class cell, tray, menu, wndw, text, butn,
          spin, list, pbar, sbox, ibox, frmt;
} SCLS;

///thread data
typedef struct {
    long size;
    UPRE func;
    sem_t *isem;
} DTHR;

/// file find data
typedef struct {
    struct dirent **dirs;
    long iter;
} FIND;

pthread_mutex_t *pmtx; /// the necessary evil: global mutex table for OpenSSL



static SEL ActionSelector() {
    static SEL what = 0;

    if (!what)
        what = sel_registerName("_A");
    return what;
}

/// NAME holds the selector associated with this function
void MAC_Handler(OnMenu, NSMenu *menu) {
    eProcessMenuItem((MENU*)tag(menu));
}



NSMenu *Submenu(MENU *menu, NSMenu *base) {
    if (!menu)
        return 0;

    NSMenuItem *item;
    NSMenu *retn = init(alloc(NSMenu()));
    NSImage *cbtn, *rbtn;
    NSString *text, *null = MAC_MakeString(0);

    cbtn = imageNamed_(NSImage(), text = MAC_MakeString("NSMenuCheckmark"));
    MAC_FreeString(text);
    rbtn = imageNamed_(NSImage(), text = MAC_MakeString("NSMenuRadio"));
    MAC_FreeString(text);
    setAutoenablesItems_(retn, false);
    while (menu->text) {
        if (!*menu->text) {
            item = separatorItem(NSMenuItem());
            addItem_(retn, item);
        }
        else {
            item = initWithTitle_action_keyEquivalent_
                       (alloc(NSMenuItem()), text = MAC_MakeString(menu->text),
                        ActionSelector(), null);
            MAC_FreeString(text);
            if (menu->flgs & MFL_CCHK) {
                setOnStateImage_(item, (menu->flgs & MFL_RCHK & ~MFL_CCHK)?
                                        rbtn : cbtn);
                setState_(item, (menu->flgs & MFL_VCHK)?
                                 NSOnState : NSOffState);
            }
            if (menu->chld) {
                NSMenu *next = Submenu(menu->chld, base);

                setSubmenu_(item, next);
                release(next);
            }
            /// might be dangerous if sizeof(NSInteger) < sizeof(MENU*)
            setTag_(item, (NSInteger)menu);

            setEnabled_(item, (menu->flgs & MFL_GRAY)? false : true);
            setTarget_(item, base);
            addItem_(retn, item);
            release(item);
        }
        menu++;
    }
    MAC_FreeString(null);
    return retn;
}



void rOpenContextMenu(MENU *tmpl) {
    NSMenu *menu, *base;
    NSPoint dptr;

    dptr = mouseLocation(NSEvent());
    menu = Submenu(tmpl, base = init(alloc(MAC_LoadClass(CLS_MENU))));
    popUpMenuPositioningItem_atLocation_inView_(menu, nil, dptr, nil);
    release(menu);
    release(base);
}



inline MENU *rOSSpecificMenu(void *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long rMessage(char *text, char *head, char *byes, char *bnay) {
    NSString *tttt, *hhhh;
    NSAlert *mbox;
    long retn;

    setAlertStyle_(mbox = init(alloc(NSAlert())), NSInformationalAlertStyle);
    addButtonWithTitle_(mbox, tttt = MAC_MakeString(byes));
    MAC_FreeString(tttt);
    if (bnay) {
        addButtonWithTitle_(mbox, tttt = MAC_MakeString(bnay));
        MAC_FreeString(tttt);
    }
    setMessageText_(mbox, hhhh = MAC_MakeString(head));
    setInformativeText_(mbox, tttt = MAC_MakeString(text));
    retn = (runModal(mbox) == NSAlertFirstButtonReturn);
    release(mbox);
    MAC_FreeString(tttt);
    MAC_FreeString(hhhh);
    return retn;
}



void MAC_Handler(OnTray) {
    MENU *mctx;

    MAC_GetIvar(self, VAR_DATA, &mctx);
    rOpenContextMenu(mctx);
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctxt = CGBitmapContextCreate
        (data, xdim, ydim, CHAR_BIT, xdim * sizeof(*data), drgb,
         kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGImageRef iref = CGBitmapContextCreateImage(ctxt);
    NSString *capt;

    NSImage *pict;
    NSButton *ibtn;
    void **retn = malloc(2 * sizeof(*retn));

    pict = initWithCGImage_size_(alloc(NSImage()), iref, (NSSize){});
    retn[0] = statusItemWithLength_(systemStatusBar(NSStatusBar()),
                                    NSVariableStatusItemLength);
    retain(retn[0]);
    retn[1] = init(alloc(MAC_LoadClass(CLS_TRAY)));
    MAC_SetIvar(retn[1], VAR_DATA, mctx);

    if (MAC_10_10_PLUS)
        ibtn = button(retn[0]); /// Yosemite or newer, so we are using buttons
    else
        setHighlightMode_(ibtn = retn[0], true);
    setImage_(ibtn, pict);
    setTarget_(ibtn, retn[1]);
    setAction_(ibtn, ActionSelector());
    setToolTip_(retn[0], capt = MAC_MakeString(text));
    MAC_FreeString(capt);
    release(pict);
    CGImageRelease(iref);
    CGContextRelease(ctxt);
    CGColorSpaceRelease(drgb);
    return (intptr_t)retn;
}



void rFreeTrayIcon(intptr_t icon) {
    void **retn = (void**)icon;

    removeStatusItem_(systemStatusBar(NSStatusBar()), retn[0]);
    release(retn[0]);
    release(retn[1]);
    free(retn);
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
        ftruncate(file, size);
        close(file);
        return size;
    }
    return 0;
}



long rMakeDir(char *name) {
    return !mkdir(name, 0755) || (errno == EEXIST);
}



long rMoveDir(char *dsrc, char *ddst) {
    char *pcmd, *ptmp, *dtmp, *dddd[] = {(ddst)? ddst : dsrc, dsrc};
    long retn, iter;

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
    NSOpenPanel *cfdp;
    NSString *path;
    NSArray *type;
    NSURL *idir;

    file = strdup(file);
    setAllowsMultipleSelection_(cfdp = openPanel(NSOpenPanel()), false);
    setCanChooseDirectories_(cfdp, !fext);
    setCanChooseFiles_(cfdp, !!fext);
    if (fext) {
        path = MAC_MakeString(fext);
        type = (NSArray*)CFArrayCreate(kCFAllocatorDefault,
                                      (const void**)&path, 1,
                                      &kCFTypeArrayCallBacks);
        setAllowedFileTypes_(cfdp, type);
        CFRelease(type);
        MAC_FreeString(path);
        for (fext = file + strlen(file);
            (fext != file) && (*fext != '/'); fext--);
        if (fext != file)
            *fext++ = 0;
        else {
            free(file);
            file = 0;
        }
    }
    if (file) {
        path = MAC_MakeString(file);
        idir = (NSURL*)CFURLCreateWithFileSystemPath
               (kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, true);
        setDirectoryURL_(cfdp, idir);
        CFRelease(idir);
        MAC_FreeString(path);
        if (fext) {
            setNameFieldStringValue_(cfdp, path = MAC_MakeString(fext));
            MAC_FreeString(path);
        }
        free(file);
    }
    fext = 0;
    if (runModal(cfdp) == NSFileHandlingPanelOKButton) {
        path = (NSString*)CFURLCopyFileSystemPath
                              (CFArrayGetValueAtIndex(URLs(cfdp), 0),
                               kCFURLPOSIXPathStyle);
        fext = MAC_LoadString(path);
        MAC_FreeString(path);
    }
    return fext;
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
    sem_post(dthr->isem);
    return 0;
}



intptr_t rMakeParallel(UPRE func, long size) {
    static unsigned long indx = 0;
    char name[64];
    DTHR *dthr;

    size *= sysconf(_SC_NPROCESSORS_ONLN);
    *(dthr = calloc(1, sizeof(*dthr))) = (DTHR){(size > 1)? size : 1, func};
    sprintf(name, "sem%ld", ++indx);
    dthr->isem = sem_open(name, O_CREAT | O_EXCL, 0, size);
    sem_unlink(name);
    if (dthr->isem == SEM_FAILED) {
        free(dthr);
        dthr = 0;
    }
    return (intptr_t)dthr;
}



void rLoadParallel(intptr_t user, intptr_t data) {
    DTHR *dthr = (DTHR*)user;
    pthread_t pthr;
    intptr_t *pass;

    pass = calloc(2, sizeof(intptr_t));
    pass[0] = user;
    pass[1] = data;
    sem_wait(dthr->isem);
    pthread_create(&pthr, 0, ParallelFunc, pass);
}



void rFreeParallel(intptr_t user) {
    DTHR *dthr = (DTHR*)user;

    for (; dthr->size; dthr->size--)
        sem_wait(dthr->isem);
    sem_close(dthr->isem);
    free(dthr);
}



size_t WriteHTTPS(char *cptr, size_t size, size_t memb, void *user) {
    intptr_t *data = (intptr_t*)user;

    data[1] = (intptr_t)realloc((char*)data[1], 1 + data[0] + (size *= memb));
    memcpy((char*)data[1] + data[0], cptr, size);
    ((char*)data[1])[data[0] += size] = 0;
    return size;
}



void rFreeHTTPS(intptr_t user) {
    free(((char**)user)[0]);
    free(((char**)user)[1]);
    free((char**)user);
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



long rLoadHTTPS(intptr_t user, char *page, char **dest) {
    char *hreq, **ctxt = (char**)user;
    intptr_t data[2] = {};
    CURL *curl;

    if (!page || !dest) {
        *dest = 0;
        return 0;
    }
    curl = curl_easy_init();
    hreq = calloc(1, 1 + strlen(ctxt[1]) + strlen(page));
    curl_easy_setopt(curl, CURLOPT_URL, strcat(strcat(hreq, ctxt[1]), page));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ctxt[0]);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteHTTPS);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    if (curl_easy_perform(curl)) {
        data[1] = (intptr_t)realloc((char*)data[1], 0);
        data[1] = data[0] = 0;
    }
    curl_easy_cleanup(curl);
    free(hreq);
    *dest = (char*)data[1];
    return data[0];
}



void TmrFunc(CFRunLoopTimerRef tmrp, void *user) {
    intptr_t *data = (intptr_t*)user;
    struct timeval spec = {};
    uint64_t time;

    gettimeofday(&spec, 0);
    time = spec.tv_sec * 1000 + spec.tv_usec / 1000;
    ((UPRE)data[0])(data[1], time);
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre, intptr_t data) {
    intptr_t user[2] = {(intptr_t)upre, data};
    CFRunLoopTimerRef tmrp;

    tmrp = MAC_MakeTimer(fram, TmrFunc, user);
    while (root->priv[2])
        run(sharedApplication(NSApplication()));
    MAC_FreeTimer(tmrp);
}



NSSize GetStringSize(NSString *text, NSSize area) {
    NSLayoutManager *mlay;
    NSTextContainer *ctxt;
    NSTextStorage *stor;
    NSRect retn;

    mlay = init(alloc(NSLayoutManager()));
    stor = initWithString_(alloc(NSTextStorage()), text);
    ctxt = initWithContainerSize_(alloc(NSTextContainer()),
                                 (NSSize){area.width, CGFLOAT_MAX});
    setLineFragmentPadding_(ctxt, 0.0);
    addTextContainer_(mlay, ctxt);
    addLayoutManager_(stor, mlay);
    addAttribute_value_range_(stor, NSFontAttributeName,
        (id)systemFontOfSize_(NSFont(), systemFontSize(NSFont())),
            CFRangeMake(0, CFStringGetLength(text)));
    glyphRangeForTextContainer_(mlay, ctxt);
    retn = usedRectForTextContainer_(mlay, ctxt);
    release(stor);
    release(mlay);
    release(ctxt);
    return retn.size;
}

void MoveControl(CTRL *ctrl, intptr_t data) {
    long xpos = ((long*)data)[0], ypos = ((long*)data)[1];
    CTRL *root = ctrl;
    NSRect rect;

    while (root->prev)
        root = root->prev;
    rect = frame((NSView*)ctrl->priv[0]);
    rect.origin.x = (xpos < 0)? -xpos : xpos * (uint16_t)(root->priv[2]      );
    rect.origin.y = (ypos < 0)? -ypos : ypos * (uint16_t)(root->priv[2] >> 16);
    setFrame_((NSView*)ctrl->priv[0], rect);
}

bool MAC_Handler(OnValidate, NSString *part, NSString *retn, NSString *desc) {
    void *temp;

    if (getObjectValue_forString_errorDescription_(self, &temp, part, desc))
        return true;
    NSBeep();
    return false;
}

void MAC_Handler(OnSpin) {
    double retn = 0.0;
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    if (!ctrl)
        return;

    retn = doubleValue((NSStepper*)ctrl->priv[6]);
    setIntegerValue_((NSTextField*)ctrl->priv[7], retn);
    ctrl->fc2e(ctrl, MSG_NSET, retn);
}

void MAC_Handler(OnEdit) {
    CTRL *ctrl = 0;
    double retn;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
        return;

    retn = doubleValue((NSTextField*)ctrl->priv[7]);
    ctrl->fe2c(ctrl, MSG_NSET, retn);
}

bool MAC_Handler(OnKeys, void *ctrl, NSView *view, SEL what) {
    static SEL MoveUp = 0, MoveDown = 0;

    if (!MoveUp) {
        MoveUp = moveUp_();
        MoveDown = moveDown_();
    }
    if ((what == MoveDown) || (what == MoveUp)) {
        CTRL *ctrl = 0;
        double retn;

        MAC_GetIvar(self, VAR_DATA, &ctrl);
        if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
            return false;

        retn = doubleValue((NSTextField*)ctrl->priv[7]);
        ctrl->fe2c(ctrl, MSG_NSET, retn + ((what == MoveUp)? 1.0 : -1.0));
        return true;
    }
    return false;
}



/// PRIV:
///  0: NSWindow
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (fontmul.x) | (fontmul.y << 16), and also a halt flag if 0
///  3: first tab-enabled control
///  4: last tab-enabled control
///  5: NSNumberFormatter for spin controls
///  6: SCLS subclass storage
///  7: NSView, the main container and delegate
intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW: {
            NSApplication *thrd = sharedApplication(NSApplication());

            if (!data)
                orderOut_((NSWindow*)ctrl->priv[0], thrd);
            else {
                activateIgnoringOtherApps_(thrd, true);
                orderFront_((NSWindow*)ctrl->priv[0], thrd);
                makeKeyWindow((NSWindow*)ctrl->priv[0]);
            }
            break;
        }
        case MSG__TXT: {
            NSString *capt;

            capt = MAC_MakeString((char*)data);
            setTitle_((NSWindow*)ctrl->priv[0], capt);
            MAC_FreeString(capt);
            break;
        }
        case MSG_WSZC: {
            NSRect area, scrn;

            scrn.size.width  = (((uint16_t)(data      )) + ctrl->xdim)
                             *   (uint16_t)(ctrl->priv[2]      );
            scrn.size.height = (((uint16_t)(data >> 16)) + ctrl->ydim)
                             *   (uint16_t)(ctrl->priv[2] >> 16);
            ctrl->priv[1] =  (uint16_t)scrn.size.width
                          | ((uint32_t)scrn.size.height << 16);
            area = frameRectForContentRect_((NSWindow*)ctrl->priv[0], scrn);
            scrn = visibleFrame(mainScreen(NSScreen()));
            area.origin.x = 0.5 * (scrn.size.width  - area.size.width )
                          + scrn.origin.x;
            area.origin.y = 0.5 * (scrn.size.height - area.size.height)
                          + scrn.origin.y;
            setFrame_display_animate_((NSWindow*)ctrl->priv[0],
                                      area, true, false);
            setMinSize_((NSWindow*)ctrl->priv[0], area.size);
            break;
        }
        case MSG_WTDA: {
            const double mrgn = 1.0; /// text margins; [TODO:] get in RT
            AINF *anim = (AINF*)data;
            CGContextRef ctxt;
            CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
            NSAutoreleasePool *pool = init(alloc(NSAutoreleasePool()));
            NSMutableParagraphStyle *psty;
            NSDictionary *dict;
            NSString *text;
            NSImage *itmp;
            NSFont *font;

            NSRect rect = {{(uint8_t)(anim->xdim >> 16) - mrgn,
                            (uint8_t)(anim->ydim >> 16) - mrgn}};

            rect.size.width  = (uint16_t)anim->xdim - rect.origin.x
                             - (uint8_t)(anim->xdim >> 24) + mrgn;
            rect.size.height = (uint16_t)anim->ydim - rect.origin.y
                             - (uint8_t)(anim->ydim >> 24) + mrgn;
            rect.origin.y = (uint16_t)anim->ydim
                          - rect.origin.y - rect.size.height;
            lockFocus(itmp = initWithSize_(alloc(NSImage()), rect.size));
            font = systemFontOfSize_(NSFont(), systemFontSize(NSFont()));
            psty = init(alloc(NSMutableParagraphStyle()));
            setAlignment_(psty, NSCenterTextAlignment);
            text = MAC_MakeString((char*)anim->time);
            dict = MAC_MakeDict(NSFontAttributeName, font,
                                NSParagraphStyleAttributeName, psty);
            drawInRect_withAttributes_(text, (NSRect){{}, rect.size}, dict);
            MAC_FreeString(text);
            MAC_FreeDict(dict);
            unlockFocus(itmp);
            release(psty);
            ctxt = CGBitmapContextCreate
                       ((void*)anim->uuid, (uint16_t)anim->xdim,
                        (uint16_t)anim->ydim, CHAR_BIT,
                        (uint16_t)anim->xdim * sizeof(uint32_t),
                         drgb, kCGImageAlphaPremultipliedFirst
                             | kCGBitmapByteOrder32Little);
            CGContextDrawImage(ctxt, rect,
                CGImageForProposedRect_context_hints_(itmp, 0, 0, 0));
            CGContextRelease(ctxt);
            CGColorSpaceRelease(drgb);
            release(itmp);
            release(pool);
            break;
        }
        case MSG_WTGD: {
            AINF *anim = (AINF*)data;
            NSString *text;
            NSSize size;

            size = GetStringSize(text = MAC_MakeString((char*)anim->time),
                       (NSSize){(anim->xdim > 0)? anim->xdim : CGFLOAT_MAX,
                                (anim->ydim > 0)? anim->ydim : CGFLOAT_MAX});
            MAC_FreeString(text);
            return (uint16_t)round(size.width)
                | ((uint32_t)round(size.height) << 16);
        }
    }
    return 0;
}



/// PRIV:
///  0: NSProgressIndicator
///  1: progress position
///  2: progress maximum
///  3: NSString with label text
///  4: vertical offset of the label
///  5: NSDictionary with centering attributes and font
///  6:
///  7:
intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PPOS:
            setDoubleValue_((NSProgressIndicator*)ctrl->priv[0],
                             ctrl->priv[1] = data);
            setNeedsDisplay_((NSProgressIndicator*)ctrl->priv[0], true);
            displayIfNeeded((NSProgressIndicator*)ctrl->priv[0]);
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
            break;

        case MSG__TXT:
            if (ctrl->priv[3])
                MAC_FreeString((NSString*)ctrl->priv[3]);
            ctrl->priv[3] = (intptr_t)MAC_MakeString((char*)data);
            break;

        case MSG_PLIM:
            ctrl->priv[2] = data;
            setMaxValue_((NSProgressIndicator*)ctrl->priv[0],
                         (ctrl->priv[2] > 0)? ctrl->priv[2] : 1);
            break;

        case MSG_PGET:
            return (data)? ctrl->priv[2] : ctrl->priv[1];
    }
    return 0;
}



/// PRIV:
///  0: NSButton
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CX(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__TXT: {
            NSString *capt;
            NSRect rect;
            NSSize dims;
            char *temp;
            long prev, size, iter;

            if ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)
                capt = MAC_MakeString((char*)data);
            else {
                rect = frame((NSButton*)ctrl->priv[0]);
                size = strlen((char*)data);
                strcpy(temp = calloc(1, 2 + size), (char*)data);
                temp[size] = ' ';
                for (prev = size = iter = 0; temp[iter]; iter++)
                    if (temp[iter] == ' ') {
                        temp[iter] = 0;
                        capt = MAC_MakeString(&temp[prev]);
                        dims = GetStringSize(capt, (NSSize){CGFLOAT_MAX,
                                                            CGFLOAT_MAX});
                        MAC_FreeString(capt);
                        temp[iter] = ' ';
                        if (size + dims.width >= rect.size.width) {
                            if (prev)
                                temp[prev] = '\n';
                            size = 0;
                        }
                        prev = iter;
                        size += dims.width;
                    }
                temp[iter - 1] = 0;
                capt = MAC_MakeString(temp);
                free(temp);
            }
            setTitle_((NSButton*)ctrl->priv[0], capt);
            MAC_FreeString(capt);
            break;
        }
        case MSG__ENB:
            setEnabled_((NSButton*)ctrl->priv[0], !!data);
            break;

        case MSG_BGST:
            return ((isEnabled((NSButton*)ctrl->priv[0]))? FCS_ENBL : 0)
                 | ((state((NSButton*)ctrl->priv[0]) == NSOnState)?
                     FCS_MARK : 0);

        case MSG_BCLK:
            cmsg = (state((NSButton*)ctrl->priv[0]) == NSOnState);
            setState_((NSButton*)ctrl->priv[0],
                      (data)? NSOnState : NSOffState);
            return !!cmsg;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1: NSButton subclass
///  2: either a data cell (< 10.7) or an array of NSButtons (>= 10.7)
///  3:
///  4: number of rows
///  5: array: [item1 caption NSString, [ctrl pointer, item1 ID], item2...]
///  6: NSTableColumn
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    NSString *capt;
    NSView *elem;
    CTRL **temp;

    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((NSTableView*)ctrl->priv[7], !!data);
            if (MAC_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setEnabled_(((NSView**)ctrl->priv[2])[cmsg], !!data);
            break;

        case MSG__TXT:
            setStringValue_(headerCell((NSTableColumn*)ctrl->priv[6]),
                            capt = MAC_MakeString((char*)data));
            MAC_FreeString(capt);
            if (MAC_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setState_(((NSView**)ctrl->priv[2])[cmsg],
                             (ctrl->fc2e(ctrl, MSG_LGST, cmsg))?
                              NSOnState : NSOffState);
            reloadData((NSTableView*)ctrl->priv[7]);
            break;

        case MSG_LADD:
            ctrl->priv[5] = (intptr_t)
                realloc((NSString**)ctrl->priv[5],
                       ++ctrl->priv[4] * 2 * sizeof(NSString*));
            *(temp = calloc(2, sizeof(CTRL*))) = ctrl;
            temp[1] = (CTRL*)(ctrl->priv[4] - 1);
            ((CTRL***)ctrl->priv[5])[ctrl->priv[4] * 2 - 1] = temp;
            ((NSString**)ctrl->priv[5])[ctrl->priv[4] * 2 - 2] =
                MAC_MakeString((char*)data);
            if (MAC_10_07_PLUS) {
                ctrl->priv[2] = (intptr_t)
                    realloc((NSView**)ctrl->priv[2],
                             ctrl->priv[4] * sizeof(NSView**));
                elem = init(alloc((NSButton*)ctrl->priv[1]));
                MAC_SetIvar(elem, VAR_DATA, temp);
                setTarget_(elem, elem);
                setAction_(elem, ActionSelector());
                setButtonType_(elem, NSSwitchButton);
                ((NSView**)ctrl->priv[2])[ctrl->priv[4] - 1] = elem;
            }
            else if (!ctrl->priv[2]) {
                ctrl->priv[2] = (intptr_t)
                    init(alloc((NSButton*)ctrl->priv[1]));
                setButtonType_((NSButton*)ctrl->priv[2], NSSwitchButton);
                setDataCell_((NSTableColumn*)ctrl->priv[6],
                             (NSCell*)ctrl->priv[2]);
            }
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSView
///  1: minimum value
///  2: maximum value
///  3: common NSNumberFormatter
///  4:
///  5:
///  6: NSStepper
///  7: NSTextField
intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            NSRect rect;

            rect = frame((NSView*)ctrl->priv[0]);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((NSView*)ctrl->priv[0], !data);
            break;

        case MSG__ENB:
            setEnabled_((NSStepper*)ctrl->priv[6], !!data);
            setEnabled_((NSTextField*)ctrl->priv[7], !!data);
            break;

        case MSG_NGET:
            return doubleValue((NSStepper*)ctrl->priv[6]);

        case MSG_NSET:
            data = (data > ctrl->priv[1])? data : ctrl->priv[1];
            data = (data < ctrl->priv[2])? data : ctrl->priv[2];
            setDoubleValue_((NSStepper*)ctrl->priv[6], data);
            OnSpin((NSStepper*)ctrl->priv[6], 0);
            break;

        case MSG_NDIM:
            ctrl->priv[1] = (int16_t)data;
            ctrl->priv[2] = (int16_t)(data >> 16);
            setMinValue_((NSStepper*)ctrl->priv[6], ctrl->priv[1]);
            setMaxValue_((NSStepper*)ctrl->priv[6], ctrl->priv[2]);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSView container (for centering)
///  1: NSTextField
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CT(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            NSRect rect;

            rect = frame((NSView*)ctrl->priv[0]);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__TXT: {
            NSRect rect, real = {{}, {CGFLOAT_MAX, CGFLOAT_MAX}};
            NSString *capt;

            capt = MAC_MakeString((char*)data);
            setStringValue_((NSTextField*)ctrl->priv[1], capt);
            real.size = GetStringSize(capt, real.size);
            MAC_FreeString(capt);
            real.size.width *= 2;  /// to guarantee that the string will fit
            real.size.height += 2; /// why 2? no idea! [TODO:] compute in RT
            rect = frame((NSView*)ctrl->priv[0]);
            if (ctrl->flgs & FST_CNTR)
                real.origin.x = 0.5 * (rect.size.width - real.size.width);
            real.origin.y = 0.5 * (rect.size.height - real.size.height);
            setFrame_((NSTextField*)ctrl->priv[1], real);
            break;
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;
        case MSG__ENB:
            setTextColor_((NSTextField*)ctrl->priv[1],
                          (data)? controlTextColor(NSColor())
                                : disabledControlTextColor(NSColor()));
            break;

        case MSG__SHW:
            setHidden_((NSView*)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (wndmove.x) | (wndmove.y << 16)
///  3: total height
///  4:
///  5:
///  6:
///  7: NSView, the sizeable container
intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    CGFloat temp;
    NSRect rect;

    switch (cmsg) {
        case MSG__GSZ:
            return ctrl->priv[1];

        case MSG_WSZC:
            if ((ctrl->prev->flgs & FCT_TTTT) != FCT_WNDW)
                return -1;

            rect.origin.x = (uint16_t)(ctrl->priv[2]      );
            rect.origin.y = (uint16_t)(ctrl->priv[2] >> 16);
            if (!data) {
                rect.size.width  = (uint16_t)(ctrl->priv[1]      );
                rect.size.height = (uint16_t)(ctrl->priv[1] >> 16);
            }
            else {
                rect.size.width  = (uint16_t)(data      ) - rect.origin.x
                                 - ctrl->prev->xdim
                                 * (uint16_t)(ctrl->prev->priv[2]      );
                rect.size.height = (uint16_t)(data >> 16) - rect.origin.y
                                 - ctrl->prev->ydim
                                 * (uint16_t)(ctrl->prev->priv[2] >> 16);
                ctrl->priv[1] =  (uint16_t)rect.size.width
                              | ((uint32_t)rect.size.height << 16);
            }
            setFrame_((NSScrollView*)ctrl->priv[0], rect);
            if (ctrl->fc2e) {
                rect = frame(verticalScroller((NSScrollView*)ctrl->priv[0]));
                cmsg = ctrl->priv[1] - rect.size.width;
                temp = (ctrl->priv[3])?
                    (CGFloat)ctrl->fe2c(ctrl, MSG_SGIP, 0) / ctrl->priv[3] : 0;
                ctrl->priv[3] = ctrl->fc2e(ctrl, MSG_SSID, cmsg);
                setFrame_((NSView*)ctrl->priv[7],
                          (NSRect){{0, 0}, {(uint16_t)cmsg, ctrl->priv[3]}});
                ctrl->fc2e(ctrl, MSG_SGIP, temp *= ctrl->priv[3]);
                scrollToPoint_(contentView((NSScrollView*)ctrl->priv[0]),
                              (NSPoint){0, temp});
            }
            setNeedsDisplay_((NSScrollView*)ctrl->priv[0], true);
            break;

        case MSG_SGIP:
            rect = documentVisibleRect
                       (contentView((NSScrollView*)ctrl->priv[0]));
            return rect.origin.y;

        case MSG_SGTH:
            return ctrl->priv[3];

        case MSG__SHW:
            setHidden_((NSScrollView*)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSView
///  1: CGContextRef
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
            setHidden_((NSView*)ctrl->priv[0], !data);
            break;

        case MSG_IFRM:
            ctrl->priv[7] = data;
            setNeedsDisplay_((NSView*)ctrl->priv[0], true);
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW: {
            /// releasing the subclass storage
            free((SCLS*)ctrl->priv[6]);
            /// releasing the formatter
            release((NSNumberFormatter*)ctrl->priv[5]);
            /// we must release neither the window (0) nor its view (7);
            /// if we do, the program segfaults, don`t know why exactly
            /// [TODO:] find out why
            ctrl->priv[0] = 0;
            break;
        }
        case FCT_TEXT:
            break;

        case FCT_BUTN:
            break;

        case FCT_CBOX:
            break;

        case FCT_SPIN:
            release((NSStepper*)ctrl->priv[6]);
            release((NSTextField*)ctrl->priv[7]);
            /// do not release the formatter, it is common between all spins
            break;

        case FCT_LIST:
            for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--) {
                if (MAC_10_07_PLUS) /// releasing per-item checkboxes
                    release(((NSView**)ctrl->priv[2])[ctrl->priv[4]]);
                /// releasing strings
                MAC_FreeString(((NSString**)ctrl->priv[5])[ctrl->priv[4] * 2]);
                /// releasing per-item arrays of [ctrl pointer, item ID]
                free(((CTRL**)ctrl->priv[5])[ctrl->priv[4] * 2 + 1]);
            }
            if (MAC_10_07_PLUS)
                free((NSView**)ctrl->priv[2]);
            else
                release((NSCell*)ctrl->priv[2]);
            free((NSString**)ctrl->priv[5]);
            release((NSTableColumn*)ctrl->priv[6]);
            release((NSTableView*)ctrl->priv[7]);
            break;

        case FCT_SBOX:
            removeObserver_name_object_
                (defaultCenter(NSNotificationCenter()),
                (id)ctrl->priv[7], NSViewBoundsDidChangeNotification,
                (id)contentView((NSScrollView*)ctrl->priv[0]));
            release((NSView*)ctrl->priv[7]);
            break;

        case FCT_IBOX:
            /// releasing the image context and freeing its backbuffer
            CGContextRelease((CGContextRef)ctrl->priv[1]);
            free((void*)ctrl->priv[2]);
            break;

        case FCT_PBAR:
            /// releasing text attributes and the string
            MAC_FreeDict((NSDictionary*)ctrl->priv[5]);
            if (ctrl->priv[3])
                MAC_FreeString((NSString*)ctrl->priv[3]);
            break;
    }
    if (ctrl->priv[0])
        release((NSObject*)ctrl->priv[0]);
}



void Reset(CTRL *ctrl, NSInteger irow) {
    ctrl->fc2e(ctrl, MSG_LSST,
              (irow << 1) | (ctrl->fc2e(ctrl, MSG_LGST, irow) ^ 1));
}

void MAC_Handler(OnReset, NSTableView *view, void *what,
                 NSTableColumn *icol, NSInteger irow) {
    CTRL *ctrl = 0;

    MAC_GetIvar(view, VAR_DATA, &ctrl);
    Reset(ctrl, irow);
}

void MAC_Handler(OnListButton) {
    CTRL **ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    Reset(ctrl[0], (NSInteger)ctrl[1]);
}

void MAC_Handler(OnButton) {
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    ctrl->fc2e(ctrl, MSG_BCLK,
              ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
              (state((NSButton*)ctrl->priv[0]) == NSOnState) : 0);
}

NSCell *MAC_Handler(OnValueOld, NSTableView *view,
                    NSTableColumn *icol, NSInteger irow) {
    return (MAC_10_07_PLUS)? 0 : dataCell(icol);
}

NSCell *MAC_Handler(OnValue, NSTableView *view,
                NSTableColumn *icol, NSInteger irow) {
    NSCell *cell;
    CTRL *ctrl = 0;

    MAC_GetIvar(view, VAR_DATA, &ctrl);
    cell = (MAC_10_07_PLUS)?
           ((NSCell**)ctrl->priv[2])[irow] : dataCell(icol);
    setTitle_(cell, ((NSString**)ctrl->priv[5])[irow * 2]);
    setState_(cell, (ctrl->fc2e(ctrl, MSG_LGST, irow))?
                     NSOnState : NSOffState);
    return cell;
}

NSInteger MAC_Handler(OnRows, NSTableView *view) {
    CTRL *ctrl = 0;

    MAC_GetIvar(view, VAR_DATA, &ctrl);
    return ctrl->priv[4];
}

void MAC_Handler(OnScroll, NSNotification *nscr) {
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    ctrl->fc2e(ctrl, MSG_SGIP, ctrl->fe2c(ctrl, MSG_SGIP, 0));
}

void MAC_Handler(PBoxDraw, NSRect rect) {
    struct objc_super prev = {self, class(NSProgressIndicator())};
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    objc_msgSendSuper(&prev, drawRect_(), rect);
    rect = frame((NSProgressIndicator*)ctrl->priv[0]);
    rect.origin.y = ctrl->priv[4];
    rect.origin.x = 0;
#ifndef MAC_OLD
    if (MAC_10_10_PLUS) {
        rect.origin.y *= 0.25;
        rect.size.width *= 0.5;
        rect.size.height *= 0.5;
    }
#endif
    drawInRect_withAttributes_((void*)ctrl->priv[3], rect,
                               (NSDictionary*)ctrl->priv[5]);
}

void MAC_Handler(IBoxDraw, NSRect rect) {
    CGImageRef pict;
    NSRect area;
    AINF anim;
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    if (!ctrl)
        return;
    anim = (AINF){(ctrl->priv[7] >> 10) & 0x3FFFFF,
                  (int16_t)ctrl->priv[3], (int32_t)ctrl->priv[3] >> 16,
                   ctrl->priv[7] & 0x3FF, (uint32_t*)ctrl->priv[2]};
    if (!anim.uuid)
        return;
    area = (NSRect){{0, 0}, {anim.xdim, anim.ydim}};
    CGContextFillRect((CGContextRef)ctrl->priv[1], area);
    CGContextFlush((CGContextRef)ctrl->priv[1]);
    ctrl->fc2e(ctrl, MSG_IFRM, (intptr_t)&anim);
    pict = CGBitmapContextCreateImage((CGContextRef)ctrl->priv[1]);
    CGContextDrawImage(graphicsPort(currentContext(NSGraphicsContext())),
                       area, pict);
    CGImageRelease(pict);
}

bool MAC_Handler(OnFalse) {
    return false;
}

bool MAC_Handler(OnTrue) {
    return true;
}

bool MAC_Handler(OnClose) {
    bool retn = false;
    CTRL *ctrl = 0;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    if (ctrl->fc2e(ctrl, MSG_WEND, 0)) {
        retn = true;
        ctrl->priv[2] = 0;
        stop_(sharedApplication(NSApplication()), self);
    }
    return retn;
}

void MAC_Handler(OnSize) {
    CTRL *ctrl = 0;
    NSRect rect;

    MAC_GetIvar(self, VAR_DATA, &ctrl);
    if (!ctrl)
        return;

    rect = frame((NSView*)ctrl->priv[0]);
    rect = contentRectForFrameRect_((NSView*)ctrl->priv[0], rect);
    ctrl->priv[1] =  (uint16_t)rect.size.width
                  | ((uint32_t)rect.size.height << 16);
    ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff) {
    CTRL *root;
    SCLS *scls;
    NSString *capt;
    NSRect dims;
    void *gwnd;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        NSInteger flgs;
        CGFloat ffsz;

        ctrl->fe2c = FE2CW;
        gwnd = systemFontOfSize_(NSFont(), ffsz = systemFontSize(NSFont()));
        ctrl->priv[2] =  (uint16_t)round(0.45 * maximumAdvancement(gwnd).width)
                      | ((uint32_t)round(0.60 * ffsz) << 16);

        ctrl->priv[6] = (intptr_t)(scls = calloc(1, sizeof(*scls)));
        scls->cell = MAC_LoadClass(CLS_CELL);
        scls->wndw = MAC_LoadClass(CLS_WNDW);
        scls->text = MAC_LoadClass(CLS_TEXT);
        scls->butn = MAC_LoadClass(CLS_BUTN);
        scls->spin = MAC_LoadClass(CLS_SPIN);
        scls->list = MAC_LoadClass(CLS_LIST);
        scls->pbar = MAC_LoadClass(CLS_PBAR);
        scls->sbox = MAC_LoadClass(CLS_SBOX);
        scls->ibox = MAC_LoadClass(CLS_IBOX);
        scls->frmt = MAC_LoadClass(CLS_FRMT);

        ctrl->priv[5] = (intptr_t)init(alloc(scls->frmt));
        setFormatterBehavior_((NSNumberFormatter*)ctrl->priv[5],
                               NSNumberFormatterBehavior10_4);
        setNumberStyle_((NSNumberFormatter*)ctrl->priv[5],
                         kCFNumberFormatterNoStyle);
        setPartialStringValidationEnabled_((NSNumberFormatter*)ctrl->priv[5],
                                            true);
        dims = (NSRect){};
        flgs = NSClosableWindowMask | NSTitledWindowMask
             | ((ctrl->flgs & FSW_SIZE)? NSMiniaturizableWindowMask
                                       | NSResizableWindowMask : 0);
        gwnd = initWithContentRect_styleMask_backing_defer_
                   (alloc(NSWindow()), dims, flgs,
                    kCGBackingStoreBuffered, false);

        ctrl->priv[7] = (intptr_t)init(alloc(scls->wndw));
        setContentView_(gwnd, (NSView*)ctrl->priv[7]);
        setDelegate_(gwnd, (NSView*)ctrl->priv[7]);
        MAC_SetIvar((NSView*)ctrl->priv[7], VAR_DATA, ctrl);
    }
    else if (root) {
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
        dims = (NSRect){{xpos * xspc, ypos * yspc}, {xdim, ydim}};
        scls = (SCLS*)root->priv[6];

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT: {
                NSRect temp = {{}, dims.size};

                ctrl->fe2c = FE2CT;
                ctrl->priv[1] = (intptr_t)(gwnd = init(alloc(scls->text)));
                MAC_SetIvar(gwnd, VAR_DATA, 0);
                setFrame_(gwnd, temp);
                setEditable_(gwnd, false);
                setBordered_(gwnd, false);
                setSelectable_(gwnd, false);
                setDrawsBackground_(gwnd, !!(ctrl->flgs & FST_SUNK));
                setScrollable_(cell(gwnd), true); /// to keep trailing spaces
                setAlignment_(cell(gwnd),
                             (ctrl->flgs & FST_CNTR)? NSCenterTextAlignment
                                                    : NSLeftTextAlignment);
                addSubview_(gwnd = init(alloc(NSView())),
                           (NSView*)ctrl->priv[1]);
                break;
            }
            case FCT_BUTN:
                ctrl->fe2c = FE2CX;
                gwnd = init(alloc(scls->butn));
                MAC_SetIvar(gwnd, VAR_DATA, ctrl);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ActionSelector());
                setButtonType_(gwnd, NSMomentaryLightButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                if ((ctrl->flgs & FSB_DFLT) && ctrl->prev
                && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
                    setInitialFirstResponder_((NSWindow*)ctrl->prev->priv[0],
                                               gwnd);
                    setDefaultButtonCell_((NSWindow*)ctrl->prev->priv[0],
                                          (NSButtonCell*)cell(gwnd));
                }
                break;

            case FCT_CBOX:
                ctrl->fe2c = FE2CX;
                gwnd = init(alloc(scls->butn));
                MAC_SetIvar(gwnd, VAR_DATA, ctrl);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ActionSelector());
                setButtonType_(gwnd, NSSwitchButton);
                setImagePosition_(gwnd, (ctrl->flgs & FSX_LEFT)?
                                         NSImageRight : NSImageLeft);
                break;

            case FCT_SPIN: {
                NSRect temp = {};
                NSSize spin;

                ctrl->fe2c = FE2CN;
                gwnd = init(alloc(NSView()));
                ctrl->priv[3] = root->priv[5];
                ctrl->priv[6] = (intptr_t)init(alloc(scls->spin));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->text));
                MAC_SetIvar((NSStepper*)ctrl->priv[6], VAR_DATA, ctrl);
                MAC_SetIvar((NSTextField*)ctrl->priv[7], VAR_DATA, ctrl);
                spin = cellSize(cell((NSStepper*)ctrl->priv[6]));
                temp.size = dims.size;
                temp.size.width -= spin.width;
                setFrame_((NSTextField*)ctrl->priv[7], temp);
                setSendsActionOnEndEditing_
                    (cell((NSTextField*)ctrl->priv[7]), true);
                setFormatter_((NSTextField*)ctrl->priv[7],
                              (NSNumberFormatter*)ctrl->priv[3]);
                setDelegate_((NSTextField*)ctrl->priv[7],
                             (NSTextField*)ctrl->priv[7]);
                setTarget_((NSTextField*)ctrl->priv[7],
                           (NSTextField*)ctrl->priv[7]);
                setAction_((NSTextField*)ctrl->priv[7], ActionSelector());
                temp.origin.x = temp.size.width;
                temp.size.width = spin.width;
                setFrame_((NSStepper*)ctrl->priv[6], temp);
                setValueWraps_((NSStepper*)ctrl->priv[6], false);
                setTarget_((NSStepper*)ctrl->priv[6],
                           (NSStepper*)ctrl->priv[6]);
                setAction_((NSStepper*)ctrl->priv[6], ActionSelector());
                addSubview_(gwnd, (NSView*)ctrl->priv[6]);
                addSubview_(gwnd, (NSView*)ctrl->priv[7]);
                break;
            }
            case FCT_LIST: {
                NSRect temp = {};

                ctrl->fe2c = FE2CL;
                temp.size = dims.size;
                gwnd = init(alloc(NSScrollView()));
                ctrl->priv[1] = (intptr_t)scls->cell;
                ctrl->priv[7] = (intptr_t)init(alloc(scls->list));
                MAC_SetIvar((NSTableView*)ctrl->priv[7], VAR_DATA, ctrl);
                setDataSource_((NSTableView*)ctrl->priv[7],
                               (NSTableView*)ctrl->priv[7]);
                setDelegate_((NSTableView*)ctrl->priv[7],
                             (NSTableView*)ctrl->priv[7]);
                setFrame_((NSTableView*)ctrl->priv[7], temp);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn()));
                setStringValue_(headerCell((NSTableColumn*)ctrl->priv[6]),
                                capt = MAC_MakeString(0));
                MAC_FreeString(capt);
                addTableColumn_((NSTableView*)ctrl->priv[7],
                                (NSTableColumn*)ctrl->priv[6]);
                setResizingMask_((NSTableColumn*)ctrl->priv[6],
                                  NSTableColumnAutoresizingMask);

                setDocumentView_(gwnd, (NSView*)ctrl->priv[7]);
                setHasVerticalScroller_(gwnd, true);
                break;
            }
            case FCT_SBOX:
                ctrl->fe2c = FE2CS;
                ctrl->priv[1] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                ctrl->priv[2] =  (uint16_t)dims.origin.x
                              | ((uint32_t)dims.origin.y << 16);
                gwnd = init(alloc(NSScrollView()));
                setDrawsBackground_(gwnd, false);
                setHasVerticalScroller_(gwnd, true);
                setPostsBoundsChangedNotifications_(contentView(gwnd), true);
                ctrl->priv[7] = (intptr_t)init(alloc(scls->sbox));
                MAC_SetIvar((NSView*)ctrl->priv[7], VAR_DATA, ctrl);
                setDocumentView_(gwnd, (NSView*)ctrl->priv[7]);
                addObserver_selector_name_object_
                    (defaultCenter(NSNotificationCenter()), (id)ctrl->priv[7],
                     ActionSelector(), NSViewBoundsDidChangeNotification,
                    (id)contentView(gwnd));
                break;

            case FCT_IBOX: {
                CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
                long line = dims.size.width * 4;

                ctrl->fe2c = FE2CI;
                gwnd = init(alloc(scls->ibox));
                MAC_SetIvar(gwnd, VAR_DATA, ctrl);
                ctrl->priv[3] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                ctrl->priv[2] = (uintptr_t)malloc(line * dims.size.height);
                ctrl->priv[1] = (uintptr_t)CGBitmapContextCreate
                    ((void*)ctrl->priv[2], dims.size.width, dims.size.height,
                      CHAR_BIT, line, drgb, kCGImageAlphaPremultipliedFirst
                                          | kCGBitmapByteOrder32Little);
                CGContextSetBlendMode((CGContextRef)ctrl->priv[1],
                                       kCGBlendModeClear);
                CGColorSpaceRelease(drgb);
                break;
            }
            case FCT_PBAR: {
                CGFloat ffsz;
                NSFont *font;
                NSMutableParagraphStyle *psty;

                ctrl->fe2c = FE2CP;
                gwnd = init(alloc(scls->pbar));
                MAC_SetIvar(gwnd, VAR_DATA, ctrl);
                setIndeterminate_(gwnd, false);
                setWantsLayer_(gwnd, true);
                psty = init(alloc(NSMutableParagraphStyle()));
                setAlignment_(psty, NSCenterTextAlignment);
                ffsz = systemFontSize(NSFont()) * 0.85;
#ifndef MAC_OLD
                if (MAC_10_10_PLUS) {
                    scaleUnitSquareToSize_(gwnd, (NSSize){2.0, 2.0});
                    ffsz *= 0.5;
                }
#endif
                font = systemFontOfSize_(NSFont(), ffsz);
                ctrl->priv[5] = (intptr_t)MAC_MakeDict
                    (NSFontAttributeName, font,
                     NSParagraphStyleAttributeName, psty);
                release(psty);
                ctrl->priv[4] = 0.5 * (dims.size.height - ffsz) - 1.0;
                break;
            }
        }
        setFrame_(gwnd, dims);
        addSubview_((NSView*)ctrl->prev->priv[7], gwnd);
        if (ctrl->prev && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
            NSView *resp; /// tab-stop responder

            switch (ctrl->flgs & FCT_TTTT) {
                case FCT_TEXT:
                case FCT_PBAR:
                case FCT_SBOX: resp = 0; break;
                case FCT_SPIN: resp = (NSView*)ctrl->priv[7]; break;
                default:       resp = gwnd; break;
            }
            if (resp) {
                if (ctrl->prev->priv[4]) {
                    setNextKeyView_((NSView*)ctrl->prev->priv[4], resp);
                    setNextKeyView_(resp, (NSView*)ctrl->prev->priv[3]);
                    ctrl->prev->priv[4] = (intptr_t)resp;
                }
                else {
                    ctrl->prev->priv[3] = ctrl->prev->priv[4] = (intptr_t)resp;
                    /// shall do nothing if there already is a first responder
                    setInitialFirstResponder_((NSWindow*)ctrl->prev->priv[0],
                                               resp);
                }
            }
        }
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}



void lockfunc(int mode, int indx, const char *file, int line) {
    if (mode & 0x01 /** = CRYPTO_LOCK **/)
        pthread_mutex_lock(&pmtx[indx]);
    else
        pthread_mutex_unlock(&pmtx[indx]);
}



int main(int argc, char *argv[]) {
    #define CLS_MAKE(n, p, ...) \
        MAC_MakeClass(n, p, MAC_TempArray(VAR_DATA), \
                            MAC_TempArray(__VA_ARGS__))
    NSAutoreleasePool *pool = init(alloc(NSAutoreleasePool()));
    Class scls[] = {
        CLS_MAKE(CLS_CELL, (MAC_10_07_PLUS)? NSButton() : NSButtonCell(),
                 ActionSelector(), OnListButton),
        CLS_MAKE(CLS_TRAY, NSObject(),
                 ActionSelector(), OnTray),
        CLS_MAKE(CLS_MENU, NSObject(),
                 ActionSelector(), OnMenu),
        CLS_MAKE(CLS_WNDW, NSView(),
                 windowShouldClose_(), OnClose,
                 windowDidResize_(), OnSize, isFlipped(), OnTrue),
        CLS_MAKE(CLS_TEXT, NSTextField(),
                 ActionSelector(), OnEdit,
                 control_textView_doCommandBySelector_(), OnKeys),
        CLS_MAKE(CLS_BUTN, NSButton(),
                 ActionSelector(), OnButton),
        CLS_MAKE(CLS_SPIN, NSStepper(),
                 ActionSelector(), OnSpin),
        CLS_MAKE(CLS_LIST, NSTableView(),
                (MAC_10_07_PLUS)? tableView_viewForTableColumn_row_()
               : tableView_dataCellForTableColumn_row_(),        OnValue,
                 tableView_objectValueForTableColumn_row_(),     OnValueOld,
                 tableView_setObjectValue_forTableColumn_row_(), OnReset,
                 numberOfRowsInTableView_(),                     OnRows),
        CLS_MAKE(CLS_PBAR, NSProgressIndicator(),
                 drawRect_(), PBoxDraw),
        CLS_MAKE(CLS_SBOX, NSView(),
                 ActionSelector(), OnScroll,
                 isFlipped(), OnTrue),
        CLS_MAKE(CLS_IBOX, NSView(),
                 drawRect_(), IBoxDraw),
        CLS_MAKE(CLS_FRMT, NSNumberFormatter(),
                 isPartialStringValid_newEditingString_errorDescription_(),
                 OnValidate)
    };
    void (*CSIC)(pthread_t (*func)()) =
        dlsym(RTLD_DEFAULT, "CRYPTO_set_id_callback");
    void (*CSLC)(void (*func)(int, int, const char*, int)) =
        dlsym(RTLD_DEFAULT, "CRYPTO_set_locking_callback");
    int  (*CNLK)() = dlsym(RTLD_DEFAULT, "CRYPTO_num_locks");
    char *conf, *home;
    NSString *path;
    NSArray *urls;
    long iter, cmtx;
    CGFloat icon;
    NSRect dims;

//    setBool_forKey_(standardUserDefaults(NSUserDefaults()),
//                    false, CFSTR("NSShowNonLocalizedStrings"));

    if (!MAC_10_07_PLUS) {
        pmtx = calloc(cmtx = CNLK(), sizeof(*pmtx));
        for (iter = 0; iter < cmtx; iter++)
            pthread_mutex_init(&pmtx[iter], 0);
        CSIC(pthread_self);
        CSLC(lockfunc);
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    setActivationPolicy_(sharedApplication(NSApplication()),
                         NSApplicationActivationPolicyAccessory);
    home = MAC_LoadString(bundlePath(mainBundle(NSBundle())));
    strcat(home = realloc(home, 64 + strlen(home)), "/Contents/Resources");

    urls = URLsForDirectory_inDomains_(defaultManager(NSFileManager()),
                                       NSApplicationSupportDirectory,
                                       NSUserDomainMask);
    path = (NSString*)CFURLCopyFileSystemPath
           (CFArrayGetValueAtIndex(urls, 0), kCFURLPOSIXPathStyle);
    conf = MAC_LoadString(path);
    if (!rMakeDir(strcat(conf = realloc(conf, 64 + strlen(conf)), DEF_OPTS)))
        printf("WARNING: cannot create '%s'!", conf);
    MAC_FreeString(path);

    dims = visibleFrame(mainScreen(NSScreen()));
    icon = thickness(systemStatusBar(NSStatusBar()));

    eExecuteEngine(conf, home, icon, icon, dims.origin.x, dims.origin.y,
                   dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    release(pool);
    free(conf);
    free(home);
    /// it is crucial to free all subclasses AFTER freeing the release pool
    for (argc = 0; argc < sizeof(scls) / sizeof(*scls); argc++)
        if (scls[argc])
            MAC_FreeClass(scls[argc]);
    curl_global_cleanup();
    if (!MAC_10_07_PLUS) {
        CSLC(0);
        CSIC(0);
        for (iter = 0; iter < cmtx; iter++)
            pthread_mutex_destroy(&pmtx[iter]);
        free(pmtx);
    }
    return 0;
    #undef CLS_MAKE
}
