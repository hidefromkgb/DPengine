#include <limits.h>
#include <sys/time.h>
#include <ogl/oglstd.h>
#include <core.h>

/// contains variable definitions and function implementations
/// shall -NOT- be reincluded
#include "mac.h"



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



void MenuResponder(id send) {
    MENU *item =
        (typeof(item))objc_getAssociatedObject(send, MenuResponderSelector);

    if (item) {
        ProcessMenuItem(item);
        if (item->flgs & MFL_CCHK)
            setState_(objc_getAssociatedObject(send, MenuResponder),
                     (item->flgs & MFL_VCHK)? NSOnState : NSOffState);
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
                  (alloc(NSMenuItem), UTF8(menu->text),
                   MenuResponderSelector, null);
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
            setEnabled_(item, (menu->flgs & MFL_GRAY)? false : true);
            id prox = alloc(base);
            objc_setAssociatedObject(prox, MenuResponderSelector, (id)menu,
                                     OBJC_ASSOCIATION_ASSIGN);
            objc_setAssociatedObject(prox, MenuResponder, item,
                                     OBJC_ASSOCIATION_ASSIGN);
            setTarget_(item, prox);
            addItem_(retn, item);
            /// SHALL BE FREED, BUT IT BREAKS MENU HANDLING; MEMLEAK FOR NOW
//            release(prox);
            release(item);
            release(null);
        }
        menu++;
    }
    return retn;
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
    ENGD *engd = (typeof(engd))data;
    long pick, flgs;
    CGPoint spot;

    engd->tfrm = engd->time;
    if (!engd->draw)
        return;

//    mouseLocationOutsideOfEventStream((id)engd->user[2], &spot);

    flgs = ((/**  left  mouse button pressed **/0)? UFR_LBTN : 0)
         | ((/** middle mouse button pressed **/0)? UFR_MBTN : 0)
         | ((/**  right mouse button pressed **/0)? UFR_RBTN : 0)
         | ((/** the main window got focused **/0)? UFR_MOUS : 0);
    pick = SelectUnit(engd->uarr, engd->data, engd->size, spot.x, spot.y);
    engd->size = engd->ufrm((uintptr_t)engd, engd->udat, engd->data,
                            &engd->time, flgs, spot.x, spot.y, pick);
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
            /// get current context here
            ///
            DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                            engd->size, engd->flgs & COM_IOPQ);
            break;
        }
    }
    setNeedsDisplay_((id)engd->user[1], true);
    engd->fram++;
}



void OnDraw(CGRect rect) {
    id thrd = sharedApplication(NSApplication);
    ENGD *engd = (typeof(engd))objc_getAssociatedObject(thrd, RunMainLoop);
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
        case SCM_ROGL: {
            break;
        }
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

    static SEL DrawRect;

    /// very important call; without it, nothing below would ever work
    if (LoadObjC()) {
        DrawRect = N("drawRect:");
        MenuResponderSelector = N("_M");
    }
//    printf("BEGIN\n");

    /// menu delegate class, view delegate class, view delegate instance
    id mdlg, vdlg, view;

    id pool = init(alloc(NSAutoreleasePool));
    id thrd = sharedApplication(NSApplication);
    setActivationPolicy_(thrd, NSApplicationActivationPolicyAccessory);
    objc_setAssociatedObject(thrd, RunMainLoop, (id)engd,
                             OBJC_ASSOCIATION_ASSIGN);

    CGRect dims = {{0, 0}, {engd->pict.xdim, engd->pict.ydim}};

    id hwnd = initWithContentRect_styleMask_backing_defer_
             (alloc(NSWindow), dims, NSClosableWindowMask,
              NSBackingStoreBuffered, false);
    setLevel_(hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(hwnd, colorWithCalibratedRed_green_blue_alpha_(NSColor, 0.0, 0.0, 0.0, 0.0));
    setHasShadow_(hwnd, false);
    setOpaque_(hwnd, false);

    /// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME
    engd->rscm = SCM_RSTD;

    switch (engd->rscm) {
        case SCM_ROGL:
            if (!LoadOpenGLFunctions()) {
                printf(TXL_FAIL" %s\n", engd->tran[TXT_NOGL]);
                engd->rscm = SCM_RSTD;
            }
            else {
                vdlg = Overload(NSOpenGLView, "MyNSOpenGLView",
                               (OVER[]){{DrawRect, OnDraw}, {}});
                uint32_t attr[] = {
                    NSOpenGLPFADoubleBuffer,
                    NSOpenGLPFADepthSize, 32,
                    0
                };
                id pfmt =
                    initWithAttributes_(alloc(NSOpenGLPixelFormat), attr);
                view = initWithFrame_pixelFormat_(alloc(vdlg), dims, pfmt);
                setContentView_(hwnd, view);
                setDelegate_(hwnd, view);
                break;
            }
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

            vdlg = Overload(NSView, "MyNSView",
                           (OVER[]){{DrawRect, OnDraw}, {}});
            view = initWithFrame_(alloc(vdlg), dims);
            setContentView_(hwnd, view);
            setDelegate_(hwnd, view);
            break;
        }
    }
    InitRenderer(engd);

    id sbar = systemStatusBar(NSStatusBar);
    id icon = statusItemWithLength_(sbar, NSVariableStatusItemLength);
    setHighlightMode_(icon, true);



    size_t sdim = thickness(sbar);

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



    mdlg = Overload(NSMenuItem, "MyNSMenuItem",
                   (OVER[]){{MenuResponderSelector, MenuResponder}, {}});
    id menu = Submenu(engd->menu, mdlg);
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

    /// dispatch loop starts here
    run(thrd);

    CFRunLoopTimerInvalidate(tmrd);
    CFRunLoopTimerInvalidate(tmrf);
    CFRunLoopTimerInvalidate(tmrt);
    release(view);
    release(hwnd);
    release(pool);
    objc_disposeClassPair((Class)mdlg);
    objc_disposeClassPair((Class)vdlg);

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
//    printf("END!\n");
}
