#include <time.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <engine.h>



/// a macro to count the capacity of static arrays
#define countof(a) (sizeof(a) / sizeof(*(a)))

#ifndef min
#define min(a, b) (((a) < (b))? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b))? (a) : (b))
#endif



/// default animation folder
#define DEF_FLDR "Ponies"
/// default config file
#define DEF_CONF "pony.ini"
/// default comment character
#define DEF_CMNT '\''
/// default token separator
#define DEF_TSEP ','

/// degrees to radians conversion coef
#define DTR_CONV (M_PI / 180.0)
/// radians to degrees conversion coef
#define RTD_CONV (1.0 / DTR_CONV)



/// invalid hash
#define ERR_HASH 0x00000000



/// truth values

/// 'true'
#define VAL_TRUE 0x390A9E10
/// 'false'
#define VAL_FALS 0xD6A90B70



/// section value types

/// 'name'
#define SVT_NAME 0x692C5651
/// 'effect'
#define SVT_EFCT 0x80720D9F
/// 'behavior'
#define SVT_BHVR 0x532A0FD4
/// 'behaviorgroup'
#define SVT_BGRP 0xA40004B2
/// 'categories'
#define SVT_CTGS 0x21179D08
/// 'speak'
#define SVT_PHRS 0xF708913D



/// behaviour movement types

/// 'none'
#define BMT_NONM 0xF3B3E074
/// 'horizontal_only'
#define BMT_HORM 0x2359740E
/// 'vertical_only'
#define BMT_VERM 0x4753BD0C
/// 'diagonal_only'
#define BMT_DIAM 0xA988F1D9
/// 'horizontal_vertical'
#define BMT_HNVM 0x9D44367E
/// 'diagonal_horizontal'
#define BMT_HNDM 0x590262FB
/// 'diagonal_vertical'
#define BMT_DNVM 0xE2676419
/// 'all'
#define BMT_ALLM 0x43E72DD0
/// 'mouseover'
#define BMT_OVRM 0xB73EDCDE
/// 'dragged'
#define BMT_DRGM 0x4A4CCCE1
/// 'sleep'
#define BMT_SLPM 0x62B9B962



/// effect alignment types

/// 'top_left'
#define EMT_TNLA 0xE73713ED
/// 'top'
#define EMT_TOPA 0xA47C2B7E
/// 'top_right'
#define EMT_TNRA 0xECF514DD
/// 'left'
#define EMT_CNLA 0x7D6BA6E7
/// 'center'
#define EMT_CNTA 0x4E745BAB
/// 'right'
#define EMT_CNRA 0x0F854D3F
/// 'bottom_left'
#define EMT_BNLA 0x884F61CE
/// 'bottom'
#define EMT_BTMA 0x8819E73B
/// 'bottom_right'
#define EMT_BNRA 0xB9049E02
/// 'any'
#define EMT_RNDA 0x43E92567
/// 'any-not_center'
#define EMT_RCLA 0xD76D8510



/// behaviour/effect flags

/// no movement at all
#define BHV_NONM (0      )
/// horizontal movement allowed
#define BHV_HORM (1 <<  0)
/// diagonal movement allowed
#define BHV_DIAM (1 <<  1)
/// vertical movement allowed
#define BHV_VERM (1 <<  2)
/// movement control flag
#define BHV_CTLM (1 <<  3)

/// horizontal + vertical movement
#define BHV_HNVM (BHV_HORM | BHV_VERM)
/// horizontal + diagonal movement
#define BHV_HNDM (BHV_HORM | BHV_DIAM)
/// diagonal + vertical movement
#define BHV_DNVM (BHV_DIAM | BHV_VERM)
/// horizontal + diagonal + vertical movement
#define BHV_ALLM (BHV_HORM | BHV_DIAM | BHV_VERM)
/// 'mouse-over' movement state
#define BHV_OVRM (BHV_CTLM | BHV_HORM)
/// 'dragged' movement state
#define BHV_DRGM (BHV_CTLM | BHV_DIAM)
/// 'sleep' movement state
#define BHV_SLPM (BHV_CTLM | BHV_VERM)
/// all movement flags together
#define BHV_MMMM (BHV_HORM | BHV_DIAM | BHV_VERM | BHV_CTLM)

/// this item can be executed at random
#define BHV_EXEC (1 << 27)
/// [a flag yet to be understood]
#define BHV_____ (1 << 28)
/// the target offset shall be mirrored
#define BHV_MIRR (1 << 29)

/// top-left alignment
#define EFF_TNLA 0
/// top alignment
#define EFF_TOPA 1
/// top-right alignment
#define EFF_TNRA 2
/// center-left alignment
#define EFF_CNLA 3
/// center alignment
#define EFF_CNTA 4
/// center-right alignment
#define EFF_CNRA 5
/// bottom-left alignment
#define EFF_BNLA 6
/// bottom alignment
#define EFF_BTMA 7
/// bottom-right alignment
#define EFF_BNRA 8
/// random alignment
#define EFF_RNDA 9
/// random centerless alignment
#define EFF_RCLA 10
/// alignment extraction value
#define EFF_AAAA 0xF

/// this item shall stay where it is and not follow its parent
#define EFF_STAY (1 << 29)

/// this item`s animation can be looped
#define FLG_LOOP (1 << 30)

/// this item is an effect
#define FLG_EFCT (1 << 31)



/// follow offset type values

/// 'fixed'
#define FOT_FIXD 0x9A8F97BD
/// 'mirror'
#define FOT_MIRR 0x304E7075



/// default frame rate limiter in msec
#define FRM_WAIT 40



/// menu constants

/// menu item is disabled
#define MFL_GRAY  (1 << 0)
/// current checkbox value
#define MFL_VCHK  (1 << 1)
/// menu item has a checkbox
#define MFL_CCHK  (1 << 2)
/// checkbox is a radiobutton
#define MFL_RCHK ((1 << 3) | MFL_CCHK)



/// localized text constants
#define TXT_CDEL  0
#define TXT_ADEL  1
#define TXT_CSLP  2
#define TXT_ASLP  3
#define TXT_PONY  4
#define TXT_HOUS  5
#define TXT_TPL1  6
#define TXT_TPL2  7
#define TXT_OPTS  8
#define TXT_RETN  9

#define TXT_HEAD 10
#define TXT_SPEC 11
#define TXT_OPAQ 12
#define TXT_DRAW 13
#define TXT_SHOW 14
#define TXT_EXIT 15
#define TXT_RGPU 16
#define TXT_NONE 17

#define TXT_CONS 18
#define TXT_IRGN 19
#define TXT_IBGR 20
#define TXT_IPBO 21
#define TXT_UOFO 22
#define TXT_UWGL 23



/// unit library info (write-once, read-only), opaque outside the module
typedef struct LINF LINF;

/// actual on-screen sprite, opaque outside the module
typedef struct PICT PICT;

/// menu item
typedef struct _MENU {
    struct
    _MENU    *chld; /// submenu (or 0 if none)
    uint8_t  *text; /// item text
    uintptr_t data; /// item data
    uint32_t  flgs, /// item flags
              uuid; /// item ID
    void    (*func)(struct _MENU*); /// item handler
} MENU;

/// engine data (client side)
typedef struct _ENGC {
    MENU     *mspr, /// per-sprite context menu
             *mctx; /// engine`s main context menu
    T4FV     *data; /// main display sequence passed to the renderer
    ENGD     *engd; /// rendering engine handle
    LINF     *libs; /// sprite libraries linked list
    PICT     *pcur, /// the sprite currently picked
            **parr; /// on-screen sprite pointers array
    uint8_t **tran; /// localized text array (ASCIIZ; last item is also 0)
    uint32_t  pcnt, /// number of on-screen sprites
              pmax, /// max. PARR capacity (realloc on exceed)
              seed; /// random seed
    T2IV      dims; /// drawing area dimensions
    T3IV      ppos; /// mouse pointer position (z = flags)
} ENGC;



uint32_t PRNG(uint32_t *seed);
MENU *MenuFromTemplate(MENU *tmpl);
void  ProcessMenuItem(MENU *item);
void  AppendLib(ENGC *engc, char *pcnf, char *base, char *path);
void  ExecuteEngine(ENGC *engc, long xpos, long ypos, ulong xdim, ulong ydim,
                    uintptr_t icon, uint32_t flgs, uint8_t *lang);



/// external functions, have to be implemented or imported
void  SetTrayIconText(uintptr_t icon, char *text);
void  OpenContextMenu(MENU *menu);
MENU *OSSpecificMenu(ENGC *engc);
char *ConvertUTF8(char *utf8);
char *LoadFileZ(char *name, long *size);

/// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME /// DEL ME ///
void __DEL_ME__SetLibUses(ENGC *engc, int32_t uses);
