#include <sys/time.h>

#include <core.h>
#include "load/mac_load.h"



#define CLS_WDLG "lNSW"
#define CLS_VCPU "lNSC"
#define CLS_VGPU "lNSG"

/// name of the instance variable to access the engine structure
#define VAR_ENGD "engd"



struct SEMD {
    pthread_mutex_t cmtx;
    pthread_cond_t cvar;
    SEM_TYPE list, full;
};

/// lPickSemaphore() moves LIST by the atomic builtins and outside CMTX, so a
/// plain read of it here races the write however indivisible the latter is,
/// and, having no barrier of its own, is also free to be reordered with the
/// reads of the THRD that the bit stands for. FULL needs none of this, being
/// set before the first thread exists and read-only ever since
#define SEM_LIST(semd) __atomic_load_n(&(semd)->list, __ATOMIC_SEQ_CST)

typedef struct _DRAW {
    uint32_t attr;       /// last known mouse attributes
    long xdim, ydim;     /// drawing area dimensions
    intptr_t user;       /// keypress handling routine data
    UKEY ukey;           /// keypress handling routine
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
    return (temp > 0)? (temp < SEM_NTHR)? temp : SEM_NTHR : 1;
}



long lMakeThread(void *thrd) {
    pthread_t pthr;

    /// nobody ever joins the pool, and StopThreads() lets every worker exit on
    /// each switch between the loading and the drawing modes, so a joinable one
    /// would leave its descriptor and its stack behind at every single switch
    if (pthread_create(&pthr, 0, cThrdFunc, thrd) != 0)
        return 0;
    pthread_detach(pthr);
    return !0;
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
        (*retn)->full = SEM_BITS(nthr);
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
        while ((SEM_LIST(wait) ^ wait->full) & mask)
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    else
        while (!SEM_LIST(wait))
            pthread_cond_wait(&wait->cvar, &wait->cmtx);
    mask = SEM_LIST(wait);
    pthread_mutex_unlock(&wait->cmtx);
    return mask;
}



static bool OnTrue() {
    return true;
}



static bool OnFalse() {
    return false;
}



static void MAC_Handler(OnKey, NSEvent *ekey) {
    static const int8_t keys[128] = { /** see the list of kVK_* for info **/
        KEY_A         , KEY_S         , KEY_D         , KEY_F         ,
        KEY_H         , KEY_G         , KEY_Z         , KEY_X         ,
        KEY_C         , KEY_V         , KEY_CHR_NONE  , KEY_B         ,
        KEY_Q         , KEY_W         , KEY_E         , KEY_R         ,
        KEY_Y         , KEY_T         , KEY_1         , KEY_2         ,
        KEY_3         , KEY_4         , KEY_6         , KEY_5         ,
        KEY_EQUAL     , KEY_9         , KEY_7         , KEY_HYPHEN    ,
        KEY_8         , KEY_0         , KEY_RBRACKET  , KEY_O         ,
        KEY_U         , KEY_LBRACKET  , KEY_I         , KEY_P         ,
        KEY_ENTER     , KEY_L         , KEY_J         , KEY_QUOTE     ,
        KEY_K         , KEY_COLON     , KEY_PIPE      , KEY_LESS      ,
        KEY_QUESTION  , KEY_N         , KEY_M         , KEY_GREATER   ,
        KEY_TAB       , KEY_SPACE     , KEY_TILDE     , KEY_BACKSPACE ,
        KEY_CHR_NONE  , KEY_ESC       , KEY_RSYSTEM   , KEY_LSYSTEM   ,
        KEY_LSHIFT    , KEY_CAPSLOCK  , KEY_LALT      , KEY_LCTRL     ,
        KEY_RSHIFT    , KEY_RALT      , KEY_RCTRL     , KEY_CHR_NONE  ,
        KEY_CHR_NONE  , KEY_NUM_DEL   , KEY_CHR_NONE  , KEY_NUM_MUL   ,
        KEY_CHR_NONE  , KEY_NUM_ADD   , KEY_CHR_NONE  , KEY_NUM_LOCK  ,
        KEY_CHR_NONE  , KEY_CHR_NONE  , KEY_CHR_NONE  , KEY_NUM_DIV   ,
        KEY_NUM_ENTER , KEY_CHR_NONE  , KEY_NUM_SUB   , KEY_CHR_NONE  ,
        KEY_CHR_NONE  , KEY_NUM_EQUAL , KEY_NUM_0     , KEY_NUM_1     ,
        KEY_NUM_2     , KEY_NUM_3     , KEY_NUM_4     , KEY_NUM_5     ,
        KEY_NUM_6     , KEY_NUM_7     , KEY_CHR_NONE  , KEY_NUM_8     ,
        KEY_NUM_9     , KEY_CHR_NONE  , KEY_CHR_NONE  , KEY_CHR_NONE  ,
        KEY_F5        , KEY_F6        , KEY_F7        , KEY_F3        ,
        KEY_F8        , KEY_F9        , KEY_CHR_NONE  , KEY_F11       ,
        KEY_CHR_NONE  , KEY_PRTSCR    , KEY_CHR_NONE  , KEY_SCRLOCK   ,
        KEY_CHR_NONE  , KEY_F10       , KEY_RMENU     , KEY_F12       ,
        KEY_CHR_NONE  , KEY_PAUSE     , KEY_INSERT    , KEY_HOME      ,
        KEY_PAGEUP    , KEY_DELETE    , KEY_F4        , KEY_END       ,
        KEY_F2        , KEY_PAGEDOWN  , KEY_F1        , KEY_LEFT      ,
        KEY_RIGHT     , KEY_DOWN      , KEY_UP        , KEY_CHR_NONE  ,
    };
    DRAW *draw;

    MAC_GetIvar(self, VAR_ENGD, &draw);
    cEngineCallback((ENGD*)draw, ECB_GUSR, (intptr_t)&draw);
    draw = (DRAW*)((intptr_t*)draw)[0];
    if (!draw->ukey) return;
    const int8_t code = keys[keyCode(ekey) & 0x7F];
    const NSUInteger e_ty = type(ekey);
    bool down = (e_ty == NSKeyDown);
    if (e_ty == NSFlagsChanged) {
        const NSUInteger flgs = modifierFlags(ekey);
        switch (code) {
            case KEY_CAPSLOCK: down = flgs & NSEventModifierFlagCapsLock; break;
            case KEY_LSHIFT:   down = flgs & NX_DEVICELSHIFTKEYMASK;      break;
            case KEY_RSHIFT:   down = flgs & NX_DEVICERSHIFTKEYMASK;      break;
            case KEY_LCTRL:    down = flgs & NX_DEVICELCTLKEYMASK;        break;
            case KEY_RCTRL:    down = flgs & NX_DEVICERCTLKEYMASK;        break;
            case KEY_LALT:     down = flgs & NX_DEVICELALTKEYMASK;        break;
            case KEY_RALT:     down = flgs & NX_DEVICERALTKEYMASK;        break;
            case KEY_LSYSTEM:  down = flgs & NX_DEVICELCMDKEYMASK;        break;
            case KEY_RSYSTEM:  down = flgs & NX_DEVICERCMDKEYMASK;        break;
            case KEY_INSERT:   down = flgs & NSEventModifierFlagHelp;     break;
        }
    }
    draw->ukey(draw->user, (!down) ? -code - 1 : code);
}



void OnFPS(CFRunLoopTimerRef runp, void *user) {
    char fout[128];

    cOutputFPS((ENGD*)user, fout);
    printf("%s\n", fout);
}



void OnCalc(CFRunLoopObserverRef runp, CFRunLoopActivity acti, void *user) {
    NSPoint dptr;
    uint32_t attr;
    DRAW *draw;

    cEngineCallback((ENGD*)user, ECB_GUSR, (intptr_t)&draw);
    draw = (DRAW*)((intptr_t*)draw)[0];

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



static void MAC_Handler(OnDraw, NSRect rect) {
    CGContextRef ctxt;
    CGImageRef iref;

    uint32_t  flgs;
    ENGD *engd;
    DRAW *draw;

    MAC_GetIvar(self, VAR_ENGD, &engd);
    cEngineCallback(engd, ECB_GFLG, (intptr_t)&flgs);
    cEngineCallback(engd, ECB_GUSR, (intptr_t)&draw);
    draw = (DRAW*)((intptr_t*)draw)[0];

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



static void MAC_Handler(OnCPUViewDealloc) {
    super_dealloc(self);
    MAC_FreeClass(MAC_LoadClass(CLS_VCPU));
}



static void MAC_Handler(OnGPUViewDealloc) {
    super_dealloc(self);
    MAC_FreeClass(MAC_LoadClass(CLS_VGPU));
}



static void MAC_Handler(OnWindowDealloc) {
    super_dealloc(self);
    MAC_FreeClass(MAC_LoadClass(CLS_WDLG));
}



void lRunMainLoop(ENGD *engd, long xpos, long ypos, long xdim, long ydim,
        BGRA **bptr, intptr_t *data, uint32_t flgs, intptr_t user, UKEY ukey) {
    NSString *scib;
    NSApplication *thrd;
    NSAutoreleasePool *pool;
    CFRunLoopObserverRef idle;
    CFRunLoopTimerRef tmrf;
    NSRect dims;
    Class wpnl;
    Class view;
    DRAW draw;

    pool = init(alloc(_(NSAutoreleasePool)));
    thrd = sharedApplication(_(NSApplication));

    /// a dirty hack to become capable of changing cursors at will
    CGSSetConnectionProperty
        (_CGSDefaultConnection(), _CGSDefaultConnection(),
         scib = MAC_MakeString("SetsCursorInBackground"), kCFBooleanTrue);
    MAC_FreeString(scib);

    data[0] = (intptr_t)&draw;
    draw.hand = pointingHandCursor(_(NSCursor));
    draw.attr = 0;
    draw.user = user;
    draw.ukey = ukey;
    draw.xdim = xdim - xpos;
    draw.ydim = ydim - ypos;
    dims = (NSRect){{0, 0}, {draw.xdim, draw.ydim}};

    if (~flgs & COM_RGPU) {
        long line = draw.xdim * sizeof(**bptr);
        CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();

        draw.hctx = CGBitmapContextCreate(*bptr = calloc(line, draw.ydim),
                                          draw.xdim, draw.ydim, CHAR_BIT, line,
                                          drgb, kCGImageAlphaPremultipliedFirst
                                              | kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(drgb);

        view = MAC_MakeClass(CLS_VCPU, _(NSView), MAC_TempArray(VAR_ENGD),
                             MAC_TempArray(_(drawRect_), OnDraw,
                                           _(dealloc), OnCPUViewDealloc,
                                           _(acceptsFirstResponder), OnTrue,
                                           _(keyDown_), OnKey, _(keyUp_), OnKey,
                                           _(flagsChanged_), OnKey,
                                           _(isOpaque), OnFalse));
        draw.view = autorelease(init(alloc(view)));
        setFrame_(draw.view, dims);
    }
    else {
        int attr[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 32, 0},
            opaq = 0;
        NSOpenGLPixelFormat *pfmt;
        NSOpenGLContext *ctxt;

        pfmt = initWithAttributes_(alloc(_(NSOpenGLPixelFormat)), attr);
        view = MAC_MakeClass(CLS_VGPU, _(NSOpenGLView), MAC_TempArray(VAR_ENGD),
                             MAC_TempArray(_(drawRect_), OnDraw,
                                           _(dealloc), OnGPUViewDealloc,
                                           _(acceptsFirstResponder), OnTrue,
                                           _(keyDown_), OnKey, _(keyUp_), OnKey,
                                           _(flagsChanged_), OnKey,
                                           _(isOpaque), OnFalse));
        draw.view = autorelease(
                        initWithFrame_pixelFormat_(alloc(view), dims, pfmt));
        makeCurrentContext(ctxt = openGLContext(draw.view));
        setValues_forParameter_(ctxt, &opaq, NSOpenGLCPSurfaceOpacity);
        release(pfmt);
    }
    MAC_SetIvar(draw.view, VAR_ENGD, engd);
    dims.origin.x = xpos;
    dims.origin.y = ypos;

    wpnl = MAC_MakeClass(CLS_WDLG, _(NSPanel), nil,
                         MAC_TempArray(_(dealloc), OnWindowDealloc,
                                       _(canBecomeKeyWindow), OnTrue));
    /// [TODO:] NSNonactivatingPanelMask is flagged for deprecation in 10.12
    ///         so a suitable alternative has to be found
    draw.hwnd = autorelease(initWithContentRect_styleMask_backing_defer_
                        (alloc(wpnl), dims, NSBorderlessWindowMask
                                          | NSNonactivatingPanelMask,
                         kCGBackingStoreBuffered, false));

    setContentView_(draw.hwnd, draw.view);
    setDelegate_(draw.hwnd, draw.view);
    setLevel_(draw.hwnd, NSMainMenuWindowLevel + 1);
    setBackgroundColor_(draw.hwnd, clearColor(_(NSColor)));
    setHasShadow_(draw.hwnd, false);
    setOpaque_(draw.hwnd, false);

    tmrf = MAC_MakeTimer(1000, OnFPS, engd);
    idle = MAC_MakeIdleFunc(OnCalc, engd);
    orderFront_(draw.hwnd, thrd);
    makeKeyWindow(draw.hwnd);

    run(thrd);

    // close(NSWindow*) would have conflicted with close(int) from <unistd.h>,
    // so the only way to indeed close the NSPanel is this abomination; alas!
    const SEL id_close = sel_registerName("close");
    ((void (*)(NSWindow*, SEL))objc_msgSend)(draw.hwnd, id_close);

    MAC_FreeIdleFunc(idle);
    MAC_FreeTimer(tmrf);

    cDeallocFrame(engd, 0);
    if (~flgs & COM_RGPU) {
        CGContextRelease(draw.hctx);
        free(*bptr);
    }
    if (draw.attr & PFR_PICK)
        pop(draw.hand);
    release(pool);
    // cannot release the classes right away, since releasing the window and
    // the view seems asynchronous (?); it is why dealloc handlers are there
}
