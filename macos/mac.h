#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGImage.h>
#include <objc/objc-runtime.h>



/// non-integer precision, so #define instead of enum
#define NSVariableStatusItemLength (-1.0)
#define NSSquareStatusItemLength   (-2.0)

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
enum {
    NSOpenGLCPSwapInterval           = 222,
    NSOpenGLCPSurfaceOrder           = 235,
    NSOpenGLCPSurfaceOpacity         = 236,
    NSOpenGLCPSurfaceBackingSize     = 304,
    NSOpenGLCPReclaimResources       = 308,
    NSOpenGLCPCurrentRendererID      = 309,
    NSOpenGLCPGPUVertexProcessing    = 310,
    NSOpenGLCPGPUFragmentProcessing  = 311,
    NSOpenGLCPHasDrawable            = 314,
    NSOpenGLCPMPSwapsInFlight        = 315,
};



/// overloaded method for a subclass
typedef struct _OMSC {
    SEL name;
    void *func;
} OMSC;



#define STRING_OBJC_CLASSES    \
       "NSObject",             \
       "NSApplication",        \
       "NSAutoreleasePool",    \
       "NSColor",              \
       "NSImage",              \
       "NSMenu",               \
       "NSMenuItem",           \
       "NSScreen",             \
       "NSStatusBar",          \
       "NSString",             \
       "NSWindow",             \
       "NSView",               \
       "NSOpenGLView",         \
       "NSOpenGLPixelFormat",  \
       "NSGraphicsContext",    \
       "NSCursor",             \
       "NSEvent"

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



#define STRING_OBJC_SELECTORS                          \
       "init",                                         \
       "alloc",                                        \
       "release",                                      \
       "autorelease",                                  \
       "activateIgnoringOtherApps:",                   \
       "addItem:",                                     \
       "clearColor",                                   \
       "imageNamed:",                                  \
       "initWithCGImage:size:",                        \
       "initWithContentRect:styleMask:backing:defer:", \
       "initWithTitle:action:keyEquivalent:",          \
       "makeKeyAndOrderFront:",                        \
       "run",                                          \
       "separatorItem",                                \
       "setActivationPolicy:",                         \
       "setAutoenablesItems:",                         \
       "setBackgroundColor:",                          \
       "setDelegate:",                                 \
       "setEnabled:",                                  \
       "setHasShadow:",                                \
       "setHighlightMode:",                            \
       "setImage:",                                    \
       "setMenu:",                                     \
       "setOnStateImage:",                             \
       "setOpaque:",                                   \
       "setState:",                                    \
       "setSubmenu:",                                  \
       "sharedApplication",                            \
       "statusItemWithLength:",                        \
       "stringWithUTF8String:",                        \
       "systemStatusBar",                              \
       "thickness",                                    \
       "mainScreen",                                   \
       "stop:",                                        \
       "drain",                                        \
       "delegate",                                     \
       "setTag:",                                      \
       "tag",                                          \
       "setRepresentedObject:",                        \
       "representedObject",                            \
       "setTarget:",                                   \
       "target",                                       \
       "setIgnoresMouseEvents:",                       \
       "hide:",                                        \
       "unhide:",                                      \
       "retain",                                       \
       "retainCount",                                  \
       "initWithFrame:",                               \
       "initWithFrame:pixelFormat:",                   \
       "setContentView:",                              \
       "initWithAttributes:",                          \
       "currentContext",                               \
       "graphicsPort",                                 \
       "setLevel:",                                    \
       "pointingHandCursor",                           \
       "set",                                          \
       "setNeedsDisplay:",                             \
       "drawRect:",                                    \
       "_M",                                           \
       "mouseLocationOutsideOfEventStream",            \
       "pressedMouseButtons",                          \
       "openGLContext",                                \
       "flushBuffer",                                  \
       "makeCurrentContext",                           \
       "setValues:forParameter:"



#define init(inst)                                                     objc_msgSend(inst, LoadedObjCSelectors[ 0])
#define alloc(inst)                                                    objc_msgSend(inst, LoadedObjCSelectors[ 1])
#define release(inst)                                                  objc_msgSend(inst, LoadedObjCSelectors[ 2])
#define autorelease(inst)                                              objc_msgSend(inst, LoadedObjCSelectors[ 3])
#define activateIgnoringOtherApps_(inst, b)                            objc_msgSend(inst, LoadedObjCSelectors[ 4], (bool)(b))
#define addItem_(inst, i)                                              objc_msgSend(inst, LoadedObjCSelectors[ 5], i)
#define clearColor(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 6])
#define imageNamed_(inst, s)                                           objc_msgSend(inst, LoadedObjCSelectors[ 7], s)
#define initWithCGImage_size_(inst, i, s)                              objc_msgSend(inst, LoadedObjCSelectors[ 8], i, s)
#define initWithContentRect_styleMask_backing_defer_(inst, r, m, b, d) objc_msgSend(inst, LoadedObjCSelectors[ 9], r, (unsigned)(m), (unsigned)(b), (bool)(d))
#define initWithTitle_action_keyEquivalent_(inst, t, a, k)             objc_msgSend(inst, LoadedObjCSelectors[10], t, a, k)
#define makeKeyAndOrderFront_(inst, w)                                 objc_msgSend(inst, LoadedObjCSelectors[11], w)
#define run(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[12])
#define separatorItem(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[13])
#define setActivationPolicy_(inst, p)                                  objc_msgSend(inst, LoadedObjCSelectors[14], (unsigned)(p))
#define setAutoenablesItems_(inst, b)                                  objc_msgSend(inst, LoadedObjCSelectors[15], (bool)(b))
#define setBackgroundColor_(inst, c)                                   objc_msgSend(inst, LoadedObjCSelectors[16], c)
#define setDelegate_(inst, d)                                          objc_msgSend(inst, LoadedObjCSelectors[17], d)
#define setEnabled_(inst, b)                                           objc_msgSend(inst, LoadedObjCSelectors[18], (bool)(b))
#define setHasShadow_(inst, b)                                         objc_msgSend(inst, LoadedObjCSelectors[19], (bool)(b))
#define setHighlightMode_(inst, b)                                     objc_msgSend(inst, LoadedObjCSelectors[20], (bool)(b))
#define setImage_(inst, i)                                             objc_msgSend(inst, LoadedObjCSelectors[21], i)
#define setMenu_(inst, m)                                              objc_msgSend(inst, LoadedObjCSelectors[22], m)
#define setOnStateImage_(inst, i)                                      objc_msgSend(inst, LoadedObjCSelectors[23], i)
#define setOpaque_(inst, b)                                            objc_msgSend(inst, LoadedObjCSelectors[24], (bool)(b))
#define setState_(inst, s)                                             objc_msgSend(inst, LoadedObjCSelectors[25], (unsigned)(s))
#define setSubmenu_(inst, m)                                           objc_msgSend(inst, LoadedObjCSelectors[26], m)
#define sharedApplication(inst)                                        objc_msgSend(inst, LoadedObjCSelectors[27])
#define statusItemWithLength_(inst, l)                                 objc_msgSend(inst, LoadedObjCSelectors[28], (double)(l))
#define stringWithUTF8String_(inst, s)                                 objc_msgSend(inst, LoadedObjCSelectors[29], (char*)(s))
#define systemStatusBar(inst)                                          objc_msgSend(inst, LoadedObjCSelectors[30])
#define thickness                                                                         LoadedObjCSelectors[31]
#define mainScreen(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[32])
#define stop_(inst, s)                                                 objc_msgSend(inst, LoadedObjCSelectors[33], s)
#define drain(inst)                                                    objc_msgSend(inst, LoadedObjCSelectors[34])
#define delegate(inst)                                                 objc_msgSend(inst, LoadedObjCSelectors[35])
#define setTag_(inst, t)                                               objc_msgSend(inst, LoadedObjCSelectors[36], t)
#define tag(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[37])
#define setRepresentedObject_(inst, o)                                 objc_msgSend(inst, LoadedObjCSelectors[38], o)
#define representedObject(inst)                                        objc_msgSend(inst, LoadedObjCSelectors[39])
#define setTarget_(inst, t)                                            objc_msgSend(inst, LoadedObjCSelectors[40], t)
#define target(inst)                                                   objc_msgSend(inst, LoadedObjCSelectors[41])
#define setIgnoresMouseEvents_(inst, i)                                objc_msgSend(inst, LoadedObjCSelectors[42], (bool)(i))
#define hide_(inst, e)                                                 objc_msgSend(inst, LoadedObjCSelectors[43], e)
#define unhide_(inst, e)                                               objc_msgSend(inst, LoadedObjCSelectors[44], e)
#define retain(inst)                                                   objc_msgSend(inst, LoadedObjCSelectors[45])
#define retainCount(inst)                                              objc_msgSend(inst, LoadedObjCSelectors[46])
#define initWithFrame_(inst, r)                                        objc_msgSend(inst, LoadedObjCSelectors[47], r)
#define initWithFrame_pixelFormat_(inst, f, p)                         objc_msgSend(inst, LoadedObjCSelectors[48], f, p)
#define setContentView_(inst, v)                                       objc_msgSend(inst, LoadedObjCSelectors[49], v)
#define initWithAttributes_(inst, a)                                   objc_msgSend(inst, LoadedObjCSelectors[50], (uint32_t*)(a))
#define currentContext(inst)                                           objc_msgSend(inst, LoadedObjCSelectors[51])
#define graphicsPort(inst)                                             objc_msgSend(inst, LoadedObjCSelectors[52])
#define setLevel_(inst, l)                                             objc_msgSend(inst, LoadedObjCSelectors[53], (long)(l))
#define pointingHandCursor(inst)                                       objc_msgSend(inst, LoadedObjCSelectors[54])
#define set(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[55])
#define setNeedsDisplay_(inst, d)                                      objc_msgSend(inst, LoadedObjCSelectors[56], (bool)(d))
#define drawRect_                                                                         LoadedObjCSelectors[57]
#define MenuSelector                                                                      LoadedObjCSelectors[58]
#define mouseLocationOutsideOfEventStream                                                 LoadedObjCSelectors[59]
#define pressedMouseButtons(inst)                                (long)objc_msgSend(inst, LoadedObjCSelectors[60])
#define openGLContext(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[61])
#define flushBuffer(inst)                                              objc_msgSend(inst, LoadedObjCSelectors[62])
#define makeCurrentContext(inst)                                       objc_msgSend(inst, LoadedObjCSelectors[63])
#define setValues_forParameter_(inst, v, p)                            objc_msgSend(inst, LoadedObjCSelectors[64], (GLint*)(v), (GLint)(p))
