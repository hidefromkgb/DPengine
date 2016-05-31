#include "exec.h"

/// a macro to count the capacity of static arrays
#define countof(a) (sizeof(a) / sizeof(*(a)))

/** convert degrees to radians  **/ #define DTR_CONV (M_PI / 180.0)
/** convert radians to degrees  **/ #define RTD_CONV (1.0 / DTR_CONV)

/** framerate limiter in msec   **/ #define FRM_WAIT 40
/** invalid hash                **/ #define ERR_HASH 0x00000000

/** default comment character   **/ #define DEF_CMNT '\''
/** default token separator     **/ #define DEF_TSEP ','

/// /// /// /// /// /// /// /// /// truth values
/** 'true'                      **/ #define VAL_TRUE 0x390A9E10
/** 'false'                     **/ #define VAL_FALS 0xD6A90B70

/// /// /// /// /// /// /// /// /// section value types
/** 'name'                      **/ #define SVT_NAME 0x692C5651
/** 'effect'                    **/ #define SVT_EFCT 0x80720D9F
/** 'behavior'                  **/ #define SVT_BHVR 0x532A0FD4
/** 'behaviorgroup'             **/ #define SVT_BGRP 0xA40004B2
/** 'categories'                **/ #define SVT_CTGS 0x21179D08
/** 'speak'                     **/ #define SVT_PHRS 0xF708913D

/// /// /// /// /// /// /// /// /// behaviour movement types
/** 'none'                      **/ #define BMT_NONM 0xF3B3E074
/** 'horizontal_only'           **/ #define BMT_HORM 0x2359740E
/** 'vertical_only'             **/ #define BMT_VERM 0x4753BD0C
/** 'diagonal_only'             **/ #define BMT_DIAM 0xA988F1D9
/** 'horizontal_vertical'       **/ #define BMT_HNVM 0x9D44367E
/** 'diagonal_horizontal'       **/ #define BMT_HNDM 0x590262FB
/** 'diagonal_vertical'         **/ #define BMT_DNVM 0xE2676419
/** 'all'                       **/ #define BMT_ALLM 0x43E72DD0
/** 'mouseover'                 **/ #define BMT_OVRM 0xB73EDCDE
/** 'dragged'                   **/ #define BMT_DRGM 0x4A4CCCE1
/** 'sleep'                     **/ #define BMT_SLPM 0x62B9B962

/// /// /// /// /// /// /// /// /// effect alignment types
/** 'top_left'                  **/ #define EMT_TNLA 0xE73713ED
/** 'top'                       **/ #define EMT_TOPA 0xA47C2B7E
/** 'top_right'                 **/ #define EMT_TNRA 0xECF514DD
/** 'left'                      **/ #define EMT_CNLA 0x7D6BA6E7
/** 'center'                    **/ #define EMT_CNTA 0x4E745BAB
/** 'right'                     **/ #define EMT_CNRA 0x0F854D3F
/** 'bottom_left'               **/ #define EMT_BNLA 0x884F61CE
/** 'bottom'                    **/ #define EMT_BTMA 0x8819E73B
/** 'bottom_right'              **/ #define EMT_BNRA 0xB9049E02
/** 'any'                       **/ #define EMT_RNDA 0x43E92567
/** 'any-not_center'            **/ #define EMT_RCLA 0xD76D8510

/// /// /// /// /// /// /// /// /// behaviour/effect flags
/** no movement at all          **/ #define BHV_NONM (0      )
/** horizontal movement         **/ #define BHV_HORM (1 <<  0)
/** diagonal movement           **/ #define BHV_DIAM (1 <<  1)
/** vertical movement           **/ #define BHV_VERM (1 <<  2)
/** movement control flag       **/ #define BHV_CTLM (1 <<  3)

/** horz + vert movement        **/ #define BHV_HNVM (BHV_HORM | BHV_VERM)
/** horz + diag movement        **/ #define BHV_HNDM (BHV_HORM | BHV_DIAM)
/** diag + vert movement        **/ #define BHV_DNVM (BHV_DIAM | BHV_VERM)
/** horz + diag + vert          **/ #define BHV_ALLM (BHV_HORM | BHV_DIAM | BHV_VERM)
/** 'mouse-over' state          **/ #define BHV_OVRM (BHV_CTLM | BHV_HORM)
/** 'dragged' state             **/ #define BHV_DRGM (BHV_CTLM | BHV_DIAM)
/** 'sleep' state               **/ #define BHV_SLPM (BHV_CTLM | BHV_VERM)
/** [extractor]                 **/ #define BHV_MMMM (BHV_HORM | BHV_DIAM | BHV_VERM | BHV_CTLM)

/** can be executed at random   **/ #define BHV_EXEC (1 << 27)
/** [yet to be understood]      **/ #define BHV_____ (1 << 28)
/** tgt offs has to be mirrored **/ #define BHV_MIRR (1 << 29)

/** top-left alignment          **/ #define EFF_TNLA 0
/** top alignment               **/ #define EFF_TOPA 1
/** top-right alignment         **/ #define EFF_TNRA 2
/** center-left alignment       **/ #define EFF_CNLA 3
/** center alignment            **/ #define EFF_CNTA 4
/** center-right alignment      **/ #define EFF_CNRA 5
/** bottom-left alignment       **/ #define EFF_BNLA 6
/** bottom alignment            **/ #define EFF_BTMA 7
/** bottom-right alignment      **/ #define EFF_BNRA 8
/** random alignment            **/ #define EFF_RNDA 9
/** random centerless align     **/ #define EFF_RCLA 10
/** [extractor]                 **/ #define EFF_AAAA 0xF

/** do not follow parent        **/ #define EFF_STAY (1 << 29)
/** animation can be looped     **/ #define FLG_LOOP (1 << 30)
/** this item is an effect      **/ #define FLG_EFCT (1 << 31)

/// /// /// /// /// /// /// /// /// follow offset type values
/** 'fixed'                     **/ #define FOT_FIXD 0x9A8F97BD
/** 'mirror'                    **/ #define FOT_MIRR 0x304E7075

/// /// /// /// /// /// /// /// /// localized text constants
/** Remove pony                 **/ #define TXT_CDEL  0
/** Remove all similar          **/ #define TXT_ADEL  1
/** Sleep/pause                 **/ #define TXT_CSLP  2
/** Sleep/pause all similar     **/ #define TXT_ASLP  3
/** Add pony >>>                **/ #define TXT_PONY  4
/** Add house >>>               **/ #define TXT_HOUS  5
/** Take control: Player 1      **/ #define TXT_TPL1  6
/** Take control: Player 2      **/ #define TXT_TPL2  7
/** Show options...             **/ #define TXT_OPTS  8
/** Return to menu...           **/ #define TXT_RETN  9

/** [ Desktop Ponies Engine ]   **/ #define TXT_HEAD 10
/** OS specific options         **/ #define TXT_SPEC 11
/** Disable transparency        **/ #define TXT_OPAQ 12
/** Play animation              **/ #define TXT_DRAW 13
/** Show window                 **/ #define TXT_SHOW 14
/** Exit                        **/ #define TXT_EXIT 15
/** Use GPU for drawing         **/ #define TXT_RGPU 16
/** [ none ]                    **/ #define TXT_NONE 17

/** Show console                **/ #define TXT_CONS 18
/** Use regions                 **/ #define TXT_IRGN 19
/** Enable BGRA                 **/ #define TXT_IBGR 20
/** Enable pixel buffers        **/ #define TXT_IPBO 21
/** Useless on full opacity!    **/ #define TXT_UOFO 22
/** Useless without GPU!        **/ #define TXT_UWGL 23
/** Cannot initialize GPU!      **/ #define TXT_CIGL 24

/** Desktop Ponies              **/ #define TXT_CAPT 25
/** Enable filters              **/ #define TXT_FLTR 26
/** Exact matching              **/ #define TXT_EXAC 27
/** [At least one:]             **/ #define TXT_OGRP 28
/** [All at once:]              **/ #define TXT_AGRP 29
/** Random selection            **/ #define TXT_SRND 30
/** Group selection             **/ #define TXT_SGRP 31
/** Add                         **/ #define TXT_BADD 32
/** Copies                      **/ #define TXT_BDUP 33
/** Total:                      **/ #define TXT_ITTL 34
/** More options...             **/ #define TXT_MORE 35
/** GO!                         **/ #define TXT_GOGO 36

/// doubly-linked list header
#define HDR_LIST \
    struct _LHDR *prev, *next;
typedef struct _LHDR {
    HDR_LIST;
} LHDR;

/// doubly-linked list iterator
typedef void (*ITER)(LHDR *item, intptr_t data);

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

/// unit library info (write-once, read-only), opaque outside the module
/// [TODO:] speech
/// [TODO:] interactions
/// [TODO:] categories
typedef struct _LINF {
    HDR_LIST;     /// list header
    BINF  *barr,  /// available behaviours ordered by name
          *earr,  /// available effects ordered by parent bhv. name
         **bgrp;  /// nonzero-probability BARR elements ordered by bhv. group
    char **bimp,  /// image paths for behaviours (0 if loaded)
         **eimp,  /// image paths for effects (0 if loaded)
          *path,  /// the folder from which the library was built
          *name;  /// human-readable name (may differ from PATH!)
    long  *ngrp,  /// bounds of behaviour groups in BGRP: [0~~)[G0~~~)[G1...
           flgs,  /// flags
           gcnt,  /// behaviour groups count
           zcnt,  /// nonzero probability behaviours count
           bcnt,  /// total behaviours count
           ecnt,  /// total effects count
           icnt;  /// number of on-screen bhv. sprites from the library
} LINF;

/// actual on-screen sprite, opaque outside the module
typedef struct _PICT {
    struct
    _PICT   *next;  /// linked list support for SortByY(), only used there
    LINF    *ulib;  /// unit library which the sprite belongs to
    T2FV     move,  /// movement direction
             offs;  /// position of the unit`s lower-left corner
    uint32_t indx,  /// behaviour index and direction (lowest bit)
             fram;  /// current frame
    uint64_t tfrm,  /// timestamp of the next frame in msec
             tmov,  /// timestamp of the next movement in msec
             tbhv;  /// timestamp of the next behaviour in msec
} PICT;

/// engine data (client side), opaque outside the module
struct ENGC {
    MENU     *mspr, /// per-sprite context menu
             *mctx; /// engine`s main context menu
    T4FV     *data; /// main display sequence passed to the renderer
    ENGD     *engd; /// rendering engine handle
    LINF     *libs; /// sprite libraries linked list
    PICT     *pcur, /// the sprite currently picked
            **parr; /// on-screen sprite pointers array
    char    **tran; /// localized text array (ASCIIZ; last item is also 0)
    uint32_t  pcnt, /// number of on-screen sprites
              pmax, /// max. PARR capacity (realloc on exceed)
              seed; /// random seed
    T2IV      dims; /// drawing area dimensions
    T3IV      ppos; /// mouse pointer position (z = flags)
};



/// Random number generator modulo
#define RNG_TRIM 0xFFFFFFFB
/// Random number generator multiplier
#define RNG_MULT 0x10A860C1
/// Random number generator shift
#define RNG_PLUS 0x01

uint32_t PRNG(uint32_t *seed) {
    return *seed = RNG_PLUS + (*seed * RNG_MULT) % RNG_TRIM;
}



/// String-oriented linear hash multiplier
#define SLH_MULT 0xFBC5
/// String-oriented linear hash shift
#define SLH_PLUS 0x11

uint32_t HashLine(char *line, long size) {
    uint32_t hash = 0;

    if (!size)
        size--;
    while (*line && size--)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



static inline long ClampToBounds(long what, long bmin, long bmax) {
    what = (what > bmin)? what : bmin;
    return (what < bmax)? what : bmax;
}



float StrToFloat(char *data) {
    char temp[32] = {};
    double retn = 0.0;
    long iter = 0;

    while ((iter < 31) && *data) {
        if ((*data != '.') && (*data != ',')) {
            if ((retn > 0.0) && (*data >= '0') && (*data <= '9'))
                retn *= 0.1;
            temp[iter++] = *data;
        }
        else if (retn == 0.0)
            retn = 1.0;
        data++;
    }
    return ((retn > 0.0)? retn : 1.0) * strtol(temp, 0, 10);
}



/// [TODO:] make this mess UTF8-compliant
char *ToLower(char *uppr, long size) {
    long iter;

    if (uppr) {
        if (!size)
            size = strlen(uppr);
        for (iter = 0; iter < size; iter++)
            uppr[iter] = tolower(uppr[iter]);
    }
    return uppr;
}



char *Dequote(char *quot) {
    long size;

    if (!quot || !(size = strlen(quot)))
        return 0;

    if (*quot == '"') {
        quot++;
        size--;
    }
    if (quot[size - 1] == '"')
        quot[size - 1] = '\0';

    return quot;
}



char *SkipCharUTF8(char *line) {
    static char skip[] = {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};
    return line + ((*line & 0x80)? skip[((*line) >> 2) & 0x0F] : 1);
}



long WhitespaceUTF8(char *line) {
    switch (line[0]) {
        case '\x09':
        case '\x20':
            return ~0;
        case '\xC2':
            if (line[1] == '\xA0') return ~0; break;
        case '\xE1':
            if (line[1] == '\x9A' && line[2] == '\x80') return ~0; break;
        case '\xE3':
            if (line[1] == '\x80' && line[2] == '\x80') return ~0; break;
        case '\xEF':
            if (line[1] == '\xBB' && line[2] == '\xBF') return ~0; break;
        case '\xE2':
            switch (line[1]) {
                case '\x81': if (line[2] == '\x9F') return ~0; break;
                case '\x80':
                    switch (line[2]) {
                        case '\x80':
                        case '\x81':
                        case '\x82':
                        case '\x83':
                        case '\x84':
                        case '\x85':
                        case '\x86':
                        case '\x87':
                        case '\x88':
                        case '\x89':
                        case '\x8A':
                        case '\x8B':
                        case '\xAF': return ~0;
                    }
            }
    }
    return 0;
}



char *SplitLine(char **tail, char tsep, long keep) {
    char *retn, *temp, *iter = *tail;

    if (*tail) {
        while (WhitespaceUTF8(iter))
            iter = SkipCharUTF8(iter);
        if (*iter) {
            if (!(*tail = strchr(iter, tsep)))
                *tail = iter + strlen(iter);
            temp = retn = iter;
            while (iter < *tail) {
                if (!WhitespaceUTF8(iter))
                    temp = iter;
                iter = SkipCharUTF8(iter);
            }
            *tail = (**tail)? *tail + 1 : 0;
            if (*temp) {
                if (*temp != tsep)
                    temp++;
                if (!keep)
                    *temp = 0;
            }
            return retn;
        }
        *tail = 0;
    }
    return *tail;
}



uint32_t DetermineType(char **tail) {
    char *temp = SplitLine(tail, DEF_TSEP, 1);

    if (temp && (*temp != DEF_CMNT))
        return HashLine(ToLower(temp, *tail - temp - 1), *tail - temp - 1);
    return ERR_HASH;
}



char *ConcatPath(char *base, char *path) {
    char *retn;
    long  iter;

    if (*path == '"')
        path++;
    retn = malloc(strlen(base) + strlen(path) + 2);
    iter = sprintf(retn, "%s/%s", base, path) - 1;
    if (retn[iter] == '"')
        retn[iter] = '\0';
    return retn;
}



char *GetNextLine(char **file) {
    char *retn;
    long  iter;

    if ((retn = SplitLine(file, '\n', 0)))
        if ((iter = strlen(retn)) > 0)
            if (retn[iter - 1] == '\r')
                retn[iter - 1] = '\0';
    return retn;
}



void ListAppendHead(LHDR **list, long size) {
    LHDR *retn = calloc(1, size);

    if (!*list)
         *list = retn;
    else {
        if ((*list)->prev) {
            retn->prev = (*list)->prev;
            retn->prev->next = retn;
        }
        retn->next = *list;
        (*list)->prev = retn;
        *list = retn;
    }
}



void ListAppendTail(LHDR **list, long size) {
    LHDR *retn = calloc(1, size);

    if (!*list)
         *list = retn;
    else {
        if ((*list)->next) {
            retn->next = (*list)->next;
            retn->next->prev = retn;
        }
        retn->prev = *list;
        (*list)->next = retn;
        *list = retn;
    }
}



LHDR *ListIterateHeadToTail(LHDR *list, ITER iter, intptr_t data) {
    LHDR *elem;

    if (!list)
        return 0;
    do {
        elem = list;
        list = list->next;
        iter(elem, data);
    } while (list);
    return elem;
}



LHDR *ListIterateTailToHead(LHDR *list, ITER iter, intptr_t data) {
    LHDR *elem;

    if (!list)
        return 0;
    do {
        elem = list;
        list = list->prev;
        iter(elem, data);
    } while (list);
    return elem;
}



int igrpcmp(const void *a, const void *b) {
    return (*(BINF**)b)->igrp - (*(BINF**)a)->igrp;
}

int namecmp(const void *a, const void *b) {
    return ((BINF*)b)->name - ((BINF*)a)->name;
}

int uintcmp(const void *a, const void *b) {
    return (*(uint32_t*)a) - (*(uint32_t*)b);
}

#define HTT_ITER(list, iter, data) \
    ListIterateHeadToTail((LHDR*)(list), (ITER)(iter), (intptr_t)(data))

#define TTH_ITER(list, iter, data) \
    ListIterateTailToHead((LHDR*)(list), (ITER)(iter), (intptr_t)(data))

#define GET_TEMP(conf) (temp = SplitLine(conf, DEF_TSEP, 0))

#define TRY_TEMP(conf) (GET_TEMP(conf) && *temp)

#define SET_FLAG(flgs, temp, hash, flag) \
    flgs = ((HashLine(ToLower(temp, 0), 0) == hash)? flag : 0) | (flgs & ~flag)

#define IF_BIN_FIND(elem, list, temp) \
    elem = HashLine(ToLower(GET_TEMP(temp), 0), 0); \
    iter = bsearch(&elem, list, countof(list), sizeof(elem), uintcmp); \
    if ((elem = (iter)? iter - list + 1 : 0))

void MakeSpritePair(char **dest, char *path, char **conf) {
    long iter;

    for (iter = 0; iter <= 1; iter++)
        dest[iter] = ConcatPath(path, Dequote(SplitLine(conf, DEF_TSEP, 0)));
}

void ParseBehaviour(ENGC *engc, BINF *retn, char **imgp, char **conf) {
    static uint32_t
        uBMT[] = {BMT_HORM, BMT_ALLM, BMT_VERM, BMT_DRGM, BMT_HNDM, BMT_SLPM,
                  BMT_HNVM, BMT_DIAM, BMT_OVRM, BMT_DNVM, BMT_NONM},
        uBHV[] = {BHV_HORM, BHV_ALLM, BHV_VERM, BHV_DRGM, BHV_HNDM, BHV_SLPM,
                  BHV_HNVM, BHV_DIAM, BHV_OVRM, BHV_DNVM, BHV_NONM};
    uint32_t elem, *iter;
    char *temp;

    /// defaults
    *retn = (BINF){{}, {}, {}, 0, 5000, 15000, 0.1 * FRM_WAIT, 0,
                   BHV_ALLM | BHV_EXEC | BHV_____ | FLG_LOOP, 0, 0};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// probability of this behaviour..........................................  def = 0
    if (TRY_TEMP(conf))
        retn->prob = ClampToBounds(StrToFloat(temp) * 1000.0, 0, 1000);

    /// maximum duration in sec................................................  def = 15
    if (TRY_TEMP(conf))
        retn->dmax = StrToFloat(temp) * 1000.0;

    /// minimum duration in sec................................................  def = 5
    if (TRY_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// movement speed (*100/3 for pix/sec)....................................  def = 3
    if (TRY_TEMP(conf))
        retn->move = StrToFloat(temp) * FRM_WAIT * 0.1 / 3.0;

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(imgp, engc->libs->path, conf);

    /// possible movement directions...........................................  def = All
    IF_BIN_FIND(elem, uBMT, conf)
        retn->flgs = uBHV[elem - 1] | (retn->flgs & ~BHV_MMMM);

    /// linked behaviour name..................................................  def = ""
    if (TRY_TEMP(conf))
        retn->link = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// speech said on behaviour start.........................................  def = ""
    if (TRY_TEMP(conf));

/// [TODO:]
    /// speech said on behaviour end...........................................  def = ""
    if (TRY_TEMP(conf));

    /// flag to never exec this behaviour at random............................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, BHV_EXEC);

    /// X target to follow.....................................................  def = 0
    if (TRY_TEMP(conf))
        retn->ptgt.x = StrToFloat(temp);

    /// Y target to follow.....................................................  def = 0
    if (TRY_TEMP(conf))
        retn->ptgt.y = StrToFloat(temp);

    /// name of the target.....................................................  def = ""
    if (TRY_TEMP(conf))
        retn->trgt = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// [something unintelligible].............................................  def = True
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_TRUE, BHV_____);

/// [TODO:]
    /// [something unintelligible].............................................  def = ""
    if (TRY_TEMP(conf));

/// [TODO:]
    /// [something unintelligible].............................................  def = ""
    if (TRY_TEMP(conf));

    /// right image center (natural center if "0,0")...........................  def = "0,0"
    if (TRY_TEMP(conf)) {
        retn->cntr[0].x = StrToFloat(Dequote(temp));
        retn->cntr[0].y = StrToFloat(Dequote(GET_TEMP(conf)));
    }
    /// left image center (natural center if "0,0")............................  def = "0,0"
    if (TRY_TEMP(conf)) {
        retn->cntr[1].x = StrToFloat(Dequote(temp));
        retn->cntr[1].y = StrToFloat(Dequote(GET_TEMP(conf)));
    }
    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, FLG_LOOP);

    /// behaviour group index..................................................  def = 0
    if (TRY_TEMP(conf)) {
        retn->igrp = StrToFloat(temp);
        retn->igrp = (retn->igrp > 0)? retn->igrp : 0;
    }
    /// whether target offset shall be mirrored................................  def = Fixed
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, FOT_MIRR, BHV_MIRR);
}

void ParseEffect(ENGC *engc, BINF *retn, char **imgp, char **conf) {
    static uint32_t
        uEMT[] = {EMT_CNRA, EMT_RNDA, EMT_CNTA, EMT_CNLA, EMT_BTMA, EMT_BNLA,
                  EMT_TOPA, EMT_BNRA, EMT_RCLA, EMT_TNLA, EMT_TNRA},
        uEFF[] = {EFF_CNRA, EFF_RNDA, EFF_CNTA, EFF_CNLA, EFF_BTMA, EFF_BNLA,
                  EFF_TOPA, EFF_BNRA, EFF_RCLA, EFF_TNLA, EFF_TNRA};
    uint32_t elem, *iter;
    char *temp;

    /// defaults
    *retn = (BINF){{}, {}, {}, 0, 5000, 0, 0, 0,
                   FLG_EFCT | EFF_STAY | (EFF_RNDA * 0x1111), 0, 0};

    /// effect name (skipped intentionally).................................... !def
    if (TRY_TEMP(conf));

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(imgp, engc->libs->path, conf);

    /// duration in sec........................................................  def = 5
    if (TRY_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// cooldown in sec........................................................  def = 0 (no repeating)
    if (TRY_TEMP(conf))
        retn->dmax = StrToFloat(temp) * 1000.0;

    /// possible right placements..............................................  def = Any
    IF_BIN_FIND(elem, uEMT, conf)
        retn->flgs = (uEFF[elem - 1] <<  0) | (retn->flgs & ~(EFF_AAAA <<  0));

    /// possible right centerings..............................................  def = Any
    IF_BIN_FIND(elem, uEMT, conf)
        retn->flgs = (uEFF[elem - 1] <<  4) | (retn->flgs & ~(EFF_AAAA <<  4));

    /// possible left placements...............................................  def = Any
    IF_BIN_FIND(elem, uEMT, conf)
        retn->flgs = (uEFF[elem - 1] <<  8) | (retn->flgs & ~(EFF_AAAA <<  8));

    /// possible left centerings...............................................  def = Any
    IF_BIN_FIND(elem, uEMT, conf)
        retn->flgs = (uEFF[elem - 1] << 12) | (retn->flgs & ~(EFF_AAAA << 12));

    /// flag to keep animation where it is.....................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, EFF_STAY);

    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, FLG_LOOP);

    if (!retn->dmax)
        retn->flgs &= ~FLG_LOOP;
}

void eAppendLib(ENGC *engc, char *pcnf, char *base, char *path) {
    char *file, *fptr, *conf, *temp;
    long bcnt, ecnt;

    conf = ConcatPath(fptr = ConcatPath(base, path), pcnf);
    if ((file = rLoadFile(conf, 0))) {
        free(conf);

        ListAppendTail((LHDR**)&engc->libs, sizeof(*engc->libs));
        engc->libs->path = fptr;

        fptr = file;
        engc->libs->bcnt = engc->libs->ecnt = 0;
        while ((conf = SplitLine(&fptr, '\n', 1)))
            switch (DetermineType(&conf)) {
                case SVT_BHVR:
                    engc->libs->bcnt++;
                    break;

                case SVT_EFCT:
                    engc->libs->ecnt++;
                    break;
            }

        if ((bcnt = engc->libs->bcnt)) {
            engc->libs->barr = calloc(bcnt,     sizeof(*engc->libs->barr));
            engc->libs->bimp = calloc(bcnt * 2, sizeof(*engc->libs->bimp));
        }
        if ((ecnt = engc->libs->ecnt)) {
            engc->libs->earr = calloc(ecnt,     sizeof(*engc->libs->earr));
            engc->libs->eimp = calloc(ecnt * 2, sizeof(*engc->libs->eimp));
        }
        fptr = file;
        bcnt = ecnt = 0;
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case SVT_NAME:
                    engc->libs->name = strdup(GET_TEMP(&conf));
                    break;

                case SVT_EFCT:
                    ParseEffect(engc, &engc->libs->earr[ecnt],
                               &engc->libs->eimp[ecnt * 2], &conf);
                    ecnt++;
                    break;

                case SVT_BHVR:
                    ParseBehaviour(engc, &engc->libs->barr[bcnt],
                                  &engc->libs->bimp[bcnt * 2], &conf);
                    bcnt++;
                    break;

                case SVT_BGRP:
                    /// doesn`t help much, skipping
                    break;

                case SVT_CTGS:
                    break;

                case SVT_PHRS:
                    break;
            }
        if (!engc->libs->name)
            engc->libs->name = strdup(path);
        free(file);
        conf = 0;
    }
    else
        free(fptr);
    free(conf);
}



void FreeLib(LINF *elem) {
    long iter = elem->bcnt * 2;

    while (elem->bimp || elem->eimp) {
        if (elem->bimp)
            while (iter)
                free(elem->bimp[--iter]);
        free(elem->bimp);
        iter = elem->ecnt * 2;
        elem->bimp = elem->eimp;
        elem->eimp = 0;
    }
    free(elem->path);
    free(elem->name);
    free(elem->barr);
    free(elem->earr);
    free(elem->bgrp);
    free(elem->ngrp);
    free(elem);
}



void LoadLib(LINF *elem, ENGD *engd) {
    long iter, indx, ncnt;
    char **nimp;
    BINF *narr;

    if (!elem->bimp || !elem->icnt)
        return;
    for (indx = 0; indx <= 1; indx++) {
        narr =  (indx)? elem->earr : elem->barr;
        nimp =  (indx)? elem->eimp : elem->bimp;
        ncnt = ((indx)? elem->ecnt : elem->bcnt) * 2;
        for (iter = 0; iter < ncnt; iter++)
            if (nimp[iter]) {
                cEngineLoadAnimAsync(engd, (uint8_t*)nimp[iter], 0,
                                    &narr[iter >> 1].unit[iter & 1]);
                free(nimp[iter]);
                nimp[iter] = 0;
            }
    }
}



void LoadTemplateFromLib(LINF *elem, ENGD *engd) {
    if (!elem->bimp || !elem->bimp[0])
        return;
    cEngineLoadAnimAsync(engd, (uint8_t*)elem->bimp[0], 0,
                        &elem->barr[0].unit[0]);
    free(elem->bimp[0]);
    elem->bimp[0] = 0;
}



void SortByY(ENGC *engc) {
    #define MAX_YDIM 0x1000
    PICT *temp, *elem[MAX_YDIM + 1] = {};
    long ymin, ymax, ytmp, iter;

    if (engc->pcnt < 2)
        return;

    ymin = MAX_YDIM + 1;
    ymax = -1;

    for (iter = 0; iter < engc->pcnt; iter++) {
        ytmp = engc->parr[iter]->offs.y;
        if ((ytmp >= 0) && (ytmp < MAX_YDIM)) {
            if (ytmp > ymax)
                ymax = ytmp;
            else if (ytmp < ymin)
                ymin = ytmp;
        }
    }
    for (iter = engc->pcnt - 1; iter >= 0; iter--) {
        temp = engc->parr[iter];
        ytmp = temp->offs.y - ymin + 1;
        if ((ytmp < 0) || (temp->offs.y >= MAX_YDIM))
            ytmp = 0;
        temp->next = elem[ytmp];
        elem[ytmp] = temp;
    }
    if ((ymax -= ymin - 1) < 0)
        ymax = 1;
    for (iter = 0; ymax >= 0; ymax--)
        while (elem[ymax]) {
            engc->parr[iter++] = elem[ymax];
            elem[ymax] = elem[ymax]->next;
        }
    #undef MAX_YDIM
}



void PlaceRandomly(ENGC *engc, PICT *pict) {
    AINF *anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];

    pict->offs.x = PRNG(&engc->seed) % (engc->dims.x - anim->xdim);
    pict->offs.y = PRNG(&engc->seed) % (engc->dims.y - anim->ydim)
                                                     + anim->ydim;
}



long BoundCrossed(float move, float offs, long bmin, long bmax) {
    if ((move < 0) && (offs + move <= bmin))
        return -1;
    if ((move > 0) && (offs + move >= bmax))
        return 1;
    return 0;
}



void ChooseDirection(ENGC *engc, PICT *pict) {
    BINF *bhvr = &pict->ulib->barr[pict->indx >> 1];
    float angl;
    long flag;

    if (!(bhvr->flgs & BHV_CTLM) && (bhvr->flgs & BHV_ALLM)) {
        flag = PRNG(&engc->seed) >> 3;
        switch (bhvr->flgs & BHV_ALLM) {
            /// horizontal + diagonal + vertical movement
            case BHV_ALLM:
                flag %= 3;
                flag = (flag)? (flag != 1)? BHV_HORM : BHV_DIAM : BHV_VERM;
                break;

            /// horizontal + vertical movement
            case BHV_HNVM:
                flag = (flag & 1)? BHV_HORM : BHV_VERM;
                break;

            /// horizontal + diagonal movement
            case BHV_HNDM:
                flag = (flag & 1)? BHV_HORM : BHV_DIAM;
                break;

            /// diagonal + vertical movement
            case BHV_DNVM:
                flag = (flag & 1)? BHV_DIAM : BHV_VERM;
                break;

            /// separate movement
            default:
                flag = bhvr->flgs;
                break;
        }
        switch (flag) {
            case BHV_DIAM:
                flag = (((bhvr->flgs & BHV_HNVM) == BHV_HNVM) ||
                        !(bhvr->flgs & BHV_HNVM))? 61 : 31;
                angl =  ((bhvr->flgs & BHV_HNVM) == BHV_VERM)? 45.0 : 15.0;
                angl = (angl + ((PRNG(&engc->seed) >> 3) % flag)) * DTR_CONV;
                break;

            case BHV_VERM:
                angl = 0.5 * M_PI;
                break;

            default:
                angl = 0.0;
                break;
        }
        pict->move = (T2FV){{cosf(angl), sinf(angl)}};
    }
    else
        pict->move = (T2FV){{0.0, 0.0}};

    pict->move.x *= bhvr->move;
    pict->move.y *= bhvr->move;

    pict->indx = (pict->indx & -2) | ((PRNG(&engc->seed) >> 3) & 1);
    if (!bhvr->unit[pict->indx & 1].uuid)
        pict->indx ^= 1;
    if (pict->indx & 1)
        pict->move.x = -pict->move.x;
    if ((PRNG(&engc->seed) >> 3) & 1)
        pict->move.y = -pict->move.y;
}



void ChooseBehaviour(ENGC *engc, PICT *pict) {
    LINF *ulib = pict->ulib;
    long seed, lbgn, lend;

    if (ulib->barr[pict->indx >> 1].link)
        pict->indx = (ulib->barr[pict->indx >> 1].link - 1) << 1;
    else {
        seed = ulib->barr[pict->indx >> 1].igrp;
        lbgn = (seed)? ulib->ngrp[seed - 1] : 0;
        lend = ulib->ngrp[seed];
        seed = PRNG(&engc->seed) % ulib->bgrp[lend - 1]->prob;

        /// nonexact binary search: finding the first item greater than SEED;
        /// bsearch() won`t help here, so let`s reinvent the wheel
        while (lbgn < lend)
            if (ulib->bgrp[(lend + lbgn) >> 1]->prob <= seed)
                lbgn = (lend + lbgn + 2) >> 1;
            else
                lend = (lend + lbgn + 0) >> 1;
        pict->indx = (ulib->bgrp[lbgn] - ulib->barr) << 1;
    }
    ChooseDirection(engc, pict);
}



long TruncateAndSort(BINF **base, long *rcnt) {
    BINF *root = *base, *iter = root - 1;
    long indx;

    for (indx = 0; indx < *rcnt; indx++)
        if ((root[indx].unit[0].uuid | root[indx].unit[1].uuid)
        &&  (++iter != &root[indx]))
            *iter = root[indx];

    if ((indx = iter - root + 1) < *rcnt)
        *base = realloc(root, (*rcnt = indx) * sizeof(*root));
    if (*rcnt)
        qsort(*base, *rcnt, sizeof(**base), namecmp);
    return *rcnt;
}



void FreeSpriteArr(ENGC *engc) {
    long iter;

    for (iter = 0; iter < engc->pcnt; iter++)
        free(engc->parr[iter]);
    free(engc->parr);
    engc->parr = 0;
    engc->pcnt = 0;
}



void AppendSpriteArr(LINF *elem, ENGC *engc) {
    BINF temp, *iter;
    long indx, turn;

    if (elem->bimp || elem->eimp) {
        turn = 0;
        indx = elem->bcnt * 2;
        while (indx && !turn)
            turn |= !!elem->bimp[--indx];
        indx = elem->ecnt * 2;
        while (indx && !turn)
            turn |= !!elem->eimp[--indx];

        /// skip the lib if it`s got animations pending upload
        if (turn)
            return;

        /// string arrays exist, but are empty? we`ve got an unprepared lib!
        free(elem->bimp);
        free(elem->eimp);
        elem->bimp = elem->eimp = 0;

        /// sorting all behaviours and effects by name hash
        TruncateAndSort(&elem->earr, &elem->ecnt);
        if (!TruncateAndSort(&elem->barr, &elem->bcnt)) {
            /// freeing the library in case it`s got no behaviours
            if (elem->prev)
                elem->prev->next = elem->next;
            if (elem->next)
                elem->next->prev = elem->prev;
            if (elem == engc->libs)
                engc->libs = (LINF*)((elem->prev)? elem->prev : elem->next);
            FreeLib(elem);
            return;
        }
        /// determining the effect count and min. index for every behaviour
        /// (note that some behaviours may have no effects)
        for (indx = elem->ecnt - 1; indx >= 0; indx--)
            if ((iter = bsearch(&elem->earr[indx], elem->barr, elem->bcnt,
                                sizeof(*elem->barr), namecmp))) {
                iter->ieff = indx;
                iter->neff++;
            }
        /// iterating over all behaviours
        for (indx = 0; indx < elem->bcnt; indx++) {
            /// resolving the linked behaviour, if any
            if (elem->barr[indx].link) {
                temp.name = elem->barr[indx].link;
                if ((iter = bsearch(&temp, elem->barr, elem->bcnt,
                                    sizeof(*elem->barr), namecmp)))
                    elem->barr[indx].link = iter - elem->barr + 1;
                else
                    elem->barr[indx].link = 0;
            }
            /// resolving image centers if unset (only possible here!)
            for (iter = &elem->barr[indx], turn = 0; turn <= 1; turn++)
                if (!(iter->cntr[turn].x | iter->cntr[turn].y))
                    iter->cntr[turn] = (T2IV){{iter->unit[turn].xdim >> 1,
                                               iter->unit[turn].ydim >> 1}};
        }

        /// populating BGRP with nonzero-probability behaviours, ZCNT := len
        elem->bgrp = malloc(elem->bcnt * sizeof(*elem->bgrp));
        for (elem->zcnt = indx = 0; indx < elem->bcnt; indx++)
            if (elem->barr[indx].prob)
                elem->bgrp[elem->zcnt++] = &elem->barr[indx];
        elem->bgrp = realloc(elem->bgrp, elem->zcnt * sizeof(*elem->bgrp));
        /// sorting BGRP by group number
        qsort(elem->bgrp, elem->zcnt, sizeof(*elem->bgrp), igrpcmp);

        /// counting groups + normalizing their indices to [0; GCNT) interval
        for (elem->gcnt = turn = indx = 0; indx < elem->zcnt; indx++) {
            if (elem->bgrp[indx]->igrp != turn) {
                turn = elem->bgrp[indx]->igrp;
                elem->gcnt++;
            }
            elem->bgrp[indx]->igrp = elem->gcnt;
        }
        elem->gcnt++;

        /// filling NGRP with behaviour group boundaries
        elem->ngrp = calloc(elem->gcnt, sizeof(*elem->ngrp));
        for (turn = indx = 0; indx < elem->zcnt; indx++)
            elem->ngrp[elem->bgrp[indx]->igrp] = indx + 1;
        /// turning probabilities into integral probabilities
        for (turn = indx = 0; indx < elem->gcnt; indx++) {
            for (++turn; turn < elem->ngrp[indx]; ++turn)
                elem->bgrp[turn]->prob += elem->bgrp[turn - 1]->prob;
            turn = elem->ngrp[indx];
        }
    }

    if (!elem->icnt)
        return;

    engc->pcnt += elem->icnt;
    engc->parr = realloc(engc->parr, engc->pcnt * sizeof(*engc->parr));

    for (indx = engc->pcnt - elem->icnt; indx < engc->pcnt; indx++) {
        engc->parr[indx] = calloc(1, sizeof(*engc->parr[indx]));
        engc->parr[indx]->ulib = elem;
        ChooseBehaviour(engc, engc->parr[indx]);
        PlaceRandomly(engc, engc->parr[indx]);
    }
    elem->icnt = 0;
}



void MMH(MENU *item) {
    switch (item->uuid) {
        case TXT_CDEL:
            break;

        case TXT_ADEL:
            break;

        case TXT_CSLP:
            break;

        case TXT_ASLP:
            break;

        case TXT_PONY:
            break;

        case TXT_HOUS:
            break;

        case TXT_TPL1:
            break;

        case TXT_TPL2:
            break;

        case TXT_OPTS:
            break;

        case TXT_RETN:
            break;

        case TXT_RGPU: {
            ENGC *engc = (ENGC*)item->data;
            uint32_t flgs;

            cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= COM_RGPU;
            else
                flgs &= ~COM_RGPU;
            cEngineCallback(engc->engd, ECB_SFLG, flgs);
            break;
        }
        case TXT_SHOW:
        case TXT_DRAW:
        case TXT_OPAQ: {
            ENGC *engc = (ENGC*)item->data;
            uint32_t flag = (item->uuid != TXT_OPAQ)? (item->uuid != TXT_DRAW)?
                             COM_SHOW : COM_DRAW : COM_OPAQ, flgs;

            cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= flag;
            else
                flgs &= ~flag;
            cEngineCallback(engc->engd, ECB_SFLG, flgs);
            break;
        }
        case TXT_EXIT:
            cEngineCallback(((ENGC*)item->data)->engd, ECB_QUIT, ~0);
            break;
    }
}



ulong LoadLocalization(char ***text, char *data, ulong size) {
    long line, nlin, iter, prev;
    char **retn;

    if (!data)
        return 0;

    /// skipping byte-order mark
    if ((size >= 3)
    && ((uint8_t)data[0] == 0xEF)
    && ((uint8_t)data[1] == 0xBB)
    && ((uint8_t)data[2] == 0xBF)) {
        data += 3;
        size -= 3;
    }
    if (*text)
        for (nlin = 0; (*text)[nlin]; nlin++);
    else {
        for (nlin = 2, iter = 0; iter < size; iter++)
            if (data[iter] == '\n')
                nlin++;
        *text = calloc(nlin, sizeof(*retn));
    }
    retn = *text;
    for (line = iter = prev = 0; (line < nlin) && (iter < size); iter++)
        if ((data[iter] == '\n') || (iter == size - 1)) {
            if (iter - ((data[prev] == '\r')? 1 : 0) > prev) {
                free(retn[line]);
                retn[line] = calloc(iter - prev + 1, sizeof(**retn));
                strncpy((char*)retn[line], (char*)&data[prev],
                        iter - prev - ((data[iter - 1] == '\r')? 1 : 0));
            }
            line++;
            prev = iter + 1;
        }
    return nlin;
}



void FreeLocalization(char ***text) {
    long iter = -1;

    if (text && *text) {
        while ((*text)[++iter])
            free((*text)[iter]);
        free(*text);
        *text = 0;
    }
}



void eProcessMenuItem(MENU *item) {
    MENU *indx;

    if (item->flgs & MFL_CCHK) {
        if (item->flgs & MFL_RCHK & ~MFL_CCHK) {
            if (item->flgs & MFL_VCHK)
                return;
            item->flgs |= MFL_VCHK;
            indx = item;
            while ((--indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
            indx = item;
            while ((++indx)->flgs & MFL_RCHK & ~MFL_CCHK)
                indx->flgs &= ~MFL_VCHK;
        }
        else
            item->flgs ^= MFL_VCHK;
    }
    if (!item->chld && item->func)
        item->func(item);
}



void FreeMenu(MENU **menu) {
    if (!menu || !*menu)
        return;

    MENU *iter = *menu;

    while (iter->text) {
        if (iter->chld)
            FreeMenu(&iter->chld);
        free(iter->text);
        iter++;
    }
    free(*menu - 1);
    *menu = 0;
}



void UpdateMenuItemText(MENU *item, char *text) {
    if (!item || !text)
        return;

    free(item->text);
    item->text = rConvertUTF8(text);
}



MENU *MenuFromTemplate(MENU *tmpl) {
    MENU *retn, *iter;

    retn = 0;
    if ((iter = tmpl)) {
        while (iter++->text);
        if (iter > tmpl + 1) {
            retn = calloc(iter - tmpl + 1, sizeof(*iter));
            iter = ++retn;
            do {
                *iter = *tmpl;
                if (tmpl->chld)
                    iter->chld = MenuFromTemplate(tmpl->chld);
                iter++->text = (tmpl->text)? rConvertUTF8(tmpl->text) : 0;
            } while (tmpl++->text);
        }
    }
    return retn;
}



uint32_t eUpdFlags(ENGD *engd, intptr_t user, uint32_t flgs) {
    ENGC *engc = (ENGC*)user;

    if (!(flgs & COM_RGPU) && (engc->mctx[3].flgs & MFL_VCHK)) {
        /// this happens because the engine cannot activate the GPU
        /// so the best reaction is to complain
        rMessage(engc->tran[TXT_CIGL], 0, 0);
    }
    #define FLAG(t, f) t = ((t) & ~MFL_VCHK) | ((flgs & (f))? MFL_VCHK : 0)
    FLAG(engc->mctx[3].flgs, COM_RGPU);
    FLAG(engc->mctx[4].flgs, COM_OPAQ);
    FLAG(engc->mctx[5].flgs, COM_DRAW);
    FLAG(engc->mctx[6].flgs, COM_SHOW);
    #undef FLAG

    return flgs;
}



uint32_t eUpdFrame(ENGD *engd, intptr_t user,
                   T4FV **data, uint64_t *time, uint32_t attr,
                   int32_t xptr, int32_t yptr, int32_t isel) {
    ENGC *engc = (ENGC*)user;
    PICT *pict = engc->pcur;
    BINF *binf;
    AINF *anim;

    char *temp;
    long  indx;
    uint64_t curr;

//    cEngineCallback(engd, ECB_LOAD, ~0);
//    /// here you can add new sprites!
//    cEngineCallback(engd, ECB_LOAD, 0);

    if ((attr & UFR_MOUS) && ((isel >= 0) || pict)) {
        if (!pict && ((engc->ppos.z ^ attr) & UFR_LBTN)) {
            pict = engc->pcur = engc->parr[isel];
            printf("[GRABBED] %s\n", pict->ulib->name);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
        }
        if (pict && (attr & UFR_LBTN)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict)
                printf("[DROPPED] %s\n", pict->ulib->name);
            engc->pcur = 0;
        }
        if (~attr & engc->ppos.z & UFR_RBTN) {
            if (!pict)
                pict = engc->parr[isel];
            temp = malloc(32 + strlen(pict->ulib->name));
            sprintf(temp, "[ %s ]", pict->ulib->name);
            UpdateMenuItemText(&engc->mspr[0], temp);
            free(temp);
            rOpenContextMenu(engc->mspr);
        }
        engc->ppos.z = attr;
    }
    for (indx = 0; indx < engc->pcnt; indx++) {
        curr = *time;
        pict = engc->parr[indx];
        binf = &pict->ulib->barr[pict->indx >> 1];
        anim = &binf->unit[pict->indx & 1];
        if ((curr >= pict->tbhv)
        || ((pict->fram >= anim->fcnt) && !(binf->flgs & FLG_LOOP))) {
            ChooseBehaviour(engc, pict);
            binf = &pict->ulib->barr[pict->indx >> 1];
            anim = &binf->unit[pict->indx & 1];
            pict->fram = -1;
            pict->tfrm = curr;
            pict->tbhv = curr + binf->dmin;
            if (binf->dmax > binf->dmin)
                pict->tbhv += PRNG(&engc->seed) % (binf->dmax - binf->dmin);
        }
        if (curr >= pict->tfrm) {
            pict->fram =
                (pict->fram + 1 >= anim->fcnt)? (binf->flgs & FLG_LOOP)?
                 0 : pict->fram : pict->fram + 1;
            pict->tfrm = curr + anim->time[pict->fram];
        }
        if (curr - pict->tmov >= FRM_WAIT) {
            if (BoundCrossed(pict->move.x, pict->offs.x + anim->xdim,
                             anim->xdim, engc->dims.x)) {
                pict->move.x = -pict->move.x;
                pict->indx ^= 1;
            }
            if (BoundCrossed(pict->move.y, pict->offs.y,
                             anim->ydim, engc->dims.y))
                pict->move.y = -pict->move.y;

            pict->offs.x += pict->move.x;
            pict->offs.y += pict->move.y;
            pict->tmov = curr;
        }
    }
    SortByY(engc);

    /// ALL EFFECTS GO HERE!!!

    for (indx = 0; indx < engc->pcnt; indx++) {
        pict = engc->parr[indx];
        anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
        engc->data[indx] = (T4FV){{pict->offs.x, pict->offs.y,
                                   pict->fram, anim->uuid}};
    }
    *data = engc->data;
    return engc->pcnt;
}



void eReallocEngine(ENGC **retn, char *lang) {
    INCBIN("../core/en.lang", DefaultLanguage);

    if (!retn)
        return;
    if (!*retn)
        *retn = calloc(1, sizeof(**retn));

    ENGC *engc = *retn;
    char *data;
    long size;

    FreeLocalization(&engc->tran);
    LoadLocalization(&engc->tran, DefaultLanguage, strlen(DefaultLanguage));
    if (lang) {
        data = rLoadFile(lang, &size);
        LoadLocalization(&engc->tran, data, size);
        free(data);
    }

    MENU *spec = 0, *temp = 0,

    mspr[] =
   {{.text = "",                   .flgs = MFL_GRAY},
    {.text = ""},
    {.text = engc->tran[TXT_CDEL], .uuid = TXT_CDEL, .func = MMH},
    {.text = engc->tran[TXT_ADEL], .uuid = TXT_ADEL, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_CSLP], .uuid = TXT_CSLP, .func = MMH},
    {.text = engc->tran[TXT_ASLP], .uuid = TXT_ASLP, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_PONY], .uuid = TXT_PONY, .func = MMH},
    {.text = engc->tran[TXT_HOUS], .uuid = TXT_HOUS, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_TPL1], .uuid = TXT_TPL1, .func = MMH},
    {.text = engc->tran[TXT_TPL2], .uuid = TXT_TPL2, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_OPTS], .uuid = TXT_OPTS, .func = MMH},
    {.text = engc->tran[TXT_RETN], .uuid = TXT_RETN, .func = MMH},
    {.text = engc->tran[TXT_EXIT], .uuid = TXT_EXIT, .func = MMH,
     .data = (intptr_t)engc},
    {}},

    mctx[] =
   {{.text = engc->tran[TXT_HEAD], .flgs = MFL_GRAY},
    {.text = ""},
    {.text = engc->tran[TXT_SPEC]},
    {.text = engc->tran[TXT_RGPU], .uuid = TXT_RGPU, .func = MMH,
     .flgs = MFL_CCHK, .data = (intptr_t)engc},
    {.text = engc->tran[TXT_OPAQ], .uuid = TXT_OPAQ, .func = MMH,
     .flgs = MFL_CCHK, .data = (intptr_t)engc},
    {.text = engc->tran[TXT_DRAW], .uuid = TXT_DRAW, .func = MMH,
     .flgs = MFL_CCHK, .data = (intptr_t)engc},
    {.text = engc->tran[TXT_SHOW], .uuid = TXT_SHOW, .func = MMH,
     .flgs = MFL_CCHK, .data = (intptr_t)engc},
    {.text = ""},
    {.text = engc->tran[TXT_EXIT], .uuid = TXT_EXIT, .func = MMH,
     .data = (intptr_t)engc},
    {}},

    none[] =
   {{.text = engc->tran[TXT_NONE], .flgs = MFL_GRAY},
    {}};

    if (engc->mspr) {
        spec = engc->mspr[8].chld; /// Add pony
        temp = engc->mspr[9].chld; /// Add house
        engc->mspr[8].chld = engc->mspr[9].chld = 0;
    }
    FreeMenu(&engc->mspr);
    FreeMenu(&engc->mctx);
    engc->mspr = MenuFromTemplate(mspr);
    engc->mspr[8].chld = spec;
    engc->mspr[9].chld = temp;
    engc->mctx = MenuFromTemplate(mctx);
    engc->mctx[2].chld = ((spec = rOSSpecificMenu(engc)))?
                           spec : MenuFromTemplate(none);
}



void eExecuteEngine(ENGC *engc, ulong xico, ulong yico, long xpos, long ypos,
                    ulong xdim, ulong ydim, uint32_t flgs) {
    INCBIN("../core/icon.gif", MainIcon);

    AINF igif = {};
    intptr_t icon;

    cEngineCallback(0, ECB_INIT, (intptr_t)&engc->engd);
    cEngineLoadAnimAsync(engc->engd,
                       (uint8_t*)"/Icon/", (uint8_t*)MainIcon, &igif);
    TTH_ITER(engc->libs, LoadTemplateFromLib, engc->engd);
    cEngineCallback(engc->engd, ECB_LOAD, 0);
    cEngineCallback(engc->engd, ECB_LOAD, ~0);

    engc->seed = time(0);
    printf("[((RNG))] seed = 0x%08X\n", engc->seed);

    igif.fcnt = 0;
    igif.xdim = xico;
    igif.ydim = yico;
    igif.time = calloc(sizeof(*igif.time), igif.xdim * igif.ydim);
    cEngineCallback(engc->engd, ECB_DRAW, (intptr_t)&igif);
    icon = rMakeTrayIcon(engc->mctx, engc->tran[TXT_HEAD],
                         igif.time, igif.xdim, igif.ydim);

    engc->dims = (T2IV){{xdim, ydim}};
    TTH_ITER(engc->libs, LoadLib, engc->engd);
    cEngineCallback(engc->engd, ECB_LOAD, 0);

    TTH_ITER(engc->libs, AppendSpriteArr, engc);
    engc->data = (engc->pcnt)? calloc(engc->pcnt, sizeof(*engc->data)) : 0;
    cEngineRunMainLoop(engc->engd, xpos, ypos, engc->dims.x, engc->dims.y,
                       flgs, FRM_WAIT, (intptr_t)engc, eUpdFrame, eUpdFlags);
    FreeSpriteArr(engc);
    free(engc->data);

    FreeMenu(&engc->mspr);
    FreeMenu(&engc->mctx);
    FreeLocalization(&engc->tran);
    rFreeTrayIcon(icon);
    TTH_ITER(engc->libs, FreeLib, 0);
    cEngineCallback(engc->engd, ECB_QUIT, 0);
    *engc = (ENGC){};
}



void __DEL_ME__SetLibUses(ENGC *engc, int32_t uses) {
    LINF *libs = engc->libs;
    while (libs) {
        libs->icnt = labs(uses);
        libs = (LINF*)libs->prev;
    }
}
