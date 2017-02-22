#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <engine.h>



/** default config directory    **/ #define DEF_OPTS "/DesktopPonies"
/** default core config         **/ #define DEF_CORE "/core.conf"
/** default anim directory      **/ #define DEF_FLDR "Content"
/** default config file         **/ #define DEF_CONF "pony.ini"

/// /// /// /// /// /// /// /// /// menu constants
/** item is disabled            **/ #define MFL_GRAY  (1 << 0)
/** current checkbox value      **/ #define MFL_VCHK  (1 << 1)
/** item has a checkbox         **/ #define MFL_CCHK  (1 << 2)
/** checkbox is a radiobutton   **/ #define MFL_RCHK ((1 << 3) | MFL_CCHK)

/// /// /// /// /// /// /// /// /// Flags for Controls` Types
/** Container window            **/ #define FCT_WNDW 0x000
/** Simple label                **/ #define FCT_TEXT 0x001
/** Button (may be checkable)   **/ #define FCT_BUTN 0x002
/** Check box                   **/ #define FCT_CBOX 0x003
/** Spin counter                **/ #define FCT_SPIN 0x004
/** List box (with checkboxes)  **/ #define FCT_LIST 0x005
/** Progress bar                **/ #define FCT_PBAR 0x006
/** Scroll box                  **/ #define FCT_SBOX 0x007
/** Image box                   **/ #define FCT_IBOX 0x008
/** [extractor]                 **/ #define FCT_TTTT 0x00F

/// /// /// /// /// /// /// /// /// Flags for Controls` Position inheriting
/** Horizontal inheriting       **/ #define FCP_HORZ 0x010
/** Vertical inheriting         **/ #define FCP_VERT 0x020
/** Both                        **/ #define FCP_BOTH (FCP_HORZ | FCP_VERT)

/// /// /// /// /// /// /// /// /// Flags for Controls` State
/** Control is enabled          **/ #define FCS_ENBL 0x01
/** Control is marked           **/ #define FCS_MARK 0x02

/// /// /// /// /// /// /// /// /// Flags for Style of W (FCT_WNDW)
/** Resize/minimize/restore on  **/ #define FSW_SIZE 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of T (FCT_TEXT)
/** Sunken edge                 **/ #define FST_SUNK 0x040
/** Center text                 **/ #define FST_CNTR 0x080

/// /// /// /// /// /// /// /// /// Flags for Style of B (FCT_BUTN)
/** Default button of a window  **/ #define FSB_DFLT 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of X (FCT_CBOX)
/** Checkbox text on the left   **/ #define FSX_LEFT 0x040

/// /// /// /// /// /// /// /// /// Flags for Style of N (FCT_SPIN)

/// /// /// /// /// /// /// /// /// Flags for Style of L (FCT_LIST)

/// /// /// /// /// /// /// /// /// Flags for Style of P (FCT_PBAR)

/// /// /// /// /// /// /// /// /// Flags for Style of S (FCT_SBOX)

/// /// /// /// /// /// /// /// /// controls` messages
enum {
/** empty message; does nothing **/ MSG__NUL = 0,
/** enable or disable anything  **/ MSG__ENB,
/** show or hide anything       **/ MSG__SHW,
/** get pixel size of anything  **/ MSG__GSZ,
/** set position of anything    **/ MSG__POS,
/** set title text of anything  **/ MSG__TXT,
/** close the container window  **/ MSG_WEND,
/** resize & center wnd/sizebox **/ MSG_WSZC,
/** click button or checkbox    **/ MSG_BCLK,
/** get button/checkbox state   **/ MSG_BGST,
/** get spin control position   **/ MSG_NGET,
/** set spin control position   **/ MSG_NSET,
/** set spin control limits     **/ MSG_NDIM,
/** set progressbar upper limit **/ MSG_PLIM,
/** set progressbar position    **/ MSG_PPOS,
/** get progressbar properties  **/ MSG_PGET,
/** set scrollbox internal dims **/ MSG_SMAX,
/** add item to listbox         **/ MSG_LADD,
/** get listbox item state      **/ MSG_LGST,
/** set listbox item state      **/ MSG_LSST,
/** imagebox update frame       **/ MSG_IFRM,
};

/// /// /// /// /// /// /// /// /// message box flags
/** "OK" button only            **/ #define RMF_BTOK   0
/** "Accept" and "Deny" buttons **/ #define RMF_BTAD  (1 << 0)

/// preview updater function
typedef void (*UPRE)(intptr_t data, uint64_t time);

/// control handler function (either control-to-exec or exec-to-control)
struct _CTRL;
typedef intptr_t (*FCTL)(struct _CTRL *ctrl, uint32_t cmsg, intptr_t data);

/// control (checkbox, listbox, counter, etc.)
typedef struct _CTRL {
    struct          /// __
    _CTRL   *prev;  /// r \ parent
    intptr_t data;  /// e | control-specific data
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



void  eProcessMenuItem(MENU *item);
void  eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                     long xpos, long ypos, ulong xdim, ulong ydim);



/// external functions, have to be implemented or imported
intptr_t rMakeParallel(UPRE func, long size);
intptr_t rFindMake(char *base);
intptr_t rMakeHTTPS(char *user, char *serv);
intptr_t rMakeTrayIcon(MENU *mctx, char *text,
                       uint32_t *data, long xdim, long ydim);
long  rMessage(char *text, char *head, uint32_t flgs);
long  rSaveFile(char *name, char *data, long size);
long  rLoadHTTPS(intptr_t user, char *page, char **dest);
long  rMoveDir(char *dsrc, char *ddst);
long  rMakeDir(char *name);
void  rInternalMainLoop(CTRL *root, uint32_t fram, UPRE upre, intptr_t data);
void  rMakeControl(CTRL *ctrl, long *xoff, long *yoff);
void  rFreeControl(CTRL *ctrl);
void  rFreeHTTPS(intptr_t user);
void  rFreeParallel(intptr_t user);
void  rLoadParallel(intptr_t user, intptr_t data);
void  rFreeTrayIcon(intptr_t icon);
void  rOpenContextMenu(MENU *menu);
char *rConvertUTF8(char *utf8);
char *rChooseDir(CTRL *root, char *base);
char *rChooseFile(CTRL *root, char *fext, char *file);
char *rFindFile(intptr_t data);
char *rLoadFile(char *name, long *size);
MENU *rOSSpecificMenu(void *engc);
