#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <engine.h>



/** default config directory    **/ #define DEF_OPTS "/DesktopPonies"
/** default core config         **/ #define DEF_CORE "/core.conf"
/** default anim directory      **/ #define DEF_FLDR "Ponies"
/** default config file         **/ #define DEF_CONF "pony.ini"

/// /// /// /// /// /// /// /// /// menu constants
/** item is disabled            **/ #define MFL_GRAY  (1 << 0)
/** current checkbox value      **/ #define MFL_VCHK  (1 << 1)
/** item has a checkbox         **/ #define MFL_CCHK  (1 << 2)
/** checkbox is a radiobutton   **/ #define MFL_RCHK ((1 << 3) | MFL_CCHK)

/// /// /// /// /// /// /// /// /// Flags for Control Types
/** Container window            **/ #define FCT_WNDW 0x000
/** Simple label                **/ #define FCT_TEXT 0x001
/** Button (may be checkable)   **/ #define FCT_BUTN 0x002
/** Check box                   **/ #define FCT_CBOX 0x003
/** Radio box                   **/ #define FCT_RBOX 0x004
/** Spin counter                **/ #define FCT_SPIN 0x005
/** List box (with checkboxes)  **/ #define FCT_LIST 0x006
/** Progress bar                **/ #define FCT_PBAR 0x007
/** Scroll box                  **/ #define FCT_SBOX 0x008
/** Image box                   **/ #define FCT_IBOX 0x009
/** [extractor]                 **/ #define FCT_TTTT 0x00F

/// /// /// /// /// /// /// /// /// Flags for Controls` Position inheriting
/** Horizontal inheriting       **/ #define FCP_HORZ 0x010
/** Vertical inheriting         **/ #define FCP_VERT 0x020
/** Both                        **/ #define FCP_BOTH (FCP_HORZ | FCP_VERT)

/// /// /// /// /// /// /// /// /// Flags for Style of W (FCT_WNDW)

/// /// /// /// /// /// /// /// /// Flags for Style of T (FCT_TEXT)
/** Sunken edge                 **/ #define FST_SUNK 0x040
/** Center text                 **/ #define FST_CNTR 0x080

/// /// /// /// /// /// /// /// /// Flags for Style of B (FCT_BUTN)
/** Default button of a window  **/ #define FSB_DFLT 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of X (FCT_CBOX)
/** Checkbox text on the left   **/ #define FSX_LEFT 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of R (FCT_RBOX)
/** New radio group             **/ #define FSR_NGRP 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of N (FCT_SPIN)

/// /// /// /// /// /// /// /// /// Flags for Style of L (FCT_LIST)

/// /// /// /// /// /// /// /// /// Flags for Style of P (FCT_PBAR)

/// /// /// /// /// /// /// /// /// Flags for Style of S (FCT_SBOX)

/// /// /// /// /// /// /// /// /// controls` messages
/** enable or disable anything  **/ #define MSG__ENB  1
/** show or hide anything       **/ #define MSG__SHW  2
/** get pixel size of anything  **/ #define MSG__GSZ  3
/** set position  of anything   **/ #define MSG__POS  4
/** resize & center wnd/sizebox **/ #define MSG_WSZC  5
/** click button/check/radio    **/ #define MSG_BCLK  6
/** get spin position           **/ #define MSG_NGET  7
/** set spin limits & reset pos **/ #define MSG_NSET  8
/** set progressbar text        **/ #define MSG_PTXT  9
/** set progressbar upper limit **/ #define MSG_PLIM 10
/** set progressbar position    **/ #define MSG_PPOS 11
/** set scrollbox internal dims **/ #define MSG_SMAX 13
/** rename listbox column       **/ #define MSG_LCOL 14
/** add item to listbox column  **/ #define MSG_LADD 15
/** imagebox update frame       **/ #define MSG_IFRM 16
/** imagebox set animation ID   **/ #define MSG_IANI 17

/// engine data (client side), opaque outside the module
typedef struct ENGC ENGC;

/// preview updater function
typedef void (*UPRE)(ENGC *engc, intptr_t data, uint64_t time);

/// control handler function (either control-to-exec or exec-to-control)
struct _CTRL;
typedef intptr_t (*FCTL)(struct _CTRL *ctrl, uint32_t cmsg, intptr_t data);

/// control (checkbox, listbox, counter, etc.)
typedef struct _CTRL {
    struct          /// __
    _CTRL   *prev;  /// r \ parent
    ENGC    *engc;  /// e | engine
    int32_t  uuid,  /// a | unique identifier
             flgs;  /// d | type (lowest nibble) and style flags
    long     xpos,  /// o | X position
             ypos,  /// n | Y position
             xdim,  /// l | width (absolute if negative)
             ydim;  /// y | height (absolute if negative)
    FCTL     fc2e,  /// __/ control-to-exec handler (actions)
             fe2c;  ///     exec-to-control handler (properties)
    intptr_t priv[8]; ///   implementation-defined private data
} CTRL;

/// menu item
typedef struct _MENU {
    struct
    _MENU   *chld;  /// submenu (or 0 if none)
    char    *text;  /// item text
    intptr_t data;  /// item data
    uint32_t flgs,  /// item flags
             uuid;  /// item ID
    void   (*func)(struct _MENU*); /// item handler
} MENU;



void  eAppendLib(ENGC *engc, char *pcnf, char *base, char *path);
void  eProcessMenuItem(MENU *item);
ENGC *eInitializeEngine(char *conf);
void  eExecuteEngine(ENGC *engc, ulong xico, ulong yico,
                     long xpos, long ypos, ulong xdim, ulong ydim);



/// external functions, have to be implemented or imported
void  rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre,
                        ENGC *engc, intptr_t data);
void  rMakeControl(CTRL *ctrl, long *xoff, long *yoff, char *text);
void  rFreeControl(CTRL *ctrl);
long  rMessage(char *text, char *head, uint32_t flgs);
intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim);
void  rFreeTrayIcon(intptr_t icon);
void  rOpenContextMenu(MENU *menu);
MENU *rOSSpecificMenu(ENGC *engc);
char *rConvertUTF8(char *utf8);
char *rLoadFile(char *name, long *size);
long  rSaveFile(char *name, char *data, long size);
