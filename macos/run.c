#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../exec/exec.h"
#include "mac.h"

/// name of the instance variable to access the CTRL structure
#define VAR_CTRL "c"



void rOpenContextMenu(MENU *menu) {
    /// [TODO:]
}



inline MENU *rOSSpecificMenu(ENGC *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long rMessage(char *text, char *head, uint32_t flgs) {
    /// [TODO:]
    return 0;
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    id sbar = systemStatusBar(NSStatusBar);
    id icon = statusItemWithLength_(sbar, NSVariableStatusItemLength);

    setHighlightMode_(icon, true);
    /// [TODO:]
    return 0;
}



void rFreeTrayIcon(intptr_t icon) {
    /// [TODO:]
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



void TmrFunc(CFRunLoopTimerRef tmrp, void *user) {
    intptr_t *data = (intptr_t*)user;
    struct timeval spec = {};
    uint64_t time;

    gettimeofday(&spec, 0);
    time = spec.tv_sec * 1000 + spec.tv_usec / 1000;
    ((UPRE)data[0])((ENGC*)data[1], data[2], time);
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                       ENGC *engc, intptr_t data) {
    intptr_t user[3] = {(intptr_t)upre, (intptr_t)engc, data};
    CFRunLoopTimerContext ctxt = {0, (void*)user};
    CFRunLoopTimerRef tmrp =
        CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent(),
                             0.001 * fram, 0, 0, TmrFunc, &ctxt);

    CFRunLoopAddTimer(CFRunLoopGetCurrent(), tmrp, kCFRunLoopCommonModes);
    run(sharedApplication(NSApplication));
    CFRunLoopTimerInvalidate(tmrp);
}



void MoveControl(CTRL *ctrl, intptr_t data) {
    long xpos = (int16_t)data, ypos = (int32_t)data >> 16;
    CTRL *root = ctrl;
    CGRect rect;

    while (root->prev)
        root = root->prev;
    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    rect.origin.x = (xpos < 0)? -xpos : xpos * (uint16_t)(root->priv[2]      );
    rect.origin.y = (ypos < 0)? -ypos : ypos * (uint16_t)(root->priv[2] >> 16);
    setFrame_((id)ctrl->priv[0], rect);
}



/// PRIV:
///  0: NSWindow
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (fontmul.x) | (fontmul.y << 16)
///  3:
///  4:
///  5:
///  6: subclass delegate
///  7: NSView subclass (flipped)
intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW: {
            id thrd = sharedApplication(NSApplication);

            if (data)
                orderFront_((id)ctrl->priv[0], thrd);
            else
                orderOut_((id)ctrl->priv[0], thrd);
            break;
        }
        case MSG_WSZC: {
            CGRect area, scrn;

            area.size.width  = (((uint16_t)(data      )) + ctrl->xdim)
                             *   (uint16_t)(ctrl->priv[2]      );
            area.size.height = (((uint16_t)(data >> 16)) + ctrl->ydim)
                             *   (uint16_t)(ctrl->priv[2] >> 16);
            ctrl->priv[1] =  (uint16_t)area.size.width
                          | ((uint32_t)area.size.height << 16);
            GetT4DV2(area, (id)ctrl->priv[0], FrameRectForContentRect_, area);
            GetT4DV(scrn, mainScreen(NSScreen), VisibleFrame);
            area.origin.x = 0.5 * (scrn.size.width  - area.size.width )
                          + scrn.origin.x;
            area.origin.y = 0.5 * (scrn.size.height - area.size.height)
                          + scrn.origin.y;
            setFrame_display_animate_((id)ctrl->priv[0], area, true, false);
            setMinSize_((id)ctrl->priv[0], area.size);
            orderFront_((id)ctrl->priv[0], sharedApplication(NSApplication));
            break;
        }
    }
    return 0;
}



/// PRIV:
///  0: NSProgressIndicator
///  1: progress position
///  2: progress maximum
///  3: UTF-8 text label
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PPOS:
            setDoubleValue_((id)ctrl->priv[0], ctrl->priv[1] = data);
            break;

        case MSG_PTXT:
            free((void*)ctrl->priv[3]);
            ctrl->priv[3] = (intptr_t)strdup((char*)data);
            break;

        case MSG_PLIM:
            ctrl->priv[2] = data;
            setMaxValue_((id)ctrl->priv[0],
                         (ctrl->priv[2] > 0)? ctrl->priv[2] : 1);
            break;

        case MSG_PGET:
            return (data)? ctrl->priv[2] : ctrl->priv[1];
    }
    return 0;
}



/// PRIV:
///  0: NS
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CX(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            break;

        case MSG_BGST:
            return 0;

        case MSG_BCLK:
            return 0;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1:
///  2:
///  3:
///  4:
///  5:
///  6: NSTableColumn
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            break;

        case MSG_LCOL: {
            id estr;

            setStringValue_(headerCell((id)ctrl->priv[6]),
                            estr = UTF8((char*)data));
            release(estr);
            break;
        }
        case MSG_LADD: {
            break;
        }
    }
    return 0;
}



/// PRIV:
///  0: NSView
///  1:
///  2:
///  3:
///  4:
///  5:
///  6: NSStepper
///  7: NSTextField
intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            CGRect rect;

            GetT4DV(rect, (id)ctrl->priv[0], Frame);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;

        case MSG__ENB:
            break;

        case MSG_NGET:
            return 0;

        case MSG_NSET:
            break;

        case MSG_NDIM:
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSTextField
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CT(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            CGRect rect;

            GetT4DV(rect, (id)ctrl->priv[0], Frame);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (wndmove.x) | (wndmove.y << 16)
///  3:
///  4:
///  5:
///  6:
///  7: NSScrollView (the same as in [0])
intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_WSZC: {
            CGRect rect;

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
            setFrame_((id)ctrl->priv[0], rect);
            if (ctrl->fc2e)
                ctrl->fc2e(ctrl, MSG_SMAX, ctrl->priv[1]);
            setNeedsDisplay_((id)ctrl->priv[0], true);
            break;
        }
        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NS
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;

        case MSG_IFRM:
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW:
            objc_disposeClassPair((Class)ctrl->priv[6]); /// releasing subclass
            /// we shouldn`t release neither the window nor its NSView;
            /// if we do, the program segfaults, don`t know why exactly
            return;

        case FCT_TEXT:
            break;

        case FCT_BUTN:
            break;

        case FCT_CBOX:
            break;

        case FCT_RBOX:
            break;

        case FCT_SPIN:
            release((id)ctrl->priv[6]); /// releasing NSStepper
            release((id)ctrl->priv[7]); /// releasing NSTextField
            break;

        case FCT_LIST:
            release((id)ctrl->priv[6]); /// releasing NSTableColumn
            release((id)ctrl->priv[7]); /// releasing NSTableView
            break;

        case FCT_SBOX:
            break;

        case FCT_IBOX:
            break;

        case FCT_PBAR:
            free((void*)ctrl->priv[3]); /// freeing the text label
            break;
    }
    release((id)ctrl->priv[0]);
}



bool OnFlip() {
    return true;
}

bool OnClose(id init) {
    stop_(sharedApplication(NSApplication), init);
    return true;
}

void OnSize(id view) {
    CTRL *ctrl = 0;
    CGRect rect;

    GET_IVAR(view /** NSView window delegate **/, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    GetT4DV2(rect, (id)ctrl->priv[0], ContentRectForFrameRect_, rect);
    ctrl->priv[1] =  (uint16_t)rect.size.width
                  | ((uint32_t)rect.size.height << 16);
    ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    CTRL *root;
    CGRect dims;
    id gwnd, capt;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        char *vars[] = {VAR_CTRL, 0};
        OMSC acts[] = {{WindowShouldClose_, OnClose}, {IsFlipped, OnFlip},
                       {WindowDidResize_, OnSize}, {}};
        CGPoint fadv;
        CGFloat fasc;

        ctrl->fe2c = FE2CW;
        gwnd = systemFontOfSize_(NSFont, 0);
        GetT1DV(fasc, gwnd, Ascender);
        GetT2DV(fadv, gwnd, MaximumAdvancement);
        ctrl->priv[2] =  (uint16_t)round(0.45 * fadv.x)
                      | ((uint32_t)round(0.60 * fasc) << 16);

        dims = (CGRect){};
        gwnd = initWithContentRect_styleMask_backing_defer_
                   (alloc(NSWindow), dims, NSTitledWindowMask
                                         | NSClosableWindowMask
                                         | NSResizableWindowMask
                                         | NSMiniaturizableWindowMask,
                    NSBackingStoreBuffered, false);

        ctrl->priv[6] = (intptr_t)Subclass(NSView, "NSV", vars, acts);
        ctrl->priv[7] = (intptr_t)init(alloc((id)ctrl->priv[6]));
        setContentView_(gwnd, (id)ctrl->priv[7]);
        setDelegate_(gwnd, (id)ctrl->priv[7]);
        setTitle_(gwnd, UTF8(text)); /// no need to release NSString!
        SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
        makeKeyWindow(gwnd);
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
        dims = (CGRect){{xpos * xspc, ypos * yspc}, {xdim, ydim}};

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT:
                ctrl->fe2c = FE2CT;
                gwnd = init(alloc(NSTextField));
                setStringValue_(gwnd, capt = UTF8(text));
                setDrawsBackground_(gwnd, false);
                setSelectable_(gwnd, false);
                setEditable_(gwnd, false);
                setBordered_(gwnd, false);
                setBezeled_(gwnd, false);
                release(capt);
                break;

            case FCT_BUTN:
                gwnd = init(alloc(NSButton));
                setButtonType_(gwnd, NSMomentaryLightButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                setTitle_(gwnd, capt = UTF8(text));
                release(capt);
                break;

            case FCT_CBOX:
                ctrl->fe2c = FE2CX;
                gwnd = init(alloc(NSButton));
                setButtonType_(gwnd, NSSwitchButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                setImagePosition_(gwnd, (ctrl->flgs & FSX_LEFT)?
                                         NSImageRight : NSImageLeft);
                setTitle_(gwnd, capt = UTF8(text));
                release(capt);
                break;

            case FCT_RBOX:
                /// [TODO:] do we really need radio boxes?
                break;

            case FCT_SPIN: {
                CGPoint spin;
                CGRect temp;

                ctrl->fe2c = FE2CN;
                gwnd = init(alloc(NSView));
                ctrl->priv[6] = (intptr_t)init(alloc(NSStepper));
                ctrl->priv[7] = (intptr_t)init(alloc(NSTextField));
                GetT2DV(spin, (id)ctrl->priv[6], IntrinsicContentSize);
                temp = dims;
                temp.origin.x = temp.origin.y = 0;
                temp.size.width -= spin.x;
                setFrame_((id)ctrl->priv[7], temp);
                temp.origin.x = temp.size.width;
                temp.size.width = spin.x;
                setFrame_((id)ctrl->priv[6], temp);
                addSubview_(gwnd, (id)ctrl->priv[6]);
                addSubview_(gwnd, (id)ctrl->priv[7]);
                break;
            }
            case FCT_LIST:
                ctrl->fe2c = FE2CL;
                gwnd = init(alloc(NSScrollView));
                ctrl->priv[7] = (intptr_t)init(alloc(NSTableView));
                setFrame_((id)ctrl->priv[7], dims);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn));
                setWidth_((id)ctrl->priv[6], (int)dims.size.width);

                setStringValue_(headerCell((id)ctrl->priv[6]),
                                capt = UTF8(""));
                addTableColumn_((id)ctrl->priv[7], (id)ctrl->priv[6]);
                /// [(id)ctrl->priv[7] setDelegate:self];
                /// [(id)ctrl->priv[7] setDataSource:self];
                /// [(id)ctrl->priv[7] reloadData];

                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                setHasVerticalScroller_(gwnd, true);
                release(capt);
                break;

            case FCT_SBOX:
                ctrl->fe2c = FE2CS;
                ctrl->priv[1] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                ctrl->priv[2] =  (uint16_t)dims.origin.x
                              | ((uint32_t)dims.origin.y << 16);
                gwnd = init(alloc(NSScrollView));
                setHasVerticalScroller_(gwnd, true);
                setBackgroundColor_(gwnd, controlColor(NSColor));
                ctrl->priv[7] = (intptr_t)gwnd;
                break;

            case FCT_IBOX:
                ctrl->fe2c = FE2CI;
                gwnd = init(alloc(NSView));
                ctrl->priv[3] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                break;

            case FCT_PBAR:
                ctrl->fe2c = FE2CP;
                gwnd = init(alloc(NSProgressIndicator));
                setIndeterminate_(gwnd, false);
                break;
        }
        setFrame_(gwnd, dims);
        addSubview_((id)ctrl->prev->priv[7], gwnd);
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}



int main(int argc, char *argv[]) {
    ssize_t sdim;
    CGRect dims;
    id pool;

    char *home, *conf;
    struct dirent **dirs;
    ENGC *engc;

    LoadObjC();

    pool = URLsForDirectory_inDomains_(defaultManager(NSFileManager),
                                       NSApplicationSupportDirectory,
                                       NSUserDomainMask);
    home = UTF8String(path(objectAtIndex_(pool, 0)));
    conf = calloc(32 + strlen(home), sizeof(*conf));
    strcat(conf, home);
    strcat(conf, DEF_OPTS);
    if (!(home = (mkdir(conf, 0755))? (errno != EEXIST)? 0 : conf : conf))
        printf("WARNING: cannot create '%s'!", conf);

    pool = init(alloc(NSAutoreleasePool));
    setActivationPolicy_(sharedApplication(NSApplication),
                         NSApplicationActivationPolicyAccessory);

    engc = eInitializeEngine(conf);
    free(conf);
    if ((sdim = scandir(DEF_FLDR, &dirs, 0, alphasort)) >= 0) {
        while (sdim--) {
            if ((dirs[sdim]->d_type == DT_DIR)
            &&  strcmp(dirs[sdim]->d_name, ".")
            &&  strcmp(dirs[sdim]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, DEF_FLDR, dirs[sdim]->d_name);
            free(dirs[sdim]);
        }
        free(dirs);
    }
    GetT4DV(dims, mainScreen(NSScreen), VisibleFrame);
    GetT1DV(sdim, systemStatusBar(NSStatusBar), Thickness);
    eExecuteEngine(engc, sdim, sdim, dims.origin.x, dims.origin.y,
                   dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    release(pool);
    return 0;
}
