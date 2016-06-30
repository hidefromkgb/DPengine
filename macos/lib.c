#include <sys/time.h>

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



struct SEMD {
    pthread_mutex_t cmtx;
    pthread_cond_t cvar;
    SEM_TYPE list, full;
};

typedef struct _DRAW {
    long xdim, ydim;   /// drawing area dimensions
    id pool, thrd;     /// autorelease pool, main NSApplication instance
    id adlg, ains;     /// app delegate class, app delegate instance
    id vdlg, vins;     /// view delegate class, view delegate instance
    id hwnd;           /// main window
    CGContextRef hctx; /// output context handle
} DRAW;



uint64_t lTimeFunc() {
    struct timeval spec = {};

    gettimeofday(&spec, 0);
    return spec.tv_sec * 1000 + spec.tv_usec / 1000;
}



void lRestartEngine(ENGD *engd) {
    id thrd = sharedApplication(NSApplication);
    CGPoint dptr = {};

    stop_(thrd, 0);
    /// if called from inside a timer context, bump an event to the loop
    /// to ensure that it did process the stop notification we just sent;
    /// if not in a timer context, a dummy event will be rejected anyway
    postEvent_atStart_(thrd, MakeEvent(NSApplicationDefined, dptr,
                                       NSApplicationDefined, 0, 0, 0), true);
}



void lShowMainWindow(ENGD *engd, long show) {
    id thrd = sharedApplication(NSApplication);

    if (show)
        unhide_(thrd, 0);
    else
        hide_(thrd, 0);
}



char *lLoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen);
        if (read(file, retn, flen) == flen) {
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



long lCountCPUs() {
    return min(sizeof(SEM_TYPE) * CHAR_BIT,
               max(1, sysconf(_SC_NPROCESSORS_ONLN)));
}



void lMakeThread(THRD *thrd) {
    pthread_t pthr;

    pthread_create(&pthr, 0, (void *(*)(void*))cThrdFunc, thrd);
}



void lFreeSemaphore(SEMD **retn, long nthr) {
    if (retn && *retn) {
        pthread_cond_destroy(&(*retn)->cvar);
        pthread_mutex_destroy(&(*retn)->cmtx);
        free(*retn);
        *retn = 0;
    }
}



void lMakeSemaphore(SEMD **retn, long nthr, SEM_TYPE mask) {
    if (retn) {
        *retn = malloc(sizeof(**retn));
        pthread_mutex_init(&(*retn)->cmtx, 0);
        pthread_cond_init(&(*retn)->cvar, 0);
        (*retn)->full = (1 << nthr) - 1;
        (*retn)->list = (*retn)->full & mask;
    }
}



long lPickSemaphore(SEMD *drop, SEMD *pick, SEM_TYPE mask) {
    long retn;

    retn = (__sync_fetch_and_and(&drop->list, ~(drop->full & mask)) & mask)?
            TRUE : FALSE;
    __sync_or_and_fetch(&pick->list, pick->full & mask);

    pthread_mutex_lock(&pick->cmtx);
    pthread_cond_broadcast(&pick->cvar);
    pthread_mutex_unlock(&pick->cmtx);
    return retn;
}



SEM_TYPE lWaitSemaphore(SEMD *wait, SEM_TYPE mask) {
    pthread_mutex_lock(&wait->cmtx);
    if (mask)
        while ((wait->list ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!wait->list)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = wait->list;
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



bool OnOpaq() {
    return false;
}



void OnCalc(CFRunLoopTimerRef tmrp, void *user) {
    CGPoint dptr;
    uint32_t attr;
    intptr_t *data;
    DRAW *draw;

    cEngineCallback((ENGD*)user, ECB_GUSR, (intptr_t)&data);
    draw = (DRAW*)data[0];

    GetT2DV(dptr, draw->hwnd, MouseLocationOutsideOfEventStream);
    attr = pressedMouseButtons(NSEvent);

    /// [TODO] properly determine if the window is active
    attr = ((attr & 1)? UFR_LBTN : 0)
         | ((attr & 2)? UFR_RBTN : 0)
         | ((attr & 4)? UFR_MBTN : 0)
         | UFR_MOUS;

    attr = cPrepareFrame((ENGD*)user, dptr.x, draw->ydim - dptr.y, attr);
    if (attr & PFR_SKIP)
        usleep(1000);
    if (attr & PFR_HALT)
        return;

    /// [TODO] manual mouse transparency does not work, commented out for now
//    setIgnoresMouseEvents_((id)engd->user[2],
//                           (pick >= 0) || (engd->flgs & COM_IOPQ));

    cEngineCallback((ENGD*)user, ECB_GFLG, (intptr_t)&attr);
    if (~attr & COM_RGPU) {
        CGContextSetBlendMode(draw->hctx, kCGBlendModeClear);
        CGContextFillRect(draw->hctx,
                         (CGRect){{0, 0}, {draw->xdim, draw->ydim}});
    }
    cOutputFrame((ENGD*)user, 0);
    setNeedsDisplay_(draw->vins, true);
}



void OnDraw(CGRect rect) {
    CGContextRef ctxt;
    CGImageRef iref;

    uint32_t  flgs;
    intptr_t *data;
    ENGD *engd;
    DRAW *draw;

    GET_IVAR(delegate(sharedApplication(NSApplication)), VAR_ENGD, &engd);
    cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    draw = (DRAW*)data[0];

    if (flgs & COM_RGPU)
        flushBuffer(openGLContext(draw->vins));
    else {
        iref = CGBitmapContextCreateImage(draw->hctx);
        ctxt = (CGContextRef)graphicsPort(currentContext(NSGraphicsContext));
        CGContextSetBlendMode(ctxt, kCGBlendModeCopy);
        CGContextDrawImage
            (ctxt, (CGRect){{0, 0}, {draw->xdim, draw->ydim}}, iref);
        CGImageRelease(iref);
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



void lRunMainLoop(ENGD *engd, long xpos, long ypos, long xdim, long ydim,
                  BGRA **bptr, intptr_t *data, uint32_t flgs) {

//    printf("BEGIN\n");

    DRAW draw;

    CGRect dims = {{0, 0}, {draw.xdim = xdim - xpos, draw.ydim = ydim - ypos}};
    /// view delegate`s methods (line 1) and custom fields (line 2)
    OMSC vmet[] = {{DrawRect_, OnDraw}, {IsOpaque,  OnOpaq}, {}};
    char *vfld[] = {0};

    LoadObjC();

    data[0] = (intptr_t)&draw;
    draw.adlg = Subclass(NSObject, SUB_ADLG,
                        (char*[]){VAR_HWND, VAR_ENGD, VAR_VIEW, 0},
                        (OMSC[]){{}});

    draw.pool = init(alloc(NSAutoreleasePool));
    draw.thrd = sharedApplication(NSApplication);
    setDelegate_(draw.thrd, (draw.ains = init(alloc(draw.adlg))));

    if (~flgs & COM_RGPU) {
        long line = draw.xdim * sizeof(**bptr);
        CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();

        draw.hctx = CGBitmapContextCreate(*bptr = calloc(line, draw.ydim),
                                          draw.xdim, draw.ydim, CHAR_BIT, line,
                                          drgb, kCGImageAlphaPremultipliedFirst
                                              | kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(drgb);

        draw.vdlg = Subclass(NSView, SUB_VDLG, vfld, vmet);
        draw.vins = init(alloc(draw.vdlg));
        setFrame_(draw.vins, dims);
    }
    else {
        int attr[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 32, 0},
            opaq = 0;
        id pfmt, ctxt;

        pfmt = initWithAttributes_(alloc(NSOpenGLPixelFormat), attr);
        draw.vdlg = Subclass(NSOpenGLView, SUB_VDLG, vfld, vmet);
        draw.vins = initWithFrame_pixelFormat_(alloc(draw.vdlg), dims, pfmt);
        makeCurrentContext((ctxt = openGLContext(draw.vins)));
        setValues_forParameter_(ctxt, &opaq, NSOpenGLCPSurfaceOpacity);
        release(pfmt);
    }
    SET_IVAR(draw.vins, VAR_ENGD, engd);

    draw.hwnd = initWithContentRect_styleMask_backing_defer_
                    (alloc(NSWindow), dims, NSBorderlessWindowMask,
                     NSBackingStoreBuffered, false);

    setContentView_(draw.hwnd, draw.vins);
    setDelegate_(draw.hwnd, draw.vins);
    setLevel_(draw.hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(draw.hwnd, clearColor(NSColor));
    setHasShadow_(draw.hwnd, false);
    setOpaque_(draw.hwnd, false);

    CFRunLoopTimerRef ///tmrf = AddTimer(1000, OnFPS,  engd),
                      tmrd = AddTimer(   1, OnCalc, engd);

    SET_IVAR(draw.ains, VAR_ENGD, engd);
    SET_IVAR(draw.ains, VAR_HWND, draw.hwnd);
    SET_IVAR(draw.ains, VAR_VIEW, draw.vins);

    activateIgnoringOtherApps_(draw.thrd, true);
    makeKeyWindow(draw.hwnd);
    orderFront_(draw.hwnd, draw.thrd);
//    enableCursorRects(draw.hwnd);
//    objc_msgSend(draw.vins, resetCursorRects);

    run(draw.thrd);

    CFRunLoopTimerInvalidate(tmrd);
///    CFRunLoopTimerInvalidate(tmrf);

    cDeallocFrame(engd, 0);
    if (~flgs & COM_RGPU) {
        CGContextRelease(draw.hctx);
        free(*bptr);
    }
    release(draw.vins);
    release(draw.hwnd);
    release(draw.ains);
    release(draw.pool);
    objc_disposeClassPair((Class)draw.vdlg);
    objc_disposeClassPair((Class)draw.adlg);

//    printf("END!\n");
}
