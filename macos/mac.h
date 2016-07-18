#ifndef HDR_OBJC
#define HDR_OBJC

/// [IMPORTANT:] use CoreFoundation classes wherever possible!
/// There is no ARC in pure C, so the bridging will be direct!

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc-runtime.h>
#include <stdarg.h>



static struct {
    Class uuid;
    char *name;
    long icnt;
} *SubclassedObjCClasses = 0;
static id *LoadedObjCClasses = 0;
static SEL *LoadedObjCSelectors = 0;



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
#define OSX_10_05_PLUS (kCFCoreFoundationVersionNumber >=  476.00)
#define OSX_10_06_PLUS (kCFCoreFoundationVersionNumber >=  550.00)
#define OSX_10_07_PLUS (kCFCoreFoundationVersionNumber >=  635.00)
#define OSX_10_08_PLUS (kCFCoreFoundationVersionNumber >=  744.00)
#define OSX_10_09_PLUS (kCFCoreFoundationVersionNumber >=  855.11)
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



/// NSString creation macro
#define UTF8(s) CFStringCreateWithBytes(0, (s)? (uint8_t*)(s) : (uint8_t*)"", strlen((s)? (char*)(s) : (char*)""), kCFStringEncodingUTF8, false)

/// class instance variable management macros
#define GET_IVAR(inst, name, data) object_getInstanceVariable(inst, name, (void**)(data))
#define SET_IVAR(inst, name, data) object_setInstanceVariable(inst, name, (void*)(data))

/// getter macros for double floats and structures of double floats
#define GetT1DV(r, i, ...) { void *GetT1DV = objc_msgSend_fpret; r = ((typeof(r) (*)(id, ...))GetT1DV)(i, __VA_ARGS__); }
#define GetT2DV(r, i, ...) { void *GetT2DV = objc_msgSend;       r = ((typeof(r) (*)(id, ...))GetT2DV)(i, __VA_ARGS__); }
#define GetT4DV(r, i, ...) { void *GetT4DV = objc_msgSend_stret; r = ((typeof(r) (*)(id, ...))GetT4DV)(i, __VA_ARGS__); }



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
/// If there are none in CF, just import them via "extern id NS<Whatever>".
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



/// no __attribute__((unused)) here: this function has to be called ASAP
static void LoadObjC(char *clas[], char *sele[]) {
    long iter;

    if (!LoadedObjCClasses) {
        for (iter = 0; clas[iter]; iter++);
        LoadedObjCClasses = malloc(iter * sizeof(*LoadedObjCClasses));
        for (iter = 0; sele[iter]; iter++);
        LoadedObjCSelectors = malloc(iter * sizeof(*LoadedObjCSelectors));
        for (iter = 0; clas[iter]; iter++)
            LoadedObjCClasses[iter] = objc_getClass(clas[iter]);
        for (iter = 0; sele[iter]; iter++)
            LoadedObjCSelectors[iter] = sel_registerName(sele[iter]);
    }
}

#endif
