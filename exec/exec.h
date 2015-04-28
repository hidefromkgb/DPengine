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



/// default directory separator
#define DEF_DSEP "/"
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



/// doubly-linked list header
#define HDR_LIST \
    struct _LHDR *prev, *next;
typedef struct _LHDR {
    HDR_LIST;
} LHDR;

/// doubly-linked list iterator
typedef void (*ITER)(LHDR *item, uintptr_t data);

/// behaviour/effect unit info (write-once, read-only)
typedef struct _BINF {
    AINF  unit[2];  /// image pair
    T2IV  cntr[2];  /// image centers
    T2IV  ptgt;     /// follow target relative coords
    long  prob,     /// probability, 0-1000
          dmin,     /// minimum duration in msec
          dmax,     /// maximum duration / cooldown in msec
          neff,     /// number of linked effects
          ieff,     /// effect array index of the first linked effect
          igrp;     /// behaviour group index
    float move;     /// movement speed in pixels per frame
    uint32_t name,  /// name hash for behaviour / target for effect
             flgs,  /// behaviour / effect flags
             link,  /// linked behaviour index
             trgt;  /// follow target name hash
} BINF;

/// unit library info (write-once, read-only)
/// [TODO] behaviour groups
/// [TODO] speech
/// [TODO] interactions
/// [TODO] categories
typedef struct _LINF {
    HDR_LIST;       /// list header
    BINF *barr,     /// available behaviours
         *earr,     /// available effects
        **bgrp;     /// BARR elements ordered by behaviour group + probability
    char *path,     /// the folder from which the library was built
         *name;     /// human-readable name (may differ from PATH!)
    long *ngrp,     /// bounds of behaviour groups in BGRP: [0~~)[G0~~~)[G1...
         *prob,     /// sum of all behaviour probability coeffs per group
          flgs,     /// flags
          gcnt,     /// behaviour groups count
          zcnt,     /// nonzero probability behaviours count
          bcnt,     /// total behaviours count
          ecnt,     /// total effects count
          icnt;     /// number of on-screen sprites from the library
} LINF;

/// actual on-screen sprite
typedef struct _PICT {
    HDR_LIST;       /// list header
    T2FV  move,     /// movement direction
          offs;     /// position of the unit`s lower-left corner
    LINF *ulib;     /// unit library which the sprite belongs to
    uint32_t indx,  /// behaviour index and direction (lowest bit)
             fram;  /// current frame
    uint64_t tfrm,  /// timestamp of the next frame in msec
             tmov,  /// timestamp of the next movement in msec
             tbhv;  /// timestamp of the next behaviour in msec
} PICT;

/// engine data (client side)
typedef struct _ENGC {
    MENU *menu;     /// per-sprite context menu
    LINF *libs;     /// sprite libraries list
    PICT *pcur,     /// the sprite currently picked
         *plst,     /// on-screen sprites list
        **parr;     /// on-screen sprite pointers array
    T2IV  ppos,     /// mouse pointer position
          dims;     /// drawing area dimensions
    uintptr_t engh; /// rendering engine handle
    uint32_t  pcnt, /// number of on-screen sprites
              seed, /// random seed
              flgs, /// flags
              quit; /// special termination flag
} ENGC;



void  ListAppendHead(LHDR **list, long size);
void  ListAppendTail(LHDR **list, long size);
LHDR *ListIterateHeadToTail(LHDR *list, ITER iter, uintptr_t data);
LHDR *ListIterateTailToHead(LHDR *list, ITER iter, uintptr_t data);

#define HTT_ITER(list, iter, data) \
    ListIterateHeadToTail((LHDR*)(list), (ITER)(iter), (uintptr_t)(data))
#define TTH_ITER(list, iter, data) \
    ListIterateTailToHead((LHDR*)(list), (ITER)(iter), (uintptr_t)(data))

void InitMainMenu(ENGC *engc);
long MakeSpriteArr(ENGC *engc);
void FreeEverything(ENGC *engc);
void PrepareSpriteArr(LINF *elem, LINF **edge);
void AppendLib(ENGC *engc, char *pcnf, char *base, char *path);

uint32_t UpdateFrame(uintptr_t engh, uintptr_t user,
                     T4FV *data, uint64_t *time, uint32_t flgs,
                     int32_t xptr, int32_t yptr, int32_t isel);



/// external functions, have to be implemented or imported
char *LoadFileZ(char *name, long *size);
