#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGImage.h>
#include <objc/objc-runtime.h>



#define C(x) objc_getClass(x)
#define N(x) sel_registerName(x)
#define M(o, ...) objc_msgSend(o, __VA_ARGS__)

#define countof(a) (sizeof(a) / sizeof(*(a)))



enum {
    NSBorderlessWindowMask         = (0     ),
    NSTitledWindowMask             = (1 << 0),
    NSClosableWindowMask           = (1 << 1),
    NSMiniaturizableWindowMask     = (1 << 2),
    NSResizableWindowMask          = (1 << 3),
    NSTexturedBackgroundWindowMask = (1 << 8),
};
enum {
    NSBackingStoreRetained    = 0,
    NSBackingStoreNonretained = 1,
    NSBackingStoreBuffered    = 2,
};
enum {
    NSApplicationActivationPolicyRegular    = 0,
    NSApplicationActivationPolicyAccessory  = 1,
    NSApplicationActivationPolicyProhibited = 2,
};
enum {
    /// these must have double FP precision when passed!
    NSVariableStatusItemLength	 = -1,
    NSSquareStatusItemLength	 = -2,
};
enum {
    NSMixedState = -1,
    NSOffState   =  0,
    NSOnState    =  1,
};
enum {
    NSNormalWindowLevel   =  0,
    NSFloatingWindowLevel =  3,
    NSDockWindowLevel     =  5,
    NSSubmenuWindowLevel  = 10,
    NSMainMenuWindowLevel = 20,
};
enum {
    NSOpenGLPFAAllRenderers          =   1,
    NSOpenGLPFATripleBuffer          =   3,
    NSOpenGLPFADoubleBuffer          =   5,
    NSOpenGLPFAStereo                =   6,
    NSOpenGLPFAAuxBuffers            =   7,
    NSOpenGLPFAColorSize             =   8,
    NSOpenGLPFAAlphaSize             =  11,
    NSOpenGLPFADepthSize             =  12,
    NSOpenGLPFAStencilSize           =  13,
    NSOpenGLPFAAccumSize             =  14,
    NSOpenGLPFAMinimumPolicy         =  51,
    NSOpenGLPFAMaximumPolicy         =  52,
    NSOpenGLPFAOffScreen             =  53,
    NSOpenGLPFAFullScreen            =  54,
    NSOpenGLPFASampleBuffers         =  55,
    NSOpenGLPFASamples               =  56,
    NSOpenGLPFAAuxDepthStencil       =  57,
    NSOpenGLPFAColorFloat            =  58,
    NSOpenGLPFAMultisample           =  59,
    NSOpenGLPFASupersample           =  60,
    NSOpenGLPFASampleAlpha           =  61,
    NSOpenGLPFARendererID            =  70,
    NSOpenGLPFASingleRenderer        =  71,
    NSOpenGLPFANoRecovery            =  72,
    NSOpenGLPFAAccelerated           =  73,
    NSOpenGLPFAClosestPolicy         =  74,
    NSOpenGLPFARobust                =  75,
    NSOpenGLPFABackingStore          =  76,
    NSOpenGLPFAMPSafe                =  78,
    NSOpenGLPFAWindow                =  80,
    NSOpenGLPFAMultiScreen           =  81,
    NSOpenGLPFACompliant             =  83,
    NSOpenGLPFAScreenMask            =  84,
    NSOpenGLPFAPixelBuffer           =  90,
    NSOpenGLPFARemotePixelBuffer     =  91,
    NSOpenGLPFAAllowOfflineRenderers =  96,
    NSOpenGLPFAAcceleratedCompute    =  97,
    NSOpenGLPFAOpenGLProfile         =  99,
    NSOpenGLPFAVirtualScreenCount    = 128,
};



double (*D)(id, ...);
SEL MenuResponderSelector;

char *StringObjCClasses[] = {
    "NSObject",
    "NSApplication",
    "NSAutoreleasePool",
    "NSColor",
    "NSImage",
    "NSMenu",
    "NSMenuItem",
    "NSScreen",
    "NSStatusBar",
    "NSString",
    "NSWindow",
    "NSView",
    "NSOpenGLView",
    "NSOpenGLPixelFormat",
    "NSGraphicsContext",
    "NSCursor",
    "NSEvent",
    0
};
id LoadedObjCClasses[countof(StringObjCClasses)] = {};

#define NSObject             (LoadedObjCClasses[ 0])
#define NSApplication        (LoadedObjCClasses[ 1])
#define NSAutoreleasePool    (LoadedObjCClasses[ 2])
#define NSColor              (LoadedObjCClasses[ 3])
#define NSImage              (LoadedObjCClasses[ 4])
#define NSMenu               (LoadedObjCClasses[ 5])
#define NSMenuItem           (LoadedObjCClasses[ 6])
#define NSScreen             (LoadedObjCClasses[ 7])
#define NSStatusBar          (LoadedObjCClasses[ 8])
#define NSString             (LoadedObjCClasses[ 9])
#define NSWindow             (LoadedObjCClasses[10])
#define NSView               (LoadedObjCClasses[11])
#define NSOpenGLView         (LoadedObjCClasses[12])
#define NSOpenGLPixelFormat  (LoadedObjCClasses[13])
#define NSGraphicsContext    (LoadedObjCClasses[14])
#define NSCursor             (LoadedObjCClasses[15])
#define NSEvent              (LoadedObjCClasses[16])


char *StringObjCSelectors[] = {
    "init",
    "alloc",
    "release",
    "autorelease",
    "activateIgnoringOtherApps:",
    "addItem:",
    "colorWithCalibratedRed:green:blue:alpha:",
    "imageNamed:",
    "initWithCGImage:size:",
    "initWithContentRect:styleMask:backing:defer:",
    "initWithTitle:action:keyEquivalent:",
    "makeKeyAndOrderFront:",
    "run",
    "separatorItem",
    "setActivationPolicy:",
    "setAutoenablesItems:",
    "setBackgroundColor:",
    "setDelegate:",
    "setEnabled:",
    "setHasShadow:",
    "setHighlightMode:",
    "setImage:",
    "setMenu:",
    "setOnStateImage:",
    "setOpaque:",
    "setState:",
    "setSubmenu:",
    "sharedApplication",
    "statusItemWithLength:",
    "stringWithUTF8String:",
    "systemStatusBar",
    "thickness",
    "visibleFrame",
    "mainScreen",
    "stop:",
    "drain",
    "delegate",
    "setTag:",
    "tag",
    "setRepresentedObject:",
    "representedObject",
    "setTarget:",
    "target",
    "setIgnoresMouseEvents:",
    "hide:",
    "unhide:",
    "retain",
    "retainCount",
    "initWithFrame:",
    "initWithFrame:pixelFormat:",
    "setContentView:",
    "initWithAttributes:",
    "currentContext",
    "graphicsPort",
    "setLevel:",
    "pointingHandCursor",
    "mouseLocationOutsideOfEventStream",
    "set",
    "setNeedsDisplay:",
    0
};
SEL LoadedObjCSelectors[countof(StringObjCSelectors)] = {};

#define init(inst)                                                     M(inst, LoadedObjCSelectors[ 0])
#define alloc(inst)                                                    M(inst, LoadedObjCSelectors[ 1])
#define release(inst)                                                  M(inst, LoadedObjCSelectors[ 2])
#define autorelease(inst)                                              M(inst, LoadedObjCSelectors[ 3])
#define activateIgnoringOtherApps_(inst, b)                            M(inst, LoadedObjCSelectors[ 4], (bool)(b))
#define addItem_(inst, i)                                              M(inst, LoadedObjCSelectors[ 5], i)
#define colorWithCalibratedRed_green_blue_alpha_(inst, r, g, b, a)     M(inst, LoadedObjCSelectors[ 6], (double)(r), (double)(g), (double)(b), (double)(a))
#define imageNamed_(inst, s)                                           M(inst, LoadedObjCSelectors[ 7], s)
#define initWithCGImage_size_(inst, i, s)                              M(inst, LoadedObjCSelectors[ 8], i, s)
#define initWithContentRect_styleMask_backing_defer_(inst, r, m, b, d) M(inst, LoadedObjCSelectors[ 9], r, (unsigned)(m), (unsigned)(b), (bool)(d))
#define initWithTitle_action_keyEquivalent_(inst, t, a, k)             M(inst, LoadedObjCSelectors[10], t, a, k)
#define makeKeyAndOrderFront_(inst, w)                                 M(inst, LoadedObjCSelectors[11], w)
#define run(inst)                                                      M(inst, LoadedObjCSelectors[12])
#define separatorItem(inst)                                            M(inst, LoadedObjCSelectors[13])
#define setActivationPolicy_(inst, p)                                  M(inst, LoadedObjCSelectors[14], (unsigned)(p))
#define setAutoenablesItems_(inst, b)                                  M(inst, LoadedObjCSelectors[15], (bool)(b))
#define setBackgroundColor_(inst, c)                                   M(inst, LoadedObjCSelectors[16], c)
#define setDelegate_(inst, d)                                          M(inst, LoadedObjCSelectors[17], d)
#define setEnabled_(inst, b)                                           M(inst, LoadedObjCSelectors[18], (bool)(b))
#define setHasShadow_(inst, b)                                         M(inst, LoadedObjCSelectors[19], (bool)(b))
#define setHighlightMode_(inst, b)                                     M(inst, LoadedObjCSelectors[20], (bool)(b))
#define setImage_(inst, i)                                             M(inst, LoadedObjCSelectors[21], i)
#define setMenu_(inst, m)                                              M(inst, LoadedObjCSelectors[22], m)
#define setOnStateImage_(inst, i)                                      M(inst, LoadedObjCSelectors[23], i)
#define setOpaque_(inst, b)                                            M(inst, LoadedObjCSelectors[24], (bool)(b))
#define setState_(inst, s)                                             M(inst, LoadedObjCSelectors[25], (unsigned)(s))
#define setSubmenu_(inst, m)                                           M(inst, LoadedObjCSelectors[26], m)
#define sharedApplication(inst)                                        M(inst, LoadedObjCSelectors[27])
#define statusItemWithLength_(inst, l)                                 M(inst, LoadedObjCSelectors[28], (double)(l))
#define stringWithUTF8String_(inst, s)                                 M(inst, LoadedObjCSelectors[29], (char*)(s))
#define systemStatusBar(inst)                                          M(inst, LoadedObjCSelectors[30])
#define thickness(inst)                                                D(inst, LoadedObjCSelectors[31])
#define visibleFrame(inst, f)     objc_msgSend_stret((void*)(f), (void*)(inst),LoadedObjCSelectors[32])
#define mainScreen(inst)                                               M(inst, LoadedObjCSelectors[33])
#define stop_(inst, s)                                                 M(inst, LoadedObjCSelectors[34], s)
#define drain(inst)                                                    M(inst, LoadedObjCSelectors[35])
#define delegate(inst)                                                 M(inst, LoadedObjCSelectors[36])
#define setTag_(inst, t)                                               M(inst, LoadedObjCSelectors[37], t)
#define tag(inst)                                                      M(inst, LoadedObjCSelectors[38])
#define setRepresentedObject_(inst, o)                                 M(inst, LoadedObjCSelectors[39], o)
#define representedObject(inst)                                        M(inst, LoadedObjCSelectors[40])
#define setTarget_(inst, t)                                            M(inst, LoadedObjCSelectors[41], t)
#define target(inst)                                                   M(inst, LoadedObjCSelectors[42])
#define setIgnoresMouseEvents_(inst, i)                                M(inst, LoadedObjCSelectors[43], (bool)(i))
#define hide_(inst, e)                                                 M(inst, LoadedObjCSelectors[44], e)
#define unhide_(inst, e)                                               M(inst, LoadedObjCSelectors[45], e)
#define retain(inst)                                                   M(inst, LoadedObjCSelectors[46])
#define retainCount(inst)                                              M(inst, LoadedObjCSelectors[47])
#define initWithFrame_(inst, r)                                        M(inst, LoadedObjCSelectors[48], r)
#define initWithFrame_pixelFormat_(inst, f, p)                         M(inst, LoadedObjCSelectors[49], f, p)
#define setContentView_(inst, v)                                       M(inst, LoadedObjCSelectors[50], v)
#define initWithAttributes_(inst, a)                                   M(inst, LoadedObjCSelectors[51], (uint32_t*)(a))
#define currentContext(inst)                                           M(inst, LoadedObjCSelectors[52])
#define graphicsPort(inst)                                             M(inst, LoadedObjCSelectors[53])
#define setLevel_(inst, l)                                             M(inst, LoadedObjCSelectors[54], (long)(l))
#define pointingHandCursor(inst)                                       M(inst, LoadedObjCSelectors[55])
#define mouseLocationOutsideOfEventStream(inst, s) objc_msgSend_stret((void*)(s), (void*)(inst),LoadedObjCSelectors[56])
#define set(inst)                                                      M(inst, LoadedObjCSelectors[57])
#define setNeedsDisplay_(inst, d)                                      M(inst, LoadedObjCSelectors[58], (bool)(d))



bool LoadObjC() {
    long iter = false;

    if (!*LoadedObjCClasses) {
        iter = -1;
        while (StringObjCClasses[++iter])
            LoadedObjCClasses[iter] = C(StringObjCClasses[iter]);
        iter = -1;
        while (StringObjCSelectors[++iter])
            LoadedObjCSelectors[iter] = N(StringObjCSelectors[iter]);
#ifdef __i386__
        D = (typeof(D))objc_msgSend_fpret;
#else
        D = (typeof(D))objc_msgSend;
#endif
        iter = true;
    }
    return iter;
}



typedef struct _OVER {
    SEL name;
    void *func;
} OVER;



id Overload(id base, char *name, OVER *over) {
    Class retn = objc_allocateClassPair((Class)base, name, 0);
    long size = -1;

    while (over[++size].func)
        class_addMethod(retn, over[size].name, over[size].func, 0);
    objc_registerClassPair(retn);
    return (id)retn;
}



inline id UTF8(void *utf8) {
    return stringWithUTF8String_(NSString, utf8);
}
