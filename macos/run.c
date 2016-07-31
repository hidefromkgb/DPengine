#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../exec/exec.h"
#include "mac.h"



#define STR_OBJC_CLAS             \
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
       "NSImage",                 \
       "NSMenu",                  \
       "NSMenuItem",              \
       "NSStatusBar",             \
       "NSScreen",                \
       "NSWindow",                \
       "NSTextField",             \
       "NSButtonCell",            \
       "NSButton",                \
       "NSProgressIndicator",     \
       "NSStepper",               \
       "NSView",                  \
       "NSScrollView",            \
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
#define NSImage                 (LoadedObjCClasses[10])
#define NSMenu                  (LoadedObjCClasses[11])
#define NSMenuItem              (LoadedObjCClasses[12])
#define NSStatusBar             (LoadedObjCClasses[13])
#define NSScreen                (LoadedObjCClasses[14])
#define NSWindow                (LoadedObjCClasses[15])
#define NSTextField             (LoadedObjCClasses[16])
#define NSButtonCell            (LoadedObjCClasses[17])
#define NSButton                (LoadedObjCClasses[18])
#define NSProgressIndicator     (LoadedObjCClasses[19])
#define NSStepper               (LoadedObjCClasses[20])
#define NSView                  (LoadedObjCClasses[21])
#define NSScrollView            (LoadedObjCClasses[22])
#define NSTableView             (LoadedObjCClasses[23])
#define NSTableColumn           (LoadedObjCClasses[24])

#define STR_OBJC_SELE                                            \
       "init",                                                   \
       "alloc",                                                  \
       "release",                                                \
       "retain",                                                 \
       "class",                                                  \
       "cell",                                                   \
       "button",                                                 \
       "_A",                                                     \
       "setAction:",                                             \
       "setTarget:",                                             \
       "setActivationPolicy:",                                   \
       "activateIgnoringOtherApps:",                             \
       "sharedApplication",                                      \
       "run",                                                    \
       "stop:",                                                  \
       "mainBundle",                                             \
       "bundlePath",                                             \
       "URLsForDirectory:inDomains:",                            \
       "separatorItem",                                          \
       "addItem:",                                               \
       "setAutoenablesItems:",                                   \
       "imageNamed:",                                            \
       "setImage:",                                              \
       "setOnStateImage:",                                       \
       "setSubmenu:",                                            \
       "popUpMenuPositioningItem:atLocation:inView:",            \
       "initWithCGImage:size:",                                  \
       "initWithContentRect:styleMask:backing:defer:",           \
       "initWithTitle:action:keyEquivalent:",                    \
       "contentRectForFrameRect:",                               \
       "frameRectForContentRect:",                               \
       "visibleFrame",                                           \
       "frame",                                                  \
       "setFrame:",                                              \
       "setFrame:display:animate:",                              \
       "setInitialFirstResponder:",                              \
       "setMinSize:",                                            \
       "setTitle:",                                              \
       "setStringValue:",                                        \
       "windowShouldClose:",                                     \
       "windowDidResize:",                                       \
       "makeKeyWindow",                                          \
       "orderFront:",                                            \
       "orderOut:",                                              \
       "setNeedsDisplay:",                                       \
       "setDelegate:",                                           \
       "setEnabled:",                                            \
       "setNextKeyView:",                                        \
       "setDefaultButtonCell:",                                  \
       "verticalScroller",                                       \
       "setHasVerticalScroller:",                                \
       "isEnabled",                                              \
       "state",                                                  \
       "setState:",                                              \
       "setToolTip:",                                            \
       "setButtonType:",                                         \
       "setBezelStyle:",                                         \
       "setImagePosition:",                                      \
       "setSendsActionOnEndEditing:",                            \
       "control:textView:doCommandBySelector:",                  \
       "moveDown:",                                              \
       "moveUp:",                                                \
       "setEditable:",                                           \
       "setSelectable:",                                         \
       "setBezeled:",                                            \
       "setBordered:",                                           \
       "setDrawsBackground:",                                    \
       "statusItemWithLength:",                                  \
       "removeStatusItem:",                                      \
       "systemStatusBar",                                        \
       "mainScreen",                                             \
       "thickness",                                              \
       "cellSize",                                               \
       "tag",                                                    \
       "setTag:",                                                \
       "setHighlightMode:",                                      \
       "setHidden:",                                             \
       "setContentView:",                                        \
       "setDocumentView:",                                       \
       "addSubview:",                                            \
       "isFlipped",                                              \
       "drawRect:",                                              \
       "mouseLocation",                                          \
       "graphicsPort",                                           \
       "currentContext",                                         \
       "defaultManager",                                         \
       "systemFontOfSize:",                                      \
       "systemFontSize",                                         \
       "maximumAdvancement",                                     \
       "setIndeterminate:",                                      \
       "setAlignment:",                                          \
       "addTableColumn:",                                        \
       "headerCell",                                             \
       "setWantsLayer:",                                         \
       "scaleUnitSquareToSize:",                                 \
       "drawInRect:withAttributes:",                             \
       "textDidChange:",                                         \
       "stringValue",                                            \
       "doubleValue",                                            \
       "setDoubleValue:",                                        \
       "setIntValue:",                                           \
       "displayIfNeeded",                                        \
       "setMinValue:",                                           \
       "setMaxValue:",                                           \
       "setValueWraps:",                                         \
       "setFormatter:",                                          \
       "setFormatterBehavior:",                                  \
       "setNumberStyle:",                                        \
       "setPartialStringValidationEnabled:",                     \
       "isPartialStringValid:newEditingString:errorDescription:",\
       "getObjectValue:forString:errorDescription:",             \
       "reloadData",                                             \
       "dataCell",                                               \
       "setDataCell:",                                           \
       "setDataSource:",                                         \
       "setResizingMask:",                                       \
       "numberOfRowsInTableView:",                               \
       "tableView:objectValueForTableColumn:row:",               \
       "tableView:setObjectValue:forTableColumn:row:",           \
       "tableView:dataCellForTableColumn:row:",                  \
       "tableView:viewForTableColumn:row:"

#define init(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[  0])
#define alloc(inst)                                                     objc_msgSend(inst, LoadedObjCSelectors[  1])
#define release(inst)                                                   objc_msgSend(inst, LoadedObjCSelectors[  2])
#define retain(inst)                                                    objc_msgSend(inst, LoadedObjCSelectors[  3])
#define class(inst)                                              (Class)objc_msgSend(inst, LoadedObjCSelectors[  4])
#define cell(inst)                                                      objc_msgSend(inst, LoadedObjCSelectors[  5])
#define button(inst)                                                    objc_msgSend(inst, LoadedObjCSelectors[  6])
#define ActionSelector                                                                     LoadedObjCSelectors[  7]
#define setAction_(inst, a)                                             objc_msgSend(inst, LoadedObjCSelectors[  8], a)
#define setTarget_(inst, t)                                             objc_msgSend(inst, LoadedObjCSelectors[  9], t)
#define setActivationPolicy_(inst, p)                                   objc_msgSend(inst, LoadedObjCSelectors[ 10], (unsigned)(p))
#define activateIgnoringOtherApps_(inst, b)                             objc_msgSend(inst, LoadedObjCSelectors[ 11], (bool)(b))
#define sharedApplication(inst)                                         objc_msgSend(inst, LoadedObjCSelectors[ 12])
#define run(inst)                                                       objc_msgSend(inst, LoadedObjCSelectors[ 13])
#define stop_(inst, s)                                                  objc_msgSend(inst, LoadedObjCSelectors[ 14], s)
#define mainBundle(inst)                                                objc_msgSend(inst, LoadedObjCSelectors[ 15])
#define bundlePath(inst)                                                objc_msgSend(inst, LoadedObjCSelectors[ 16])
#define URLsForDirectory_inDomains_(inst, u, d)                         objc_msgSend(inst, LoadedObjCSelectors[ 17], u, d)
#define separatorItem(inst)                                             objc_msgSend(inst, LoadedObjCSelectors[ 18])
#define addItem_(inst, i)                                               objc_msgSend(inst, LoadedObjCSelectors[ 19], i)
#define setAutoenablesItems_(inst, b)                                   objc_msgSend(inst, LoadedObjCSelectors[ 20], (bool)(b))
#define imageNamed_(inst, s)                                            objc_msgSend(inst, LoadedObjCSelectors[ 21], s)
#define setImage_(inst, i)                                              objc_msgSend(inst, LoadedObjCSelectors[ 22], i)
#define setOnStateImage_(inst, i)                                       objc_msgSend(inst, LoadedObjCSelectors[ 23], i)
#define setSubmenu_(inst, m)                                            objc_msgSend(inst, LoadedObjCSelectors[ 24], m)
#define popUpMenuPositioningItem_atLocation_inView_(inst, m, l, v)      objc_msgSend(inst, LoadedObjCSelectors[ 25], m, (CGPoint)(l), v)
#define initWithCGImage_size_(inst, i, s)                               objc_msgSend(inst, LoadedObjCSelectors[ 26], i, s)
#define initWithContentRect_styleMask_backing_defer_(inst, r, m, b, d)  objc_msgSend(inst, LoadedObjCSelectors[ 27], r, (unsigned)(m), (unsigned)(b), (bool)(d))
#define initWithTitle_action_keyEquivalent_(inst, t, a, k)              objc_msgSend(inst, LoadedObjCSelectors[ 28], t, a, k)
#define ContentRectForFrameRect_                                                           LoadedObjCSelectors[ 29]
#define FrameRectForContentRect_                                                           LoadedObjCSelectors[ 30]
#define VisibleFrame                                                                       LoadedObjCSelectors[ 31]
#define Frame                                                                              LoadedObjCSelectors[ 32]
#define setFrame_(inst, f)                                              objc_msgSend(inst, LoadedObjCSelectors[ 33], f)
#define setFrame_display_animate_(inst, f, d, a)                        objc_msgSend(inst, LoadedObjCSelectors[ 34], f, d, a)
#define setInitialFirstResponder_(inst, r)                              objc_msgSend(inst, LoadedObjCSelectors[ 35], r)
#define setMinSize_(inst, s)                                            objc_msgSend(inst, LoadedObjCSelectors[ 36], s)
#define setTitle_(inst, t)                                              objc_msgSend(inst, LoadedObjCSelectors[ 37], t)
#define setStringValue_(inst, s)                                        objc_msgSend(inst, LoadedObjCSelectors[ 38], s)
#define WindowShouldClose_                                                                 LoadedObjCSelectors[ 39]
#define WindowDidResize_                                                                   LoadedObjCSelectors[ 40]
#define makeKeyWindow(inst)                                             objc_msgSend(inst, LoadedObjCSelectors[ 41])
#define orderFront_(inst, w)                                            objc_msgSend(inst, LoadedObjCSelectors[ 42], w)
#define orderOut_(inst, w)                                              objc_msgSend(inst, LoadedObjCSelectors[ 43], w)
#define setNeedsDisplay_(inst, d)                                       objc_msgSend(inst, LoadedObjCSelectors[ 44], (bool)(d))
#define setDelegate_(inst, d)                                           objc_msgSend(inst, LoadedObjCSelectors[ 45], d)
#define setEnabled_(inst, b)                                            objc_msgSend(inst, LoadedObjCSelectors[ 46], (bool)(b))
#define setNextKeyView_(inst, v)                                        objc_msgSend(inst, LoadedObjCSelectors[ 47], v)
#define setDefaultButtonCell_(inst, c)                                  objc_msgSend(inst, LoadedObjCSelectors[ 48], c)
#define verticalScroller(inst)                                          objc_msgSend(inst, LoadedObjCSelectors[ 49])
#define setHasVerticalScroller_(inst, s)                                objc_msgSend(inst, LoadedObjCSelectors[ 50], s)
#define isEnabled(inst)                                           (bool)objc_msgSend(inst, LoadedObjCSelectors[ 51])
#define state(inst)                                               (long)objc_msgSend(inst, LoadedObjCSelectors[ 52])
#define setState_(inst, s)                                              objc_msgSend(inst, LoadedObjCSelectors[ 53], (int)(s))
#define setToolTip_(inst, t)                                            objc_msgSend(inst, LoadedObjCSelectors[ 54], t)
#define setButtonType_(inst, t)                                         objc_msgSend(inst, LoadedObjCSelectors[ 55], t)
#define setBezelStyle_(inst, s)                                         objc_msgSend(inst, LoadedObjCSelectors[ 56], s)
#define setImagePosition_(inst, p)                                      objc_msgSend(inst, LoadedObjCSelectors[ 57], p)
#define setSendsActionOnEndEditing_(inst, b)                            objc_msgSend(inst, LoadedObjCSelectors[ 58], (bool)(b))
#define Control_textView_doCommandBySelector_                                              LoadedObjCSelectors[ 59]
#define MoveDown_                                                                          LoadedObjCSelectors[ 60]
#define MoveUp_                                                                            LoadedObjCSelectors[ 61]
#define setEditable_(inst, e)                                           objc_msgSend(inst, LoadedObjCSelectors[ 62], e)
#define setSelectable_(inst, s)                                         objc_msgSend(inst, LoadedObjCSelectors[ 63], s)
#define setBezeled_(inst, b)                                            objc_msgSend(inst, LoadedObjCSelectors[ 64], b)
#define setBordered_(inst, b)                                           objc_msgSend(inst, LoadedObjCSelectors[ 65], b)
#define setDrawsBackground_(inst, d)                                    objc_msgSend(inst, LoadedObjCSelectors[ 66], d)
#define statusItemWithLength_(inst, l)                                  objc_msgSend(inst, LoadedObjCSelectors[ 67], PassT1FV(l))
#define removeStatusItem_(inst, i)                                      objc_msgSend(inst, LoadedObjCSelectors[ 68], i)
#define systemStatusBar(inst)                                           objc_msgSend(inst, LoadedObjCSelectors[ 69])
#define mainScreen(inst)                                                objc_msgSend(inst, LoadedObjCSelectors[ 70])
#define Thickness                                                                          LoadedObjCSelectors[ 71]
#define CellSize                                                                           LoadedObjCSelectors[ 72]
#define tag(inst)                                                       objc_msgSend(inst, LoadedObjCSelectors[ 73])
#define setTag_(inst, t)                                                objc_msgSend(inst, LoadedObjCSelectors[ 74], t)
#define setHighlightMode_(inst, b)                                      objc_msgSend(inst, LoadedObjCSelectors[ 75], (bool)(b))
#define setHidden_(inst, h)                                             objc_msgSend(inst, LoadedObjCSelectors[ 76], (bool)(h))
#define setContentView_(inst, v)                                        objc_msgSend(inst, LoadedObjCSelectors[ 77], v)
#define setDocumentView_(inst, v)                                       objc_msgSend(inst, LoadedObjCSelectors[ 78], v)
#define addSubview_(inst, v)                                            objc_msgSend(inst, LoadedObjCSelectors[ 79], v)
#define IsFlipped                                                                          LoadedObjCSelectors[ 80]
#define DrawRect_                                                                          LoadedObjCSelectors[ 81]
#define MouseLocation                                                                      LoadedObjCSelectors[ 82]
#define graphicsPort(inst)                                (CGContextRef)objc_msgSend(inst, LoadedObjCSelectors[ 83])
#define currentContext(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[ 84])
#define defaultManager(inst)                                            objc_msgSend(inst, LoadedObjCSelectors[ 85])
#define systemFontOfSize_(inst, s)                                      objc_msgSend(inst, LoadedObjCSelectors[ 86], PassT1FV(s))
#define SystemFontSize                                                                     LoadedObjCSelectors[ 87]
#define MaximumAdvancement                                                                 LoadedObjCSelectors[ 88]
#define setIndeterminate_(inst, i)                                      objc_msgSend(inst, LoadedObjCSelectors[ 89], (bool)(i))
#define setAlignment_(inst, a)                                          objc_msgSend(inst, LoadedObjCSelectors[ 90], (int)(a))
#define addTableColumn_(inst, c)                                        objc_msgSend(inst, LoadedObjCSelectors[ 91], (id)(c))
#define headerCell(inst)                                                objc_msgSend(inst, LoadedObjCSelectors[ 92])
#define setWantsLayer_(inst, w)                                         objc_msgSend(inst, LoadedObjCSelectors[ 93], (bool)(w))
#define scaleUnitSquareToSize_(inst, s)                                 objc_msgSend(inst, LoadedObjCSelectors[ 94], (CGSize)(s))
#define drawInRect_withAttributes_(inst, r, a)                          objc_msgSend(inst, LoadedObjCSelectors[ 95], (CGRect)(r), (id)(a))
#define TextDidChange_                                                                     LoadedObjCSelectors[ 96]
#define stringValue(inst)                                               objc_msgSend(inst, LoadedObjCSelectors[ 97])
#define DoubleValue                                                                        LoadedObjCSelectors[ 98]
#define setDoubleValue_(inst, v)                                        objc_msgSend(inst, LoadedObjCSelectors[ 99], (double)(v))
#define setIntValue_(inst, v)                                           objc_msgSend(inst, LoadedObjCSelectors[100], (NSInteger)(v))
#define displayIfNeeded(inst)                                           objc_msgSend(inst, LoadedObjCSelectors[101])
#define setMinValue_(inst, v)                                           objc_msgSend(inst, LoadedObjCSelectors[102], (double)(v))
#define setMaxValue_(inst, v)                                           objc_msgSend(inst, LoadedObjCSelectors[103], (double)(v))
#define setValueWraps_(inst, w)                                         objc_msgSend(inst, LoadedObjCSelectors[104], w)
#define setFormatter_(inst, f)                                          objc_msgSend(inst, LoadedObjCSelectors[105], f)
#define setFormatterBehavior_(inst, b)                                  objc_msgSend(inst, LoadedObjCSelectors[106], b)
#define setNumberStyle_(inst, s)                                        objc_msgSend(inst, LoadedObjCSelectors[107], s)
#define setPartialStringValidationEnabled(inst, b)                      objc_msgSend(inst, LoadedObjCSelectors[108], b)
#define IsPartialStringValid_newEditingString_errorDescription_                            LoadedObjCSelectors[109]
#define getObjectValue_forString_errorDescription_(inst, v, s, e) (bool)objc_msgSend(inst, LoadedObjCSelectors[110], v, s, e)
#define reloadData(inst)                                                objc_msgSend(inst, LoadedObjCSelectors[111])
#define dataCell(inst)                                                  objc_msgSend(inst, LoadedObjCSelectors[112])
#define setDataCell_(inst, c)                                           objc_msgSend(inst, LoadedObjCSelectors[113], c)
#define setDataSource_(inst, d)                                         objc_msgSend(inst, LoadedObjCSelectors[114], d)
#define setResizingMask_(inst, m)                                       objc_msgSend(inst, LoadedObjCSelectors[115], m)
#define NumberOfRowsInTableView_                                                           LoadedObjCSelectors[116]
#define TableView_objectValueForTableColumn_row_                                           LoadedObjCSelectors[117]
#define TableView_setObjectValue_forTableColumn_row_                                       LoadedObjCSelectors[118]
#define TableView_dataCellForTableColumn_row_                                              LoadedObjCSelectors[119]
#define TableView_viewForTableColumn_row_                                                  LoadedObjCSelectors[120]

/// name of the menu responder class
#define CLS_MENU "lNSM"
/// name of the instance variable to access the CTRL structure
#define VAR_CTRL "ctrl"
/// name of the instance variable to access the associated data
#define VAR_DATA "data"



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



/// subclass storage
typedef union {
    struct {
        id wndw,
           text,
           butn, /// +cbox/rbox
           spin,
           list,
           pbar,
           sbox,
           ibox,
           frmt; /// number formatter
    };
    id _sub[9];
} SCLS;



/// NAME holds the selector associated with this function
void OnMenu(id this, SEL name, id menu) {
    eProcessMenuItem((MENU*)tag(menu));
}



id Submenu(MENU *menu, id base) {
    if (!menu)
        return 0;

    id cbtn, rbtn, item, retn = init(alloc(NSMenu));
    CFStringRef text, null = UTF8(0);

    cbtn = imageNamed_(NSImage, text = UTF8("NSMenuCheckmark"));
    CFRelease(text);
    rbtn = imageNamed_(NSImage, text = UTF8("NSMenuRadio"));
    CFRelease(text);
    setAutoenablesItems_(retn, false);
    while (menu->text) {
        if (!*menu->text) {
            item = separatorItem(NSMenuItem);
            addItem_(retn, item);
        }
        else {
            item = initWithTitle_action_keyEquivalent_
                       (alloc(NSMenuItem),
                        text = UTF8(menu->text), ActionSelector, null);
            CFRelease(text);
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
        }
        menu++;
    }
    CFRelease(null);
    return retn;
}



void rOpenContextMenu(MENU *tmpl) {
    id menu, base;
    CGPoint dptr;

    /// getting the pre-allocated menu responder class
    menu = NewClass(0, CLS_MENU, 0, 0);
    base = init(alloc(menu));
    DelClass((Class)menu);

    menu = Submenu(tmpl, base);
    GetT2DV(dptr, NSEvent, MouseLocation);
    popUpMenuPositioningItem_atLocation_inView_(menu, nil, dptr, nil);
    release(menu);
    release(base);
}



inline MENU *rOSSpecificMenu(ENGC *engc) {
    return 0;
}



char *rConvertUTF8(char *utf8) {
    return strdup(utf8);
}



void *MsgFunc(void *data) {
    intptr_t *user = (intptr_t*)data;
    CFOptionFlags retn = 0;

    CFUserNotificationDisplayAlert
        (0, kCFUserNotificationNoteAlertLevel, 0, 0, 0,
        (CFStringRef)user[1], (CFStringRef)user[2], 0, 0, 0, &retn);
    CFRelease((CFStringRef)user[1]);
    CFRelease((CFStringRef)user[2]);
    free(user);
    return 0;
}

long rMessage(char *text, char *head, uint32_t flgs) {
    pthread_t pthr;
    intptr_t *user;

    user = calloc(3, sizeof(*user));
    user[0] = flgs;
    user[1] = (intptr_t)UTF8(head);
    user[2] = (intptr_t)UTF8(text);
    pthread_create(&pthr, 0, MsgFunc, user);
    return 0;
}



void OnTray(id this) {
    MENU *mctx;

    GET_IVAR(this, VAR_DATA, &mctx);
    rOpenContextMenu(mctx);
}



intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim) {
    CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctxt = CGBitmapContextCreate
        (data, xdim, ydim, CHAR_BIT, xdim * sizeof(*data), drgb,
         kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGImageRef iref = CGBitmapContextCreateImage(ctxt);
    CFStringRef capt;

    id ibtn, pict, *vfld, *vmet, *retn = malloc(3 * sizeof(*retn));

    pict = initWithCGImage_size_(alloc(NSImage), iref, ((CGPoint){}));
    retn[0] = statusItemWithLength_(systemStatusBar(NSStatusBar),
                                    NSVariableStatusItemLength);
    retain(retn[0]);
    retn[1] = NewClass(NSObject, "lNST", vfld = PutToArr(VAR_CTRL, VAR_DATA),
                       vmet = PutToArr(ActionSelector, OnTray));
    retn[2] = init(alloc(retn[1]));
    SET_IVAR(retn[2], VAR_CTRL, retn);
    SET_IVAR(retn[2], VAR_DATA, mctx);
    free(vmet);
    free(vfld);

    if (OSX_10_10_PLUS)
        ibtn = button(retn[0]); /// Yosemite or newer, so we are using buttons
    else
        setHighlightMode_(ibtn = retn[0], true);
    setImage_(ibtn, pict);
    setTarget_(ibtn, retn[2]);
    setAction_(ibtn, ActionSelector);
    setToolTip_(retn[0], capt = UTF8(text));
    CFRelease(capt);
    release(pict);
    CGImageRelease(iref);
    CGContextRelease(ctxt);
    CGColorSpaceRelease(drgb);
    return (intptr_t)retn;
}



void rFreeTrayIcon(intptr_t icon) {
    id *retn = (id*)icon;

    removeStatusItem_(systemStatusBar(NSStatusBar), retn[0]);
    release(retn[0]);
    release(retn[2]);
    DelClass((Class)retn[1]);
    free(retn);
}



char *rLoadFile(char *name, long *size) {
    char *retn = 0;
    long file, flen;

    if ((file = open(name, O_RDONLY)) > 0) {
        flen = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        retn = malloc(flen + 1);
        read(file, retn, flen);
        retn[flen] = '\0';
        close(file);
        if (size)
            *size = flen;
    }
    return retn;
}



long rSaveFile(char *name, char *data, long size) {
    long file;

    if ((file = open(name, O_CREAT | O_WRONLY, 0644)) > 0) {
        size = write(file, data, size);
        close(file);
        return size;
    }
    return 0;
}



void TmrFunc(CFRunLoopTimerRef tmrp, void *user) {
    intptr_t *data = (intptr_t*)user;
    struct timeval spec = {};
    uint64_t time;

    gettimeofday(&spec, 0);
    time = spec.tv_sec * 1000 + spec.tv_usec / 1000;
    ((UPRE)data[0])((ENGC*)data[1], data[2], time);
}



void rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                       ENGC *engc, intptr_t data) {
    intptr_t user[3] = {(intptr_t)upre, (intptr_t)engc, data};
    CFRunLoopTimerContext ctxt = {0, (void*)user};
    CFRunLoopTimerRef tmrp =
        CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent(),
                             0.001 * fram, 0, 0, TmrFunc, &ctxt);

    CFRunLoopAddTimer(CFRunLoopGetCurrent(), tmrp, kCFRunLoopCommonModes);
    while (root->priv[2])
        run(sharedApplication(NSApplication));
    CFRunLoopTimerInvalidate(tmrp);
}



void MoveControl(CTRL *ctrl, intptr_t data) {
    long xpos = (int16_t)data, ypos = (int32_t)data >> 16;
    CTRL *root = ctrl;
    CGRect rect;

    while (root->prev)
        root = root->prev;
    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    rect.origin.x = (xpos < 0)? -xpos : xpos * (uint16_t)(root->priv[2]      );
    rect.origin.y = (ypos < 0)? -ypos : ypos * (uint16_t)(root->priv[2] >> 16);
    setFrame_((id)ctrl->priv[0], rect);
}

bool OnValidate(id this, SEL name, id part, id retn, id desc) {
    extern void NSBeep();
    id temp;

    if (getObjectValue_forString_errorDescription_(this, &temp, part, desc))
        return true;
    NSBeep();
    return false;
}

void OnSpin(id this) {
    double retn = 0.0;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT1DV(retn, (id)ctrl->priv[6], DoubleValue);
    setIntValue_((id)ctrl->priv[7], retn);
    ctrl->fc2e(ctrl, MSG_NSET, retn);
}

void OnEdit(id this) {
    CTRL *ctrl = 0;
    double retn;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
        return;

    GetT1DV(retn, (id)ctrl->priv[7], DoubleValue);
    ctrl->fe2c(ctrl, MSG_NSET, retn);
}

bool OnKeys(id this, SEL name, id ctrl, id view, SEL what) {
    if ((what == MoveDown_) || (what == MoveUp_)) {
        CTRL *ctrl = 0;
        double retn;

        GET_IVAR(this, VAR_CTRL, &ctrl);
        if (!ctrl || ((ctrl->flgs & FCT_TTTT) != FCT_SPIN))
            return false;

        GetT1DV(retn, (id)ctrl->priv[7], DoubleValue);
        ctrl->fe2c(ctrl, MSG_NSET, retn + ((what == MoveUp_)? 1.0 : -1.0));
        return true;
    }
    return false;
}



/// PRIV:
///  0: NSWindow
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (fontmul.x) | (fontmul.y << 16), and also a halt flag if 0
///  3: first tab-enabled control
///  4: last tab-enabled control
///  5: NSNumberFormatter for spin controls
///  6: SCLS subclass storage
///  7: NSView, the main container and delegate
intptr_t FE2CW(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__SHW: {
            id thrd = sharedApplication(NSApplication);

            if (!data)
                orderOut_((id)ctrl->priv[0], thrd);
            else {
                activateIgnoringOtherApps_
                    (sharedApplication(NSApplication), true);
                orderFront_((id)ctrl->priv[0], thrd);
            }
            break;
        }
        case MSG_WSZC: {
            CGRect area, scrn;

            scrn.size.width  = (((uint16_t)(data      )) + ctrl->xdim)
                             *   (uint16_t)(ctrl->priv[2]      );
            scrn.size.height = (((uint16_t)(data >> 16)) + ctrl->ydim)
                             *   (uint16_t)(ctrl->priv[2] >> 16);
            ctrl->priv[1] =  (uint16_t)scrn.size.width
                          | ((uint32_t)scrn.size.height << 16);
            GetT4DV(area, (id)ctrl->priv[0], FrameRectForContentRect_, scrn);
            GetT4DV(scrn, mainScreen(NSScreen), VisibleFrame);
            area.origin.x = 0.5 * (scrn.size.width  - area.size.width )
                          + scrn.origin.x;
            area.origin.y = 0.5 * (scrn.size.height - area.size.height)
                          + scrn.origin.y;
            setFrame_display_animate_((id)ctrl->priv[0], area, true, false);
            setMinSize_((id)ctrl->priv[0], area.size);
            orderFront_((id)ctrl->priv[0], sharedApplication(NSApplication));
            break;
        }
    }
    return 0;
}



/// PRIV:
///  0: NSProgressIndicator
///  1: progress position
///  2: progress maximum
///  3: CFStringRef with label text
///  4: vertical offset of the label
///  5: CFDictionaryRef with centering attributes and font
///  6:
///  7:
intptr_t FE2CP(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_PPOS:
            setDoubleValue_((id)ctrl->priv[0], ctrl->priv[1] = data);
            setNeedsDisplay_((id)ctrl->priv[0], true);
            displayIfNeeded((id)ctrl->priv[0]);
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
            break;

        case MSG_PTXT:
            if (ctrl->priv[3])
                CFRelease((void*)ctrl->priv[3]);
            ctrl->priv[3] = (intptr_t)UTF8(data);
            break;

        case MSG_PLIM:
            ctrl->priv[2] = data;
            setMaxValue_((id)ctrl->priv[0],
                         (ctrl->priv[2] > 0)? ctrl->priv[2] : 1);
            break;

        case MSG_PGET:
            return (data)? ctrl->priv[2] : ctrl->priv[1];
    }
    return 0;
}



/// PRIV:
///  0: NSButton
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CX(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((id)ctrl->priv[0], !!data);
            break;

        case MSG_BGST:
            return ((isEnabled((id)ctrl->priv[0]))? FCS_ENBL : 0)
                 | ((state((id)ctrl->priv[0]) == NSOnState)? FCS_MARK : 0);

        case MSG_BCLK:
            cmsg = (state((id)ctrl->priv[0]) == NSOnState);
            setState_((id)ctrl->priv[0], (data)? NSOnState : NSOffState);
            return !!cmsg;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1: NSButton subclass
///  2: either a data cell (< 10.7) or an array of NSButtons (>= 10.7)
///  3:
///  4: number of rows
///  5: array of CFStringRefs (item captions)
///  6: NSTableColumn
///  7: NSTableView
intptr_t FE2CL(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    CFStringRef capt;

    switch (cmsg) {
        case MSG__ENB:
            setEnabled_((id)ctrl->priv[7], !!data);
            if (OSX_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setEnabled_(((id*)ctrl->priv[2])[cmsg], !!data);
            break;

        case MSG_LCOL:
            setStringValue_(headerCell((id)ctrl->priv[6]), capt = UTF8(data));
            CFRelease(capt);
            if (OSX_10_07_PLUS)
                for (cmsg = 0; cmsg < ctrl->priv[4]; cmsg++)
                    setState_(((id*)ctrl->priv[2])[cmsg],
                             (ctrl->fc2e(ctrl, MSG_LGST, cmsg))?
                              NSOnState : NSOffState);
            reloadData((id)ctrl->priv[7]);
            break;

        case MSG_LADD:
            ctrl->priv[5] = (intptr_t)realloc((CFStringRef*)ctrl->priv[5],
                                             ++ctrl->priv[4] * sizeof(id*));
            ((CFStringRef*)ctrl->priv[5])[ctrl->priv[4] - 1] = UTF8(data);
            if (OSX_10_07_PLUS) {
                id elem;

                ctrl->priv[2] = (intptr_t)realloc((id*)ctrl->priv[2],
                                                  ctrl->priv[4] * sizeof(id*));
                elem = init(alloc((id)ctrl->priv[1]));
                SET_IVAR(elem, VAR_CTRL, ctrl);
                SET_IVAR(elem, VAR_DATA, ctrl->priv[4] - 1);
                setTarget_(elem, elem);
                setAction_(elem, ActionSelector);
                setButtonType_(elem, NSSwitchButton);
                ((id*)ctrl->priv[2])[ctrl->priv[4] - 1] = elem;
            }
            else if (!ctrl->priv[2]) {
                ctrl->priv[2] = (intptr_t)init(alloc((id)ctrl->priv[1]));
                setButtonType_((id)ctrl->priv[2], NSSwitchButton);
                setDataCell_((id)ctrl->priv[6], (id)ctrl->priv[2]);
            }
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSView
///  1: minimum value
///  2: maximum value
///  3: common NSNumberFormatter
///  4:
///  5:
///  6: NSStepper
///  7: NSTextField
intptr_t FE2CN(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            CGRect rect;

            GetT4DV(rect, (id)ctrl->priv[0], Frame);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;

        case MSG__ENB:
            setEnabled_((id)ctrl->priv[6], !!data);
            setEnabled_((id)ctrl->priv[7], !!data);
            break;

        case MSG_NGET: {
            CGFloat retn = 0;

            GetT1DV(retn, (id)ctrl->priv[6], DoubleValue);
            return retn;
        }
        case MSG_NSET:
            data = (data > ctrl->priv[1])? data : ctrl->priv[1];
            data = (data < ctrl->priv[2])? data : ctrl->priv[2];
            setDoubleValue_((id)ctrl->priv[6], data);
            OnSpin((id)ctrl->priv[6]);
            break;

        case MSG_NDIM:
            ctrl->priv[1] = -(uint16_t)data;
            ctrl->priv[2] = (uint16_t)(data >> 16);
            setMinValue_((id)ctrl->priv[6], ctrl->priv[1]);
            setMaxValue_((id)ctrl->priv[6], ctrl->priv[2]);
            setDoubleValue_((id)ctrl->priv[6], 0);
            OnSpin((id)ctrl->priv[6]);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSTextField
///  1:
///  2:
///  3:
///  4:
///  5:
///  6:
///  7:
intptr_t FE2CT(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__GSZ: {
            CGRect rect;

            GetT4DV(rect, (id)ctrl->priv[0], Frame);
            return (uint16_t)rect.size.width
                | ((uint32_t)rect.size.height << 16);
        }
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSScrollView
///  1: (wndsize.x) | (wndsize.y << 16)
///  2: (wndmove.x) | (wndmove.y << 16)
///  3:
///  4:
///  5:
///  6:
///  7: NSView, the sizeable container
intptr_t FE2CS(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG_WSZC: {
            CGRect rect;

            if ((ctrl->prev->flgs & FCT_TTTT) != FCT_WNDW)
                return -1;

            rect.origin.x = (uint16_t)(ctrl->priv[2]      );
            rect.origin.y = (uint16_t)(ctrl->priv[2] >> 16);
            if (!data) {
                rect.size.width  = (uint16_t)(ctrl->priv[1]      );
                rect.size.height = (uint16_t)(ctrl->priv[1] >> 16);
            }
            else {
                rect.size.width  = (uint16_t)(data      ) - rect.origin.x
                                 - ctrl->prev->xdim
                                 * (uint16_t)(ctrl->prev->priv[2]      );
                rect.size.height = (uint16_t)(data >> 16) - rect.origin.y
                                 - ctrl->prev->ydim
                                 * (uint16_t)(ctrl->prev->priv[2] >> 16);
                ctrl->priv[1] =  (uint16_t)rect.size.width
                              | ((uint32_t)rect.size.height << 16);
            }
            setFrame_((id)ctrl->priv[0], rect);
            if (ctrl->fc2e) {
                data = rect.size.height;
                GetT4DV(rect, verticalScroller((id)ctrl->priv[0]), Frame);
                cmsg = ctrl->priv[1] - rect.size.width;
                data += ctrl->fc2e(ctrl, MSG_SMAX, cmsg);
                setFrame_((id)ctrl->priv[7],
                         ((CGRect){{0, 0}, {(uint16_t)cmsg, data}}));
            }
            setNeedsDisplay_((id)ctrl->priv[0], true);
            break;
        }
        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;
    }
    return 0;
}



/// PRIV:
///  0: NSView
///  1: CGContextRef
///  2: data array
///  3: (xdim) | (ydim << 16)
///  4:
///  5:
///  6:
///  7: (animation ID << 10) | (current frame)
intptr_t FE2CI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (cmsg) {
        case MSG__POS:
            MoveControl(ctrl, data);
            break;

        case MSG__SHW:
            setHidden_((id)ctrl->priv[0], !data);
            break;

        case MSG_IFRM:
            ctrl->priv[7] = data;
            setNeedsDisplay_((id)ctrl->priv[0], true);
            break;
    }
    return 0;
}



void rFreeControl(CTRL *ctrl) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_WNDW: {
            SCLS *scls = (SCLS*)ctrl->priv[6];
            long iter;

            /// releasing the formatter
            release((id)ctrl->priv[5]);
            /// releasing classes
            for (iter = sizeof(scls->_sub) / sizeof(*scls->_sub) - 1;
                 iter >= 0; iter--)
                DelClass((Class)scls->_sub[iter]);
            free(scls);
            /// we must release neither the window nor its NSView;
            /// if we do, the program segfaults, don`t know why exactly
            return;
        }
        case FCT_TEXT:
            break;

        case FCT_BUTN:
            break;

        case FCT_CBOX:
            break;

        case FCT_RBOX:
            break;

        case FCT_SPIN:
            release((id)ctrl->priv[6]); /// releasing NSStepper
            release((id)ctrl->priv[7]); /// releasing NSTextField
            /// do not release the formatter, it is common between all spins
            break;

        case FCT_LIST:
            /// releasing cells/strings and deleting their common subclass
            if (!OSX_10_07_PLUS) {
                for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--)
                    CFRelease(((CFStringRef*)ctrl->priv[5])[ctrl->priv[4]]);
                release((id)ctrl->priv[2]);
            }
            else {
                for (--ctrl->priv[4]; ctrl->priv[4] >= 0; ctrl->priv[4]--) {
                    release(((id*)ctrl->priv[2])[ctrl->priv[4]]);
                    CFRelease(((CFStringRef*)ctrl->priv[5])[ctrl->priv[4]]);
                }
                free((id*)ctrl->priv[2]);
            }
            DelClass((Class)ctrl->priv[1]);
            free((CFStringRef*)ctrl->priv[5]);
            release((id)ctrl->priv[6]); /// releasing NSTableColumn
            release((id)ctrl->priv[7]); /// releasing NSTableView
            break;

        case FCT_SBOX:
            release((id)ctrl->priv[7]); /// releasing NSView
            break;

        case FCT_IBOX:
            /// releasing the image context and freeing its backbuffer
            CGContextRelease((CGContextRef)ctrl->priv[1]);
            free((void*)ctrl->priv[2]);
            break;

        case FCT_PBAR:
            CFRelease((void*)ctrl->priv[5]);     /// releasing text attributes
            if (ctrl->priv[3])
                CFRelease((void*)ctrl->priv[3]); /// freeing the text string
            break;
    }
    release((id)ctrl->priv[0]);
}



NSInteger OnRows(id this, SEL name, id view) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    return ctrl->priv[4];
}

id OnValueOld(id this, SEL name, id view, id icol, NSInteger irow) {
    return (OSX_10_07_PLUS)? 0 : dataCell(icol);
}

id OnValue(id this, SEL name, id view, id icol, NSInteger irow) {
    CTRL *ctrl = 0;
    id cell;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    cell = (OSX_10_07_PLUS)? ((id*)ctrl->priv[2])[irow] : dataCell(icol);
    setTitle_(cell, ((CFStringRef*)ctrl->priv[5])[irow]);
    setState_(cell, (ctrl->fc2e(ctrl, MSG_LGST, irow))?
                     NSOnState : NSOffState);
    return cell;
}

void OnReset(id this, SEL name, id view, id what, id icol, NSInteger irow) {
    CTRL *ctrl = 0;

    GET_IVAR(view, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_LSST,
              (irow << 1) | (ctrl->fc2e(ctrl, MSG_LGST, irow) ^ 1));
}

void OnListButton(id this) {
    CTRL *ctrl = 0;
    intptr_t irow;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    GET_IVAR(this, VAR_DATA, &irow);
    OnReset(0, 0, this, 0, 0, irow);
}

void OnButton(id this) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->fc2e(ctrl, MSG_BCLK, ((ctrl->flgs & FCT_TTTT) != FCT_BUTN)?
                                (state((id)ctrl->priv[0]) == NSOnState) : 0);
}

void PBoxDraw(id this, SEL name, CGRect rect) {
    struct objc_super prev = {this, class(NSProgressIndicator)};
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    objc_msgSendSuper(&prev, DrawRect_, rect);
    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    rect.origin.y = ctrl->priv[4];
    rect.origin.x = 0;
    if (OSX_10_10_PLUS) {
        rect.origin.y *= 0.25;
        rect.size.width *= 0.5;
        rect.size.height *= 0.5;
    }
    drawInRect_withAttributes_((id)ctrl->priv[3], rect, (id)ctrl->priv[5]);
}

void IBoxDraw(id this, SEL name, CGRect rect) {
    CGImageRef pict;
    CGRect area;
    AINF anim;
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;
    anim = (AINF){(ctrl->priv[7] >> 10) & 0x3FFFFF,
                  (int16_t)ctrl->priv[3], (int32_t)ctrl->priv[3] >> 16,
                   ctrl->priv[7] & 0x3FF, (uint32_t*)ctrl->priv[2]};
    if (!anim.uuid)
        return;
    area = (CGRect){{0, 0}, {anim.xdim, anim.ydim}};
    CGContextFillRect((CGContextRef)ctrl->priv[1], area);
    CGContextFlush((CGContextRef)ctrl->priv[1]);
    ctrl->fc2e(ctrl, MSG_IFRM, (intptr_t)&anim);
    pict = CGBitmapContextCreateImage((CGContextRef)ctrl->priv[1]);
    CGContextDrawImage(graphicsPort(currentContext(NSGraphicsContext)),
                       area, pict);
    CGImageRelease(pict);
}

bool OnFalse() {
    return false;
}

bool OnTrue() {
    return true;
}

bool OnClose(id this) {
    CTRL *ctrl = 0;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    ctrl->priv[2] = 0; /// requesting a halt, or just crashing if CTRL is 0
                       /// (the program is stopping anyway)
    stop_(sharedApplication(NSApplication), this);
    return true;
}

void OnSize(id this) {
    CTRL *ctrl = 0;
    CGRect rect;

    GET_IVAR(this, VAR_CTRL, &ctrl);
    if (!ctrl)
        return;

    GetT4DV(rect, (id)ctrl->priv[0], Frame);
    GetT4DV(rect, (id)ctrl->priv[0], ContentRectForFrameRect_, rect);
    ctrl->priv[1] =  (uint16_t)rect.size.width
                  | ((uint32_t)rect.size.height << 16);
    ctrl->fc2e(ctrl, MSG_WSZC, ctrl->priv[1]);
}

void rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text) {
    #define CLS_MAKE(r, p, n, f, ...) \
        { id *CLS_MAKE = PutToArr(__VA_ARGS__); \
          r = NewClass(p, n, f, CLS_MAKE); free(CLS_MAKE); }
    CTRL *root;
    SCLS *scls;
    CFStringRef capt;
    CGRect dims;
    id gwnd;

    gwnd = 0;
    root = ctrl->prev;
    if ((ctrl->flgs & FCT_TTTT) == FCT_WNDW) {
        id *vfld, thrd;
        CGPoint fadv;
        CGFloat ffsz;

        ctrl->fe2c = FE2CW;
        GetT1DV(ffsz, NSFont, SystemFontSize);
        gwnd = systemFontOfSize_(NSFont, ffsz);
        GetT2DV(fadv, gwnd, MaximumAdvancement);
        ctrl->priv[2] =  (uint16_t)round(0.45 * fadv.x)
                      | ((uint32_t)round(0.60 * ffsz) << 16);

        vfld = PutToArr(VAR_CTRL);
        ctrl->priv[6] = (intptr_t)(scls = calloc(1, sizeof(*scls)));
        CLS_MAKE(scls->wndw, NSView,              "rNSW", vfld,
                 WindowShouldClose_, OnClose,
                 WindowDidResize_, OnSize, IsFlipped, OnTrue);
        CLS_MAKE(scls->text, NSTextField,         "rNST", vfld,
                 ActionSelector, OnEdit,
                 Control_textView_doCommandBySelector_, OnKeys);
        CLS_MAKE(scls->butn, NSButton,            "rNSB", vfld,
                 ActionSelector, OnButton);
        CLS_MAKE(scls->spin, NSStepper,           "rNSN", vfld,
                 ActionSelector, OnSpin);
        CLS_MAKE(scls->list, NSTableView,         "rNSL", vfld,
                (OSX_10_07_PLUS)? TableView_viewForTableColumn_row_
               : TableView_dataCellForTableColumn_row_,        OnValue,
                 TableView_objectValueForTableColumn_row_,     OnValueOld,
                 TableView_setObjectValue_forTableColumn_row_, OnReset,
                 NumberOfRowsInTableView_,                     OnRows);
        CLS_MAKE(scls->pbar, NSProgressIndicator, "rNSP", vfld,
                 DrawRect_, PBoxDraw);
        CLS_MAKE(scls->sbox, NSView,              "rNSS", vfld,
                 IsFlipped, OnTrue);
        CLS_MAKE(scls->ibox, NSView,              "rNSI", vfld,
                 DrawRect_, IBoxDraw);
        CLS_MAKE(scls->frmt, NSNumberFormatter,   "rNSF", vfld,
                 IsPartialStringValid_newEditingString_errorDescription_,
                 OnValidate);
        free(vfld);

        ctrl->priv[5] = (intptr_t)init(alloc(scls->frmt));
        setFormatterBehavior_((id)ctrl->priv[5],
                               NSNumberFormatterBehavior10_4);
        setNumberStyle_((id)ctrl->priv[5], kCFNumberFormatterNoStyle);
        setPartialStringValidationEnabled((id)ctrl->priv[5], true);

        dims = (CGRect){};
        gwnd = initWithContentRect_styleMask_backing_defer_
                   (alloc(NSWindow), dims, NSTitledWindowMask
                                         | NSClosableWindowMask
                                         | NSResizableWindowMask
                                         | NSMiniaturizableWindowMask,
                    kCGBackingStoreBuffered, false);

        ctrl->priv[7] = (intptr_t)init(alloc(scls->wndw));
        setContentView_(gwnd, (id)ctrl->priv[7]);
        setDelegate_(gwnd, (id)ctrl->priv[7]);
        setTitle_(gwnd, capt = UTF8(text));
        CFRelease(capt);
        SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
        makeKeyWindow(gwnd);
        thrd = sharedApplication(NSApplication);
        activateIgnoringOtherApps_(thrd, true);
        orderFront_(gwnd, thrd);
    }
    else if (root) {
        long xspc, yspc, xpos, ypos, xdim, ydim;

        while (root->prev)
            root = root->prev;
        xspc = (uint16_t)(root->priv[2]);
        yspc = (uint16_t)(root->priv[2] >> 16);
        xpos =  ctrl->xpos + ((xoff && (ctrl->flgs & FCP_HORZ))?
                              *xoff : root->xpos);
        ypos =  ctrl->ypos + ((yoff && (ctrl->flgs & FCP_VERT))?
                              *yoff : root->ypos);
        xdim = (ctrl->xdim < 0)? -ctrl->xdim : ctrl->xdim * xspc;
        ydim = (ctrl->ydim < 0)? -ctrl->ydim : ctrl->ydim * yspc;
        if (xoff)
            *xoff = xpos
                  + ((ctrl->xdim < 0)? 1 - ctrl->xdim / xspc : ctrl->xdim);
        if (yoff)
            *yoff = ypos
                  + ((ctrl->ydim < 0)? 1 - ctrl->ydim / yspc : ctrl->ydim);
        dims = (CGRect){{xpos * xspc, ypos * yspc}, {xdim, ydim}};
        scls = (SCLS*)root->priv[6];

        switch (ctrl->flgs & FCT_TTTT) {
            case FCT_TEXT:
                ctrl->fe2c = FE2CT;
                gwnd = init(alloc(scls->text));
                SET_IVAR(gwnd, VAR_CTRL, 0);
                setStringValue_(gwnd, capt = UTF8(text));
                setAlignment_(gwnd, (ctrl->flgs & FST_CNTR)?
                                     NSCenterTextAlignment :
                                     NSLeftTextAlignment);
                setDrawsBackground_(gwnd, false);
                setSelectable_(gwnd, false);
                setEditable_(gwnd, false);
                setBordered_(gwnd, false);
                setBezeled_(gwnd, false);
                CFRelease(capt);
                break;

            case FCT_BUTN: {
                char *temp;
                long size;

                /// a very simple hack to force multiline text wrapping
                temp = malloc(size = strlen(text) + 3);
                temp[0] = '\n';
                temp[1] = '\0';
                strcat(temp, text);
                temp[size - 2] = '\n';
                temp[size - 1] = '\0';

                gwnd = init(alloc(scls->butn));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                SET_IVAR(gwnd, VAR_DATA, nil);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ActionSelector);
                setButtonType_(gwnd, NSMomentaryLightButton);
                setBezelStyle_(gwnd, NSSmallSquareBezelStyle);
                if ((ctrl->flgs & FSB_DFLT) && ctrl->prev
                && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
                    setInitialFirstResponder_((id)ctrl->prev->priv[0], gwnd);
                    setDefaultButtonCell_((id)ctrl->prev->priv[0], cell(gwnd));
                }
                setTitle_(gwnd, capt = UTF8(temp));
                CFRelease(capt);
                free(temp);
                break;
            }
            case FCT_CBOX:
                ctrl->fe2c = FE2CX;
                gwnd = init(alloc(scls->butn));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setTarget_(gwnd, gwnd);
                setAction_(gwnd, ActionSelector);
                setButtonType_(gwnd, NSSwitchButton);
                setImagePosition_(gwnd, (ctrl->flgs & FSX_LEFT)?
                                         NSImageRight : NSImageLeft);
                setTitle_(gwnd, capt = UTF8(text));
                CFRelease(capt);
                break;

            case FCT_RBOX:
                /// [TODO:] do we really need radio boxes?
                break;

            case FCT_SPIN: {
                CGRect temp = {};
                CGPoint spin;

                ctrl->fe2c = FE2CN;
                gwnd = init(alloc(NSView));
                ctrl->priv[3] = root->priv[5];
                ctrl->priv[6] = (intptr_t)init(alloc(scls->spin));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->text));
                SET_IVAR((id)ctrl->priv[6], VAR_CTRL, ctrl);
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
                GetT2DV(spin, cell((id)ctrl->priv[6]), CellSize);
                temp.size = dims.size;
                temp.size.width -= spin.x;
                setFrame_((id)ctrl->priv[7], temp);
                setSendsActionOnEndEditing_(cell((id)ctrl->priv[7]), true);
                setFormatter_((id)ctrl->priv[7], (id)ctrl->priv[3]);
                setDelegate_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setTarget_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setAction_((id)ctrl->priv[7], ActionSelector);
                temp.origin.x = temp.size.width;
                temp.size.width = spin.x;
                setFrame_((id)ctrl->priv[6], temp);
                setValueWraps_((id)ctrl->priv[6], false);
                setTarget_((id)ctrl->priv[6], (id)ctrl->priv[6]);
                setAction_((id)ctrl->priv[6], ActionSelector);
                addSubview_(gwnd, (id)ctrl->priv[6]);
                addSubview_(gwnd, (id)ctrl->priv[7]);
                break;
            }
            case FCT_LIST: {
                id *vfld, *vmet;
                CGRect temp = {};

                ctrl->fe2c = FE2CL;
                temp.size = dims.size;
                ctrl->priv[1] = (intptr_t)NewClass
                    ((OSX_10_07_PLUS)? NSButton : NSButtonCell,
                     "rNSX", vfld = PutToArr(VAR_CTRL, VAR_DATA),
                      vmet = PutToArr(ActionSelector, OnListButton));
                free(vmet);
                free(vfld);

                gwnd = init(alloc(NSScrollView));
                ctrl->priv[7] = (intptr_t)init(alloc(scls->list));
                SET_IVAR((id)ctrl->priv[7], VAR_CTRL, ctrl);
                setDataSource_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setDelegate_((id)ctrl->priv[7], (id)ctrl->priv[7]);
                setFrame_((id)ctrl->priv[7], temp);

                ctrl->priv[6] = (intptr_t)init(alloc(NSTableColumn));
                setStringValue_(headerCell((id)ctrl->priv[6]), capt = UTF8(0));
                CFRelease(capt);
                addTableColumn_((id)ctrl->priv[7], (id)ctrl->priv[6]);
                setResizingMask_((id)ctrl->priv[6],
                                  NSTableColumnAutoresizingMask);

                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                setHasVerticalScroller_(gwnd, true);
                break;
            }
            case FCT_SBOX:
                ctrl->fe2c = FE2CS;
                ctrl->priv[1] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                ctrl->priv[2] =  (uint16_t)dims.origin.x
                              | ((uint32_t)dims.origin.y << 16);
                gwnd = init(alloc(NSScrollView));
                setHasVerticalScroller_(gwnd, true);
                setDrawsBackground_(gwnd, false);
                ctrl->priv[7] = (intptr_t)init(alloc(scls->sbox));
                setDocumentView_(gwnd, (id)ctrl->priv[7]);
                break;

            case FCT_IBOX: {
                CGColorSpaceRef drgb = CGColorSpaceCreateDeviceRGB();
                long line = dims.size.width * 4;

                ctrl->fe2c = FE2CI;
                gwnd = init(alloc(scls->ibox));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                ctrl->priv[3] =  (uint16_t)dims.size.width
                              | ((uint32_t)dims.size.height << 16);
                ctrl->priv[2] = (uintptr_t)malloc(line * dims.size.height);
                ctrl->priv[1] = (uintptr_t)CGBitmapContextCreate
                    ((void*)ctrl->priv[2], dims.size.width, dims.size.height,
                      CHAR_BIT, line, drgb, kCGImageAlphaPremultipliedFirst
                                          | kCGBitmapByteOrder32Little);
                CGContextSetBlendMode((CGContextRef)ctrl->priv[1],
                                       kCGBlendModeClear);
                CGColorSpaceRelease(drgb);
                break;
            }
            case FCT_PBAR: {
                CGFloat ffsz;
                id psty, font;

                ctrl->fe2c = FE2CP;
                gwnd = init(alloc(scls->pbar));
                SET_IVAR(gwnd, VAR_CTRL, ctrl);
                setIndeterminate_(gwnd, false);
                setWantsLayer_(gwnd, true);
                psty = init(alloc(NSMutableParagraphStyle));
                setAlignment_(psty, NSCenterTextAlignment);
                GetT1DV(ffsz, NSFont, SystemFontSize);
                ffsz *= 0.85;
                if (OSX_10_10_PLUS) {
                    scaleUnitSquareToSize_(gwnd, ((CGSize){2.0, 2.0}));
                    ffsz *= 0.5;
                }
                font = systemFontOfSize_(NSFont, ffsz);
                ctrl->priv[5] = (intptr_t)MakeDict
                    (kCTFontAttributeName,           font,
                     kCTParagraphStyleAttributeName, psty);
                release(psty);
                ctrl->priv[4] = 0.5 * (dims.size.height - ffsz) - 1.0;
                break;
            }
        }
        setFrame_(gwnd, dims);
        addSubview_((id)ctrl->prev->priv[7], gwnd);
        if (ctrl->prev && ((ctrl->prev->flgs & FCT_TTTT) == FCT_WNDW)) {
            id resp; /// tab-stop responder

            switch (ctrl->flgs & FCT_TTTT) {
                case FCT_TEXT:
                case FCT_PBAR:
                case FCT_SBOX: resp = 0; break;
                case FCT_SPIN: resp = (id)ctrl->priv[7]; break;
                default:       resp = gwnd; break;
            }
            if (resp) {
                if (ctrl->prev->priv[4]) {
                    setNextKeyView_((id)ctrl->prev->priv[4], resp);
                    setNextKeyView_(resp, (id)ctrl->prev->priv[3]);
                    ctrl->prev->priv[4] = (intptr_t)resp;
                }
                else {
                    ctrl->prev->priv[3] = ctrl->prev->priv[4] = (intptr_t)resp;
                    /// shall do nothing if there already is a first responder
                    setInitialFirstResponder_((id)ctrl->prev->priv[0], resp);
                }
            }
        }
    }
    ctrl->priv[0] = (intptr_t)gwnd;
    #undef CLS_MAKE
}



int main(int argc, char *argv[]) {
    id *vmet, pool, urls, menu;
    CFStringRef path;
    CGFloat icon;
    CGRect dims;

    struct dirent **dirs;
    ENGC *engc;
    char *home;
    long  iter;

    LoadObjC((char*[]){STR_OBJC_CLAS, 0}, (char*[]){STR_OBJC_SELE, 0});

    pool = init(alloc(NSAutoreleasePool));
    urls = URLsForDirectory_inDomains_(defaultManager(NSFileManager),
                                       NSApplicationSupportDirectory,
                                       NSUserDomainMask);
    path = CFURLCopyFileSystemPath
               (CFArrayGetValueAtIndex((CFArrayRef)urls, 0),
                                        kCFURLPOSIXPathStyle);
    home = CopyUTF8(path);
    CFRelease(path);
    home = realloc(home, strlen(home) + 32);
    strcat(home, DEF_OPTS);
    if (!((mkdir(home, 0755))? (errno != EEXIST)? 0 : 1 : 2))
        printf("WARNING: cannot create '%s'!", home);

    engc = eInitializeEngine(home);
    free(home);

    home = CopyUTF8(path = (CFStringRef)bundlePath(mainBundle(NSBundle)));
    home = realloc(home, strlen(home) + 32);
    strcat(home, "/Contents/MacOS/"DEF_FLDR);

    setActivationPolicy_(sharedApplication(NSApplication),
                         NSApplicationActivationPolicyAccessory);

    if ((iter = scandir(home, &dirs, 0, alphasort)) >= 0) {
        while (iter--) {
            if ((dirs[iter]->d_type == DT_DIR)
            &&  strcmp(dirs[iter]->d_name, ".")
            &&  strcmp(dirs[iter]->d_name, ".."))
                eAppendLib(engc, DEF_CONF, home, dirs[iter]->d_name);
            free(dirs[iter]);
        }
        free(dirs);
    }
    free(home);
    GetT4DV(dims, mainScreen(NSScreen), VisibleFrame);
    GetT1DV(icon, systemStatusBar(NSStatusBar), Thickness);
    menu = NewClass(NSObject, CLS_MENU, 0,
                    vmet = PutToArr(ActionSelector, OnMenu));
    free(vmet);
    eExecuteEngine(engc, icon, icon, dims.origin.x, dims.origin.y,
                   dims.size.width  + dims.origin.x,
                   dims.size.height + dims.origin.y);
    DelClass((Class)menu);
    release(pool);
    return 0;
}
