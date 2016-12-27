#ifndef HDR_OBJC
#define HDR_OBJC

/// [IMPORTANT:] use CoreFoundation classes wherever possible!
/// There is no ARC in pure C, so the bridging will be direct!

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <stdarg.h>



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
    NSSquareStatusItemLength   = -2,
    NSVariableStatusItemLength = -1,
};
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
    NSNormalWindowLevel   =  0,
    NSFloatingWindowLevel =  3,
    NSDockWindowLevel     =  5,
    NSSubmenuWindowLevel  = 10,
    NSMainMenuWindowLevel = 20,
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



static struct {
    Class uuid;
    char *name;
    long icnt;
} *SubclassedObjCClasses = 0;



/// OSX versions
#define OSX_10_05_PLUS (kCFCoreFoundationVersionNumber >=  476.00)
#define OSX_10_06_PLUS (kCFCoreFoundationVersionNumber >=  550.00)
#define OSX_10_07_PLUS (kCFCoreFoundationVersionNumber >=  635.00)
#define OSX_10_08_PLUS (kCFCoreFoundationVersionNumber >=  744.00)
#define OSX_10_09_PLUS (kCFCoreFoundationVersionNumber >=  855.11)
#define OSX_10_10_PLUS (kCFCoreFoundationVersionNumber >= 1151.16)

/// NSString creation macro
#define UTF8(s) CFStringCreateWithBytes(0, (s)? (uint8_t*)(s) : (uint8_t*)"", strlen((s)? (char*)(s) : (char*)""), kCFStringEncodingUTF8, false)

/// class instance variable management macros
#define GET_IVAR(inst, name, data) object_getInstanceVariable((void*)(inst), name, (void**)(data))
#define SET_IVAR(inst, name, data) object_setInstanceVariable((void*)(inst), name, (void*)(data))



__attribute__((unused))
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
/// If there are none in CF, just import them: "extern void *NS<Whatever>".
#define MakeDict(k, ...) __MakeDict(k, ##__VA_ARGS__, nil)
__attribute__((unused))
static CFDictionaryRef __MakeDict(CFStringRef key1, ...) {
    CFDictionaryRef retn = 0;
    CFStringRef iter;
    va_list list;
    long size;

    CFStringRef *keys;
    void **vals;

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



#define PutToArr(...) __PutToArr(nil, ##__VA_ARGS__, nil)
__attribute__((unused))
static void **__PutToArr(void *head, ...) {
    va_list list;
    long size;
    void **retn;

    retn = 0;
    va_start(list, head);
    for (size = 0; va_arg(list, typeof(*retn)); size++);
    va_end(list);
    if (size) {
        retn = malloc((size + 1) * sizeof(*retn));
        va_start(list, head);
        for (size = 0; (retn[size] = va_arg(list, typeof(*retn))); size++);
        va_end(list);
    }
    return retn;
}

__attribute__((unused))
static void *NewClass(void *base, char *name, void **flds, void **mths) {
    Class retn;
    long iter;

    for (retn = 0, iter = 0;
         SubclassedObjCClasses && SubclassedObjCClasses[iter].name; iter++)
        if (!strcmp(name, SubclassedObjCClasses[iter].name)) {
            retn = SubclassedObjCClasses[iter].uuid;
            SubclassedObjCClasses[iter].icnt++;
            break;
        }
    if (!retn) {
        retn = objc_allocateClassPair((Class)base, name, 0);
        SubclassedObjCClasses = realloc(SubclassedObjCClasses, (iter + 2)
                                      * sizeof(*SubclassedObjCClasses));
        SubclassedObjCClasses[iter] =
            (typeof(*SubclassedObjCClasses)){retn, strdup(name), 1};
        SubclassedObjCClasses[iter + 1].name = 0;

        iter = -1;
        /// adding fields
        while (flds && flds[++iter])
            class_addIvar(retn, (char*)flds[iter],
                          sizeof(void*), (sizeof(void*) >= 8)? 3 : 2, 0);
        iter = -2;
        /// overloading methods
        while (mths && mths[iter += 2])
            class_addMethod(retn, (SEL)mths[iter], (IMP)mths[iter + 1], 0);

        objc_registerClassPair(retn);
    }
    return (void*)retn;
}



__attribute__((unused))
static void DelClass(Class uuid) {
    long iter, size = 0;

    for (; SubclassedObjCClasses && SubclassedObjCClasses[size].name; size++);
    for (iter = 0; iter < size; iter++)
        if (uuid == SubclassedObjCClasses[iter].uuid) {
            SubclassedObjCClasses[iter].icnt--;
            if (!SubclassedObjCClasses[iter].icnt) {
                objc_disposeClassPair(SubclassedObjCClasses[iter].uuid);
                free(SubclassedObjCClasses[iter].name);
                if (iter < --size) {
                    SubclassedObjCClasses[iter] = SubclassedObjCClasses[size];
                    iter = size;
                }
                SubclassedObjCClasses[iter].name = 0;
            }
            break;
        }
    if (SubclassedObjCClasses && !SubclassedObjCClasses[0].name) {
        free(SubclassedObjCClasses);
        SubclassedObjCClasses = 0;
    }
}



#define L(c, ...) \
L4(c,1,0,,,,,,,,,,,,,##__VA_ARGS__) L4(c,0,1,,,,,,,,,##__VA_ARGS__) \
L4(c,0,2,,,,,        ##__VA_ARGS__) L4(c,0,3,        ##__VA_ARGS__)

#define L4(c, f, n, ...) \
L3(c,f,n##0,,,,__VA_ARGS__) L3(c,0,n##1,,,__VA_ARGS__) \
L3(c,0,n##2,,  __VA_ARGS__) L3(c,0,n##3,  __VA_ARGS__)

#define L3(...) L2(__VA_ARGS__, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, )

#define L2(c, f, \
n00,n01,n02,n03, n04,n05,n06,n07, n08,n09,n0A,n0B, n0C,n0D,n0E,n0F, \
a00,a01,a02,a03, a04,a05,a06,a07, a08,a09,a0A,a0B, a0C,a0D,a0E,a0F, \
s, ...) L##s(c, f, n00, a00)

#define L1(c, f, n, a) c##f(n, a)
#define L0(c, f, n, a)

#define P1(n, a) , a _##n
#define P0(n, a) P1(n, a)
#define A1(n, a) P1(n,  )
#define A0(n, a) P0(n,  )

#define F(tnfv, text, retn, name, ...) __attribute__((unused))        \
static SEL __ ##name() { static SEL what = 0;                         \
    if (!what) what = sel_registerName(text); return what;            \
} __attribute__((unused))                                             \
static retn name(void *inst L(P, ##__VA_ARGS__)) {                    \
    static retn (*func)(void*, SEL, ##__VA_ARGS__) = 0;               \
    static SEL what = 0;                                              \
    if (!func) {                                                      \
        what = __ ##name();                                           \
        func = (retn (*)(void*, SEL, ##__VA_ARGS__))GetMsgSend(tnfv); \
    }                                                                 \
    return func(inst, what L(A, ##__VA_ARGS__));                      \
}

#define T(name) __attribute__((unused))                               \
static void *name() { static void *what = 0;                          \
    if (!what) what = (void*)objc_getClass(#name); return what;       \
} typedef struct __ ##name __ ##name

T(NSObject);
T(NSApplication);
T(NSAutoreleasePool);
T(NSBundle);
T(NSEvent);
T(NSFont);
T(NSMutableParagraphStyle);
T(NSFileManager);
T(NSNumberFormatter);
T(NSGraphicsContext);
T(NSImage);
T(NSMenu);
T(NSMenuItem);
T(NSStatusItem);
T(NSStatusBar);
T(NSScreen);
T(NSWindow);
T(NSTextField);
T(NSButtonCell);
T(NSButton);
T(NSProgressIndicator);
T(NSStepper);
T(NSView);
T(NSCell);
T(NSScrollView);
T(NSTableView);
T(NSTableColumn);
T(NSColor);
T(NSCursor);
T(NSPanel);
T(NSOpenGLView);
T(NSOpenGLContext);
T(NSOpenGLPixelFormat);

#if __LP64__ || TARGET_OS_EMBEDDED || TARGET_OS_IPHONE || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
    typedef long __NSInteger;
    typedef unsigned long __NSUInteger;
#else
    typedef int __NSInteger;
    typedef unsigned int __NSUInteger;
#endif

static void *GetMsgSend(long tnfv) {
    switch (tnfv) {
        case 4:  return objc_msgSend_stret;
        case 1:  return objc_msgSend_fpret;
        default: return objc_msgSend;
    }
}

F(0, "init", void*,
      init);
F(0, "alloc", void*,
      alloc);
F(0, "release", void*,
      release);
F(0, "retain", void*,
      retain);
F(0, "class", Class,
      class);
F(0, "cell", __NSCell*,
      cell);
F(0, "button", __NSButton*,
      button);
F(0, "setAction:", void,
      setAction_,
      SEL);
F(0, "setTarget:", void,
      setTarget_,
      void*);
F(0, "setActivationPolicy:", bool,
      setActivationPolicy_,
      __NSInteger);
F(0, "activateIgnoringOtherApps:", void,
      activateIgnoringOtherApps_,
      bool);
F(0, "sharedApplication", __NSApplication*,
      sharedApplication);
F(0, "run", void,
      run);
F(0, "stop:", void,
      stop_,
      void*);
F(0, "mainBundle", __NSBundle*,
      mainBundle);
F(0, "bundlePath", CFStringRef,
      bundlePath);
F(0, "URLsForDirectory:inDomains:", CFArrayRef,
      URLsForDirectory_inDomains_,
      __NSInteger, __NSInteger);
F(0, "separatorItem", __NSMenuItem*,
      separatorItem);
F(0, "addItem:", void,
      addItem_,
      __NSMenuItem*);
F(0, "setAutoenablesItems:", void,
      setAutoenablesItems_,
      bool);
F(0, "imageNamed:", __NSImage*,
      imageNamed_,
      CFStringRef);
F(0, "setImage:", void,
      setImage_,
      __NSImage*);
F(0, "setOnStateImage:", void,
      setOnStateImage_,
      __NSImage*);
F(0, "setSubmenu:", void,
      setSubmenu_,
      __NSMenu*);
F(0, "popUpMenuPositioningItem:atLocation:inView:", bool,
      popUpMenuPositioningItem_atLocation_inView_,
      __NSMenuItem*, CGPoint, __NSView*);
F(0, "initWithCGImage:size:", __NSImage*,
      initWithCGImage_size_,
      CGImageRef, CGPoint);
F(0, "initWithContentRect:styleMask:backing:defer:", __NSWindow*,
      initWithContentRect_styleMask_backing_defer_,
      CGRect, __NSInteger, __NSInteger, bool);
F(0, "initWithTitle:action:keyEquivalent:", __NSMenuItem*,
      initWithTitle_action_keyEquivalent_,
      CFStringRef, SEL, CFStringRef);
F(4, "contentRectForFrameRect:", CGRect,
      contentRectForFrameRect_,
      CGRect);
F(4, "frameRectForContentRect:", CGRect,
      frameRectForContentRect_,
      CGRect);
F(4, "visibleFrame", CGRect,
      visibleFrame);
F(4, "frame", CGRect,
      frame);
F(0, "setFrame:", void,
      setFrame_,
      CGRect);
F(0, "setFrame:display:animate:", void,
      setFrame_display_animate_,
      CGRect, bool, bool);
F(0, "setInitialFirstResponder:", void,
      setInitialFirstResponder_,
      __NSView*);
F(0, "setMinSize:", void,
      setMinSize_,
      CGSize);
F(0, "setTitle:", void,
      setTitle_,
      CFStringRef);
F(0, "setStringValue:", void,
      setStringValue_,
      CFStringRef);
F(0, "windowShouldClose:", bool,
      windowShouldClose_,
      void*);
F(0, "windowDidResize:", void,
      windowDidResize_,
      void*);
F(0, "makeKeyWindow", void,
      makeKeyWindow);
F(0, "orderFront:", void,
      orderFront_,
      void*);
F(0, "orderOut:", void,
      orderOut_,
      void*);
F(0, "setNeedsDisplay:", void,
      setNeedsDisplay_,
      bool);
F(0, "setDelegate:", void,
      setDelegate_,
      void*);
F(0, "setEnabled:", void,
      setEnabled_,
      bool);
F(0, "setNextKeyView:", void,
      setNextKeyView_,
      __NSView*);
F(0, "setDefaultButtonCell:", void,
      setDefaultButtonCell_,
      __NSButtonCell*);
F(0, "verticalScroller", __NSView*,
      verticalScroller);
F(0, "setHasVerticalScroller:", void,
      setHasVerticalScroller_,
      bool);
F(0, "isEnabled", bool,
      isEnabled);
F(0, "state", __NSInteger,
      state);
F(0, "setState:", void,
      setState_,
      __NSInteger);
F(0, "setToolTip:", void,
      setToolTip_,
      CFStringRef);
F(0, "setButtonType:", void,
      setButtonType_,
      __NSInteger);
F(0, "setBezelStyle:", void,
      setBezelStyle_,
      __NSInteger);
F(0, "setImagePosition:", void,
      setImagePosition_,
      __NSInteger);
F(0, "setSendsActionOnEndEditing:", void,
      setSendsActionOnEndEditing_,
      bool);
F(0, "control:textView:doCommandBySelector:", bool,
      control_textView_doCommandBySelector_,
      void*, __NSView*, SEL);
F(0, "moveDown:", void,
      moveDown_,
      void*);
F(0, "moveUp:", void,
      moveUp_,
      void*);
F(0, "setEditable:", void,
      setEditable_,
      bool);
F(0, "setSelectable:", void,
      setSelectable_,
      bool);
F(0, "setBezeled:", void,
      setBezeled_,
      bool);
F(0, "setBordered:", void,
      setBordered_,
      bool);
F(0, "setDrawsBackground:", void,
      setDrawsBackground_,
      bool);
F(0, "statusItemWithLength:", __NSStatusItem*,
      statusItemWithLength_,
      CGFloat);
F(0, "removeStatusItem:", void,
      removeStatusItem_,
      __NSStatusItem*);
F(0, "systemStatusBar", __NSStatusBar*,
      systemStatusBar);
F(0, "mainScreen", __NSScreen*,
      mainScreen);
F(1, "thickness", CGFloat,
      thickness);
F(2, "cellSize", CGPoint,
      cellSize);
F(0, "tag", __NSInteger,
      tag);
F(0, "setTag:", void,
      setTag_,
      __NSInteger);
F(0, "setHighlightMode:", void,
      setHighlightMode_,
      __NSInteger);
F(0, "setHidden:", void,
      setHidden_,
      bool);
F(0, "setContentView:", void,
      setContentView_,
      __NSView*);
F(0, "setDocumentView:", void,
      setDocumentView_,
      __NSView*);
F(0, "addSubview:", void,
      addSubview_,
      __NSView*);
F(0, "isFlipped", bool,
      isFlipped);
F(0, "drawRect:", void,
      drawRect_,
      CGRect);
F(2, "mouseLocation", CGPoint,
      mouseLocation);
F(0, "pressedMouseButtons", __NSInteger,
      pressedMouseButtons);
F(0, "setIgnoresMouseEvents:", void,
      setIgnoresMouseEvents_,
      bool);
F(0, "graphicsPort", CGContextRef,
      graphicsPort);
F(0, "currentContext", __NSGraphicsContext*,
      currentContext);
F(0, "defaultManager", __NSFileManager*,
      defaultManager);
F(0, "systemFontOfSize:", __NSFont*,
      systemFontOfSize_,
      CGFloat);
F(1, "systemFontSize", CGFloat,
      systemFontSize);
F(2, "maximumAdvancement", CGPoint,
      maximumAdvancement);
F(0, "setIndeterminate:", void,
      setIndeterminate_,
      bool);
F(0, "setAlignment:", void,
      setAlignment_,
      __NSInteger);
F(0, "addTableColumn:", void,
      addTableColumn_,
      __NSTableColumn*);
F(0, "headerCell", __NSCell*,
      headerCell);
F(0, "setWantsLayer:", void,
      setWantsLayer_,
      bool);
F(0, "scaleUnitSquareToSize:", void,
      scaleUnitSquareToSize_,
      CGSize);
F(0, "drawInRect:withAttributes:", void,
      drawInRect_withAttributes_,
      CGRect, CFDictionaryRef);
F(0, "textDidChange:", void,
      textDidChange_,
      void*);
F(0, "stringValue", CFStringRef,
      stringValue);
F(1, "doubleValue", double,
      doubleValue);
F(0, "setDoubleValue:", void,
      setDoubleValue_,
      double);
F(0, "setIntegerValue:", void,
      setIntegerValue_,
      __NSInteger);
F(0, "displayIfNeeded", void,
      displayIfNeeded);
F(0, "setMinValue:", void,
      setMinValue_,
      double);
F(0, "setMaxValue:", void,
      setMaxValue_,
      double);
F(0, "setValueWraps:", void,
      setValueWraps_,
      bool);
F(0, "setFormatter:", void,
      setFormatter_,
      __NSNumberFormatter*);
F(0, "setFormatterBehavior:", void,
      setFormatterBehavior_,
      __NSInteger);
F(0, "setNumberStyle:", void,
      setNumberStyle_,
      __NSInteger);
F(0, "setPartialStringValidationEnabled:", void,
      setPartialStringValidationEnabled_,
      bool);
F(0, "isPartialStringValid:newEditingString:errorDescription:", bool,
      isPartialStringValid_newEditingString_errorDescription_,
      CFStringRef, CFStringRef, CFStringRef);
F(0, "getObjectValue:forString:errorDescription:", bool,
      getObjectValue_forString_errorDescription_,
      void**, CFStringRef, CFStringRef);
F(0, "reloadData", void,
      reloadData);
F(0, "dataCell", void*,
      dataCell);
F(0, "setDataCell:", void,
      setDataCell_,
      __NSCell*);
F(0, "setDataSource:", void,
      setDataSource_,
      void*);
F(0, "setResizingMask:", void,
      setResizingMask_,
      __NSInteger);
F(0, "numberOfRowsInTableView:", __NSInteger,
      numberOfRowsInTableView_,
      __NSTableView*);
F(0, "tableView:objectValueForTableColumn:row:", void*,
      tableView_objectValueForTableColumn_row_,
      __NSTableView*, __NSTableColumn*, __NSInteger);
F(0, "tableView:setObjectValue:forTableColumn:row:", void,
      tableView_setObjectValue_forTableColumn_row_,
      __NSTableView*, void*, __NSTableColumn*, __NSInteger);
F(0, "tableView:dataCellForTableColumn:row:", __NSCell*,
      tableView_dataCellForTableColumn_row_,
      __NSTableView*, __NSTableColumn*, __NSInteger);
F(0, "tableView:viewForTableColumn:row:", __NSView*,
      tableView_viewForTableColumn_row_,
      __NSTableView*, __NSTableColumn*, __NSInteger);
F(0, "push", void,
      push);
F(0, "pop", void,
      pop);
F(0, "pointingHandCursor", __NSCursor*,
      pointingHandCursor);
F(0, "flushBuffer", void,
      flushBuffer);
F(0, "openGLContext", __NSOpenGLContext*,
      openGLContext);
F(0, "isOpaque", bool,
      isOpaque);
F(0, "setOpaque:", void,
      setOpaque_,
      bool);
F(0, "initWithAttributes:", __NSOpenGLPixelFormat*,
      initWithAttributes_,
      int*);
F(0, "initWithFrame:pixelFormat:", __NSOpenGLView*,
      initWithFrame_pixelFormat_,
      CGRect, __NSOpenGLPixelFormat*);
F(0, "makeCurrentContext", void,
      makeCurrentContext);
F(0, "setValues:forParameter:", void,
      setValues_forParameter_,
      int*, __NSInteger);
F(0, "setLevel:", void,
      setLevel_,
      __NSInteger);
F(0, "setHasShadow:", void,
      setHasShadow_,
      bool);
F(0, "clearColor", __NSColor*,
      clearColor);
F(0, "setBackgroundColor:", void,
      setBackgroundColor_,
      __NSColor*);
F(0, "postEvent:atStart:", void,
      postEvent_atStart_,
      __NSEvent*, bool);
F(0, "otherEventWithType:location:modifierFlags:timestamp:windowNumber:context:subtype:data1:data2:", __NSEvent*,
      otherEventWithType_location_modifierFlags_timestamp_windowNumber_context_subtype_data1_data2_,
      __NSInteger, CGPoint, __NSInteger, CGFloat, __NSInteger, __NSGraphicsContext*, short, __NSInteger, __NSInteger);

/// [TODO:] implement this:
/**
F(0, __NSEvent*,
     otherEventWithType, __NSInteger, location, CGPoint,
     modifierFlags, __NSInteger, timestamp, CGFloat,
     windowNumber, __NSInteger, context, __NSGraphicsContext*,
     subtype, short, data1, __NSInteger, data2, __NSInteger);
 **/

#undef L
#undef L4
#undef L3
#undef L2
#undef L1
#undef L0
#undef P1
#undef P0
#undef A1
#undef A0
#undef F
#undef T

extern void NSBeep();
extern void CGSSetConnectionProperty(int, int, CFStringRef, CFBooleanRef);
extern int _CGSDefaultConnection();

#endif
