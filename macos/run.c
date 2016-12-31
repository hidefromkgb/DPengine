#include <errno.h>
#include <dirent.h>
#include <pthread.h>
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
        void *wndw,
             *text,
             *butn, /// +cbox/rbox
             *spin,
             *list,
             *pbar,
             *sbox,
             *ibox,
             *frmt; /// number formatter
    };
    void *_sub[9];
} SCLS;

/// file find data
typedef struct {
    struct dirent **dirs;
    long hlen, iter;
    char *home;
} FIND;



static SEL ActionSelector() {
    static SEL what = 0;

    if (!what)
        what = sel_registerName("_A");
    return what;
}

/// NAME holds the selector associated with this function
void OnMenu(void *this, SEL name, NSMenu *menu) {
    eProcessMenuItem((MENU*)tag(menu));
}



NSMenu *Submenu(MENU *menu, NSMenu *base) {
    if (!menu)
        return 0;

    NSMenuItem *item;
    NSMenu *retn = init(alloc(NSMenu()));
    NSImage *cbtn, *rbtn;
    CFStringRef text, null = UTF8(0);

    cbtn = imageNamed_(NSImage(), text = UTF8("NSMenuCheckmark"));
    CFRelease(text);
    rbtn = imageNamed_(NSImage(), text = UTF8("NSMenuRadio"));
    CFRelease(text);
    setAutoenablesItems_(retn, false);
    while (menu->text) {
        if (!*menu->text) {
            item = separatorItem(NSMenuItem());
            addItem_(retn, item);
        }
        else {
            item = initWithTitle_action_keyEquivalent_
                       (alloc(NSMenuItem()),
                        text = UTF8(menu->text), ActionSelector(), null);
            CFRelease(text);
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
    CFRelease(null);
    return retn;
}



void rOpenContextMenu(MENU *tmpl) {
    NSMenu *menu, *base;
    CGPoint dptr;

    /// getting the pre-allocated menu responder class
    menu = NewClass(0, CLS_MENU, 0, 0);
    base = init(alloc(menu));
    DelClass((Class)menu);

    menu = Submenu(tmpl, base);
    dptr = mouseLocation(NSEvent());
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



void *MsgFunc(void *data) {
    intptr_t *user = (intptr_t*)data;
    CFOptionFlags retn = 0;

    CFUserNotificationDisplayAlert
        (0, kCFUserNotificationNoteAlertLevel, 0, 0, 0,
        (CFStringRef)user[1], (CFStringRef)user[2], 0, 0, 0, &retn);
    CFRelease((CFStringRef)user[1]);
    CFRelease((CFStringRef)user[2]);
    free(user);
    return 0;
}

long rMessage(char *text, char *head, uint32_t flgs) {
    pthread_t pthr;
    intptr_t *user;

    user = calloc(3, sizeof(*user));
    user[0] = flgs;
    user[1] = (intptr_t)UTF8(head);
    user[2] = (intptr_t)UTF8(text);
    pthread_create(&pthr, 0, MsgFunc, user);
    return 0;
}



void OnTray(void *this, SEL name) {
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

    NSImage *pict;
    NSButton *ibtn;
    void **vfld, **vmet, **retn = malloc(3 * sizeof(*retn));

    pict = initWithCGImage_size_(alloc(NSImage()), iref, ((CGPoint){}));
    retn[0] = statusItemWithLength_(systemStatusBar(NSStatusBar()),
                                    NSVariableStatusItemLength);
    retain(retn[0]);
    retn[1] = NewClass(NSObject(), "lNST", vfld = PutToArr(VAR_CTRL, VAR_DATA),
                       vmet = PutToArr(ActionSelector(), OnTray));
    retn[2] = init(alloc(retn[1]));
    SET_IVAR(retn[2], VAR_CTRL, retn);
    SET_IVAR(retn[2], VAR_DATA, mctx);
    free(vmet);
    free(vfld);

    if (OSX_10_10_PLUS)
        ibtn = button(retn[0]); /// Yosemite or newer, so we are using buttons
    else
        setHighlightMode_(ibtn = retn[0], true);
    setImage_(ibtn, pict);
    setTarget_(ibtn, retn[2]);
    setAction_(ibtn, ActionSelector());
    setToolTip_(retn[0], capt = UTF8(text));
    CFRelease(capt);
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
    release(retn[2]);
    DelClass((Class)retn[1]);
    free(retn);
}



char *rFindFile(intptr_t data) {
    FIND *find = (FIND*)data;
    char *retn = 0;

    if (!find->iter) {
        free(find->home);
        free(find->dirs);
        return 0;
    }
    if ((find->dirs[--find->iter]->d_type != DT_DIR)
    || !strcmp(find->dirs[find->iter]->d_name, ".")
    || !strcmp(find->dirs[find->iter]->d_name, ".."))
        retn = (char*)1;
    else {
        retn = calloc(1, find->hlen + strlen(find->dirs[find->iter]->d_name));
        strcat(retn, find->home);
        strcat(retn, find->dirs[find->iter]->d_name);
    }
    free(find->dirs[find->iter]);
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
    ((UPRE)data[0])(data[1], time);
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre, intptr_t data) {
    intptr_t user[2] = {(intptr_t)upre, data};
    CFRunLoopTimerContext ctxt = {0, (void*)user};
    CFRunLoopTimerRef tmrp =
        CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent(),
                             0.001 * fram, 0, 0, TmrFunc, &ctxt);

    CFRunLoopAddTimer(CFRunLoopGetCurrent(), tmrp, kCFRunLoopCommonModes);
    while (root->priv[2])
        run(sharedApplication(NSApplication()));
    CFRunLoopTimerInvalidate(tmrp);
}



void MoveControl(CTRL *ctrl, intptr_t data) {
    long xpos = (int16_t)data, ypos = (int32_t)data >> 16;
    CTRL *root = ctrl;
    CGRect rect;

    while (root->prev)
        root = root->prev;
    rect = frame((void*)ctrl->priv[0]);
    rect.origin.x = (xpos < 0)? -xpos : xpos * (uint16_t)(root->priv[2]      );
    rect.origin.y = (ypos < 0)? -ypos : ypos * (uint16_t)(root->priv[2] >> 16);
    setFrame_((NSView*)ctrl->priv[0], rect);
}

bool OnValidate(void *this, SEL name,
                CFStringRef part, CFStringRef retn, CFStringRef desc) {
    void *temp;

    if (getObjectValue_forString_errorDescription_(this, &temp, part, desc))
        return true;
    NSBeep();
    return false;
}

void OnSpin(void *this, SEL name) {
    double retn = 0.0;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    retn = doubleValue((NSStepper*)ctrl->priv[6]);
    setIntegerValue_((NSTextField*)ctrl->priv[7], retn);
    ctrl->fc2e(ctrl, MSG_NSET, retn);
}

void OnEdit(void *this, SEL name) {
    CTRL *ctrl = 0;
    double retn;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
        return;

    retn = doubleValue((NSTextField*)ctrl->priv[7]);
    ctrl->fe2c(ctrl, MSG_NSET, retn);
}

bool OnKeys(void *this, SEL name, void *ctrl, NSView *view, SEL what) {
    static SEL MoveUp = 0, MoveDown = 0;

    if (!MoveUp) {
        MoveUp = moveUp_();
        MoveDown = moveDown_();
    }
    if ((what == MoveDown) || (what == MoveUp)) {
        CTRL *ctrl = 0;
        double retn;

        GET_IVAR(this, VAR_CTRL, &ctrl);
        if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
            return false;

        retn = doubleValue((void*)ctrl->priv[7]);
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
                activateIgnoringOtherApps_
                    (sharedApplication(NSApplication()), true);
                orderFront_((NSWindow*)ctrl->priv[0], thrd);
            }
            break;
        }
        case MSG_WSZC: {
            CGRect area, scrn;

            scrn.size.width  = (((uint16_t)(data      )) + ctrl->xdim)
                             *   (uint16_t)(ctrl->priv[2]      );
            scrn.size.height = (((uint16_t)(data >> 16)) + ctrl->ydim)
                             *   (uint16_t)(ctrl->priv[2] >> 16);
            ctrl->priv[1] =  (uint16_t)scrn.size.width
                          | ((uint32_t)scrn.size.height << 16);
            area = frameRectForContentRect_((void*)ctrl->priv[0], scrn);
            scrn = visibleFrame(mainScreen(NSScreen()));
            area.origin.x = 0.5 * (scrn.size.width  - area.size.width )
                          + scrn.origin.x;
            area.origin.y = 0.5 * (scrn.size.height - area.size.height)
                          + scrn.origin.y;
            setFrame_display_animate_((NSWindow*)ctrl->priv[0],
                                      area, true, false);
            setMinSize_((NSWindow*)ctrl->priv[0], area.size);
            orderFront_((NSWindow*)ctrl->priv[0],
                         sharedApplication(NSApplication()));
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
            setDoubleValue_((NSProgressIndicator*)ctrl->priv[0],
                             ctrl->priv[1] = data);
            setNeedsDisplay_((NSProgressIndicator*)ctrl->priv[0], true);
            displayIfNeeded((NSProgressIndicator*)ctrl->priv[0]);
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
            break;

        case MSG_PTXT:
            if (ctrl->priv[3])
                CFRelease((void*)ctrl->priv[3]);
            ctrl->priv[3] = (intptr_t)UTF8(data);
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
///  5: array of CFStringRefs (item captions)
///  6: NSTableColumn
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    CFStringRef capt;

    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((NSTableView*)ctrl->priv[7], !!data);
            if (OSX_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setEnabled_(((NSView**)ctrl->priv[2])[cmsg], !!data);
            break;

        case MSG_LCOL:
            setStringValue_(headerCell((NSTableColumn*)ctrl->priv[6]),
                            capt = UTF8(data));
            CFRelease(capt);
            if (OSX_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setState_(((NSView**)ctrl->priv[2])[cmsg],
                             (ctrl->fc2e(ctrl, MSG_LGST, cmsg))?
                              NSOnState : NSOffState);
            reloadData((NSTableView*)ctrl->priv[7]);
            break;

        case MSG_LADD:
            ctrl->priv[5] = (intptr_t)
                realloc((CFStringRef*)ctrl->priv[5],
                       ++ctrl->priv[4] * sizeof(CFStringRef));
            ((CFStringRef*)ctrl->priv[5])[ctrl->priv[4] - 1] = UTF8(data);
            if (OSX_10_07_PLUS) {
                NSView *elem;

                ctrl->priv[2] = (intptr_t)
                    realloc((NSView**)ctrl->priv[2],
                             ctrl->priv[4] * sizeof(NSView**));
                elem = init(alloc((NSButton*)ctrl->priv[1]));
                SET_IVAR(elem, VAR_CTRL, ctrl);
                SET_IVAR(elem, VAR_DATA, ctrl->priv[4] - 1);
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
            CGRect rect;

            rect = frame((void*)ctrl->priv[0]);
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
            ctrl->priv[1] = -(uint16_t)data;
            ctrl->priv[2] = (uint16_t)(data >> 16);
            setMinValue_((NSStepper*)ctrl->priv[6], ctrl->priv[1]);
            setMaxValue_((NSStepper*)ctrl->priv[6], ctrl->priv[2]);
            setDoubleValue_((NSStepper*)ctrl->priv[6], 0);
            OnSpin((NSStepper*)ctrl->priv[6], 0);
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

            rect = frame((NSTextField*)ctrl->priv[0]);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((NSTextField*)ctrl->priv[0], !data);
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
            setFrame_((NSScrollView*)ctrl->priv[0], rect);
            if (ctrl->fc2e) {
                data = rect.size.height;
                rect = frame(verticalScroller((NSScrollView*)ctrl->priv[0]));
                cmsg = ctrl->priv[1] - rect.size.width;
                data += ctrl->fc2e(ctrl, MSG_SMAX, cmsg);
                setFrame_((NSView*)ctrl->priv[7],
                          (CGRect){{0, 0}, {(uint16_t)cmsg, data}});
            }
            setNeedsDisplay_((NSScrollView*)ctrl->priv[0], true);
            break;
        }
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
            SCLS *scls = (SCLS*)ctrl->priv[6];
            long iter;

            /// releasing the formatter
            release((NSNumberFormatter*)ctrl->priv[5]);
            /// releasing classes
            for (iter = sizeof(scls->_sub) / sizeof(*scls->_sub) - 1;
                 iter >= 0; iter--)
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
            release((NSStepper*)ctrl->priv[6]);
            release((NSTextField*)ctrl->priv[7]);
            /// do not release the formatter, it is common between all spins
            break;

        case FCT_LIST:
            /// releasing cells/strings and deleting their common subclass
            if (!OSX_10_07_PLUS) {
                for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--)
                    CFRelease(((CFStringRef*)ctrl->priv[5])[ctrl->priv[4]]);
                release((NSCell*)ctrl->priv[2]);
            }
            else {
                for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--) {
                    release(((NSView**)ctrl->priv[2])[ctrl->priv[4]]);
                    CFRelease(((CFStringRef*)ctrl->priv[5])[ctrl->priv[4]]);
                }
                free((NSView**)ctrl->priv[2]);
            }
            DelClass((Class)ctrl->priv[1]);
            free((CFStringRef*)ctrl->priv[5]);
            release((NSTableColumn*)ctrl->priv[6]);
            release((NSTableView*)ctrl->priv[7]);
            break;

        case FCT_SBOX:
            release((NSView*)ctrl->priv[7]);
            break;

        case FCT_IBOX:
            /// releasing the image context and freeing its backbuffer
            CGContextRelease((CGContextRef)ctrl->priv[1]);
            free((void*)ctrl->priv[2]);
            break;

        case FCT_PBAR:
            /// releasing text attributes and the string
            CFRelease((CFArrayRef)ctrl->priv[5]);
            if (ctrl->priv[3])
                CFRelease((CFStringRef)ctrl->priv[3]);
            break;
    }
    release((NSView*)ctrl->priv[0]);
}



NSInteger OnRows(void *this, SEL name, NSTableView *view) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return ctrl->priv[4];
}

NSCell *OnValueOld(void *this, SEL name, NSTableView *view,
                   NSTableColumn *icol, NSInteger irow) {
    return (OSX_10_07_PLUS)? 0 : dataCell(icol);
}

NSCell *OnValue(void *this, SEL name, NSTableView *view,
                NSTableColumn *icol, NSInteger irow) {
    NSCell *cell;
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    cell = (OSX_10_07_PLUS)?
           ((NSCell**)ctrl->priv[2])[irow] : dataCell(icol);
    setTitle_(cell, ((CFStringRef*)ctrl->priv[5])[irow]);
    setState_(cell, (ctrl->fc2e(ctrl, MSG_LGST, irow))?
                     NSOnState : NSOffState);
    return cell;
}

void OnReset(void *this, SEL name, NSTableView *view,
             void *what, NSTableColumn *icol, NSInteger irow) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_LSST,
              (irow << 1) | (ctrl->fc2e(ctrl, MSG_LGST, irow) ^ 1));
}

void OnListButton(void *this, SEL name) {
    CTRL *ctrl = 0;
    intptr_t irow;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    GET_IVAR(this, VAR_DATA, &irow);
    OnReset(0, 0, this, 0, 0, irow);
}

void OnButton(void *this, SEL name) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_BCLK,
              ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
              (state((NSButton*)ctrl->priv[0]) == NSOnState) : 0);
}

void PBoxDraw(void *this, SEL name, CGRect rect) {
    struct objc_super prev = {this, class(NSProgressIndicator())};
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    objc_msgSendSuper(&prev, drawRect_(), rect);
    rect = frame((void*)ctrl->priv[0]);
    rect.origin.y = ctrl->priv[4];
    rect.origin.x = 0;
    if (OSX_10_10_PLUS) {
        rect.origin.y *= 0.25;
        rect.size.width *= 0.5;
        rect.size.height *= 0.5;
    }
    drawInRect_withAttributes_((void*)ctrl->priv[3], rect,
                               (CFDictionaryRef)ctrl->priv[5]);
}

void IBoxDraw(void *this, SEL name, CGRect rect) {
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
    CGContextDrawImage(graphicsPort(currentContext(NSGraphicsContext())),
                       area, pict);
    CGImageRelease(pict);
}

bool OnFalse(void *this, SEL name) {
    return false;
}

bool OnTrue(void *this, SEL name) {
    return true;
}

bool OnClose(void *this, SEL name) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->priv[2] = 0; /// requesting a halt, or just crashing if CTRL is 0
                       /// (the program is stopping anyway)
    stop_(sharedApplication(NSApplication()), this);
    return true;
}

void OnSize(void *this, SEL name) {
    CTRL *ctrl = 0;
    CGRect rect;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    rect = frame((void*)ctrl->priv[0]);
    rect = contentRectForFrameRect_((void*)ctrl->priv[0], rect);
    ctrl->priv[1] =  (uint16_t)rect.size.width
                  | ((uint32_t)rect.size.height << 16);
    ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    #define CLS_MAKE(r, p, n, f, ...) \
        do { void **CLS_MAKE = PutToArr(__VA_ARGS__); \
             r = NewClass(p, n, f, CLS_MAKE); free(CLS_MAKE); } while (0)
    CTRL *root;
    SCLS *scls;
    CFStringRef capt;
    CGRect dims;
    void *gwnd;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        NSApplication *thrd;
        CGPoint fadv;
        CGFloat ffsz;
        void **vfld;

        ctrl->fe2c = FE2CW;
        ffsz = systemFontSize(NSFont());
        gwnd = systemFontOfSize_(NSFont(), ffsz);
        fadv = maximumAdvancement(gwnd);
        ctrl->priv[2] =  (uint16_t)round(0.45 * fadv.x)
                      | ((uint32_t)round(0.60 * ffsz) << 16);

        vfld = PutToArr(VAR_CTRL);
        ctrl->priv[6] = (intptr_t)(scls = calloc(1, sizeof(*scls)));
        CLS_MAKE(scls->wndw, NSView(),              "rNSW", vfld,
                 windowShouldClose_(), OnClose,
                 windowDidResize_(), OnSize, isFlipped(), OnTrue);
        CLS_MAKE(scls->text, NSTextField(),         "rNST", vfld,
                 ActionSelector(), OnEdit,
                 control_textView_doCommandBySelector_(), OnKeys);
        CLS_MAKE(scls->butn, NSButton(),            "rNSB", vfld,
                 ActionSelector(), OnButton);
        CLS_MAKE(scls->spin, NSStepper(),           "rNSN", vfld,
                 ActionSelector(), OnSpin);
        CLS_MAKE(scls->list, NSTableView(),         "rNSL", vfld,
                (OSX_10_07_PLUS)? tableView_viewForTableColumn_row_()
               : tableView_dataCellForTableColumn_row_(),        OnValue,
                 tableView_objectValueForTableColumn_row_(),     OnValueOld,
                 tableView_setObjectValue_forTableColumn_row_(), OnReset,
                 numberOfRowsInTableView_(),                     OnRows);
        CLS_MAKE(scls->pbar, NSProgressIndicator(), "rNSP", vfld,
                 drawRect_(), PBoxDraw);
        CLS_MAKE(scls->sbox, NSView(),              "rNSS", vfld,
                 isFlipped(), OnTrue);
        CLS_MAKE(scls->ibox, NSView(),              "rNSI", vfld,
                 drawRect_(), IBoxDraw);
        CLS_MAKE(scls->frmt, NSNumberFormatter(),   "rNSF", vfld,
                 isPartialStringValid_newEditingString_errorDescription_(),
                 OnValidate);
        free(vfld);

        ctrl->priv[5] = (intptr_t)init(alloc(scls->frmt));
        setFormatterBehavior_((NSNumberFormatter*)ctrl->priv[5],
                               NSNumberFormatterBehavior10_4);
        setNumberStyle_((NSNumberFormatter*)ctrl->priv[5],
                         kCFNumberFormatterNoStyle);
        setPartialStringValidationEnabled_((NSNumberFormatter*)ctrl->priv[5],
                                            true);
        dims = (CGRect){};
        gwnd = initWithContentRect_styleMask_backing_defer_
                   (alloc(NSWindow()), dims, NSTitledWindowMask
                                           | NSClosableWindowMask
                                           | NSResizableWindowMask
                                           | NSMiniaturizableWindowMask,
                    kCGBackingStoreBuffered, false);

        ctrl->priv[7] = (intptr_t)init(alloc(scls->wndw));
        setContentView_(gwnd, (NSView*)ctrl->priv[7]);
        setDelegate_(gwnd, (NSView*)ctrl->priv[7]);
        setTitle_(gwnd, capt = UTF8(text));
        CFRelease(capt);
        SET_IVAR((NSView*)ctrl->priv[7], VAR_CTRL, ctrl);
        makeKeyWindow(gwnd);
        thrd = sharedApplication(NSApplication());
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
                setAction_(gwnd, ActionSelector());
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
                gwnd = init(alloc(NSView()));
                ctrl->priv[3] = root->priv[5];
                ctrl->priv[6] = (intptr_t)init(alloc(scls->spin));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->text));
                SET_IVAR((NSStepper*)ctrl->priv[6], VAR_CTRL, ctrl);
                SET_IVAR((NSTextField*)ctrl->priv[7], VAR_CTRL, ctrl);
                spin = cellSize(cell((NSStepper*)ctrl->priv[6]));
                temp.size = dims.size;
                temp.size.width -= spin.x;
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
                temp.size.width = spin.x;
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
                void **vfld, **vmet;
                CGRect temp = {};

                ctrl->fe2c = FE2CL;
                temp.size = dims.size;
                ctrl->priv[1] = (intptr_t)NewClass
                    ((OSX_10_07_PLUS)? NSButton() : NSButtonCell(),
                     "rNSX", vfld = PutToArr(VAR_CTRL, VAR_DATA),
                      vmet = PutToArr(ActionSelector(), OnListButton));
                free(vmet);
                free(vfld);

                gwnd = init(alloc(NSScrollView()));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->list));
                SET_IVAR((NSTableView*)ctrl->priv[7], VAR_CTRL, ctrl);
                setDataSource_((NSTableView*)ctrl->priv[7],
                               (NSTableView*)ctrl->priv[7]);
                setDelegate_((NSTableView*)ctrl->priv[7],
                             (NSTableView*)ctrl->priv[7]);
                setFrame_((NSTableView*)ctrl->priv[7], temp);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn()));
                setStringValue_(headerCell((NSTableColumn*)ctrl->priv[6]),
                                capt = UTF8(0));
                CFRelease(capt);
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
                setHasVerticalScroller_(gwnd, true);
                setDrawsBackground_(gwnd, false);
                ctrl->priv[7] = (intptr_t)init(alloc(scls->sbox));
                setDocumentView_(gwnd, (NSView*)ctrl->priv[7]);
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
                CGFloat ffsz;
                NSFont *font;
                void *psty;

                ctrl->fe2c = FE2CP;
                gwnd = init(alloc(scls->pbar));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setIndeterminate_(gwnd, false);
                setWantsLayer_(gwnd, true);
                psty = init(alloc(NSMutableParagraphStyle()));
                setAlignment_(psty, NSCenterTextAlignment);
                ffsz = systemFontSize(NSFont()) * 0.85;
                if (OSX_10_10_PLUS) {
                    scaleUnitSquareToSize_(gwnd, (CGSize){2.0, 2.0});
                    ffsz *= 0.5;
                }
                font = systemFontOfSize_(NSFont(), ffsz);
                ctrl->priv[5] = (intptr_t)MakeDict
                    (kCTFontAttributeName,           font,
                     kCTParagraphStyleAttributeName, psty);
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
    #undef CLS_MAKE
}



int main(int argc, char *argv[]) {
    void **vmet, *pool, *menu;
    CFStringRef path;
    CFArrayRef urls;
    CGFloat icon;
    CGRect dims;
    char *conf;
    FIND find = {};

    pool = init(alloc(NSAutoreleasePool()));
    setActivationPolicy_(sharedApplication(NSApplication()),
                         NSApplicationActivationPolicyAccessory);

    find.home = CopyUTF8(path =
                        (CFStringRef)bundlePath(mainBundle(NSBundle())));
    find.home = realloc(find.home, find.hlen = strlen(find.home) + 32);
    strcat(find.home, "/Contents/MacOS/"DEF_FLDR"/");
    find.iter = scandir(find.home, &find.dirs, 0, alphasort);

    urls = URLsForDirectory_inDomains_(defaultManager(NSFileManager()),
                                       NSApplicationSupportDirectory,
                                       NSUserDomainMask);
    path = CFURLCopyFileSystemPath
               (CFArrayGetValueAtIndex((CFArrayRef)urls, 0),
                                        kCFURLPOSIXPathStyle);
    conf = CopyUTF8(path);
    CFRelease(path);
    conf = realloc(conf, strlen(conf) + 32);
    strcat(conf, DEF_OPTS);
    if (!((mkdir(conf, 0755))? (errno != EEXIST)? 0 : 1 : 2))
        printf("WARNING: cannot create '%s'!", conf);

    dims = visibleFrame(mainScreen(NSScreen()));
    icon = thickness(systemStatusBar(NSStatusBar()));
    menu = NewClass(NSObject(), CLS_MENU, 0,
                    vmet = PutToArr(ActionSelector(), OnMenu));
    free(vmet);
    eExecuteEngine(conf, (intptr_t)&find, icon, icon, dims.origin.x,
                   dims.origin.y, dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    DelClass((Class)menu);
    release(pool);
    free(conf);
    return 0;
}
