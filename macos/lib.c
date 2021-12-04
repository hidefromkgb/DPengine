#include <sys/time.h>

#include <core.h>
#include "load/mac_load.h"



/// name of the instance variable to access the engine structure
#define VAR_ENGD "engd"



struct SEMD {
    pthread_mutex_t cmtx;
    pthread_cond_t cvar;
    SEM_TYPE list, full;
};

typedef struct _DRAW {
    uint32_t attr;       /// last known mouse attributes
    long xdim, ydim;     /// drawing area dimensions
    NSWindow *hwnd;      /// main window
    NSView *view;        /// main view
    NSCursor *hand;      /// main cursor
    CGContextRef hctx;   /// output context handle
} DRAW;



uint64_t lTimeFunc() {
    struct timeval spec = {};

    gettimeofday(&spec, 0);
    return (uint64_t)spec.tv_sec * 1000 + (uint64_t)spec.tv_usec / 1000;
}



void lRestartEngine(ENGD *engd) {
    NSApplication *thrd = sharedApplication(_(NSApplication));
    NSEvent *post;

    /// if called from inside a timer context, bump an event to the loop
    /// to ensure that it did process the stop notification we just sent;
    /// if not in a timer context, a dummy event will be rejected anyway
    post = otherEventWithType_location_modifierFlags_timestamp_windowNumber_context_subtype_data1_data2_
               (_(NSEvent), NSApplicationDefined, (NSPoint){},
                            NSApplicationDefined, 0, 0, 0, 0, 0, 0);
    stop_(thrd, post);
    postEvent_atStart_(thrd, post, true);
}



void lShowMainWindow(ENGD *engd, long show) {
    NSApplication *thrd = sharedApplication(_(NSApplication));
    intptr_t *data;

    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    if (show)
        orderFront_(((DRAW*)data[0])->hwnd, thrd);
    else
        orderOut_(((DRAW*)data[0])->hwnd, thrd);
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
    long temp;

    temp = sysconf(_SC_NPROCESSORS_ONLN);
    return (temp > 0)? (temp < sizeof(SEM_TYPE) * CHAR_BIT)?
                        temp : sizeof(SEM_TYPE) * CHAR_BIT : 1;
}



void lMakeThread(void *thrd) {
    pthread_t pthr;

    pthread_create(&pthr, 0, cThrdFunc, thrd);
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



void OnFPS(CFRunLoopTimerRef runp, void *user) {
    char fout[128];

    cOutputFPS((ENGD*)user, fout);
    printf("%s\n", fout);
}



void OnCalc(CFRunLoopObserverRef runp, CFRunLoopActivity acti, void *user) {
    NSPoint dptr;
    uint32_t attr;
    intptr_t *data;
    DRAW *draw;

    cEngineCallback((ENGD*)user, ECB_GUSR, (intptr_t)&data);
    draw = (DRAW*)data[0];

    dptr = mouseLocation(_(NSEvent));
    attr = pressedMouseButtons(_(NSEvent));

    /// [TODO:] properly determine if the window is active
    attr = ((attr & 1)? UFR_LBTN : 0) | ((attr & 2)? UFR_RBTN : 0)
         | ((attr & 4)? UFR_MBTN : 0) | UFR_MOUS;
    attr = cPrepareFrame((ENGD*)user, dptr.x, draw->ydim - dptr.y, attr);
    if (attr & PFR_SKIP)
        usleep(1000);
    if (attr & PFR_HALT)
        return;

    if ((attr ^ draw->attr) & PFR_PICK) {
        if (attr & PFR_PICK) {
            setIgnoresMouseEvents_(draw->hwnd, false);
            push(draw->hand);
        }
        else {
            setIgnoresMouseEvents_(draw->hwnd, true);
            pop(draw->hand);
        }
    }
    draw->attr = attr;
    cEngineCallback((ENGD*)user, ECB_GFLG, (intptr_t)&attr);
    if (~attr & COM_RGPU) {
        CGContextSetBlendMode(draw->hctx, kCGBlendModeClear);
        CGContextFillRect(draw->hctx,
                         (NSRect){{0, 0}, {draw->xdim, draw->ydim}});
    }
    cOutputFrame((ENGD*)user, 0);
    setNeedsDisplay_(draw->view, true);
}



void MAC_Handler(OnDraw, NSRect rect) {
    CGContextRef ctxt;
    CGImageRef iref;

    uint32_t  flgs;
    intptr_t *data;
    ENGD *engd;
    DRAW *draw;

    MAC_GetIvar(self, VAR_ENGD, &engd);
    cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
    cEngineCallback(engd, ECB_GUSR, (intptr_t)&data);
    draw = (DRAW*)data[0];

    if (flgs & COM_RGPU)
        flushBuffer(openGLContext(draw->view));
    else {
        iref = CGBitmapContextCreateImage(draw->hctx);
        ctxt = graphicsPort(currentContext(_(NSGraphicsContext)));
        CGContextSetBlendMode(ctxt, kCGBlendModeCopy);
        CGContextDrawImage
            (ctxt, (NSRect){{0, 0}, {draw->xdim, draw->ydim}}, iref);
        CGImageRelease(iref);
    }
}



void lRunMainLoop(ENGD *engd, long xpos, long ypos, long xdim, long ydim,
                  BGRA **bptr, intptr_t *data, uint32_t flgs) {
    #define SUB_VDLG "lNSV"

    NSString *scib;
    NSApplication *thrd;
    NSAutoreleasePool *pool;
    CFRunLoopObserverRef idle;
    CFRunLoopTimerRef tmrf;
    NSRect dims;
    Class view;
    DRAW draw;

    /// a dirty hack to become capable of changing cursors at will
    CGSSetConnectionProperty
        (_CGSDefaultConnection(), _CGSDefaultConnection(),
         scib = MAC_MakeString("SetsCursorInBackground"), kCFBooleanTrue);
    MAC_FreeString(scib);

    data[0] = (intptr_t)&draw;
    draw.hand = pointingHandCursor(_(NSCursor));
    draw.attr = 0;
    draw.xdim = xdim - xpos;
    draw.ydim = ydim - ypos;
    dims = (NSRect){{0, 0}, {draw.xdim, draw.ydim}};

    pool = init(alloc(_(NSAutoreleasePool)));
    thrd = sharedApplication(_(NSApplication));
    if (~flgs & COM_RGPU) {
        long line = draw.xdim * sizeof(**bptr);
        CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();

        draw.hctx = CGBitmapContextCreate(*bptr = calloc(line, draw.ydim),
                                          draw.xdim, draw.ydim, CHAR_BIT, line,
                                          drgb, kCGImageAlphaPremultipliedFirst
                                              | kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(drgb);

        view = MAC_MakeClass(SUB_VDLG, _(NSView), MAC_TempArray(VAR_ENGD),
                             MAC_TempArray(_(drawRect_), OnDraw,
                                           _(isOpaque), OnOpaq));
        draw.view = init(alloc(view));
        setFrame_(draw.view, dims);
    }
    else {
        int attr[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 32, 0},
            opaq = 0;
        NSOpenGLPixelFormat *pfmt;
        NSOpenGLContext *ctxt;

        pfmt = initWithAttributes_(alloc(_(NSOpenGLPixelFormat)), attr);
        view = MAC_MakeClass(SUB_VDLG, _(NSOpenGLView), MAC_TempArray(VAR_ENGD),
                             MAC_TempArray(_(drawRect_), OnDraw,
                                           _(isOpaque), OnOpaq));
        draw.view =
            (NSView*)initWithFrame_pixelFormat_(alloc(view), dims, pfmt);
        makeCurrentContext(ctxt = openGLContext(draw.view));
        setValues_forParameter_(ctxt, &opaq, NSOpenGLCPSurfaceOpacity);
        release(pfmt);
    }
    MAC_SetIvar(draw.view, VAR_ENGD, engd);
    dims.origin.x = xpos;
    dims.origin.y = ypos;

    /// [TODO:] NSNonactivatingPanelMask is flagged for deprecation in 10.12
    ///         so a suitable alternative has to be found
    draw.hwnd = initWithContentRect_styleMask_backing_defer_
                    (alloc(_(NSPanel)), dims, NSBorderlessWindowMask
                                           | NSNonactivatingPanelMask,
                     kCGBackingStoreBuffered, false);

    setContentView_(draw.hwnd, draw.view);
    setDelegate_(draw.hwnd, draw.view);
    setLevel_(draw.hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(draw.hwnd, clearColor(_(NSColor)));
    setHasShadow_(draw.hwnd, false);
    setOpaque_(draw.hwnd, false);

    tmrf = MAC_MakeTimer(1000, OnFPS, engd);
    idle = MAC_MakeIdleFunc(OnCalc, engd);
    orderFront_(draw.hwnd, thrd);

    run(thrd);

    MAC_FreeIdleFunc(idle);
    MAC_FreeTimer(tmrf);

    cDeallocFrame(engd, 0);
    if (~flgs & COM_RGPU) {
        CGContextRelease(draw.hctx);
        free(*bptr);
    }
    if (draw.attr & PFR_PICK)
        pop(draw.hand);
    release(draw.view);
    release(draw.hwnd);
    release(pool);
    MAC_FreeClass(view);

    #undef SUB_VDLG
}
