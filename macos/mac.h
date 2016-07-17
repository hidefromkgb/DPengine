#ifndef HDR_OBJC
#define HDR_OBJC

/// [IMPORTANT:] use CoreFoundation classes wherever possible!
/// There is no ARC in pure C, so the bridging will be direct!

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <stdarg.h>



/// overloaded method for a subclass
typedef struct _OMSC {
    SEL name;
    void *func;
} OMSC;

/// useful when including in parallel with ObjC headers
#ifndef NON_ENUM

#if __LP64__ || TARGET_OS_EMBEDDED || TARGET_OS_IPHONE || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
    typedef long NSInteger;
    typedef unsigned long NSUInteger;
#else
    typedef int NSInteger;
    typedef unsigned int NSUInteger;
#endif



/// OSX versions
#define OSX_10_07_PLUS (kCFCoreFoundationVersionNumber >=  635.00)
#define OSX_10_10_PLUS (kCFCoreFoundationVersionNumber >= 1151.16)

#ifdef __i386__
    #define NSVariableStatusItemLength 0xBF800000 /** -1.0f **/
    #define NSSquareStatusItemLength   0xC0000000 /** -2.0f **/
#else
    #define NSVariableStatusItemLength ((double)-1.0)
    #define NSSquareStatusItemLength   ((double)-2.0)
#endif
enum {
    NSUserDomainMask    = 0x0001,
    NSLocalDomainMask   = 0x0002,
    NSNetworkDomainMask = 0x0004,
    NSSystemDomainMask  = 0x0008,
    NSAllDomainsMask    = 0xFFFF,
};
enum {
    NSApplicationDirectory          =   1,
    NSDemoApplicationDirectory      =   2,
    NSDeveloperApplicationDirectory =   3,
    NSAdminApplicationDirectory     =   4,
    NSLibraryDirectory              =   5,
    NSDeveloperDirectory            =   6,
    NSUserDirectory                 =   7,
    NSDocumentationDirectory        =   8,
    NSDocumentDirectory             =   9,
    NSCoreServiceDirectory          =  10,
    NSAutosavedInformationDirectory =  11,
    NSDesktopDirectory              =  12,
    NSCachesDirectory               =  13,
    NSApplicationSupportDirectory   =  14,
    NSDownloadsDirectory            =  15,
    NSInputMethodsDirectory         =  16,
    NSMoviesDirectory               =  17,
    NSMusicDirectory                =  18,
    NSPicturesDirectory             =  19,
    NSPrinterDescriptionDirectory   =  20,
    NSSharedPublicDirectory         =  21,
    NSPreferencePanesDirectory      =  22,
    NSItemReplacementDirectory      =  99,
    NSAllApplicationsDirectory      = 100,
    NSAllLibrariesDirectory         = 101,
};
enum {
    NSNumberFormatterBehaviorDefault =    0,
    NSNumberFormatterBehavior10_0    = 1000,
    NSNumberFormatterBehavior10_4    = 1040,
};
enum {
    NSTableColumnNoResizing        = (0     ),
    NSTableColumnAutoresizingMask  = (1 << 0),
    NSTableColumnUserResizingMask  = (1 << 1),
};
enum {
    NSBorderlessWindowMask         = (0     ),
    NSTitledWindowMask             = (1 << 0),
    NSClosableWindowMask           = (1 << 1),
    NSMiniaturizableWindowMask     = (1 << 2),
    NSResizableWindowMask          = (1 << 3),
    NSNonactivatingPanelMask       = (1 << 7),
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
    NSRegularControlSize  = 0,
    NSSmallControlSize    = 1,
    NSMiniControlSize     = 2,
};
enum {
    NSLeftTextAlignment      = 0,
    NSRightTextAlignment     = 1,
    NSCenterTextAlignment    = 2,
    NSJustifiedTextAlignment = 3,
    NSNaturalTextAlignment   = 4
};
enum {
    NSMomentaryLightButton        = 0,
    NSMomentaryPushButton         = 0,
    NSPushOnPushOffButton         = 1,
    NSToggleButton                = 2,
    NSSwitchButton                = 3,
    NSRadioButton                 = 4,
    NSMomentaryChangeButton       = 5,
    NSOnOffButton                 = 6,
    NSMomentaryLight              = 7,
    NSMomentaryPushInButton       = 7,
    NSAcceleratorButton           = 8,
    NSMultiLevelAcceleratorButton = 9,
};
enum {
    NSRoundedBezelStyle           =  1,
    NSRegularSquareBezelStyle     =  2,
    NSSmallIconButtonBezelStyle   =  2,
    NSThickSquareBezelStyle       =  3,
    NSThickerSquareBezelStyle     =  4,
    NSDisclosureBezelStyle        =  5,
    NSShadowlessSquareBezelStyle  =  6,
    NSCircularBezelStyle          =  7,
    NSTexturedSquareBezelStyle    =  8,
    NSHelpButtonBezelStyle        =  9,
    NSSmallSquareBezelStyle       = 10,
    NSTexturedRoundedBezelStyle   = 11,
    NSRoundRectBezelStyle         = 12,
    NSRecessedBezelStyle          = 13,
    NSRoundedDisclosureBezelStyle = 14,
    NSInlineBezelStyle            = 15,
};
enum {
    NSNoImage       = 0,
    NSImageOnly     = 1,
    NSImageLeft     = 2,
    NSImageRight    = 3,
    NSImageBelow    = 4,
    NSImageAbove    = 5,
    NSImageOverlaps = 6,
};
enum {
    NSTrackingMouseEnteredAndExited    = (1 << 0),
    NSTrackingMouseMoved               = (1 << 1),
    NSTrackingCursorUpdate             = (1 << 2),
    NSTrackingActiveWhenFirstResponder = (1 << 3),
    NSTrackingActiveInKeyWindow        = (1 << 4),
    NSTrackingActiveInActiveApp        = (1 << 5),
    NSTrackingActiveAlways             = (1 << 6),
    NSTrackingAssumeInside             = (1 << 7),
    NSTrackingInVisibleRect            = (1 << 8),
    NSTrackingEnabledDuringMouseDrag   = (1 << 9),
};
enum {
    NSLeftMouseDown      =  1,
    NSLeftMouseUp        =  2,
    NSRightMouseDown     =  3,
    NSRightMouseUp       =  4,
    NSMouseMoved         =  5,
    NSLeftMouseDragged   =  6,
    NSRightMouseDragged  =  7,
    NSMouseEntered       =  8,
    NSMouseExited        =  9,
    NSKeyDown            = 10,
    NSKeyUp              = 11,
    NSFlagsChanged       = 12,
    NSAppKitDefined      = 13,
    NSSystemDefined      = 14,
    NSApplicationDefined = 15,
    NSPeriodic           = 16,
    NSCursorUpdate       = 17,
    NSScrollWheel        = 22,
    NSTabletPoint        = 23,
    NSTabletProximity    = 24,
    NSOtherMouseDown     = 25,
    NSOtherMouseUp       = 26,
    NSOtherMouseDragged  = 27,
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

#endif



#define STR_OBJC_CLASSES          \
       "NSObject",                \
       "NSApplication",           \
       "NSAutoreleasePool",       \
       "NSBundle",                \
       "NSEvent",                 \
       "NSFont",                  \
       "NSMutableParagraphStyle", \
       "NSFileManager",           \
       "NSNumberFormatter",       \
       "NSGraphicsContext",       \
       "NSColor",                 \
       "NSImage",                 \
       "NSMenu",                  \
       "NSMenuItem",              \
       "NSStatusBar",             \
       "NSScreen",                \
       "NSCursor",                \
       "NSWindow",                \
       "NSPanel",                 \
       "NSTextField",             \
       "NSButtonCell",            \
       "NSButton",                \
       "NSProgressIndicator",     \
       "NSStepper",               \
       "NSView",                  \
       "NSScrollView",            \
       "NSOpenGLView",            \
       "NSOpenGLPixelFormat",     \
       "NSTableView",             \
       "NSTableColumn"

#define NSObject                (LoadedObjCClasses[ 0])
#define NSApplication           (LoadedObjCClasses[ 1])
#define NSAutoreleasePool       (LoadedObjCClasses[ 2])
#define NSBundle                (LoadedObjCClasses[ 3])
#define NSEvent                 (LoadedObjCClasses[ 4])
#define NSFont                  (LoadedObjCClasses[ 5])
#define NSMutableParagraphStyle (LoadedObjCClasses[ 6])
#define NSFileManager           (LoadedObjCClasses[ 7])
#define NSNumberFormatter       (LoadedObjCClasses[ 8])
#define NSGraphicsContext       (LoadedObjCClasses[ 9])
#define NSColor                 (LoadedObjCClasses[10])
#define NSImage                 (LoadedObjCClasses[11])
#define NSMenu                  (LoadedObjCClasses[12])
#define NSMenuItem              (LoadedObjCClasses[13])
#define NSStatusBar             (LoadedObjCClasses[14])
#define NSScreen                (LoadedObjCClasses[15])
#define NSCursor                (LoadedObjCClasses[16])
#define NSWindow                (LoadedObjCClasses[17])
#define NSPanel                 (LoadedObjCClasses[18])
#define NSTextField             (LoadedObjCClasses[19])
#define NSButtonCell            (LoadedObjCClasses[20])
#define NSButton                (LoadedObjCClasses[21])
#define NSProgressIndicator     (LoadedObjCClasses[22])
#define NSStepper               (LoadedObjCClasses[23])
#define NSView                  (LoadedObjCClasses[24])
#define NSScrollView            (LoadedObjCClasses[25])
#define NSOpenGLView            (LoadedObjCClasses[26])
#define NSOpenGLPixelFormat     (LoadedObjCClasses[27])
#define NSTableView             (LoadedObjCClasses[28])
#define NSTableColumn           (LoadedObjCClasses[29])



#define STR_OBJC_SELECTORS                             \
       "init",                                         \
       "alloc",                                        \
       "release",                                      \
       "retain",                                       \
       "class",                                        \
       "cell",                                         \
       "button",                                       \
       "_A",                                           \
       "setAction:",                                   \
       "setTarget:",                                   \
       "setActivationPolicy:",                         \
       "activateIgnoringOtherApps:",                   \
       "sharedApplication",                            \
       "run",                                          \
       "stop:",                                        \
       "mainBundle",                                   \
       "bundlePath",                                   \
       "URLsForDirectory:inDomains:",                  \
       "separatorItem",                                \
       "addItem:",                                     \
       "setAutoenablesItems:",                         \
       "imageNamed:",                                  \
       "setImage:",                                    \
       "setOnStateImage:",                             \
       "setSubmenu:",                                  \
       "popUpMenuPositioningItem:atLocation:inView:",  \
       "clearColor",                                   \
       "controlColor",                                 \
       "setBackgroundColor:",                          \
       "initWithCGImage:size:",                        \
       "initWithContentRect:styleMask:backing:defer:", \
       "initWithTitle:action:keyEquivalent:",          \
       "initWithFrame:pixelFormat:",                   \
       "initWithAttributes:",                          \
       "contentRectForFrameRect:",                     \
       "frameRectForContentRect:",                     \
       "visibleFrame",                                 \
       "frame",                                        \
       "setFrame:",                                    \
       "setFrame:display:animate:",                    \
       "setInitialFirstResponder:",                    \
       "setMinSize:",                                  \
       "setTitle:",                                    \
       "setStringValue:",                              \
       "windowShouldClose:",                           \
       "windowDidResize:",                             \
       "makeKeyWindow",                                \
       "orderFront:",                                  \
       "orderOut:",                                    \
       "setNeedsDisplay:",                             \
       "setLevel:",                                    \
       "setDelegate:",                                 \
       "setEnabled:",                                  \
       "setHasShadow:",                                \
       "setIgnoresMouseEvents:",                       \
       "setNextKeyView:",                              \
       "setDefaultButtonCell:",                        \
       "verticalScroller",                             \
       "setHasVerticalScroller:",                      \
       "isEnabled",                                    \
       "state",                                        \
       "setState:",                                    \
       "setToolTip:",                                  \
       "setButtonType:",                               \
       "setBezelStyle:",                               \
       "setImagePosition:",                            \
       "setSendsActionOnEndEditing:",                  \
       "control:textView:doCommandBySelector:",        \
       "moveDown:",                                    \
       "moveUp:",                                      \
       "setEditable:",                                 \
       "setSelectable:",                               \
       "setBezeled:",                                  \
       "setBordered:",                                 \
       "setDrawsBackground:",                          \
       "statusItemWithLength:",                        \
       "removeStatusItem:",                            \
       "systemStatusBar",                              \
       "mainScreen",                                   \
       "thickness",                                    \
       "cellSize",                                     \
       "tag",                                          \
       "setTag:",                                      \
       "setHighlightMode:",                            \
       "setHidden:",                                   \
       "setContentView:",                              \
       "setDocumentView:",                             \
       "addSubview:",                                  \
       "setOpaque:",                                   \
       "isOpaque",                                     \
       "isFlipped",                                    \
       "drawRect:",                                    \
       "mouseLocation",                                \
       "pressedMouseButtons",                          \
       "pointingHandCursor",                           \
       "push",                                         \
       "pop",                                          \
       "graphicsPort",                                 \
       "currentContext",                               \
       "openGLContext",                                \
       "makeCurrentContext",                           \
       "setValues:forParameter:",                      \
       "flushBuffer",                                  \
       "defaultManager",                               \
       "systemFontOfSize:",                            \
       "ascender",                                     \
       "descender",                                    \
       "maximumAdvancement",                           \
       "setIndeterminate:",                            \
       "setAlignment:",                                \
       "addTableColumn:",                              \
       "headerCell",                                   \
       "setWantsLayer:",                               \
       "setControlSize:",                              \
       "drawInRect:withAttributes:",                   \
       "textDidChange:",                               \
       "stringValue",                                  \
       "doubleValue",                                  \
       "setDoubleValue:",                              \
       "setIntValue:",                                 \
       "displayIfNeeded",                              \
       "setMinValue:",                                 \
       "setMaxValue:",                                 \
       "setValueWraps:",                               \
       "setFormatter:",                                \
       "setFormatterBehavior:",                        \
       "setNumberStyle:",                              \
       "setPartialStringValidationEnabled:",           \
       "isPartialStringValid:newEditingString:errorDescription:",\
       "getObjectValue:forString:errorDescription:",   \
       "reloadData",                                   \
       "dataCell",                                     \
       "setDataCell:",                                 \
       "setDataSource:",                               \
       "setResizingMask:",                             \
       "numberOfRowsInTableView:",                     \
       "tableView:objectValueForTableColumn:row:",     \
       "tableView:setObjectValue:forTableColumn:row:", \
       "tableView:dataCellForTableColumn:row:",        \
       "tableView:viewForTableColumn:row:",            \
       "postEvent:atStart:",                           \
       /** srsly, next line is just batshit insane **/ \
       "otherEventWithType:location:modifierFlags:timestamp:windowNumber:context:subtype:data1:data2:"

#define init(inst)                                                     objc_msgSend(inst, LoadedObjCSelectors[  0])
#define alloc(inst)                                                    objc_msgSend(inst, LoadedObjCSelectors[  1])
#define release(inst)                                                  objc_msgSend(inst, LoadedObjCSelectors[  2])
#define retain(inst)                                                   objc_msgSend(inst, LoadedObjCSelectors[  3])
#define class(inst)                                             (Class)objc_msgSend(inst, LoadedObjCSelectors[  4])
#define cell(inst)                                                     objc_msgSend(inst, LoadedObjCSelectors[  5])
#define button(inst)                                                   objc_msgSend(inst, LoadedObjCSelectors[  6])
#define ActionSelector                                                                    LoadedObjCSelectors[  7]
#define setAction_(inst, a)                                            objc_msgSend(inst, LoadedObjCSelectors[  8], a)
#define setTarget_(inst, t)                                            objc_msgSend(inst, LoadedObjCSelectors[  9], t)
#define setActivationPolicy_(inst, p)                                  objc_msgSend(inst, LoadedObjCSelectors[ 10], (unsigned)(p))
#define activateIgnoringOtherApps_(inst, b)                            objc_msgSend(inst, LoadedObjCSelectors[ 11], (bool)(b))
#define sharedApplication(inst)                                        objc_msgSend(inst, LoadedObjCSelectors[ 12])
#define run(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[ 13])
#define stop_(inst, s)                                                 objc_msgSend(inst, LoadedObjCSelectors[ 14], s)
#define mainBundle(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 15])
#define bundlePath(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 16])
#define URLsForDirectory_inDomains_(inst, u, d)                        objc_msgSend(inst, LoadedObjCSelectors[ 17], u, d)
#define separatorItem(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[ 18])
#define addItem_(inst, i)                                              objc_msgSend(inst, LoadedObjCSelectors[ 19], i)
#define setAutoenablesItems_(inst, b)                                  objc_msgSend(inst, LoadedObjCSelectors[ 20], (bool)(b))
#define imageNamed_(inst, s)                                           objc_msgSend(inst, LoadedObjCSelectors[ 21], s)
#define setImage_(inst, i)                                             objc_msgSend(inst, LoadedObjCSelectors[ 22], i)
#define setOnStateImage_(inst, i)                                      objc_msgSend(inst, LoadedObjCSelectors[ 23], i)
#define setSubmenu_(inst, m)                                           objc_msgSend(inst, LoadedObjCSelectors[ 24], m)
#define popUpMenuPositioningItem_atLocation_inView_(inst, m, l, v)     objc_msgSend(inst, LoadedObjCSelectors[ 25], m, (CGPoint)(l), v)
#define clearColor(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 26])
#define controlColor(inst)                                             objc_msgSend(inst, LoadedObjCSelectors[ 27])
#define setBackgroundColor_(inst, c)                                   objc_msgSend(inst, LoadedObjCSelectors[ 28], c)
#define initWithCGImage_size_(inst, i, s)                              objc_msgSend(inst, LoadedObjCSelectors[ 29], i, s)
#define initWithContentRect_styleMask_backing_defer_(inst, r, m, b, d) objc_msgSend(inst, LoadedObjCSelectors[ 30], r, (unsigned)(m), (unsigned)(b), (bool)(d))
#define initWithTitle_action_keyEquivalent_(inst, t, a, k)             objc_msgSend(inst, LoadedObjCSelectors[ 31], t, a, k)
#define initWithFrame_pixelFormat_(inst, f, p)                         objc_msgSend(inst, LoadedObjCSelectors[ 32], f, p)
#define initWithAttributes_(inst, a)                                   objc_msgSend(inst, LoadedObjCSelectors[ 33], (uint32_t*)(a))
#define ContentRectForFrameRect_                                                          LoadedObjCSelectors[ 34]
#define FrameRectForContentRect_                                                          LoadedObjCSelectors[ 35]
#define VisibleFrame                                                                      LoadedObjCSelectors[ 36]
#define Frame                                                                             LoadedObjCSelectors[ 37]
#define setFrame_(inst, f)                                             objc_msgSend(inst, LoadedObjCSelectors[ 38], f)
#define setFrame_display_animate_(inst, f, d, a)                       objc_msgSend(inst, LoadedObjCSelectors[ 39], f, d, a)
#define setInitialFirstResponder_(inst, r)                             objc_msgSend(inst, LoadedObjCSelectors[ 40], r)
#define setMinSize_(inst, s)                                           objc_msgSend(inst, LoadedObjCSelectors[ 41], s)
#define setTitle_(inst, t)                                             objc_msgSend(inst, LoadedObjCSelectors[ 42], t)
#define setStringValue_(inst, s)                                       objc_msgSend(inst, LoadedObjCSelectors[ 43], s)
#define WindowShouldClose_                                                                LoadedObjCSelectors[ 44]
#define WindowDidResize_                                                                  LoadedObjCSelectors[ 45]
#define makeKeyWindow(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[ 46])
#define orderFront_(inst, w)                                           objc_msgSend(inst, LoadedObjCSelectors[ 47], w)
#define orderOut_(inst, w)                                             objc_msgSend(inst, LoadedObjCSelectors[ 48], w)
#define setNeedsDisplay_(inst, d)                                      objc_msgSend(inst, LoadedObjCSelectors[ 49], (bool)(d))
#define setLevel_(inst, l)                                             objc_msgSend(inst, LoadedObjCSelectors[ 50], (long)(l))
#define setDelegate_(inst, d)                                          objc_msgSend(inst, LoadedObjCSelectors[ 51], d)
#define setEnabled_(inst, b)                                           objc_msgSend(inst, LoadedObjCSelectors[ 52], (bool)(b))
#define setHasShadow_(inst, b)                                         objc_msgSend(inst, LoadedObjCSelectors[ 53], (bool)(b))
#define setIgnoresMouseEvents_(inst, i)                                objc_msgSend(inst, LoadedObjCSelectors[ 54], (bool)(i))
#define setNextKeyView_(inst, v)                                       objc_msgSend(inst, LoadedObjCSelectors[ 55], v)
#define setDefaultButtonCell_(inst, c)                                 objc_msgSend(inst, LoadedObjCSelectors[ 56], c)
#define verticalScroller(inst)                                         objc_msgSend(inst, LoadedObjCSelectors[ 57])
#define setHasVerticalScroller_(inst, s)                               objc_msgSend(inst, LoadedObjCSelectors[ 58], s)
#define isEnabled(inst)                                          (bool)objc_msgSend(inst, LoadedObjCSelectors[ 59])
#define state(inst)                                              (long)objc_msgSend(inst, LoadedObjCSelectors[ 60])
#define setState_(inst, s)                                             objc_msgSend(inst, LoadedObjCSelectors[ 61], (int)(s))
#define setToolTip_(inst, t)                                           objc_msgSend(inst, LoadedObjCSelectors[ 62], t)
#define setButtonType_(inst, t)                                        objc_msgSend(inst, LoadedObjCSelectors[ 63], t)
#define setBezelStyle_(inst, s)                                        objc_msgSend(inst, LoadedObjCSelectors[ 64], s)
#define setImagePosition_(inst, p)                                     objc_msgSend(inst, LoadedObjCSelectors[ 65], p)
#define setSendsActionOnEndEditing_(inst, b)                           objc_msgSend(inst, LoadedObjCSelectors[ 66], (bool)(b))
#define Control_textView_doCommandBySelector_                                             LoadedObjCSelectors[ 67]
#define MoveDown_                                                                         LoadedObjCSelectors[ 68]
#define MoveUp_                                                                           LoadedObjCSelectors[ 69]
#define setEditable_(inst, e)                                          objc_msgSend(inst, LoadedObjCSelectors[ 70], e)
#define setSelectable_(inst, s)                                        objc_msgSend(inst, LoadedObjCSelectors[ 71], s)
#define setBezeled_(inst, b)                                           objc_msgSend(inst, LoadedObjCSelectors[ 72], b)
#define setBordered_(inst, b)                                          objc_msgSend(inst, LoadedObjCSelectors[ 73], b)
#define setDrawsBackground_(inst, d)                                   objc_msgSend(inst, LoadedObjCSelectors[ 74], d)
#define statusItemWithLength_(inst, l)                                 objc_msgSend(inst, LoadedObjCSelectors[ 75], l) /** do NOT "correct" L to CGFloat! **/
#define removeStatusItem_(inst, i)                                     objc_msgSend(inst, LoadedObjCSelectors[ 76], i)
#define systemStatusBar(inst)                                          objc_msgSend(inst, LoadedObjCSelectors[ 77])
#define mainScreen(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 78])
#define Thickness                                                                         LoadedObjCSelectors[ 79]
#define CellSize                                                                          LoadedObjCSelectors[ 80]
#define tag(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[ 81])
#define setTag_(inst, t)                                               objc_msgSend(inst, LoadedObjCSelectors[ 82], t)
#define setHighlightMode_(inst, b)                                     objc_msgSend(inst, LoadedObjCSelectors[ 83], (bool)(b))
#define setHidden_(inst, h)                                            objc_msgSend(inst, LoadedObjCSelectors[ 84], (bool)(h))
#define setContentView_(inst, v)                                       objc_msgSend(inst, LoadedObjCSelectors[ 85], v)
#define setDocumentView_(inst, v)                                      objc_msgSend(inst, LoadedObjCSelectors[ 86], v)
#define addSubview_(inst, v)                                           objc_msgSend(inst, LoadedObjCSelectors[ 87], v)
#define setOpaque_(inst, b)                                            objc_msgSend(inst, LoadedObjCSelectors[ 88], (bool)(b))
#define IsOpaque                                                                          LoadedObjCSelectors[ 89]
#define IsFlipped                                                                         LoadedObjCSelectors[ 90]
#define DrawRect_                                                                         LoadedObjCSelectors[ 91]
#define MouseLocation                                                                     LoadedObjCSelectors[ 92]
#define pressedMouseButtons(inst)                                (long)objc_msgSend(inst, LoadedObjCSelectors[ 93])
#define pointingHandCursor(inst)                                       objc_msgSend(inst, LoadedObjCSelectors[ 94])
#define push(inst)                                                     objc_msgSend(inst, LoadedObjCSelectors[ 95])
#define pop(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[ 96])
#define graphicsPort(inst)                               (CGContextRef)objc_msgSend(inst, LoadedObjCSelectors[ 97])
#define currentContext(inst)                                           objc_msgSend(inst, LoadedObjCSelectors[ 98])
#define openGLContext(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[ 99])
#define makeCurrentContext(inst)                                       objc_msgSend(inst, LoadedObjCSelectors[100])
#define setValues_forParameter_(inst, v, p)                            objc_msgSend(inst, LoadedObjCSelectors[101], (int*)(v), (int)(p))
#define flushBuffer(inst)                                              objc_msgSend(inst, LoadedObjCSelectors[102])
#define defaultManager(inst)                                           objc_msgSend(inst, LoadedObjCSelectors[103])
#define systemFontOfSize_(inst, s)                                     objc_msgSend(inst, LoadedObjCSelectors[104], s)
#define Ascender                                                                          LoadedObjCSelectors[105]
#define Descender                                                                         LoadedObjCSelectors[106]
#define MaximumAdvancement                                                                LoadedObjCSelectors[107]
#define setIndeterminate_(inst, i)                                     objc_msgSend(inst, LoadedObjCSelectors[108], (bool)(i))
#define setAlignment_(inst, a)                                         objc_msgSend(inst, LoadedObjCSelectors[109], (int)(a))
#define addTableColumn_(inst, c)                                       objc_msgSend(inst, LoadedObjCSelectors[110], (id)(c))
#define headerCell(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[111])
#define setWantsLayer_(inst, w)                                        objc_msgSend(inst, LoadedObjCSelectors[112], (bool)(w))
#define setControlSize_(inst, s)                                       objc_msgSend(inst, LoadedObjCSelectors[113], (NSUInteger)(s))
#define drawInRect_withAttributes_(inst, r, a)                         objc_msgSend(inst, LoadedObjCSelectors[114], (CGRect)(r), (id)(a))
#define TextDidChange_                                                                    LoadedObjCSelectors[115]
#define stringValue(inst)                                              objc_msgSend(inst, LoadedObjCSelectors[116])
#define DoubleValue                                                                       LoadedObjCSelectors[117]
#define setDoubleValue_(inst, v)                                       objc_msgSend(inst, LoadedObjCSelectors[118], (double)(v))
#define setIntValue_(inst, v)                                          objc_msgSend(inst, LoadedObjCSelectors[119], (NSInteger)(v))
#define displayIfNeeded(inst)                                          objc_msgSend(inst, LoadedObjCSelectors[120])
#define setMinValue_(inst, v)                                          objc_msgSend(inst, LoadedObjCSelectors[121], (double)(v))
#define setMaxValue_(inst, v)                                          objc_msgSend(inst, LoadedObjCSelectors[122], (double)(v))
#define setValueWraps_(inst, w)                                        objc_msgSend(inst, LoadedObjCSelectors[123], w)
#define setFormatter_(inst, f)                                         objc_msgSend(inst, LoadedObjCSelectors[124], f)
#define setFormatterBehavior_(inst, b)                                 objc_msgSend(inst, LoadedObjCSelectors[125], b)
#define setNumberStyle_(inst, s)                                       objc_msgSend(inst, LoadedObjCSelectors[126], s)
#define setPartialStringValidationEnabled(inst, b)                     objc_msgSend(inst, LoadedObjCSelectors[127], b)
#define IsPartialStringValid_newEditingString_errorDescription_                           LoadedObjCSelectors[128]
#define getObjectValue_forString_errorDescription_(inst, v, s, e)(bool)objc_msgSend(inst, LoadedObjCSelectors[129], v, s, e)
#define reloadData(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[130])
#define dataCell(inst)                                                 objc_msgSend(inst, LoadedObjCSelectors[131])
#define setDataCell_(inst, c)                                          objc_msgSend(inst, LoadedObjCSelectors[132], c)
#define setDataSource_(inst, d)                                        objc_msgSend(inst, LoadedObjCSelectors[133], d)
#define setResizingMask_(inst, m)                                      objc_msgSend(inst, LoadedObjCSelectors[134], m)
#define NumberOfRowsInTableView_                                                          LoadedObjCSelectors[135]
#define TableView_objectValueForTableColumn_row_                                          LoadedObjCSelectors[136]
#define TableView_setObjectValue_forTableColumn_row_                                      LoadedObjCSelectors[137]
#define TableView_dataCellForTableColumn_row_                                             LoadedObjCSelectors[138]
#define TableView_viewForTableColumn_row_                                                 LoadedObjCSelectors[139]
#define postEvent_atStart_(inst, e, s)                                 objc_msgSend(inst, LoadedObjCSelectors[140], e, (bool)(s))
#define MakeEvent(t, l, m, s, w, c)                                 objc_msgSend(NSEvent, LoadedObjCSelectors[141], t, (CGPoint)(l), m, (CGFloat)(s), (id)(w), (id)(c), nil, nil, nil)



/// a horrible abomination coming up (but this is generally the way OS X works)
#define GetT1DV(r, i, ...) { void *GetT1DV = objc_msgSend_fpret; r = ((typeof(r) (*)(id, ...))GetT1DV)(i, __VA_ARGS__); }
#define GetT2DV(r, i, ...) { void *GetT2DV = objc_msgSend;       r = ((typeof(r) (*)(id, ...))GetT2DV)(i, __VA_ARGS__); }
#define GetT4DV(r, i, ...) { void *GetT4DV = objc_msgSend_stret; r = ((typeof(r) (*)(id, ...))GetT4DV)(i, __VA_ARGS__); }

/// NSString creation macro
#define UTF8(s) CFStringCreateWithBytes(0, (s)? (uint8_t*)(s) : (uint8_t*)"", strlen((s)? (char*)(s) : (char*)""), kCFStringEncodingUTF8, false)

/// class instance variable management macros
#define GET_IVAR(inst, name, data) object_getInstanceVariable(inst, name, (void**)(data))
#define SET_IVAR(inst, name, data) object_setInstanceVariable(inst, name, (void*)(data))

/// this macro facilitates size calculation for static arrays
#define countof(s) (sizeof(s) / sizeof(*(s)))



static const char *StrObjCClasses[] = {STR_OBJC_CLASSES};
static id LoadedObjCClasses[countof(StrObjCClasses)] = {};
#undef STR_OBJC_CLASSES

static const char *StrObjCSelectors[] = {STR_OBJC_SELECTORS};
static SEL LoadedObjCSelectors[countof(StrObjCSelectors)] = {};
#undef STR_OBJC_SELECTORS

/// subclassed entries reference tracker
static struct {
    Class uuid;
    char *name;
    long icnt;
} *SubclassedObjCClasses;
static long SubclassedObjCClassesCount = 0;



__attribute__((unused)) /// signals that the function might be left unused
static char *CopyUTF8(CFStringRef cfsr) {
    CFIndex slen, size;
    uint8_t *retn = 0;

    if (CFStringGetBytes(cfsr, CFRangeMake(0, slen = CFStringGetLength(cfsr)),
                         kCFStringEncodingUTF8, '?', false, 0, 0, &size) > 0)
        CFStringGetBytes(cfsr, CFRangeMake(0, slen), kCFStringEncodingUTF8,
                         '?', false, retn = calloc(1, 1 + size), size, 0);
    return (char*)retn;
}



/// When the official documentation states that there is an NSDictionary to
/// be created with some NS<Whatever> values used as keys, remember that in
/// the CoreFoundation framework their equivalents are named kCT<Whatever>.
#define MakeDict(k, ...) __MakeDict(k, __VA_ARGS__, nil)
__attribute__((unused))
static CFDictionaryRef __MakeDict(CFStringRef key1, ...) {
    CFDictionaryRef retn = 0;
    CFStringRef iter;
    va_list list;
    long size;

    CFStringRef *keys;
    id *vals;

    size = 0;
    iter = key1;
    va_start(list, key1);
    while (iter) {
        va_arg(list, void*);
        iter = va_arg(list, typeof(iter));
        size++;
    }
    va_end(list);
    if (size) {
        keys = malloc(size * sizeof(*keys));
        vals = malloc(size * sizeof(*vals));
        size = 0;
        iter = key1;
        va_start(list, key1);
        while (iter) {
            keys[size] = iter;
            vals[size++] = va_arg(list, typeof(*vals));
            iter = va_arg(list, typeof(iter));
        }
        va_end(list);
        retn = CFDictionaryCreate(0, (const void**)keys, (const void**)vals,
                                  size, &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
        free(vals);
        free(keys);
    }
    return retn;
}



__attribute__((unused))
static id NewClass(id base, char *name, char *flds[], OMSC *mths) {
    Class retn;
    long iter;

    for (retn = 0, iter = 0; iter < SubclassedObjCClassesCount; iter++)
        if (!strcmp(name, SubclassedObjCClasses[iter].name)) {
            retn = SubclassedObjCClasses[iter].uuid;
            SubclassedObjCClasses[iter].icnt++;
            break;
        }
    if (!retn) {
        retn = objc_allocateClassPair((Class)base, name, 0);
        SubclassedObjCClasses = realloc((SubclassedObjCClassesCount)?
                                         SubclassedObjCClasses : 0,
                                        (SubclassedObjCClassesCount + 1)
                                       * sizeof(*SubclassedObjCClasses));
        SubclassedObjCClasses[SubclassedObjCClassesCount++] =
            (typeof(*SubclassedObjCClasses)){retn, strdup(name), 1};

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
    }
    return (id)retn;
}



__attribute__((unused))
static void DelClass(Class uuid) {
    long iter;

    for (iter = 0; iter < SubclassedObjCClassesCount; iter++)
        if (uuid == SubclassedObjCClasses[iter].uuid) {
            SubclassedObjCClasses[iter].icnt--;
            if (!SubclassedObjCClasses[iter].icnt) {
                objc_disposeClassPair(SubclassedObjCClasses[iter].uuid);
                free(SubclassedObjCClasses[iter].name);
                if (iter < --SubclassedObjCClassesCount)
                    SubclassedObjCClasses[iter] =
                    SubclassedObjCClasses[SubclassedObjCClassesCount];
            }
            break;
        }
    if (!SubclassedObjCClassesCount)
        free(SubclassedObjCClasses);
}



/// no __attribute__((unused)) here: this function has to be called ASAP
static void LoadObjC() {
    long iter;

    if (*LoadedObjCClasses)
        return;
    for (iter = 0; iter < countof(StrObjCClasses); iter++)
        LoadedObjCClasses[iter] = objc_getClass(StrObjCClasses[iter]);
    for (iter = 0; iter < countof(StrObjCSelectors); iter++)
        LoadedObjCSelectors[iter] = sel_registerName(StrObjCSelectors[iter]);
}

#endif
