#include <sys/time.h>

#include <ogl/oglstd.h>
#include <core.h>

#include "mac.h"



/// name for NSApplication delegate subclass
#define SUB_ADLG "a"
/// name for NSView or NSOpenGLView delegate subclass
#define SUB_VDLG "v"

/// name of the instance variable to access the engine structure
#define VAR_ENGD "e"
/// name of the instance variable to access the window
#define VAR_HWND "w"
/// name of the instance variable to access the view
#define VAR_VIEW "i"



CGFloat (*GetT1DV)(id, SEL) = (typeof(GetT1DV))
#ifdef __i386__
    /// unsure if this is the correct function
    objc_msgSend_fpret;
#else
    objc_msgSend;
#endif

CGPoint (*GetT2DV)(id, SEL) = (typeof(GetT2DV))
#ifdef __i386__
    /// unsure if this is the correct function
    objc_msgSend_fpret;
#else
    objc_msgSend;
#endif

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
                (id)objc_getClass(StringObjCClasses[iter]);
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
    id thrd = sharedApplication(NSApplication);
    CGPoint dptr = {};

    engd->draw = rscm;
    stop_(thrd, 0);
    /// if called from inside a timer context, bump an event to the loop
    /// to ensure that it did process the stop notification we just sent;
    /// if not in a timer context, a dummy event will be rejected anyway
    postEvent_atStart_(thrd, MakeEvent(NSApplicationDefined, dptr,
                                       NSApplicationDefined, 0, 0, 0), true);
}



void ShowMainWindow(ENGD *engd, ulong show) {
    id thrd = sharedApplication(NSApplication);

    if (show)
        unhide_(thrd, 0);
    else
        hide_(thrd, 0);
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



inline MENU *OSSpecificMenu(ENGD *engd) {
    return 0;
}



void EngineOpenContextMenu(MENU *menu) {
    id temp, ievt, thrd;
    CGPoint dptr;

    thrd = delegate(sharedApplication(NSApplication));
    GET_IVAR(thrd, VAR_HWND, &temp);
    dptr = GetT2DV(temp, mouseLocationOutsideOfEventStream);

    ievt = MakeEvent(NSApplicationDefined, dptr, NSApplicationDefined, 0.0,
                     windowNumber(temp), currentContext(NSGraphicsContext));

    GET_IVAR(thrd, VAR_VIEW, &temp);
    thrd = Submenu(menu, temp);
    popUpContextMenu_withEvent_forView_(NSMenu, thrd, ievt, temp);
    release(thrd);
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
    return min(sizeof(SEM_TYPE) * CHAR_BIT,
               max(1, sysconf(_SC_NPROCESSORS_ONLN)));
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



/// NAME holds the selector associated with this function
void OnMenu(id send, SEL name, id this) {
    MENU *item = (typeof(item))tag(this);

    if (item) {
        ProcessMenuItem(item);
        if (item->flgs & MFL_CCHK)
            setState_(this, (item->flgs & MFL_VCHK)? NSOnState : NSOffState);
    }
}



bool OnOpaq() {
    return false;
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
    engd->size = engd->ufrm((uintptr_t)engd, engd->udat, &engd->data,
                            &engd->time, flgs, dptr.x, dptr.y,
                             SelectUnit(engd->uarr, engd->data, engd->size,
                                        dptr.x, dptr.y));
    if (!engd->size) {
        RestartEngine(engd, SCM_QUIT);
        return;
    }
    /// [TODO] manual mouse transparency does not work, commented out for now
//    setIgnoresMouseEvents_((id)engd->user[2],
//                           (pick >= 0) || (engd->flgs & COM_IOPQ));
    switch (engd->rscm) {
        case SCM_RSTD: {
            CGContextSetBlendMode((CGContextRef)engd->user[0],
                                  kCGBlendModeClear);
            CGContextFillRect((CGContextRef)engd->user[0],
                              (CGRect){{0, 0},
                                       {engd->pict.xdim, engd->pict.ydim}});
            SwitchThreads(engd, 1);
            PickSemaphore(engd, 1, SEM_FULL);
            WaitSemaphore(engd, 1, SEM_FULL);
            break;
        }
        case SCM_ROGL: {
            if (MakeRendererOGL((ROGL**)&engd->rndr, engd->uarr,
                                         engd->uniq, engd->size, 0))
                SizeRendererOGL(engd->rndr, engd->pict.xdim, engd->pict.ydim);
            DrawRendererOGL(engd->rndr, engd->uarr, engd->data,
                            engd->size, engd->flgs & COM_IOPQ);
            break;
        }
    }
    setNeedsDisplay_((id)engd->user[1], true);
    engd->fram++;
}



void OnDraw(CGRect rect) {
    ENGD *engd;

    GET_IVAR(delegate(sharedApplication(NSApplication)), VAR_ENGD, &engd);

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
            flushBuffer(openGLContext((id)engd->user[1]));
            break;
    }
}


/*
void OnRect(id this) {
    id curs;
    CGRect dims;
    ENGD *engd;

    GET_IVAR(this, VAR_ENGD, &engd);
    dims = (CGRect){{0, 0}, {engd->pict.xdim, engd->pict.ydim}};
    curs = pointingHandCursor(NSCursor);
    addCursorRect_cursor_(this, dims, curs);
    objc_msgSend(curs, sel_registerName("setOnMouseEntered:"), true);
}
//*/


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

    /// very important call; without it, nothing below would ever work
    LoadObjC();

//    printf("BEGIN\n");

    /// view delegate`s methods (line 1) and custom fields (line 2)
    OMSC  vmet[] = {{drawRect_, OnDraw}, {MenuSelector, OnMenu},
                    {isOpaque,  OnOpaq}, /*{resetCursorRects, OnRect},*/{}};
    char *vfld[] = {0};

    /// app delegate class, app delegate instance
    id adlg, ains;
    /// view delegate class, view delegate instance
    id vdlg, vins;

    adlg = Subclass(NSObject, SUB_ADLG,
                   (char*[]){VAR_HWND, VAR_ENGD, VAR_VIEW, 0}, (OMSC[]){{}});

    id pool = init(alloc(NSAutoreleasePool));
    id thrd = sharedApplication(NSApplication);
    setActivationPolicy_(thrd, NSApplicationActivationPolicyAccessory);
    setDelegate_(thrd, (ains = init(alloc(adlg))));

    CGRect dims = {{0, 0}, {engd->pict.xdim, engd->pict.ydim}};
    switch (engd->rscm) {
        case SCM_ROGL: {
            GLchar *retn;

            if (!(retn = LoadOpenGLFunctions(NV_vertex_program3))) {
                id pfmt, ctxt;
                GLint attr[] = {NSOpenGLPFADoubleBuffer,
                                NSOpenGLPFADepthSize, 32, 0},
                      opaq = 0;

                vdlg = Subclass(NSOpenGLView, SUB_VDLG, vfld, vmet);
                pfmt = initWithAttributes_(alloc(NSOpenGLPixelFormat), attr);
                vins = initWithFrame_pixelFormat_(alloc(vdlg), dims, pfmt);

                makeCurrentContext((ctxt = openGLContext(vins)));
                setValues_forParameter_(ctxt, &opaq, NSOpenGLCPSurfaceOpacity);
                release(pfmt);
                break;
            }
            printf("\n%s\n"TXL_FAIL" %s\n", retn, engd->tran[TXT_NOGL]);
            free(retn);
            engd->rscm = SCM_RSTD;
            /// falling through
        }
        case SCM_RSTD: {
            PICT *pict = &engd->pict;
            long line = pict->xdim * sizeof(*pict->bptr);
            CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();

            engd->user[0] = (typeof(*engd->user))
                CGBitmapContextCreate(pict->bptr = calloc(line, pict->ydim),
                                      pict->xdim, pict->ydim, CHAR_BIT, line,
                                      drgb, kCGImageAlphaPremultipliedFirst
                                          | kCGBitmapByteOrder32Little);
            CGColorSpaceRelease(drgb);

            vdlg = Subclass(NSView, SUB_VDLG, vfld, vmet);
            vins = initWithFrame_(alloc(vdlg), dims);
            break;
        }
    }
    SET_IVAR(vins, VAR_ENGD, engd);

    id hwnd = initWithContentRect_styleMask_backing_defer_
             (alloc(NSWindow), dims, NSBorderlessWindowMask,
              NSBackingStoreBuffered, false);

    setContentView_(hwnd, vins);
    setDelegate_(hwnd, vins);
    setLevel_(hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(hwnd, clearColor(NSColor));
    setHasShadow_(hwnd, false);
    setOpaque_(hwnd, false);

//    id area = initWithRect_options_owner_userInfo_(alloc(NSTrackingArea), dims,
//                                                   NSTrackingCursorUpdate |
//                                                   NSTrackingActiveAlways,
//                                                   vins, 0);
//    addTrackingArea_(vins, area);
//    release(area);

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

    id menu = Submenu(engd->menu, vins);
    setMenu_(icon, menu);
    release(menu);

    engd->user[1] = (typeof(*engd->user))vins;
    engd->user[2] = (typeof(*engd->user))hwnd;
    CFRunLoopTimerRef tmrt = AddTimer(   1, OnTime, &engd->time),
                      tmrf = AddTimer(1000, OnFPS,   engd),
                      tmrd = AddTimer(  32, OnCalc,  engd);

    SET_IVAR(ains, VAR_ENGD, engd);
    SET_IVAR(ains, VAR_HWND, hwnd);
    SET_IVAR(ains, VAR_VIEW, vins);

    activateIgnoringOtherApps_(thrd, true);
    makeKeyAndOrderFront_(hwnd, thrd);
//    enableCursorRects(hwnd);
//    objc_msgSend(vins, resetCursorRects);

    /// run loop starts here
    run(thrd);

    CFRunLoopTimerInvalidate(tmrd);
    CFRunLoopTimerInvalidate(tmrf);
    CFRunLoopTimerInvalidate(tmrt);

    switch (engd->rscm) {
        case SCM_RSTD:
            StopThreads(engd);
            CGContextRelease((CGContextRef)engd->user[0]);
            free(engd->pict.bptr);
            break;

        case SCM_ROGL: {
            FreeRendererOGL((ROGL**)&engd->rndr);
            break;
        }
    }
    release(vins);
    release(hwnd);
    release(ains);
    release(pool);
    objc_disposeClassPair((Class)vdlg);
    objc_disposeClassPair((Class)adlg);

//    printf("END!\n");
}
