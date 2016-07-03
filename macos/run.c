#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../exec/exec.h"
#include "mac.h"

/// name of the instance variable to access the CTRL structure
#define VAR_CTRL "c"

/// subclass storage
typedef union {
    struct {
        id wndw,
           text,
           butn, /// +cbox/rbox
           spin,
           list,
           pbar,
           sbox,
           ibox;
    };
    id _sub[8];
} SCLS;



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
///  6: SCLS subclass storage
///  7: NSView, the main container and delegate
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
            GetT4DV(area, (id)ctrl->priv[0], FrameRectForContentRect_, area);
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
///  3: CFStringRef with label text
///  4: vertical offset of the label
///  5: CFDictionaryRef with centering attributes and font
///  6:
///  7:
intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PPOS:
            setDoubleValue_((id)ctrl->priv[0], ctrl->priv[1] = data);
            break;

        case MSG_PTXT:
            if (ctrl->priv[3])
                CFRelease((void*)ctrl->priv[3]);
            ctrl->priv[3] = (intptr_t)UTF8(data);
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
        case MSG__ENB:
            setEnabled_((id)ctrl->priv[0], !!data);
            break;

        case MSG_BGST:
            return ((isEnabled((id)ctrl->priv[0]))? FCS_ENBL : 0)
                 | ((state((id)ctrl->priv[0]) == NSOnState)? FCS_MARK : 0);

        case MSG_BCLK:
            cmsg = (state((id)ctrl->priv[0]) == NSOnState);
            setState_((id)ctrl->priv[0], (data)? NSOnState : NSOffState);
            return !!cmsg;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1:
///  2:
///  3: array of CFStringRef with string data
///  4: number of rows
///  5: NSTableColumn #1 (check marks)        <-- not implemented yet
///  6: NSTableColumn #2 (string data)
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((id)ctrl->priv[7], !!data);
            break;

        case MSG_LCOL: {
            CFStringRef head;

            setStringValue_(headerCell((id)ctrl->priv[6]), head = UTF8(data));
            CFRelease(head);
            reloadData((id)ctrl->priv[7]);
            break;
        }
        case MSG_LADD:
            ctrl->priv[3] = (intptr_t)realloc
                                ((CFStringRef*)ctrl->priv[3],
                                ++ctrl->priv[4] * sizeof(CFStringRef));
            ((CFStringRef*)ctrl->priv[3])[ctrl->priv[4] - 1] = UTF8(data);
            break;
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
            setEnabled_((id)ctrl->priv[6], !!data);
            setEnabled_((id)ctrl->priv[7], !!data);
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
///  7: NSView, the sizeable container
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
            if (ctrl->fc2e) {
                data = rect.size.height;
                GetT4DV(rect, verticalScroller((id)ctrl->priv[0]), Frame);
                cmsg = ctrl->priv[1] - rect.size.width;
                data += ctrl->fc2e(ctrl, MSG_SMAX, cmsg);
                setFrame_((id)ctrl->priv[7],
                         ((CGRect){{0, 0}, {(uint16_t)cmsg, data}}));
            }
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
            setHidden_((id)ctrl->priv[0], !data);
            break;

        case MSG_IFRM:
            ctrl->priv[7] = data;
            setNeedsDisplay_((id)ctrl->priv[0], true);
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW: {
            SCLS *scls = (SCLS*)ctrl->priv[6];
            long iter;

            /// [TODO:] do not try to free standard classes, if any
            for (iter = countof(scls->_sub) - 1; iter >= 0; iter--)
                objc_disposeClassPair((Class)scls->_sub[iter]);
            free(scls);
            /// we must release neither the window nor its NSView;
            /// if we do, the program segfaults, don`t know why exactly
            return;
        }
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
            for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--)
                CFRelease(((CFStringRef*)ctrl->priv[3])[ctrl->priv[4]]);
            free((CFStringRef*)ctrl->priv[3]);
            release((id)ctrl->priv[5]); /// releasing NSTableColumn #1
            release((id)ctrl->priv[6]); /// releasing NSTableColumn #2
            release((id)ctrl->priv[7]); /// releasing NSTableView
            break;

        case FCT_SBOX:
            release((id)ctrl->priv[7]); /// releasing NSView
            break;

        case FCT_IBOX:
            /// releasing the image context and freeing its backbuffer
            CGContextRelease((CGContextRef)ctrl->priv[1]);
            free((void*)ctrl->priv[2]);
            break;

        case FCT_PBAR:
            CFRelease((void*)ctrl->priv[5]);     /// releasing text attributes
            if (ctrl->priv[3])
                CFRelease((void*)ctrl->priv[3]); /// freeing the text string
            break;
    }
    release((id)ctrl->priv[0]);
}



NSInteger OnRows(id this, id prev, id view) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return ctrl->priv[4];
}

id OnValue(id this, id prev, id view, id icol, NSInteger irow) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return (id)(((CFStringRef*)ctrl->priv[3])[irow]);
}

void ButtonClick(id this) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_BCLK, ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
                                (state((id)ctrl->priv[0]) == NSOnState) : 0);
}

void TextChecker(id this) {
    CTRL *ctrl = 0;
    long curr;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    curr = intValue((id)ctrl->priv[7]);
    setIntValue_((id)ctrl->priv[7], curr);
}

void PBoxDraw(CGRect rect, id this) {
    struct objc_super prev = {this, class(NSProgressIndicator)};
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    objc_msgSendSuper(&prev, DrawRect_, rect);
    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    rect.origin.y = ctrl->priv[4];
    rect.origin.x = 0;
    drawInRect_withAttributes_((id)ctrl->priv[3], rect, (id)ctrl->priv[5]);
}

void IBoxDraw(CGRect rect, id this) {
    CGImageRef pict;
    CGRect area;
    AINF anim;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;
    anim = (AINF){(ctrl->priv[7] >> 10) & 0x3FFFFF,
                  (int16_t)ctrl->priv[3], (int32_t)ctrl->priv[3] >> 16,
                   ctrl->priv[7] & 0x3FF, (uint32_t*)ctrl->priv[2]};
    if (!anim.uuid)
        return;
    area = (CGRect){{0, 0}, {anim.xdim, anim.ydim}};
    CGContextFillRect((CGContextRef)ctrl->priv[1], area);
    CGContextFlush((CGContextRef)ctrl->priv[1]);
    ctrl->fc2e(ctrl, MSG_IFRM, (intptr_t)&anim);
    pict = CGBitmapContextCreateImage((CGContextRef)ctrl->priv[1]);
    CGContextDrawImage(graphicsPort(currentContext(NSGraphicsContext)),
                       area, pict);
    CGImageRelease(pict);
}

bool OnFalse() {
    return false;
}

bool OnTrue() {
    return true;
}

bool OnClose(id view) {
    stop_(sharedApplication(NSApplication), view);
    return true;
}

void OnSize(id view) {
    CTRL *ctrl = 0;
    CGRect rect;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    GetT4DV(rect, (id)ctrl->priv[0], ContentRectForFrameRect_, rect);
    ctrl->priv[1] =  (uint16_t)rect.size.width
                  | ((uint32_t)rect.size.height << 16);
    ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    CTRL *root;
    SCLS *scls;
    CFStringRef capt;
    CGRect dims;
    id gwnd;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        char *vars[] = {VAR_CTRL, 0};
        OMSC wmet[] = {{WindowShouldClose_, OnClose},
                       {WindowDidResize_, OnSize}, {IsFlipped, OnTrue}, {}},
             tmet[] = {{TextDidChange_, TextChecker}, {}},
             bmet[] = {{ButtonSelector, ButtonClick}, {}},
             lmet[] = {{NumberOfRowsInTableView_,                 OnRows},
                       {TableView_objectValueForTableColumn_row_, OnValue}},
             pmet[] = {{DrawRect_, PBoxDraw}, {}},
             smet[] = {{IsFlipped, OnTrue}, {}},
             imet[] = {{DrawRect_, IBoxDraw}, {}};
        CGPoint fadv;
        CGFloat fasc;

        ctrl->fe2c = FE2CW;
        gwnd = systemFontOfSize_(NSFont, 0);
        GetT1DV(fasc, gwnd, Ascender);
        GetT2DV(fadv, gwnd, MaximumAdvancement);
        ctrl->priv[2] =  (uint16_t)round(0.45 * fadv.x)
                      | ((uint32_t)round(0.60 * fasc) << 16);

        ctrl->priv[6] = (intptr_t)(scls = calloc(1, sizeof(*scls)));
        scls->wndw = Subclass(NSView,              "NSW", vars, wmet);
        scls->text = Subclass(NSTextField,         "NST", vars, tmet);
        scls->butn = Subclass(NSButton,            "NSB", vars, bmet);
        scls->spin = NSStepper;
        scls->list = Subclass(NSTableView,         "NSL", vars, lmet);
        scls->pbar = Subclass(NSProgressIndicator, "NSP", vars, pmet);
        scls->sbox = Subclass(NSView,              "NSS", vars, smet);
        scls->ibox = Subclass(NSView,              "NSI", vars, imet);

        dims = (CGRect){};
        gwnd = initWithContentRect_styleMask_backing_defer_
                   (alloc(NSWindow), dims, NSTitledWindowMask
                                         | NSClosableWindowMask
                                         | NSResizableWindowMask
                                         | NSMiniaturizableWindowMask,
                    NSBackingStoreBuffered, false);

        ctrl->priv[7] = (intptr_t)init(alloc(scls->wndw));
        setContentView_(gwnd, (id)ctrl->priv[7]);
        setDelegate_(gwnd, (id)ctrl->priv[7]);
        setTitle_(gwnd, capt = UTF8(text));
        CFRelease(capt);
        SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
        makeKeyWindow(gwnd);
        setLevel_(gwnd, NSNormalWindowLevel + 1); /// just a bit above others
        orderFront_(gwnd, sharedApplication(NSApplication));
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
        scls = (SCLS*)root->priv[6];

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT:
                ctrl->fe2c = FE2CT;
                gwnd = init(alloc(scls->text));
                SET_IVAR(gwnd, VAR_CTRL, 0);
                setStringValue_(gwnd, capt = UTF8(text));
                setAlignment_(gwnd, (ctrl->flgs & FST_CNTR)?
                                     NSCenterTextAlignment :
                                     NSLeftTextAlignment);
                setDrawsBackground_(gwnd, false);
                setSelectable_(gwnd, false);
                setEditable_(gwnd, false);
                setBordered_(gwnd, false);
                setBezeled_(gwnd, false);
                CFRelease(capt);
                break;

            case FCT_BUTN: {
                char *temp;
                long size;

                /// a very simple hack to force multiline text wrapping
                temp = malloc(size = strlen(text) + 3);
                temp[0] = '\n';
                temp[1] = '\0';
                strcat(temp, text);
                temp[size - 2] = '\n';
                temp[size - 1] = '\0';
                gwnd = init(alloc(scls->butn));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ButtonSelector);
                setButtonType_(gwnd, NSMomentaryLightButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                setTitle_(gwnd, capt = UTF8(temp));
                CFRelease(capt);
                free(temp);
                break;
            }
            case FCT_CBOX:
                ctrl->fe2c = FE2CX;
                gwnd = init(alloc(scls->butn));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ButtonSelector);
                setButtonType_(gwnd, NSSwitchButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                setImagePosition_(gwnd, (ctrl->flgs & FSX_LEFT)?
                                         NSImageRight : NSImageLeft);
                setTitle_(gwnd, capt = UTF8(text));
                CFRelease(capt);
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
                ctrl->priv[7] = (intptr_t)init(alloc(scls->text));
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
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
                ctrl->priv[7] = (intptr_t)init(alloc(scls->list));
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
                setDataSource_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setFrame_((id)ctrl->priv[7], dims);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn));
                setStringValue_(headerCell((id)ctrl->priv[6]), capt = UTF8(0));
                CFRelease(capt);
                addTableColumn_((id)ctrl->priv[7], (id)ctrl->priv[6]);

                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                setHasVerticalScroller_(gwnd, true);
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
                ctrl->priv[7] = (intptr_t)init(alloc(scls->sbox));
                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                break;

            case FCT_IBOX: {
                CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
                long line = dims.size.width * 4;

                ctrl->fe2c = FE2CI;
                gwnd = init(alloc(scls->ibox));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
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
                CGFloat fasc, fdsc;
                id psty, font;

                ctrl->fe2c = FE2CP;
                gwnd = init(alloc(scls->pbar));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setIndeterminate_(gwnd, false);
                setWantsLayer_(gwnd, true);
                psty = init(alloc(NSMutableParagraphStyle));
                setAlignment_(psty, NSCenterTextAlignment);
                font = systemFontOfSize_(NSFont, 0);
                ctrl->priv[5] = (intptr_t)MakeDict
                    (kCTFontAttributeName,           font,
                     kCTParagraphStyleAttributeName, psty, 0);
                release(psty);
                GetT1DV(fasc, font, Ascender);
                GetT1DV(fdsc, font, Descender);
                ctrl->priv[4] = 0.5 * (dims.size.height - fasc) - fabs(fdsc);
                break;
            }
        }
        setFrame_(gwnd, dims);
        addSubview_((id)ctrl->prev->priv[7], gwnd);
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}



int main(int argc, char *argv[]) {
    ssize_t sdim;
    CFStringRef path;
    CGRect dims;
    id pool;

    struct dirent **dirs;
    char *home;
    ENGC *engc;

    LoadObjC();

    pool = URLsForDirectory_inDomains_(defaultManager(NSFileManager),
                                       NSApplicationSupportDirectory,
                                       NSUserDomainMask);
    path = CFURLCopyFileSystemPath
               (CFArrayGetValueAtIndex((CFArrayRef)pool, 0),
                                        kCFURLPOSIXPathStyle);
    home = CopyUTF8(path);
    CFRelease(path);
    home = realloc(home, strlen(home) + 32);
    strcat(home, DEF_OPTS);
    if (!((mkdir(home, 0755))? (errno != EEXIST)? 0 : 1 : 2))
        printf("WARNING: cannot create '%s'!", home);
    release(pool);

    engc = eInitializeEngine(home);
    free(home);

    home = CopyUTF8(path = (CFStringRef)bundlePath(mainBundle(NSBundle)));
    release((id)path);
    home = realloc(home, strlen(home) + 32);
    strcat(home, "/Contents/MacOS/"DEF_FLDR);

    pool = init(alloc(NSAutoreleasePool));
    setActivationPolicy_(sharedApplication(NSApplication),
                         NSApplicationActivationPolicyAccessory);

    if ((sdim = scandir(home, &dirs, 0, alphasort)) >= 0) {
        while (sdim--) {
            if ((dirs[sdim]->d_type == DT_DIR)
            &&  strcmp(dirs[sdim]->d_name, ".")
            &&  strcmp(dirs[sdim]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, home, dirs[sdim]->d_name);
            free(dirs[sdim]);
        }
        free(dirs);
    }
    free(home);
    GetT4DV(dims, mainScreen(NSScreen), VisibleFrame);
    GetT1DV(sdim, systemStatusBar(NSStatusBar), Thickness);
    eExecuteEngine(engc, sdim, sdim, dims.origin.x, dims.origin.y,
                   dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    release(pool);
    return 0;
}
