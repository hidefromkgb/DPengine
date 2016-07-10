#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../exec/exec.h"
#include "mac.h"

/// name of the menu responder class
#define CLS_MENU "lNSM"
/// name of the instance variable to access the CTRL structure
#define VAR_CTRL "ctrl"
/// name of the instance variable to access the associated data
#define VAR_DATA "data"

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



/// NAME holds the selector associated with this function
void OnMenu(id this, SEL name, id menu) {
    MENU *item = (MENU*)tag(menu);

    eProcessMenuItem(item);
}



id Submenu(MENU *menu, id base) {
    if (!menu)
        return 0;

    id cbtn, rbtn, item, retn = init(alloc(NSMenu));
    CFStringRef text, null = UTF8(0);

    cbtn = imageNamed_(NSImage, text = UTF8("NSMenuCheckmark"));
    CFRelease(text);
    rbtn = imageNamed_(NSImage, text = UTF8("NSMenuRadio"));
    CFRelease(text);
    setAutoenablesItems_(retn, false);
    while (menu->text) {
        if (!*menu->text) {
            item = separatorItem(NSMenuItem);
            addItem_(retn, item);
        }
        else {
            item = initWithTitle_action_keyEquivalent_
                       (alloc(NSMenuItem),
                        text = UTF8(menu->text), ActionSelector, null);
            CFRelease(text);
            if (menu->flgs & MFL_CCHK) {
                setOnStateImage_(item, (menu->flgs & MFL_RCHK & ~MFL_CCHK)?
                                        rbtn : cbtn);
                setState_(item, (menu->flgs & MFL_VCHK)?
                                 NSOnState : NSOffState);
            }
            if (menu->chld) {
                id next = Submenu(menu->chld, base);
                setSubmenu_(item, next);
                release(next);
            }
            /// might be dangerous if sizeof(long) is less than sizeof(MENU*)
            setTag_(item, menu);

            setEnabled_(item, (menu->flgs & MFL_GRAY)? false : true);
            setTarget_(item, base);
            addItem_(retn, item);
            release(item);
        }
        menu++;
    }
    CFRelease(null);
    return retn;
}



void rOpenContextMenu(MENU *tmpl) {
    id menu, base;
    CGPoint dptr;

    /// getting the pre-allocated menu responder class
    menu = NewClass(0, CLS_MENU, 0, 0);
    base = init(alloc(menu));
    DelClass((Class)menu);

    menu = Submenu(tmpl, base);
    GetT2DV(dptr, NSEvent, MouseLocation);
    popUpMenuPositioningItem_atLocation_inView_(menu, nil, dptr, nil);
    release(menu);
    release(base);
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



void OnTray(id this) {
    MENU *mctx;

    GET_IVAR(this, VAR_DATA, &mctx);
    rOpenContextMenu(mctx);
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctxt = CGBitmapContextCreate
        (data, xdim, ydim, CHAR_BIT, xdim * sizeof(*data), drgb,
         kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGImageRef iref = CGBitmapContextCreateImage(ctxt);
    CFStringRef capt;

    id ibtn, pict, *retn = malloc(3 * sizeof(*retn));

    pict = initWithCGImage_size_(alloc(NSImage), iref, ((CGPoint){}));
    retn[0] = statusItemWithLength_(systemStatusBar(NSStatusBar),
                                    NSVariableStatusItemLength),
    retn[1] = NewClass(NSObject, "lNST", (char*[]){VAR_CTRL, VAR_DATA, 0},
                      (OMSC[]){{ActionSelector, OnTray}, {}});
    retn[2] = init(alloc(retn[1]));
    SET_IVAR(retn[2], VAR_CTRL, retn);
    SET_IVAR(retn[2], VAR_DATA, mctx);

    /// kCFCoreFoundationVersionNumber10_10 == 1151.16
    if (kCFCoreFoundationVersionNumber >= 1151.0)
        ibtn = button(retn[0]); /// Yosemite or newer, so we are using buttons
    else
        setHighlightMode_(ibtn = retn[0], true);
    setImage_(ibtn, pict);
    setTarget_(ibtn, retn[2]);
    setAction_(ibtn, ActionSelector);
    setToolTip_(retn[0], capt = UTF8(text));
    CFRelease(capt);
    release(pict);
    CGImageRelease(iref);
    CGContextRelease(ctxt);
    CGColorSpaceRelease(drgb);
    return (intptr_t)retn;
}



void rFreeTrayIcon(intptr_t icon) {
    id *retn = (id*)icon;

    removeStatusItem_(systemStatusBar(NSStatusBar), retn[0]);
    release(retn[2]);
    DelClass((Class)retn[1]);
    free(retn);
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
    while (root->priv[2])
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

void OnSpin(id this) {
    CGFloat spos = 0.0;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT1DV(spos, (id)ctrl->priv[6], DoubleValue);
    setIntValue_((id)ctrl->priv[7], spos);
    ctrl->fc2e(ctrl, MSG_NSET, spos);
}

void TextChecker(id this) {
    CGFloat spos = 0.0;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT1DV(spos, (id)ctrl->priv[6], DoubleValue);
    setIntValue_((id)ctrl->priv[7], spos);
}



/// PRIV:
///  0: NSWindow
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (fontmul.x) | (fontmul.y << 16), and also a halt flag if 0
///  3: first tab-enabled control
///  4: last tab-enabled control
///  5:
///  6: SCLS subclass storage
///  7: NSView, the main container and delegate
intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW: {
            id thrd = sharedApplication(NSApplication);

            if (!data)
                orderOut_((id)ctrl->priv[0], thrd);
            else {
                setLevel_((id)ctrl->priv[0], NSMainMenuWindowLevel + 1);
                orderFront_((id)ctrl->priv[0], thrd);
                setLevel_((id)ctrl->priv[0], NSNormalWindowLevel);
            }
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
///  1: NSButton subclass
///  2:
///  3:
///  4: number of rows
///  5: array of NSButtons with elements
///  6: NSTableColumn
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    CFStringRef capt;

    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((id)ctrl->priv[7], !!data);
            for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                setEnabled_(((id*)ctrl->priv[5])[cmsg], !!data);
            break;

        case MSG_LCOL:
            setStringValue_(headerCell((id)ctrl->priv[6]), capt = UTF8(data));
            CFRelease(capt);
            for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                setState_(((id*)ctrl->priv[5])[cmsg],
                         (ctrl->fc2e(ctrl, MSG_LGST, cmsg))? true : false);
            reloadData((id)ctrl->priv[7]);
            break;

        case MSG_LADD: {
            id elem;

            elem = init(alloc((id)ctrl->priv[1]));
            SET_IVAR(elem, VAR_DATA, ctrl->priv[4]);
            SET_IVAR(elem, VAR_CTRL, ctrl);
            setTarget_(elem, elem);
            setAction_(elem, ActionSelector);
            setButtonType_(elem, NSSwitchButton);
            setTitle_(elem, capt = UTF8(data));
            CFRelease(capt);
            ctrl->priv[5] = (intptr_t)realloc((id*)ctrl->priv[5],
                                             ++ctrl->priv[4] * sizeof(id));
            ((id*)ctrl->priv[5])[ctrl->priv[4] - 1] = elem;
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
            setEnabled_((id)ctrl->priv[6], !!data);
            setEnabled_((id)ctrl->priv[7], !!data);
            break;

        case MSG_NGET: {
            CGFloat retn = 0;

            GetT1DV(retn, (id)ctrl->priv[6], DoubleValue);
            return retn;
        }
        case MSG_NSET:
            data = (data > ctrl->priv[1])? data : ctrl->priv[1];
            data = (data < ctrl->priv[2])? data : ctrl->priv[2];
            setDoubleValue_((id)ctrl->priv[6], data);
            OnSpin((id)ctrl->priv[6]);
            break;

        case MSG_NDIM:
            ctrl->priv[1] = -(uint16_t)data;
            ctrl->priv[2] = (uint16_t)(data >> 16);
            setMinValue_((id)ctrl->priv[6], ctrl->priv[1]);
            setMaxValue_((id)ctrl->priv[6], ctrl->priv[2]);
            setDoubleValue_((id)ctrl->priv[6], 0);
            OnSpin((id)ctrl->priv[6]);
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
                DelClass((Class)scls->_sub[iter]);
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
            /// releasing buttons and deleting their common subclass
            for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--)
                release(((id*)ctrl->priv[5])[ctrl->priv[4]]);
            DelClass((Class)ctrl->priv[1]);
            free((id*)ctrl->priv[5]);
            release((id)ctrl->priv[6]); /// releasing NSTableColumn
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



NSInteger OnRows(id this, SEL name, id view) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return ctrl->priv[4];
}

id OnValueOld(id this, SEL name, id view, id icol, NSInteger irow) {
    return 0;
}

id OnValue(id this, SEL name, id view, id icol, NSInteger irow) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return ((id*)ctrl->priv[5])[irow];
}

void ListButtonClick(id this) {
    CTRL *ctrl = 0;
    intptr_t irow;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    GET_IVAR(this, VAR_DATA, &irow);
    ctrl->fc2e(ctrl, MSG_LSST,
              (irow << 1) | ((state(this) == NSOnState)? 1 : 0));
}

void ButtonClick(id this) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_BCLK, ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
                                (state((id)ctrl->priv[0]) == NSOnState) : 0);
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

bool OnClose(id this) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->priv[2] = 0; /// requesting a halt, or just crashing if CTRL is 0
                       /// (the program is stopping anyway)
    stop_(sharedApplication(NSApplication), this);
    return true;
}

void OnSize(id this) {
    CTRL *ctrl = 0;
    CGRect rect;

    GET_IVAR(this, VAR_CTRL, &ctrl);
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
             bmet[] = {{ActionSelector, ButtonClick}, {}},
             nmet[] = {{ActionSelector, OnSpin}, {}},
             lmet[] = {{NumberOfRowsInTableView_,                 OnRows},
                       {TableView_objectValueForTableColumn_row_, OnValueOld},
                       {TableView_viewForTableColumn_row_,        OnValue},
                       {}},
             pmet[] = {{DrawRect_, PBoxDraw}, {}},
             smet[] = {{IsFlipped, OnTrue}, {}},
             imet[] = {{DrawRect_, IBoxDraw}, {}};
        CGPoint fadv;
        CGFloat fasc;
        id thrd;

        ctrl->fe2c = FE2CW;
        gwnd = systemFontOfSize_(NSFont, 0);
        GetT1DV(fasc, gwnd, Ascender);
        GetT2DV(fadv, gwnd, MaximumAdvancement);
        ctrl->priv[2] =  (uint16_t)round(0.45 * fadv.x)
                      | ((uint32_t)round(0.60 * fasc) << 16);

        ctrl->priv[6] = (intptr_t)(scls = calloc(1, sizeof(*scls)));
        scls->wndw = NewClass(NSView,              "rNSW", vars, wmet);
        scls->text = NewClass(NSTextField,         "rNST", vars, tmet);
        scls->butn = NewClass(NSButton,            "rNSB", vars, bmet);
        scls->spin = NewClass(NSStepper,           "rNSN", vars, nmet);
        scls->list = NewClass(NSTableView,         "rNSL", vars, lmet);
        scls->pbar = NewClass(NSProgressIndicator, "rNSP", vars, pmet);
        scls->sbox = NewClass(NSView,              "rNSS", vars, smet);
        scls->ibox = NewClass(NSView,              "rNSI", vars, imet);

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
        thrd = sharedApplication(NSApplication);
        activateIgnoringOtherApps_(thrd, true);
        orderFront_(gwnd, thrd);
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
                SET_IVAR(gwnd, VAR_DATA, nil);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ActionSelector);
                setButtonType_(gwnd, NSMomentaryLightButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                if ((ctrl->flgs & FSB_DFLT) && ctrl->prev
                && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
                    setInitialFirstResponder_((id)ctrl->prev->priv[0], gwnd);
                    setDefaultButtonCell_((id)ctrl->prev->priv[0], cell(gwnd));
                }
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
                setAction_(gwnd, ActionSelector);
                setButtonType_(gwnd, NSSwitchButton);
                setImagePosition_(gwnd, (ctrl->flgs & FSX_LEFT)?
                                         NSImageRight : NSImageLeft);
                setTitle_(gwnd, capt = UTF8(text));
                CFRelease(capt);
                break;

            case FCT_RBOX:
                /// [TODO:] do we really need radio boxes?
                break;

            case FCT_SPIN: {
                CGRect temp = {};
                CGPoint spin;

                ctrl->fe2c = FE2CN;
                gwnd = init(alloc(NSView));
                ctrl->priv[6] = (intptr_t)init(alloc(scls->spin));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->text));
                SET_IVAR((id)ctrl->priv[6], VAR_CTRL, ctrl);
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
                GetT2DV(spin, (id)ctrl->priv[6], IntrinsicContentSize);
                temp.size = dims.size;
                temp.size.width -= spin.x;
                setFrame_((id)ctrl->priv[7], temp);
                temp.origin.x = temp.size.width;
                temp.size.width = spin.x;
                setFrame_((id)ctrl->priv[6], temp);
                setValueWraps_((id)ctrl->priv[6], false);
                setTarget_((id)ctrl->priv[6], (id)ctrl->priv[6]);
                setAction_((id)ctrl->priv[6], ActionSelector);
                addSubview_(gwnd, (id)ctrl->priv[6]);
                addSubview_(gwnd, (id)ctrl->priv[7]);
                break;
            }
            case FCT_LIST: {
                CGRect temp = {};

                ctrl->fe2c = FE2CL;
                temp.size = dims.size;
                ctrl->priv[1] = (intptr_t)NewClass
                    (NSButton, "rNSX", (char*[]){VAR_CTRL, VAR_DATA, 0},
                    (OMSC[]){{ActionSelector, ListButtonClick}, {}});

                gwnd = init(alloc(NSScrollView));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->list));
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
                setDataSource_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setDelegate_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setFrame_((id)ctrl->priv[7], temp);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn));
                setStringValue_(headerCell((id)ctrl->priv[6]), capt = UTF8(0));
                CFRelease(capt);
                addTableColumn_((id)ctrl->priv[7], (id)ctrl->priv[6]);

                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                setHasVerticalScroller_(gwnd, true);
                break;
            }
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
        if (ctrl->prev && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
            id resp; /// tab-stop responder

            switch (ctrl->flgs & FCT_TTTT) {
                case FCT_TEXT:
                case FCT_PBAR:
                case FCT_SBOX: resp = 0; break;
                case FCT_LIST:
                case FCT_SPIN: resp = (id)ctrl->priv[7]; break;
                default:       resp = gwnd; break;
            }
            if (resp) {
                if (ctrl->prev->priv[4]) {
                    setNextKeyView_((id)ctrl->prev->priv[4], resp);
                    setNextKeyView_(resp, (id)ctrl->prev->priv[3]);
                    ctrl->prev->priv[4] = (intptr_t)resp;
                }
                else {
                    ctrl->prev->priv[3] = ctrl->prev->priv[4] = (intptr_t)resp;
                    /// shall do nothing if there already is a first responder
                    setInitialFirstResponder_((id)ctrl->prev->priv[0], resp);
                }
            }
        }
    }
    ctrl->priv[0] = (intptr_t)gwnd;
}



int main(int argc, char *argv[]) {
    CFStringRef path;
    CGFloat icon;
    CGRect dims;
    id pool, menu;

    struct dirent **dirs;
    ENGC *engc;
    char *home;
    long  iter;

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

    if ((iter = scandir(home, &dirs, 0, alphasort)) >= 0) {
        while (iter--) {
            if ((dirs[iter]->d_type == DT_DIR)
            &&  strcmp(dirs[iter]->d_name, ".")
            &&  strcmp(dirs[iter]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, home, dirs[iter]->d_name);
            free(dirs[iter]);
        }
        free(dirs);
    }
    free(home);
    GetT4DV(dims, mainScreen(NSScreen), VisibleFrame);
    GetT1DV(icon, systemStatusBar(NSStatusBar), Thickness);
    menu = NewClass(NSObject, CLS_MENU, (char*[]){0},
                   (OMSC[]){{ActionSelector, OnMenu}, {}});
    eExecuteEngine(engc, icon, icon, dims.origin.x, dims.origin.y,
                   dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    DelClass((Class)menu);
    release(pool);
    return 0;
}
