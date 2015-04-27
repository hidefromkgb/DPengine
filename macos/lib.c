#include <limits.h>
#include <sys/time.h>
#include <ogl/oglstd.h>
#include <core.h>

#include "mac.h"

/// name of the instance variable to access the engine structure
#define ENGD_ACCESSOR "e"



const char *StringObjCClasses[] = {STRING_OBJC_CLASSES, 0};
id LoadedObjCClasses[countof(StringObjCClasses)] = {};

const char *StringObjCSelectors[] = {STRING_OBJC_SELECTORS, 0};
SEL LoadedObjCSelectors[countof(StringObjCSelectors)] = {};



void LoadObjC() {
    long iter;

    if (!*LoadedObjCClasses) {
        iter = -1;
        while (StringObjCClasses[++iter])
            LoadedObjCClasses[iter] =
                objc_getClass(StringObjCClasses[iter]);
        iter = -1;
        while (StringObjCSelectors[++iter])
            LoadedObjCSelectors[iter] =
                sel_registerName(StringObjCSelectors[iter]);
    }
}



id Subclass(id base, char *name, char *flds[], OMSC *mths) {
    Class retn = objc_allocateClassPair((Class)base, name, 0);
    long iter;

    iter = -1;
    /// adding fields
    while (flds[++iter])
        class_addIvar(retn, flds[iter],
                      sizeof(id), (sizeof(id) >= 8)? 3 : 2, 0);
    iter = -1;
    /// overloading methods
    while (mths[++iter].func)
        class_addMethod(retn, mths[iter].name, mths[iter].func, 0);

    objc_registerClassPair(retn);
    return (id)retn;
}



inline id UTF8(void *utf8) {
    return stringWithUTF8String_(NSString, utf8);
}



void RestartEngine(ENGD *engd, ulong rscm) {
    engd->draw = rscm;
    stop_(sharedApplication(NSApplication), 0);
}



void ShowMainWindow(ENGD *engd, ulong show) {
    id thrd = sharedApplication(NSApplication);

    if (show)
        unhide_(thrd, 0);
    else
        hide_(thrd, 0);
}



inline MENU *OSSpecificMenu(ENGD *engd) {
    return NULL;
}



void EngineOpenContextMenu(MENU *menu) {
    /// [TODO]
}



char *LoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        read(file, retn, flen);
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



char *ConvertUTF8(char *utf8) {
    return strdup(utf8);
}



long CountCPUs() {
    return min(sizeof(SEM_TYPE) * 8, max(1, sysconf(_SC_NPROCESSORS_ONLN)));
}



void MakeThread(THRD *thrd) {
    pthread_t pthr;
    pthread_create(&pthr, 0, (void *(*)(void*))ThrdFunc, thrd);
}



void FreeSemaphore(SEMD *retn, long nthr) {
    pthread_cond_destroy(&retn->cvar);
    pthread_mutex_destroy(&retn->cmtx);
}



void MakeSemaphore(SEMD *retn, long nthr, SEM_TYPE mask) {
    pthread_mutex_init(&retn->cmtx, 0);
    pthread_cond_init(&retn->cvar, 0);
    retn->full = (1 << nthr) - 1;
    retn->list = retn->full & mask;
}



long PickSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *drop = (open)? &engd->osem : &engd->isem,
         *pick = (open)? &engd->isem : &engd->osem;

    open = (__sync_fetch_and_and(&drop->list, ~(drop->full & mask)) & mask)?
            TRUE : FALSE;
    __sync_or_and_fetch(&pick->list, pick->full & mask);

    pthread_mutex_lock(&pick->cmtx);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return open;
}



SEM_TYPE WaitSemaphore(ENGD *engd, long open, SEM_TYPE mask) {
    SEMD *wait = (open)? &engd->osem : &engd->isem;

    pthread_mutex_lock(&wait->cmtx);
    if (mask != SEM_NULL)
        while ((wait->list ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



uint64_t TimeFunc() {
    struct timeval spec = {};

    gettimeofday(&spec, 0);
    return spec.tv_sec * 1000 + spec.tv_usec / 1000;
}



void InitRenderer(ENGD *engd) {
    switch (engd->rscm) {
        case SCM_RSTD:
            SwitchThreads(engd, 1);
            break;

        case SCM_ROGL: {
            engd->rndr = MakeRendererOGL(engd->uarr, engd->uniq,
                                         engd->size, 0);
            SizeRendererOGL(engd->rndr, engd->pict.xdim, engd->pict.ydim);
            break;
        }
    }
}



id Submenu(MENU *menu, id base) {
    if (!menu)
        return 0;

    id retn = init(alloc(NSMenu)),
       cbtn = imageNamed_(NSImage, UTF8("NSMenuCheckmark")),
       rbtn = imageNamed_(NSImage, UTF8("NSMenuRadio")),
       null = UTF8(""),
       item;

    setAutoenablesItems_(retn, false);
    while (menu->text) {
        if (!*menu->text) {
            item = separatorItem(NSMenuItem);
            addItem_(retn, item);
        }
        else {
            item = initWithTitle_action_keyEquivalent_
                  (alloc(NSMenuItem), UTF8(menu->text), MenuSelector, null);
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
            release(null);
        }
        menu++;
    }
    return retn;
}



void OnMenu(id send, id bred, id this) {
    MENU *item = (typeof(item))tag(this);

    if (item) {
        ProcessMenuItem(item);
        if (item->flgs & MFL_CCHK)
            setState_(this, (item->flgs & MFL_VCHK)? NSOnState : NSOffState);
    }
}



void OnTime(CFRunLoopTimerRef time, void *data) {
    *(uint64_t*)data = TimeFunc();
}



void OnFPS(CFRunLoopTimerRef time, void *data) {
    ENGD *engd = (typeof(engd))data;
    char fout[64];

    OutputFPS(engd, fout);
    printf("%s\n", fout);
}



void OnCalc(CFRunLoopTimerRef time, void *data) {
    static CGPoint (*GetT2DV)(id, SEL) = (typeof(GetT2DV))
#ifdef __i386__
        /// unsure if this is the correct function
        objc_msgSend_fpret;
#else
        objc_msgSend;
#endif

    ENGD *engd = (typeof(engd))data;
    long pick, flgs;
    CGPoint dptr;

    engd->tfrm = engd->time;
    if (!engd->draw)
        return;

    dptr = GetT2DV((id)engd->user[2], mouseLocationOutsideOfEventStream);
    dptr.y = engd->pict.ydim - dptr.y;
    pick = pressedMouseButtons(NSEvent);

    /// [TODO] properly determine if the window is active
    flgs = 1;

    flgs = ((pick & 1)? UFR_LBTN : 0) | ((pick & 2)? UFR_RBTN : 0)
         | ((pick & 4)? UFR_MBTN : 0) | ((flgs)? UFR_MOUS : 0);
    pick = SelectUnit(engd->uarr, engd->data, engd->size, dptr.x, dptr.y);
    engd->size = engd->ufrm((uintptr_t)engd, engd->udat, engd->data,
                            &engd->time, flgs, dptr.x, dptr.y, pick);
    if (!engd->size) {
        RestartEngine(engd, SCM_QUIT);
        return;
    }
    switch (engd->rscm) {
        case SCM_RSTD: {
            CGContextSetBlendMode((CGContextRef)engd->user[0],
                                  kCGBlendModeClear);
            CGContextFillRect((CGContextRef)engd->user[0],
                              (CGRect){{0, 0},
                                       {engd->pict.xdim, engd->pict.ydim}});
            PickSemaphore(engd, 1, SEM_FULL);
            WaitSemaphore(engd, 1, SEM_FULL);
            break;
        }
        case SCM_ROGL: {
            DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                            engd->size, engd->flgs & COM_IOPQ);
            break;
        }
    }
    setNeedsDisplay_((id)engd->user[1], true);
    engd->fram++;
}



void OnDraw(CGRect rect, id this) {
    ENGD *engd;

    object_getInstanceVariable(this, ENGD_ACCESSOR, (void**)&engd);
    CGRect full = {{0, 0}, {engd->pict.xdim, engd->pict.ydim}};

    if (!engd->draw)
        return;

    switch (engd->rscm) {
        case SCM_RSTD: {
            CGContextRef ctxt =
                (typeof(ctxt))graphicsPort(currentContext(NSGraphicsContext));
            CGContextSetBlendMode(ctxt, kCGBlendModeCopy);

            CGImageRef iref =
                CGBitmapContextCreateImage((CGContextRef)engd->user[0]);
            CGContextDrawImage(ctxt, full, iref);
            CGImageRelease(iref);
            break;
        }
        case SCM_ROGL:
            flushBuffer(openGLContext(this));
            break;
    }
}



CFRunLoopTimerRef AddTimer(ulong time, void *func, void *data) {
    CFRunLoopTimerContext ctxt = {0, data};
    CFRunLoopTimerRef retn =
        CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent(),
                             0.001 * time, 0, 0, func, &ctxt);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), retn, kCFRunLoopCommonModes);
    return retn;
}



void RunMainLoop(ENGD *engd) {
    INCBIN("../core/icon.gif", MainIcon);

    static CGFloat (*GetT1DV)(id, SEL) = (typeof(GetT1DV))
#ifdef __i386__
        objc_msgSend_fpret;
#else
        objc_msgSend;
#endif

    /// very important call; without it, nothing below would ever work
    LoadObjC();

//    printf("BEGIN\n");

    /// view delegate class, view delegate instance
    id vdlg, view;

    /// view delegate`s methods (line 1) and custom fields (line 2)
    OMSC  vmet[] = {{drawRect_, OnDraw}, {MenuSelector, OnMenu}, {}};
    char *vfld[] = {ENGD_ACCESSOR, 0};

    id pool = init(alloc(NSAutoreleasePool));
    id thrd = sharedApplication(NSApplication);
    setActivationPolicy_(thrd, NSApplicationActivationPolicyAccessory);

    CGRect dims = {{0, 0}, {engd->pict.xdim, engd->pict.ydim}};

    id hwnd = initWithContentRect_styleMask_backing_defer_
             (alloc(NSWindow), dims, NSClosableWindowMask,
              NSBackingStoreBuffered, false);
    setLevel_(hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(hwnd, clearColor(NSColor));
    setHasShadow_(hwnd, false);
    setOpaque_(hwnd, false);

    switch (engd->rscm) {
        case SCM_ROGL:
            if (LoadOpenGLFunctions() > 0) {
                id pfmt, ctxt;
                GLint attr[] = {NSOpenGLPFADoubleBuffer,
                                NSOpenGLPFADepthSize, 32, 0},
                      opaq = 0;

                vdlg = Subclass(NSOpenGLView, "MyOpenGLView", vfld, vmet);
                pfmt = initWithAttributes_(alloc(NSOpenGLPixelFormat), attr);
                view = initWithFrame_pixelFormat_(alloc(vdlg), dims, pfmt);

                makeCurrentContext((ctxt = openGLContext(view)));
                setValues_forParameter_(ctxt, &opaq, NSOpenGLCPSurfaceOpacity);
                release(pfmt);
                break;
            }
            printf(TXL_FAIL" %s\n", engd->tran[TXT_NOGL]);
            engd->rscm = SCM_RSTD;
            /// falling through

        case SCM_RSTD: {
            CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
            engd->user[0] = (typeof(*engd->user))
                CGBitmapContextCreate(0, engd->pict.xdim, engd->pict.ydim,
                                      CHAR_BIT, engd->pict.xdim
                                      * sizeof(*engd->pict.bptr), drgb,
                                      kCGImageAlphaPremultipliedFirst
                                    | kCGBitmapByteOrder32Little);
            engd->pict.bptr =
                CGBitmapContextGetData((CGContextRef)engd->user[0]);
            CGColorSpaceRelease(drgb);

            vdlg = Subclass(NSView, "MyView", vfld, vmet);
            view = initWithFrame_(alloc(vdlg), dims);
            break;
        }
    }
    object_setInstanceVariable(view, ENGD_ACCESSOR, engd);
    setContentView_(hwnd, view);
    setDelegate_(hwnd, view);
    InitRenderer(engd);

    id sbar = systemStatusBar(NSStatusBar);
    id icon = statusItemWithLength_(sbar, NSVariableStatusItemLength);
    setHighlightMode_(icon, true);

    size_t sdim = GetT1DV(sbar, thickness);

    /// the size is wrong, but let it be: MainIcon does have a GIF ending
    ASTD *igif = MakeDataAnimStd(MainIcon, 1024 * 1024);
    BGRA *bptr = ExtractRescaleSwizzleAlign(igif, 0xC6, 0, sdim, sdim);
    FreeAnimStd(&igif);

    CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctxt =
        CGBitmapContextCreate(bptr, sdim, sdim, CHAR_BIT, sdim * sizeof(*bptr),
                              drgb, kCGImageAlphaPremultipliedFirst
                                  | kCGBitmapByteOrder32Little);
    CGImageRef iref = CGBitmapContextCreateImage(ctxt);
    CGColorSpaceRelease(drgb);

    id ibmp = initWithCGImage_size_(alloc(NSImage), iref, (CGPoint){});
    setImage_(icon, ibmp);
    release(ibmp);
    CGImageRelease(iref);
    CGContextRelease(ctxt);
    free(bptr);

    id menu = Submenu(engd->menu, view);
    setMenu_(icon, menu);
    release(menu);

    engd->user[1] = (typeof(*engd->user))view;
    engd->user[2] = (typeof(*engd->user))hwnd;
    CFRunLoopTimerRef tmrt = AddTimer(   1, OnTime, &engd->time),
                      tmrf = AddTimer(1000, OnFPS,   engd),
                      tmrd = AddTimer(  32, OnCalc,  engd);

    activateIgnoringOtherApps_(thrd, true);
    makeKeyAndOrderFront_(hwnd, hwnd);
    set(pointingHandCursor(NSCursor)); /// why does this change nothing?

    /// run loop starts here
    run(thrd);

    CFRunLoopTimerInvalidate(tmrd);
    CFRunLoopTimerInvalidate(tmrf);
    CFRunLoopTimerInvalidate(tmrt);

    switch (engd->rscm) {
        case SCM_RSTD:
            StopThreads(engd);
            CGContextRelease((CGContextRef)engd->user[0]);
            break;

        case SCM_ROGL: {
            FreeRendererOGL(engd->rndr);
            break;
        }
    }
    release(view);
    release(hwnd);
    release(pool);
    objc_disposeClassPair((Class)vdlg);

//    printf("END!\n");
}
