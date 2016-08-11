#include <stdarg.h>
#include <limits.h>
#include "exec.h"

/// a macro to count the capacity of static arrays
#define countof(a) (sizeof(a) / sizeof(*(a)))

/// FE2C / FC2E helper macros
#define RUN_FE2C(trgt, cmsg, data) trgt.fe2c(&trgt, cmsg, data)
#define RUN_FC2E(trgt, cmsg, data) trgt.fc2e(&trgt, cmsg, data)

/** convert degrees to radians  **/ #define DTR_CONV (M_PI / 180.0)
/** convert radians to degrees  **/ #define RTD_CONV (1.0 / DTR_CONV)

/** framerate limiter in msec   **/ #define FRM_WAIT 40

/** default comment character   **/ #define DEF_CMNT '\''
/** default token separator     **/ #define DEF_TSEP ','
/** default dir slash (string)  **/ #define DEF_DSEP "/"

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
/** [extractor]                 **/ #define BHV_MMMM (BHV_CTLM | BHV_ALLM)

/** can be executed at random   **/ #define BHV_EXEC (1 << 27)
/** [yet to be understood]      **/ #define BHV_____ (1 << 28)
/** tgt offs has to be mirrored **/ #define BHV_MIRR (1 << 29)

/// /// /// /// /// /// /// /// /// must stay as-is; these are used as indices
/** top-left alignment          **/ #define EFF_TNLA 0x0
/** top alignment               **/ #define EFF_TOPA 0x1
/** top-right alignment         **/ #define EFF_TNRA 0x2
/** center-left alignment       **/ #define EFF_CNLA 0x3
/** center alignment            **/ #define EFF_CNTA 0x4
/** center-right alignment      **/ #define EFF_CNRA 0x5
/** bottom-left alignment       **/ #define EFF_BNLA 0x6
/** bottom alignment            **/ #define EFF_BTMA 0x7
/** bottom-right alignment      **/ #define EFF_BNRA 0x8
/** random alignment            **/ #define EFF_RNDA 0x9
/** random centerless alignment **/ #define EFF_RCLA 0xA
/** [extractor]                 **/ #define EFF_AAAA 0xF

/** do not follow parent        **/ #define EFF_STAY (1 << 29)
/** animation can be looped     **/ #define FLG_LOOP (1 << 30)
/** this item is an effect      **/ #define FLG_EFCT (1 << 31)

/// /// /// /// /// /// /// /// /// follow offset type values
/** 'fixed'                     **/ #define FOT_FIXD 0x9A8F97BD
/** 'mirror'                    **/ #define FOT_MIRR 0x304E7075

/// /// /// /// /// /// /// /// /// config file strings
/** 'language'                  **/ #define CNF_LANG 0x1644959C
/** 'time'                      **/ #define CNF_TIME 0x8487AD87
/** 'flags'                     **/ #define CNF_FLGS 0x8ACE03CE
/** 'render'                    **/ #define CNF_RNDR 0x3C9F6676
/** 'effects'                   **/ #define CNF_EFCT 0xAB1F60DF
/** 'draw'                      **/ #define CNF_DRAW 0xE7ABD6EE
/** 'show'                      **/ #define CNF_SHOW 0x27D90DCD
/** 'gpu'                       **/ #define CNF_RGPU 0x11927E83
/** 'opaque'                    **/ #define CNF_OPAQ 0xD246CFE1
/** 'wbgra'                     **/ #define CNF_IBGR 0xABF3B1E8
/** 'wpbo'                      **/ #define CNF_IPBO 0x78FE3880
/** 'wregion'                   **/ #define CNF_IRGN 0xDE0DCCBE

/// /// /// /// /// /// /// /// /// client specific flags
/** behaviour effects are on    **/ #define CSF_EFCT (1 <<  0)

/// /// /// /// /// /// /// /// /// localized text constants
/** Remove character            **/ #define TXT_CDEL  0
/** Remove all similar          **/ #define TXT_ADEL  1
/** Sleep/pause                 **/ #define TXT_CSLP  2
/** Sleep/pause all similar     **/ #define TXT_ASLP  3
/** Add character >>>           **/ #define TXT_CHAR  4
/** Add house >>>               **/ #define TXT_HOUS  5
/** Take control: Player 1      **/ #define TXT_TPL1  6
/** Take control: Player 2      **/ #define TXT_TPL2  7
/** Show options...             **/ #define TXT_OPTS  8

/** [ Desktop Ponies Engine ]   **/ #define TXT_HEAD  9
/** OS specific options         **/ #define TXT_SPEC 10
/** Disable transparency        **/ #define TXT_OPAQ 11
/** Play animation              **/ #define TXT_DRAW 12
/** Show window                 **/ #define TXT_SHOW 13
/** Exit                        **/ #define TXT_EXIT 14
/** Use GPU for drawing         **/ #define TXT_RGPU 15
/** [ none ]                    **/ #define TXT_NONE 16

/** Show console                **/ #define TXT_CONS 17
/** Use regions                 **/ #define TXT_IRGN 18
/** Enable BGRA                 **/ #define TXT_IBGR 19
/** Enable pixel buffers        **/ #define TXT_IPBO 20
/** Useless on full opacity!    **/ #define TXT_UOFO 21
/** Useless without GPU!        **/ #define TXT_UWGL 22
/** Cannot initialize GPU!      **/ #define TXT_CIGL 23

/** Desktop Ponies              **/ #define TXT_CAPT 24
/** Enable filters              **/ #define TXT_FLTR 25
/** Exact matching              **/ #define TXT_EXAC 26
/** [At least one:]             **/ #define TXT_OGRP 27
/** [All at once:]              **/ #define TXT_AGRP 28
/** Random selection:           **/ #define TXT_SRND 29
/** Group selection:            **/ #define TXT_SGRP 30
/** Add                         **/ #define TXT_BADD 31
/** Copies                      **/ #define TXT_BDUP 32
/** Selected:                   **/ #define TXT_SELE 33
/** Loaded:                     **/ #define TXT_LOAD 34
/** GO!                         **/ #define TXT_GOGO 35

/// /// /// /// /// /// /// /// /// ENGC.CTLS array indices
/**                             **/ #define CTL_CAPT ctls[ 0]
/**                             **/ #define CTL_FLTR ctls[ 1]
/**                             **/ #define CTL_EXAC ctls[ 2]
/**                             **/ #define CTL_OGRP ctls[ 3]
/**                             **/ #define CTL_SGRP ctls[ 4]
/**                             **/ #define CTL_SPEC ctls[ 5]
/**                             **/ #define CTL_BADD ctls[ 6]
/**                             **/ #define CTL_SRND ctls[ 7]
/**                             **/ #define CTL_RGPU ctls[ 8]
/**                             **/ #define CTL_BDUP ctls[ 9]
/**                             **/ #define CTL_SELE ctls[10]
/**                             **/ #define CTL_OPTS ctls[11]
/**                             **/ #define CTL_GOGO ctls[12]
/**                             **/ #define CTL_CHAR ctls[13]

/// behaviour/effect unit info (write-once, read-only)
typedef struct _BINF {
    AINF  unit[2];  /// image pair
    T2IV  cntr[2];  /// image centers
    T2IV  ptgt;     /// follow target relative coords
    long  prob,     /// probability, 0-1000 (may become integral)
          dmin,     /// minimum duration in msec
          dmax,     /// maximum duration / respawn in msec
          neff,     /// number of linked effects
          ieff,     /// effect array index of the first linked effect
          igrp;     /// behaviour group index
    float move;     /// movement speed in pixels per frame
    uint32_t name,  /// name hash for behaviour / target for effect
             flgs,  /// behaviour / effect flags
             link,  /// linked behaviour index
             trgt;  /// follow target name hash
} BINF;

/// library categories (name string and its hash)
typedef struct _CTGS {
    uint32_t hash, flgs;
    char    *name;
} CTGS;

/// engine data (client side), prototype
typedef struct ENGC ENGC;

/// unit library info (write-once, read-only), opaque outside the module
/// [TODO:] (priority 1) behaviours
/// [TODO:] (priority 2) interactions
/// [TODO:] (priority 3) speech
typedef struct _LINF {
    CTRL     pict,  /// image box control to preview the sprite
             capt,  /// character name just below the image box
             spin;  /// spin control to set ICNT
    ENGC    *engc;  /// parent engine
    CTGS    *ctgs;  /// library categories (hashed and sorted)
    BINF    *barr,  /// available behaviours ordered by name hash
            *earr,  /// available effects ordered by parent bhv. name hash
           **bgrp;  /// nonzero-probability BARR elements ordered by bhv. group
    char   **bimp,  /// image paths for behaviours (0 if loaded)
           **eimp,  /// image paths for effects (0 if loaded)
            *path,  /// the folder from which the library was built
            *name;  /// human-readable name (may differ from PATH!)
    long    *ngrp,  /// bounds of behaviour groups in BGRP: [0~~)[G0~~~)[G1...
             prev,  /// preview index in sorted BARR
             ccnt,  /// categories count
             gcnt,  /// behaviour groups count
             zcnt,  /// nonzero probability behaviours count
             bcnt,  /// total behaviours count
             ecnt,  /// total effects count
             icnt;  /// number of on-screen bhv. sprites from the library
} LINF;

/// actual on-screen sprite, opaque outside the module (TMR_PAIR: parity flag)
#define TMR_PAIR (1ULL << 63)
typedef struct _PICT {
    struct
    _PICT   *next,  /// linked list support for SortByY(), only used there
            *boss;  /// the parent sprite (e.g. follow target or effect base)
    LINF    *ulib;  /// unit library which the sprite belongs to
    T2FV     move,  /// movement direction (or parent offset for effects)
             offs;  /// position of the unit`s lower-left corner
    uint32_t indx,  /// behaviour index, direction (lowest bit), effect flag
             fram;  /// current frame
                    /// timestamps; all are given in msec
    uint64_t tfrm,  /// BHV: next frame
                    /// EFF: next frame
             tmov,  /// BHV: next movement (no flags allowed, see SortByY)
                    /// EFF: next respawn (LLONG_MAX if already respawned,
                    ///                    highest bit = parity flag)
                    ///      parity enabled means "inactive sprite, skip it"
             tbhv;  /// BHV: next behaviour (highest bit = parity flag)
                    ///      parity is inverted on behaviour change
                    /// EFF: expected deletion (highest bit = parity flag)
                    ///      prohibit respawn if parity differs
} PICT;

/// engine data (client side)
struct ENGC {
    MENU    *mspr,  /// per-sprite context menu
            *mctx;  /// engine`s main context menu
    CTRL    *ctls;  /// GUI controls array
    T4FV    *data;  /// main display sequence passed to the renderer
    ENGD    *engd;  /// rendering engine handle
    LINF    *libs;  /// sprite libraries linked list
    CTGS    *ctgs;  /// categories array
    PICT    *pcur,  /// the sprite currently picked
           **parr;  /// on-screen sprite pointers array
    char   **tran,  /// localized text array (ASCIIZ; last item is also 0)
            *lang,  /// name of the loaded language file
            *conf;  /// name of the main configuration file
    uint32_t lcnt,  /// libraries count
             ccnt,  /// categories count
             pcnt,  /// on-screen sprites count (may differ every frame)
             pmax,  /// max. PARR capacity (realloc on exceed)
             seed,  /// random seed
             flgs,  /// client-specific flags, e.g. effects +/- (CSF_ prefix)
             ftmp;  /// temporary storage for engine flags
    float    tdil;  /// time dilation coeff (faster is > 1, slower is < 1)
    T3IV     ppos;  /// mouse pointer position (z = flags)
    T2IV     dpos,  /// drawing area position
             dims,  /// drawing area dimensions
             idim;  /// tray icon dimensions
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

    if (!line)
        return hash;
    if (!size)
        size--;
    while (*line && size--)
        hash = SLH_PLUS + SLH_MULT * hash + *line++;
    return hash;
}



static inline float ClampToBounds(float what, float bmin, float bmax) {
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



char *Concatenate(char **retn, ...) {
    va_list list;
    char *head, *temp;
    long size = 1;

    va_start(list, retn);
    while ((temp = va_arg(list, typeof(temp))))
        size += strlen(temp);
    va_end(list);

    if (!retn)
        head = calloc(1, size);
    else {
        head = *retn;
        head = realloc(head, size += (head)? strlen(head) : 0);
        if (!*retn)
            *head = 0;
        *retn = head;
    }

    va_start(list, retn);
    while ((temp = va_arg(list, typeof(temp))))
        strcat(head, temp);
    va_end(list);

    return head;
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
            if (line[1] == '\xA0') return ~0; else break;
        case '\xE1':
            if (line[1] == '\x9A' && line[2] == '\x80') return ~0; else break;
        case '\xE3':
            if (line[1] == '\x80' && line[2] == '\x80') return ~0; else break;
        case '\xEF':
            if (line[1] == '\xBB' && line[2] == '\xBF') return ~0; else break;
        case '\xE2':
            switch (line[1]) {
                case '\x81': if (line[2] == '\x9F') return ~0; else break;
                case '\x80':
                    switch (line[2]) {
                        case '\x80': case '\x81': case '\x82': case '\x83':
                        case '\x84': case '\x85': case '\x86': case '\x87':
                        case '\x88': case '\x89': case '\x8A': case '\x8B':
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
    return 0;
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



int igrpcmp(const void *a, const void *b) {
    int64_t retn = (int64_t)(*(BINF**)b)->igrp - (int64_t)(*(BINF**)a)->igrp;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int namecmp(const void *a, const void *b) {
    int64_t retn = (int64_t)((BINF*)b)->name - (int64_t)((BINF*)a)->name;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int uintcmp(const void *a, const void *b) {
    int64_t retn = (int64_t)*(uint32_t*)a - (int64_t)*(uint32_t*)b;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int ctgscmp(const void *a, const void *b) {
    int64_t retn = (int64_t)((CTGS*)a)->hash - (int64_t)((CTGS*)b)->hash;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

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
        dest[iter] = Concatenate(0, path, DEF_DSEP,
                                 Dequote(SplitLine(conf, DEF_TSEP, 0)), 0);
}

void ParseBehaviour(BINF *retn, char *path, char **imgp, char **conf) {
    static uint32_t
        uBMT[] = {BMT_HORM, BMT_ALLM, BMT_VERM, BMT_DRGM, BMT_HNDM, BMT_SLPM,
                  BMT_HNVM, BMT_DIAM, BMT_OVRM, BMT_DNVM, BMT_NONM},
        uBHV[] = {BHV_HORM, BHV_ALLM, BHV_VERM, BHV_DRGM, BHV_HNDM, BHV_SLPM,
                  BHV_HNVM, BHV_DIAM, BHV_OVRM, BHV_DNVM, BHV_NONM};
    uint32_t elem, *iter;
    char *temp;

    /// defaults         (neff, ieff, igrp)----v--v--v
    *retn = (BINF){{}, {}, {}, 0, 5000, 15000, 0, 0, 0, 0.1 * FRM_WAIT, 0,
                   BHV_ALLM | BHV_EXEC | BHV_____ | FLG_LOOP, 0, 0};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// probability of this behaviour..........................................  def = 0
    if (TRY_TEMP(conf))
        retn->prob = ClampToBounds(StrToFloat(temp) * 1000.0, 0.0, 1000.0);

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
    MakeSpritePair(imgp, path, conf);

    /// possible movement directions...........................................  def = All
    IF_BIN_FIND(elem, uBMT, conf)
        retn->flgs = uBHV[elem - 1] | (retn->flgs & ~BHV_MMMM);

    /// linked behaviour name..................................................  def = ""
    if (TRY_TEMP(conf))
        retn->link = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// speech said on behaviour start.........................................  def = ""
    if (TRY_TEMP(conf)) {};

/// [TODO:]
    /// speech said on behaviour end...........................................  def = ""
    if (TRY_TEMP(conf)) {};

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
    if (TRY_TEMP(conf)) {};

/// [TODO:]
    /// [something unintelligible].............................................  def = ""
    if (TRY_TEMP(conf)) {};

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

void ParseEffect(BINF *retn, char *path, char **imgp, char **conf) {
    static uint32_t
        uEMT[] = {EMT_CNRA, EMT_RNDA, EMT_CNTA, EMT_CNLA, EMT_BTMA, EMT_BNLA,
                  EMT_TOPA, EMT_BNRA, EMT_RCLA, EMT_TNLA, EMT_TNRA},
        uEFF[] = {EFF_CNRA, EFF_RNDA, EFF_CNTA, EFF_CNLA, EFF_BTMA, EFF_BNLA,
                  EFF_TOPA, EFF_BNRA, EFF_RCLA, EFF_TNLA, EFF_TNRA};
    uint32_t elem, *iter;
    char *temp;

    /// defaults     (neff, ieff, igrp)----v--v--v
    *retn = (BINF){{}, {}, {}, 0, 5000, 0, 0, 0, 0, 0.0, 0,
                   FLG_EFCT | FLG_LOOP | EFF_STAY | (EFF_RNDA * 0x1111), 0, 0};

    /// effect name (skipped intentionally).................................... !def
    if (TRY_TEMP(conf)) {};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(imgp, path, conf);

    /// duration in sec........................................................  def = 5
    if (TRY_TEMP(conf))
        retn->dmin = StrToFloat(temp) * 1000.0;

    /// respawn in sec.........................................................  def = 0 (no respawn)
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

    /// flag to follow parent .................................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, EFF_STAY);

    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, FLG_LOOP);
}

void AppendLib(ENGC *engc, char *pcnf, char *base, char *path) {
    char *file, *fptr, *conf, *temp;
    long bcnt, ecnt, ccnt = 0;
    uint32_t hash;
    LINF *libs;
    CTGS *ctgs;

    fptr = Concatenate(0, base, DEF_DSEP, path, 0);
    conf = Concatenate(0, fptr, DEF_DSEP, pcnf, 0);
    if ((file = rLoadFile(conf, 0))) {
        free(conf);

        engc->libs = realloc(engc->libs, ++engc->lcnt * sizeof(*engc->libs));
        engc->libs[engc->lcnt - 1] = (LINF){};
        libs = &engc->libs[engc->lcnt - 1];
        libs->engc = engc;
        libs->path = fptr;

        fptr = file;
        libs->bcnt = libs->ecnt = 0;
        while ((conf = SplitLine(&fptr, '\n', 1)))
            switch (DetermineType(&conf)) {
                case SVT_BHVR:
                    libs->bcnt++;
                    break;

                case SVT_EFCT:
                    libs->ecnt++;
                    break;
            }

        if ((bcnt = libs->bcnt)) {
            libs->barr = calloc(bcnt,     sizeof(*libs->barr));
            libs->bimp = calloc(bcnt * 2, sizeof(*libs->bimp));
        }
        else {
            /// no behaviours found, the library is broken; stopping
            engc->lcnt--;
            free(libs->path);
            free(file);
            return;
        }
        if ((ecnt = libs->ecnt)) {
            libs->earr = calloc(ecnt,     sizeof(*libs->earr));
            libs->eimp = calloc(ecnt * 2, sizeof(*libs->eimp));
        }
        fptr = file;
        bcnt = ecnt = 0;
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case SVT_NAME:
                    libs->name = strdup(GET_TEMP(&conf));
                    break;

                case SVT_EFCT:
                    ParseEffect(&libs->earr[ecnt], libs->path,
                                &libs->eimp[ecnt * 2], &conf);
                    ecnt++;
                    break;

                case SVT_BHVR:
                    ParseBehaviour(&libs->barr[bcnt], libs->path,
                                   &libs->bimp[bcnt * 2], &conf);
                    bcnt++;
                    break;

                case SVT_BGRP:
                    /// doesn`t help much, skipping
                    break;

                case SVT_PHRS:
                    break;

                case SVT_CTGS:
                    while ((temp = ToLower(Dequote(GET_TEMP(&conf)), 0))) {
                        *temp = toupper(*temp); /// capitalizing first letter
                        hash = HashLine(temp, 0);
                        if (++libs->ccnt > ccnt)
                            libs->ctgs = realloc(libs->ctgs,
                                         sizeof(*libs->ctgs) * (ccnt += 32));
                        libs->ctgs[libs->ccnt - 1] =
                            (CTGS){hash, 0, Dequote(temp)};
                    }
                    break;
            }
        if (!libs->name)
            libs->name = strdup(path);
        if (libs->ctgs) {
            /// sorting categories, removing duplicates, truncating the memory
            qsort(libs->ctgs, libs->ccnt, sizeof(*libs->ctgs), ctgscmp);
            for (bcnt = ccnt = 1; ccnt < libs->ccnt; ccnt++)
                if (libs->ctgs[ccnt - 1].hash != libs->ctgs[ccnt].hash)
                    if (bcnt++ != ccnt)
                        libs->ctgs[bcnt - 1] = libs->ctgs[ccnt];
            if (libs->ccnt != bcnt)
                libs->ctgs = realloc(libs->ctgs,
                                    (libs->ccnt = bcnt) * sizeof(*libs->ctgs));
            /// now looking for categories previously unknown
            for (hash = ccnt = 0; ccnt < libs->ccnt; ccnt++)
                if ((ctgs = bsearch(&libs->ctgs[ccnt], engc->ctgs, engc->ccnt,
                                    sizeof(*engc->ctgs), ctgscmp)))
                    libs->ctgs[ccnt] = (CTGS){0, 0, ctgs->name};
                else
                    hash++;
            /// some categories need to be added to the global category base
            if (hash)
                engc->ctgs = realloc(engc->ctgs,
                                    (engc->ccnt += hash) * sizeof(*ctgs));
            ctgs = engc->ctgs + engc->ccnt - hash;
            for (ccnt = 0; ccnt < libs->ccnt; ccnt++)
                if (!libs->ctgs[ccnt].hash)
                    libs->ctgs[ccnt].hash = HashLine(libs->ctgs[ccnt].name, 0);
                else {
                    libs->ctgs[ccnt].name = strdup(libs->ctgs[ccnt].name);
                    *ctgs++ = libs->ctgs[ccnt];
                }
            if (hash)
                qsort(engc->ctgs, engc->ccnt, sizeof(*engc->ctgs), ctgscmp);
        }
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
    free(elem->ctgs); /// nothing to free inside CTGS:
                      /// category names were copied from the engine
    free(elem->path);
    free(elem->name);
    free(elem->barr);
    free(elem->earr);
    free(elem->bgrp);
    free(elem->ngrp);
}



void LoadLib(LINF *elem, ENGD *engd) {
    long iter, indx, ncnt;
    char **nimp;
    BINF *narr;

    if (!elem->bimp || (elem->icnt <= 0))
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



void LoadLibPreview(LINF *elem, ENGD *engd) {
    if (!elem->bimp || !elem->bimp[0])
        return;
    cEngineLoadAnimAsync(engd, (uint8_t*)elem->bimp[0], 0,
                        &elem->barr[0].unit[0]);
    free(elem->bimp[0]);
    elem->bimp[0] = 0;
}



void SortByY(ENGC *engc) {
    #define MAX_YDIM 0x1000
    long ymin, ymax, ytmp, iter;
    PICT *temp, *elem[MAX_YDIM + 1];

    if (engc->pcnt < 2)
        return;

    /// manual stack checking
    for (ymax = sizeof(*elem) - 1; ymax >= 0; ymax--) {
        elem[ymax * MAX_YDIM / sizeof(*elem)] = 0;
        asm volatile("" ::: "memory");
    }
    memset(elem, 0, (MAX_YDIM + 1) * sizeof(*elem));

    ymin = MAX_YDIM + 1;
    ymax = -1;

    for (iter = 0; iter < engc->pcnt; iter++)
        if ((temp = engc->parr[iter])) {
            ytmp = temp->offs.y;
            if ((ytmp >= 0) && (ytmp < MAX_YDIM)) {
                if (ytmp > ymax)
                    ymax = ytmp;
                else if (ytmp < ymin)
                    ymin = ytmp;
            }
        }
    for (iter = engc->pcnt - 1; iter >= 0; iter--)
        if (!(temp = engc->parr[iter]))
            engc->pcnt--;
        else {
            ytmp = temp->offs.y - ymin + 1;
            if ((ytmp < 0) || (temp->offs.y >= MAX_YDIM)
            || (temp->tmov & TMR_PAIR)) /// reserved ones go to the very end
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



/// SEED = 0 means "keep centering and placement"
void MoveToParent(PICT *pict, uint32_t *seed) {
    static float
        xdim[] = {0.0, 0.5, 1.0, 0.0, 0.5, 1.0, 0.0, 0.5, 1.0, -1.0, -1.0},
        ydim[] = {1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, -1.0, -1.0};
    long iter, desc;
    AINF *anim[2];
    T2FV vect[2];

    if (seed) {
        /// parent behaviour animation
        anim[0] = &pict->boss->ulib->barr[pict->boss->indx >> 1].
                                     unit[pict->boss->indx & 1];
        /// effect animation
        anim[1] = &pict->ulib->earr[(pict->indx & ~FLG_EFCT) >> 1].
                               unit[pict->indx & 1];
        /// read centering and placement for the current effect
        desc = pict->ulib->earr[(pict->indx & ~FLG_EFCT) >> 1].flgs
             >> ((pict->indx & 1) << 3); /// direction was inherited from BOSS
        #define CALC(d) ((d[desc & EFF_AAAA] >= 0.0)? d[desc & EFF_AAAA]      \
                       : (float)((PRNG(seed) >> 4) & 0xFFF) * 0.000244140625) \
                       * (float)anim[iter]->d /* (1 / 4096)---^^^^^^^^^^^^^^ */
        /// ANIMs are only needed for dimensions
        for (iter = 0; iter < 2; iter++, desc >>= 4)
            vect[iter] = (T2FV){{CALC(xdim), CALC(ydim)}};
        #undef CALC
        /// now calculate the resulting offset
        pict->move = (T2FV){{vect[0].x - vect[1].x, vect[0].y - vect[1].y}};
    }
    pict->offs = (T2FV){{pict->boss->offs.x + pict->move.x,
                         pict->boss->offs.y - pict->move.y}};
}



long BoundCrossed(float move, float offs, long bmin, long bmax) {
    if ((move < 0) && (offs + move <= bmin))
        return -1;
    if ((move > 0) && (offs + move >= bmax))
        return 1;
    return 0;
}



void ChooseDirection(ENGC *engc, PICT *pict) {
    if (pict->indx & FLG_EFCT)
        return;

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
        pict->move = (T2FV){{cos(angl), sin(angl)}};
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



/// INDX is not used if FROM is an effect, as all we need is stored there
long SpawnEffect(PICT **retn, PICT *from, uint32_t *seed,
                 ulong indx, uint64_t time) {
    BINF *binf;

    if (from->tmov == LLONG_MAX)
        return 0;  /// no more self-replicating for this effect; exiting

    if (~from->indx & FLG_EFCT) {
        /// parent = behaviour
        *retn = calloc(1, sizeof(**retn));
        (*retn)->boss = from;
        (*retn)->ulib = from->ulib;
        (*retn)->indx = (indx << 1) | (from->indx & 1) | FLG_EFCT;
    }
    else if (*retn != from) {
        /// parent = effect that needs to be copied to RETN
        *retn = calloc(1, sizeof(**retn));
        **retn = *from;
        from->tmov = LLONG_MAX; /// only one self-replication allowed
    }
    /// ^^^^ along these there is a third way, when an effect gets reused,
    /// and no new effects are created; this keeps sprite count in bounds

    binf = &(*retn)->ulib->earr[((*retn)->indx & ~FLG_EFCT) >> 1];
    (*retn)->fram = -1; /// this means:
    (*retn)->tfrm =  0; /// "update me to 0-th frame ASAP!"
    /// effect respawn time; DMAX = 0 means "do not respawn"
    (*retn)->tmov = (binf->dmax)? time + binf->dmax : LLONG_MAX;
    /// effect ending time; DMIN = 0 means "end when the behaviour ends"
    /// if the effect duration is longer than that of the behaviour, and
    /// also it doesn`t respawn, then trim it
    (*retn)->tbhv = (*retn)->boss->tbhv;
    if (binf->dmin &&
       (binf->dmax || (time + binf->dmin < ((*retn)->tbhv & ~TMR_PAIR))))
        (*retn)->tbhv = (time + binf->dmin) | ((*retn)->tbhv & TMR_PAIR);
    /// re-adjusting turn, as it may have changed
    (*retn)->indx = ((*retn)->indx & ~1) | ((*retn)->boss->indx & 1);
    MoveToParent(*retn, seed);
    return ~0;
}



void ChooseBehaviour(ENGC *engc, PICT *pict, uint64_t time, uint32_t attr) {
    long seed, lbgn, lend;
    LINF *ulib = pict->ulib;
    BINF *binf;
    AINF *anim;

    if (pict->indx & FLG_EFCT)
        return;

    if (!attr) {
        if (ulib->barr[pict->indx >> 1].link)
            pict->indx = (ulib->barr[pict->indx >> 1].link - 1) << 1;
        else {
            seed = ulib->barr[pict->indx >> 1].igrp;
            lbgn = (seed)? ulib->ngrp[seed - 1] : 0;
            lend = ulib->ngrp[seed];
            seed = PRNG(&engc->seed) % ulib->bgrp[lend - 1]->prob;

            /// nonexact binary search: bsearch() won`t help here
            ///  0th -----,  1st -,  2nd ----,   <  these are the elements
            /// [0;       5)[5;   7)[7;     10)  <  this is the very array
            ///       |      |             `---- SEED = 9: 2nd element
            ///       |      `---- SEED = 5: 1st element
            ///       `---- SEED = 3: 0th element
            while (lbgn < lend)
                if (ulib->bgrp[(lend + lbgn) >> 1]->prob <= seed)
                    lbgn = (lend + lbgn + 2) >> 1;
                else
                    lend = (lend + lbgn + 0) >> 1;
            pict->indx = (ulib->bgrp[lbgn] - ulib->barr) << 1;
        }
    }
    else {
        /// compared to the search above, these are excruciatingly slow;
        /// however, we can afford it since any of them may be executed
        /// once per frame at most
        if (attr & UFR_LBTN) {
            return; /// [TODO:] del me
        }
        if (attr & UFR_MOUS) {
            /// no point in updating the behaviour if it`s already current
            if (((ulib->barr[pict->indx >> 1].flgs & BHV_OVRM) == BHV_OVRM)
            ||  !(ulib->barr[pict->indx >> 1].flgs & BHV_ALLM))
                return;
            return; /// [TODO:] del me
        }
        return;
    }
    binf = &pict->ulib->barr[pict->indx >> 1];
    pict->fram = -1; /// this means:
    pict->tfrm =  0; /// "update me to 0-th frame ASAP!"
    pict->tbhv = (time + binf->dmin) | (~pict->tbhv & TMR_PAIR);
    if (binf->dmax > binf->dmin)
        pict->tbhv += PRNG(&engc->seed) % (binf->dmax - binf->dmin);
    if (!time) {
        /// this is the first time this sprite appears; let`s put it somewhere
        anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
        pict->offs.x = PRNG(&engc->seed) % (engc->dims.x - anim->xdim);
        pict->offs.y = PRNG(&engc->seed) % (engc->dims.y - anim->ydim)
                                                         + anim->ydim;
    }
    ChooseDirection(engc, pict);
    /// now spawning effects for the behaviour, if any
    /// (and only if this is not the first spawn and effects are enabled)
    if (time && (engc->flgs & CSF_EFCT)) {
        lbgn = pict->ulib->barr[pict->indx >> 1].ieff - 1;
        for (lend = pict->ulib->barr[pict->indx >> 1].neff; lend; lend--)
            SpawnEffect(&engc->parr[engc->pcnt++], pict,
                        &engc->seed, lbgn + lend, time);
    }
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



/// return values:
///  -1: got animations pending upload
///   0: no behaviours found, library freed
///   1: all good, 0 sprites on the screen
///  >1: all good, (return) - 1 sprites on the screen
long AppendSpriteArr(LINF *elem, ENGC *engc) {
    long indx, turn, qmax;
    BINF temp, *iter;

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
            return -1;

        /// string arrays exist, but are empty? we`ve got an unprepared lib!
        free(elem->bimp);
        free(elem->eimp);
        elem->bimp = elem->eimp = 0;

        /// saving preview name hash
        temp.name = elem->barr[elem->prev].name;
        /// sorting all behaviours and effects by name hash
        TruncateAndSort(&elem->earr, &elem->ecnt);
        if (!TruncateAndSort(&elem->barr, &elem->bcnt)) {
            /// freeing the library in case it`s got no behaviours
            FreeLib(elem);
            *elem = (LINF){};
            return 0;
        }
        /// retrieving the new preview position
        if ((iter = bsearch(&temp, elem->barr, elem->bcnt,
                            sizeof(*elem->barr), namecmp)))
            elem->prev = iter - elem->barr;
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

    if (elem->icnt <= 0)
        return 1;

    /// now computing Q, the maximum number of effect spawns per behaviour
    /// (Q = 1 when repeat delay D = 0; otherwise, Q = ceil(((E)? E : B) / D)
    /// where E is effect duration and B is maximum behaviour duration)
    /// note that E / D is # of effects created per effect expiration window
    for (qmax = indx = 0; indx < elem->bcnt; indx++)
        for (turn = 0; turn < elem->barr[indx].neff; turn++)
            if (!(iter = &elem->earr[elem->barr[indx].ieff + turn])->dmax)
                qmax += 2; /// > 1, as ChooseBehaviour() runs before SortByY()
            else
                qmax += 2 + ((iter->dmin)? iter->dmin : elem->barr[indx].dmax)
                          / iter->dmax;
    engc->pmax += elem->icnt * ++qmax;
    engc->parr = realloc(engc->parr, engc->pmax * sizeof(*engc->parr));
    /// now spawning sprites to the screen and emptying ICNT
    for (indx = 0; indx < elem->icnt; indx++) {
        engc->pcnt++;
        engc->parr[engc->pcnt - 1] = calloc(1, sizeof(**engc->parr));
        engc->parr[engc->pcnt - 1]->ulib = elem;
        ChooseBehaviour(engc, engc->parr[engc->pcnt - 1], 0, 0);
    }
    return elem->icnt + 1;
}



void MMH(MENU *item) {
    switch (item->uuid) {
        case TXT_CDEL:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_ADEL:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_CSLP:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_ASLP:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_CHAR:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_HOUS:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_TPL1:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_TPL2:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_OPTS:
            rMessage("Not implemented yet!", "WIP", 0);
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
        /// this happens when the engine cannot activate
        /// the GPU, so the best reaction is to complain
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



/** ==== NOTES ====
 1. Effects may count on the fact that BOSS is valid, as the only way to
    invalidate it is to delete the sprite. Behaviour changing occurs at
    times that are known at the start, so all effects can know when to
    invalidate themselves.
 2. Effects are in charge of respawning themselves, as they have different
    respawn times but belong to one parent, making it very hard for him to
    handle them all.
 3. Considering [1] and [2], it`s problematic to respawn an effect if its
    run time is less than its respawn time. It becomes "reserved" and waits.
 **/
uint32_t eUpdFrame(ENGD *engd, T4FV **data, uint32_t *size,
                   uint64_t time, intptr_t user, uint32_t attr,
                   int32_t xptr, int32_t yptr, int32_t isel) {
    ENGC *engc = (ENGC*)user;
    PICT *pict = engc->pcur;
    BINF *binf;
    AINF *anim;

    char *temp;
    long  indx, elem;
    uint64_t curr;

//    cEngineCallback(engd, ECB_LOAD, ~0);
//    /// here you can add new sprites!
//    cEngineCallback(engd, ECB_LOAD, 0);

    curr = (long double)time * (long double)engc->tdil;
    /// watch out for reserved-s (they shouldn`t be in [0; ISEL], though)
    if ((attr & UFR_MOUS) && ((isel >= 0) || pict)) {
        if (!pict && ((engc->ppos.z ^ attr) & UFR_LBTN)) {
            pict = engc->pcur = engc->parr[isel];
            if (pict->indx & FLG_EFCT)
                pict = engc->pcur = pict->boss;
            printf("[GRABBED] %s\n", pict->ulib->name);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
            ChooseBehaviour(engc, pict, curr, UFR_LBTN);
        }
        if (pict && (attr & UFR_LBTN)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict)
                printf("[DROPPED] %s\n", pict->ulib->name);
            pict = engc->pcur = 0;
        }
        if (~attr & engc->ppos.z & UFR_RBTN) {
            if (!pict)
                pict = engc->parr[isel];
            if (pict->indx & FLG_EFCT)
                pict = pict->boss;
            temp = malloc(32 + strlen(pict->ulib->name));
            sprintf(temp, "[ %s ]", pict->ulib->name);
            free(engc->mspr[0].text);
            engc->mspr[0].text = rConvertUTF8(temp);
            free(temp);
            rOpenContextMenu(engc->mspr);
        }
        if (!pict && (isel >= 0)) {
            /// we`ve got a simple mouseover situation here
            if (~engc->parr[isel]->indx & FLG_EFCT) {
                /// effects shall not react to mouseover
                ChooseBehaviour(engc, engc->parr[isel], curr, UFR_MOUS);
            }
        }
        engc->ppos.z = attr;
    }
    for (indx = 0; indx < engc->pcnt; indx++) {
        pict = engc->parr[indx];
        binf = (~pict->indx & FLG_EFCT)? &pict->ulib->barr[pict->indx >> 1]
             : &pict->ulib->earr[(pict->indx & ~FLG_EFCT) >> 1];
        anim = &binf->unit[pict->indx & 1];
        if (pict->indx & FLG_EFCT) {
            /// effect
            if (~binf->flgs & EFF_STAY)
                MoveToParent(pict, 0);   /// follow the parent
            if ((~(pict->tbhv ^ pict->boss->tbhv) & TMR_PAIR) /// same bhv.
            && (curr >= (pict->tmov & ~TMR_PAIR))) {
                pict->tmov &= ~TMR_PAIR; /// drop the reserved state, if any
                if (curr >= (pict->tbhv & ~TMR_PAIR))
                    elem = indx;         /// PICT expired, let`s edit inplace
                else
                    engc->parr[elem = engc->pcnt++] = 0;
                SpawnEffect(&engc->parr[elem], pict, &engc->seed, 0, curr);
            }
            else if (curr >= (pict->tbhv & ~TMR_PAIR)) {
                /// if (respawn > runtime) and same behaviour
                if (binf->dmin && binf->dmax && (binf->dmax > binf->dmin)
                && (~(pict->tbhv ^ pict->boss->tbhv) & TMR_PAIR))
                    pict->tmov |= TMR_PAIR; /// reserved, waiting for respawn
                else {
                    free(pict);             /// either the run time is up or
                    engc->parr[indx] = 0;   /// parent behaviour has changed
                    continue;
                }
            }
        }
        else if ((curr >= (pict->tbhv & ~TMR_PAIR))
             || ((pict->fram >= anim->fcnt) && !(binf->flgs & FLG_LOOP))) {
            /// behaviour that needs being changed
            ChooseBehaviour(engc, pict, curr, 0);
            binf = &pict->ulib->barr[pict->indx >> 1];
            anim = &binf->unit[pict->indx & 1];
        }
        /// update the current frame, both for effects and behaviours;
        /// has to be precisely in this place, don`t move it elsewhere
        while (curr >= pict->tfrm) {
            pict->fram =
                (pict->fram + 1 >= anim->fcnt)? (binf->flgs & FLG_LOOP)?
                 0 : pict->fram : pict->fram + 1;
            pict->tfrm = ((pict->tfrm)? pict->tfrm : curr)
                       + anim->time[pict->fram];
            if (!anim->time[pict->fram] || (~binf->flgs & FLG_LOOP))
                break;
        }
        if (pict->indx & FLG_EFCT)
            continue;
        /// only behaviours beyond this point!
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

    /// ELEM is the number of sprites skipped, multiplied by -1
    /// (like those with respawn > runtime staying in reserve)
    for (elem = indx = 0; indx < engc->pcnt; indx++)
        if ((pict = engc->parr[indx])->tmov & TMR_PAIR)
            elem--; /// this works only because reserved-s are put among
                    /// the sprites which cannot be selected with mouse
        else {
            curr = pict->indx & ~FLG_EFCT;
            if (pict->indx & FLG_EFCT)
                anim = &pict->ulib->earr[curr >> 1].unit[curr & 1];
            else
                anim = &pict->ulib->barr[curr >> 1].unit[curr & 1];
            engc->data[indx + elem] = (T4FV){{pict->offs.x, pict->offs.y,
                                              pict->fram, anim->uuid}};
        }
    *data = engc->data;
    *size = (engc->pcnt | elem)? engc->pmax : 0;
    return engc->pcnt + elem;
}



void Relocalize(ENGC *engc, char *lang) {
    INCBIN("../exec/loc/en.lang", DefaultLanguage);

    char *data;
    long size;

    FreeLocalization(&engc->tran);
    LoadLocalization(&engc->tran, DefaultLanguage, strlen(DefaultLanguage));
    data = 0;
    if (lang) {
        data = rLoadFile(lang, &size);
        LoadLocalization(&engc->tran, data, size);
        free(data);
    }
    free(engc->lang);
    engc->lang = (data)? strdup(lang) : 0;

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
    {.text = engc->tran[TXT_CHAR], .uuid = TXT_CHAR, .func = MMH},
    {.text = engc->tran[TXT_HOUS], .uuid = TXT_HOUS, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_TPL1], .uuid = TXT_TPL1, .func = MMH},
    {.text = engc->tran[TXT_TPL2], .uuid = TXT_TPL2, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_OPTS], .uuid = TXT_OPTS, .func = MMH},
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
        spec = engc->mspr[8].chld; /// Add character
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



void RecountLibs(ENGC *engc, long tran, long frac, long full) {
    char *text;

    text = malloc(32 + strlen(engc->tran[tran]));
    sprintf(text, "%s %ld / %ld", engc->tran[tran], frac, full);
    RUN_FE2C(engc->CTL_SELE, MSG_PTXT, (intptr_t)text);
    RUN_FE2C(engc->CTL_SELE, MSG_PLIM, full);
    RUN_FE2C(engc->CTL_SELE, MSG_PPOS, frac);
    free(text);
}



void RecountLibsSelected(ENGC *engc) {
    long full, frac, indx;

    for (full = frac = indx = 0; indx < engc->lcnt; indx++)
        if (engc->libs[indx].icnt > 0)
            frac++;
        else if (!engc->libs[indx].icnt)
            full++;
    RecountLibs(engc, TXT_SELE, frac, frac + full);
}



void UpdPreview(intptr_t data, uint64_t time) {
    uint64_t *fram = (typeof(fram))data + 1; /// +0: time, +1: frame
    ENGC *engc = (ENGC*)(uintptr_t)fram[-1];
    AINF *anim;

    if (engc->parr)
        return; /// previews are hidden when the engine is active

    for (data = 0; data < engc->lcnt; data++) {
        anim = &engc->libs[data].barr[engc->libs[data].prev].unit[0];
        if (!anim->fcnt)
            continue; /// this preview has been loaded incorrectly, skipping
        if (time > fram[data * 2 + 0]) {
            if (++fram[data * 2 + 1] >= anim->fcnt)
                fram[data * 2 + 1] = 0;
            fram[data * 2 + 0] += anim->time[fram[data * 2 + 1]];
            if (time > fram[data * 2 + 0])
                fram[data * 2 + 0] = time; /// > 1 frame skipped, resetting
            RUN_FE2C(engc->libs[data].pict, MSG_IFRM,
                    (fram[data * 2 + 1] & 0x3FF) | (anim->uuid << 10));
        }
    }
}



void CategorizePreviews(ENGC *engc) {
    long iter, indx, flgs;
    CTGS temp, *elem;

    flgs = RUN_FE2C(engc->CTL_EXAC, MSG_BGST, 0);
    flgs = (flgs & FCS_ENBL)? (flgs & FCS_MARK)? 2 : 1 : 0;
    for (elem = 0, indx = 0; indx < engc->lcnt; indx++) {
        for (iter = 0; iter < engc->ccnt; iter++)
            if (engc->ctgs[iter].flgs & flgs) {
                temp.hash = engc->ctgs[iter].hash;
                elem = bsearch(&temp, engc->libs[indx].ctgs,
                                engc->libs[indx].ccnt,
                                sizeof(*engc->libs->ctgs), ctgscmp);
                if (!!elem ^ (flgs - 1))
                    break;
            }
        engc->libs[indx].icnt =
            ((!elem & !!flgs) ^ (engc->libs[indx].icnt < 0))?
            -engc->libs[indx].icnt - 1 : engc->libs[indx].icnt;
    }
    RecountLibsSelected(engc);
    RUN_FE2C(engc->CTL_CHAR, MSG_WSZC, 0);
}



intptr_t FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    INCBIN("../exec/icon.gif", MainIcon);

    switch (ctrl->uuid) {
        case TXT_CHAR: {
            if (cmsg != MSG_SMAX)
                break;

            /// rearrange the scroll area and all previews it contains
            long xdim = (uint16_t)data, ydim = (uint16_t)(data >> 16),
                 xsep, ysep, xinc, yinc, line, temp, ymax, ycap, yspi;
            ENGC *engc = (ENGC*)ctrl->data;

            if (!engc->lcnt || !engc->libs[0].capt.fe2c)
                break;
            xsep = 8; /// separator width
            ysep = 8; /// separator height
            ycap = (uint16_t)(RUN_FE2C(engc->libs[0].capt, MSG__GSZ, 0) >> 16);
            yspi = (uint16_t)(RUN_FE2C(engc->libs[0].spin, MSG__GSZ, 0) >> 16);

            xinc = xsep;
            line = yinc = ymax = 0;
            for (data = 0; data <= engc->lcnt; data++) {
                if ((data < engc->lcnt) && (engc->libs[data].icnt < 0))
                    continue; /// skipping disabled libraries
                if ((data < engc->lcnt)
                &&  (xinc + xsep - engc->libs[data].pict.xdim <= xdim)) {
                    xinc += xsep - engc->libs[data].pict.xdim;
                    ymax = (-engc->libs[data].pict.ydim > ymax)?
                            -engc->libs[data].pict.ydim : ymax;
                    continue;
                }
                /// if we are here, either the line is full or the array ended
                for (xinc = xsep; line < data; line++) {
                    temp = !(engc->libs[line].icnt < 0);
                    RUN_FE2C(engc->libs[line].pict, MSG__SHW, temp);
                    RUN_FE2C(engc->libs[line].capt, MSG__SHW, temp);
                    RUN_FE2C(engc->libs[line].spin, MSG__SHW, temp);
                    if (!temp)
                        continue;
                    temp = yinc + ymax + engc->libs[line].pict.ydim;
                    RUN_FE2C(engc->libs[line].pict, MSG__POS,
                            (uint16_t)-xinc | (uint32_t)(-temp << 16));
                    temp = yinc + ymax;
                    RUN_FE2C(engc->libs[line].capt, MSG__POS,
                            (uint16_t)-xinc | (uint32_t)(-temp << 16));
                    temp = yinc + ymax + ycap;
                    RUN_FE2C(engc->libs[line].spin, MSG__POS,
                            (uint16_t)-xinc | (uint32_t)(-temp << 16));
                    xinc += xsep - engc->libs[line].pict.xdim;
                }
                yinc += ymax + ysep + ycap + yspi;
                if (data < engc->lcnt) {
                    xinc = xsep + xsep - engc->libs[data].pict.xdim;
                    ymax = -engc->libs[data].pict.ydim;
                }
            }
            ctrl->fe2c(ctrl, MSG__SHW, 1);
            return (yinc - ysep > ydim)? yinc - ysep - ydim : 0;
        }
        case TXT_OGRP:
            if (cmsg == MSG_LGST) {
                cmsg = RUN_FE2C(((ENGC*)ctrl->data)->CTL_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                return (((ENGC*)ctrl->data)->ctgs[data].flgs & cmsg)? 1 : 0;
            }
            else if (cmsg == MSG_LSST) {
                ENGC *engc = (ENGC*)ctrl->data;
                intptr_t prev;

                cmsg = RUN_FE2C(engc->CTL_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                prev = (engc->ctgs[data >> 1].flgs & cmsg)? 1 : 0;
                engc->ctgs[data >> 1].flgs &= ~cmsg;
                engc->ctgs[data >> 1].flgs |= (data & 1)? cmsg : 0;
                CategorizePreviews(engc);
                return prev;
            }
            break;

        case TXT_BADD:
            if (cmsg == MSG_BCLK) {
                ENGC *engc = (ENGC*)ctrl->data;
                long spin;

                data = RUN_FE2C(engc->CTL_SPEC, MSG_NGET, 0);
                for (cmsg = 0; cmsg < engc->lcnt; cmsg++)
                    if (engc->libs[cmsg].icnt >= 0) {
                        spin = RUN_FE2C(engc->libs[cmsg].spin, MSG_NGET, 0);
                        spin = (spin + data > 0)? spin + data : 0;
                        RUN_FE2C(engc->libs[cmsg].spin, MSG_NSET, spin);
                        RUN_FC2E(engc->libs[cmsg].spin, MSG_NSET, spin);
                    }
            }
            break;

        case TXT_CAPT:
            if (cmsg == MSG_WSZC)
                RUN_FE2C(((ENGC*)ctrl->data)->CTL_CHAR, cmsg, data);
            break;

        case TXT_FLTR:
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->CTL_EXAC, MSG__ENB, data);
                CategorizePreviews(((ENGC*)ctrl->data));
                RUN_FE2C(((ENGC*)ctrl->data)->CTL_OGRP, MSG__ENB, data);
            }
            break;

        case TXT_EXAC:
            if (cmsg == MSG_BCLK) {
                ENGC *engc = (ENGC*)ctrl->data;

                CategorizePreviews(engc);
                RUN_FE2C(engc->CTL_OGRP, MSG_LCOL,
                        (intptr_t)engc->tran[(data)? TXT_AGRP : TXT_OGRP]);
            }
            break;

        case TXT_SRND:
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->CTL_RGPU, MSG__ENB, data);
                RUN_FE2C(((ENGC*)ctrl->data)->CTL_BDUP, MSG__ENB, data);
            }
            break;

        case TXT_BDUP:
            /// nothing goes here
            break;

        case TXT_OPTS:
            rMessage("Not implemented yet!", "WIP", 0);
            break;

        case TXT_GOGO: {
            if (cmsg != MSG_BCLK)
                break;

            LINF *libs;
            ENGC *engc = ((ENGC*)ctrl->data);
            AINF igif = {};
            intptr_t icon;
            long ilen, *irnd, *iput;

            irnd = calloc(engc->lcnt, sizeof(*irnd));
            iput = calloc(engc->lcnt, sizeof(*iput));

            /// checking if random choice is enabled
            if ((cmsg = RUN_FE2C(engc->CTL_BDUP, MSG_BGST, 0)) & FCS_ENBL) {
                /// indexing random-capable libraries
                for (ilen = icon = 0; icon < engc->lcnt; icon++)
                    if (engc->libs[icon].icnt == 0)
                        iput[ilen++] = icon;
                /// iterating over the requested random sprites count
                for (icon = RUN_FE2C(engc->CTL_RGPU, MSG_NGET, 0);
                    (icon > 0) && ilen; icon--) {
                    irnd[iput[data = PRNG(&engc->seed) % ilen]]++;
                    if ((~cmsg & FCS_MARK) && (data < --ilen))
                        iput[data] = iput[ilen];
                }
                /// finally, adding the computed random values to ICNTs
                for (icon = 0; icon < engc->lcnt; icon++)
                    engc->libs[icon].icnt += irnd[icon];
            }
            /// is there anything selected? let`s find out
            for (icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].icnt > 0)
                    break;
            if (icon >= engc->lcnt) {
                /// [TODO:] do we need to show messages here?
//                rMessage("Nothing selected!", 0, 0);
                free(irnd);
                free(iput);
                break;
            }
            /// counting the number of selected libraries
            for (cmsg = icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].icnt > 0)
                    cmsg++;
            RecountLibs(engc, TXT_LOAD, 0, cmsg);

            cEngineCallback(engc->engd, ECB_LOAD, ~0);
            for (data = icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].icnt > 0) {
                    LoadLib(&engc->libs[icon], engc->engd);
                    RecountLibs(engc, TXT_LOAD, ++data, cmsg);
                    RUN_FE2C(engc->CTL_SELE, MSG_PPOS, data);
                }
            cEngineLoadAnimAsync(engc->engd, (uint8_t*)"/Icon/",
                                (uint8_t*)MainIcon, &igif);
            cEngineCallback(engc->engd, ECB_LOAD, 0);

            for (libs = engc->libs, icon = 0; icon < engc->lcnt; icon++)
                if (AppendSpriteArr(&engc->libs[icon], engc)) {
                    engc->libs[icon].icnt -= irnd[icon]; /// revert random ICNT
                    if (++libs <= &engc->libs[icon])
                        libs[-1] = engc->libs[icon];
                }
            free(irnd);
            free(iput);
            if (libs - engc->libs < engc->lcnt) {
                engc->lcnt = libs - engc->libs;
                engc->libs = realloc(engc->libs,
                                     engc->lcnt * sizeof(*engc->libs));
            }
            igif.fcnt = 0;
            igif.xdim = engc->idim.x;
            igif.ydim = engc->idim.y;
            igif.time = calloc(sizeof(*igif.time), igif.xdim * igif.ydim);
            cEngineCallback(engc->engd, ECB_DRAW, (intptr_t)&igif);
            icon = rMakeTrayIcon(engc->mctx, engc->tran[TXT_HEAD],
                                 igif.time, igif.xdim, igif.ydim);
            free(igif.time);
            RUN_FE2C(engc->CTL_CAPT, MSG__SHW, 0);
            engc->data = (engc->pmax)? calloc(engc->pmax,
                                              sizeof(*engc->data)) : 0;
            cEngineRunMainLoop(engc->engd, engc->dpos.x, engc->dpos.y,
                               engc->dims.x + engc->dpos.x,
                               engc->dims.y + engc->dpos.y, engc->ftmp,
                               FRM_WAIT, (intptr_t)engc, eUpdFrame, eUpdFlags);
            cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&engc->ftmp);
            free(engc->data);

            rFreeTrayIcon(icon);
            for (icon = 0; icon < engc->pcnt; icon++)
                free(engc->parr[icon]);
            free(engc->parr);
            engc->parr = 0;
            engc->pmax = engc->pcnt = 0;

            /// finally showing the window
            RecountLibsSelected(engc);
            RUN_FE2C(engc->CTL_CAPT, MSG__SHW, ~0);
            break;
        }
    }
    return 0;
}



intptr_t FC2EI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (ctrl->flgs & FCT_TTTT) {
        case FCT_IBOX:
            if (cmsg == MSG_IFRM)
                cEngineCallback((ENGD*)ctrl->data, ECB_DRAW, data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) {
                LINF *libs = (LINF*)ctrl->data;

                if (!!libs->icnt ^ !!data)
                    RecountLibs(libs->engc, TXT_SELE,
                                RUN_FE2C(libs->engc->CTL_SELE, MSG_PGET, 0)
                             + ((!libs->icnt && data)? 1 : -1),
                                RUN_FE2C(libs->engc->CTL_SELE, MSG_PGET, 1));
                libs->icnt = data;
            }
            break;
    }
    return 0;
}



int linfcmp(const void *a, const void *b) {
    return strcmp(((LINF*)a)->name, ((LINF*)b)->name);
}

void eExecuteEngine(char *fcnf, intptr_t find, ulong xico, ulong yico,
                    long xpos, long ypos, ulong xdim, ulong ydim) {
    #define DEF_ENDL "\r\n"
    static uint32_t
        uCNR[] = {CNF_RGPU, CNF_SHOW, CNF_IPBO,
                  CNF_IBGR, CNF_OPAQ, CNF_IRGN, CNF_DRAW},
        uCOM[] = {COM_RGPU, COM_SHOW, WIN_IPBO,
                  WIN_IBGR, COM_OPAQ, WIN_IRGN, COM_DRAW},
        uCNF[] = {CNF_EFCT},
        uCSF[] = {CSF_EFCT};
    static char
       *uSTR[] = {"GPU",    "Show",   "wPBO",
                  "wBGRA",  "Opaque", "wRegion","Draw"},
       *uSTF[] = {"Effects"};
    char *file, *fptr, *conf, *temp, *tran = DEF_CORE;
    long indx, xmax, ymax, xoff, yoff;
    uint32_t elem, *iter; /// for IF_BIN_FIND
    uint64_t *fram;

    ENGC engc = {};

    /// default options
    engc.ftmp = COM_SHOW | COM_DRAW | COM_RGPU;
    engc.flgs = CSF_EFCT;
    engc.tdil = 1.0;
    if (!fcnf)
        tran = 0;
    else {
        engc.conf = Concatenate(0, fcnf, tran, 0);
        tran = 0;
        fptr = file = rLoadFile(engc.conf, 0);
        while ((conf = GetNextLine(&fptr))) {
            switch (DetermineType(&conf)) {
                case CNF_LANG:
                    GET_TEMP(&conf);
                    free(tran);
                    if (!(tran = (temp)? strdup(Dequote(temp)) : 0))
                        break;
                    if ((temp = rLoadFile(tran, 0)))
                        free(temp);
                    else {
                        temp = Concatenate(0, fcnf, DEF_DSEP, tran, 0);
                        free(tran);
                        tran = temp;
                    }
                    break;

                case CNF_TIME:
                    if (TRY_TEMP(&conf))
                        engc.tdil = ClampToBounds(StrToFloat(temp), 0.1, 4.0);
                    break;

                case CNF_RNDR:
                    engc.ftmp = 0;
                    while (conf) {
                        IF_BIN_FIND(elem, uCNR, &conf)
                            engc.ftmp |= uCOM[elem - 1];
                    }
                    break;

                case CNF_FLGS:
                    engc.flgs = 0;
                    while (conf) {
                        IF_BIN_FIND(elem, uCNF, &conf)
                            engc.flgs |= uCSF[elem - 1];
                    }
                    break;
            }
        }
        free(file);
    }
    Relocalize(&engc, tran);
    free(tran);

    engc.idim = (T2IV){{xico, yico}};
    engc.dpos = (T2IV){{xpos, ypos}};
    engc.dims = (T2IV){{xdim - engc.dpos.x, ydim - engc.dpos.y}};

    /// primary initialization complete, now creating GUI
    ///  0. [ FIRST AND FOREMOST! ] do not forget to edit the appropriate CTL_
    ///     constants after swapping or adding controls
    ///  1. main window`s "dimensions" are just spaces to leave between window
    ///     edges and actual controls
    CTRL ctls[] =
   {{0, 0, TXT_CAPT,            FCT_WNDW           , 1,  1,  1,  1, FC2E},
    {0, 0, TXT_FLTR,            FCT_CBOX           , 0,  0, 19,  2, FC2E},
    {0, 0, TXT_EXAC, FCP_VERT | FCT_CBOX           , 0,  0, 19,  2, FC2E},
    {0, 0, TXT_OGRP, FCP_VERT | FCT_LIST           , 0,  0, 19, 16, FC2E},
    {0, 0, TXT_SGRP, FCP_VERT | FCT_TEXT           , 0,  1, 19,  2, 0   },
    {0, 0, TXT_SPEC, FCP_VERT | FCT_SPIN           , 0,  0,  9,  3, FC2E},
    {0, 0, TXT_BADD, FCP_BOTH | FCT_BUTN           , 1, -3,  9,  3, FC2E},
    {0, 0, TXT_SRND, FCP_VERT | FCT_CBOX | FSX_LEFT, 0,  1, 19,  2, FC2E},
    {0, 0, TXT_RGPU, FCP_VERT | FCT_SPIN           , 0,  0,  9,  3, FC2E},
    {0, 0, TXT_BDUP, FCP_BOTH | FCT_CBOX           , 1, -3,  9,  3, FC2E},
    {0, 0, TXT_SELE, FCP_VERT | FCT_PBAR           , 0,  1, 19,  3, 0   },
    {0, 0, TXT_OPTS, FCP_VERT | FCT_BUTN           , 0,  1,  9,  6, FC2E},
    {0, 0, TXT_GOGO, FCP_BOTH | FCT_BUTN | FSB_DFLT, 1, -6,  9,  6, FC2E},
    {0, 0, TXT_CHAR, FCP_HORZ | FCT_SBOX           , 0,  0, 41, 43, 0   },
    {}};

    xmax = ymax = xoff = yoff = 0;
    engc.ctls = calloc(1, sizeof(ctls));
    for (indx = 0; indx < countof(ctls) - 1; indx++) {
        engc.ctls[indx] = ctls[indx];
        engc.ctls[indx].data = (intptr_t)&engc;
        if (indx)
            engc.ctls[indx].prev = &engc.ctls[0];
        rMakeControl(&engc.ctls[indx], &xoff, &yoff,
                   (((engc.ctls[indx].flgs & FCT_TTTT) == FCT_WNDW) ||
                    ((engc.ctls[indx].flgs & FCT_TTTT) == FCT_TEXT) ||
                    ((engc.ctls[indx].flgs & FCT_TTTT) == FCT_BUTN) ||
                    ((engc.ctls[indx].flgs & FCT_TTTT) == FCT_CBOX) ||
                    ((engc.ctls[indx].flgs & FCT_TTTT) == FCT_RBOX))?
                      engc.tran[engc.ctls[indx].uuid] : 0);
        xmax = (xmax > xoff)? xmax : xoff;
        ymax = (ymax > yoff)? ymax : yoff;
    }
    RUN_FE2C(engc.CTL_SPEC, MSG_NDIM, (100 << 16) | 100);
    RUN_FE2C(engc.CTL_RGPU, MSG_NDIM, 50000 << 16);
    RUN_FE2C(engc.CTL_FLTR, MSG_BCLK, 0);
    RUN_FC2E(engc.CTL_EXAC, MSG_BCLK, 0);
    RUN_FE2C(engc.CTL_SRND, MSG_BCLK, 0);
    RUN_FC2E(engc.CTL_BDUP, MSG_BCLK, 0);

    /// resizing and showing the main window
    RUN_FE2C(engc.CTL_CAPT, MSG_WSZC,
            (uint16_t)xmax | ((uint32_t)ymax << 16));

    while ((temp = rFindFile(find)))
        if ((uintptr_t)temp > (uintptr_t)sizeof(temp)) {
            AppendLib(&engc, DEF_CONF, DEF_FLDR, temp);
            free(temp);
        }
    /// sort engine`s libraries by name, initialize the rendering engine
    qsort(engc.libs, engc.lcnt, sizeof(*engc.libs), linfcmp);
    cEngineCallback(0, ECB_INIT, (intptr_t)&engc.engd);
    for (indx = 0; indx < engc.lcnt; indx++) {
        LoadLibPreview(&engc.libs[indx], engc.engd);
        RecountLibs(&engc, TXT_LOAD, indx, engc.lcnt);
    }
    cEngineCallback(engc.engd, ECB_LOAD, 0);
    cEngineCallback(engc.engd, ECB_LOAD, ~0);

    /// hiding the scroll window
    engc.CTL_CHAR.fc2e = FC2E;
    RUN_FE2C(engc.CTL_CHAR, MSG__SHW, 0);

    RecountLibs(&engc, TXT_SELE, 0, engc.lcnt);
    RUN_FE2C(engc.CTL_SELE, MSG_PPOS, 0);
    for (indx = 0; indx < engc.ccnt; indx++)
        RUN_FE2C(engc.CTL_OGRP, MSG_LADD, (intptr_t)engc.ctgs[indx].name);
    RUN_FE2C(engc.CTL_OGRP, MSG_LCOL, (intptr_t)engc.tran[TXT_OGRP]);

    /// getting minimal width for a preview from one of the spin controls
    xico = (uint16_t)RUN_FE2C(engc.CTL_SPEC, MSG__GSZ, 0);
    /// constructing previews
    /// barr[0] should be barr[linf->prev], but barr is still unsorted here
    for (indx = 0; indx < engc.lcnt; indx++) {
        xmax = engc.libs[indx].barr[0].unit[0].xdim;
        xmax = (xico > xmax)? xico : xmax;
        engc.libs[indx].pict =
            (CTRL){&engc.CTL_CHAR, (intptr_t)engc.engd, indx,
                    FCT_IBOX, 0, 0, -xmax,
                    (engc.libs[indx].barr[0].unit[0].ydim)?
                   -(long)engc.libs[indx].barr[0].unit[0].ydim : -1, FC2EI};
        engc.libs[indx].spin =
            (CTRL){&engc.CTL_CHAR, (intptr_t)&engc.libs[indx], indx,
                    FCT_SPIN, 0, 0, -xmax, 3, FC2EI};
        engc.libs[indx].capt =
            (CTRL){&engc.CTL_CHAR, (intptr_t)&engc.libs[indx], indx,
                    FCT_TEXT | FST_CNTR, 0, 0, -xmax, 2, FC2EI};
        rMakeControl(&engc.libs[indx].pict, 0, 0, 0);
        rMakeControl(&engc.libs[indx].spin, 0, 0, 0);
        rMakeControl(&engc.libs[indx].capt, 0, 0, engc.libs[indx].name);
        RUN_FE2C(engc.libs[indx].spin, MSG_NDIM, 50000 << 16);
    }
    RUN_FC2E(engc.CTL_FLTR, MSG_BCLK, 0);

    printf("[((RNG))] seed = 0x%08X\n[**INI**] %s\n",
           engc.seed = time(0), engc.conf);

    fram = calloc(sizeof(*fram), engc.lcnt * 2 + 1);
    fram[0] = (uintptr_t)&engc;
    rInternalMainLoop(&engc.ctls[0], FRM_WAIT, UpdPreview, (intptr_t)fram);
    free(fram);

    RUN_FE2C(engc.CTL_CHAR, MSG__SHW, 0);
    for (indx = 0; indx < engc.lcnt; indx++) {
        rFreeControl(&engc.libs[indx].pict);
        rFreeControl(&engc.libs[indx].spin);
        rFreeControl(&engc.libs[indx].capt);
    }
    for (indx = countof(ctls) - 2; indx >= 0; indx--)
        rFreeControl(&engc.ctls[indx]);
    free(engc.ctls);

    FreeMenu(&engc.mspr);
    FreeMenu(&engc.mctx);
    FreeLocalization(&engc.tran);

    for (; engc.lcnt; engc.lcnt--)
        FreeLib(&engc.libs[engc.lcnt - 1]);
    free(engc.libs);

    for (; engc.ccnt; engc.ccnt--)
        free(engc.ctgs[engc.ccnt - 1].name);
    free(engc.ctgs);

    cEngineCallback(engc.engd, ECB_QUIT, 0);

    /// now saving configs and exiting
    conf = 0;
    temp = calloc(1, 128);
    sprintf(temp, "%0.2f", engc.tdil);
    Concatenate(&conf, "Language,", engc.lang, 0);
    Concatenate(&conf, DEF_ENDL, "Time,", temp, 0);
    Concatenate(&conf, DEF_ENDL, "Render,", 0);
    for (indx = 0; indx < countof(uCOM); indx++)
        if (engc.ftmp & uCOM[indx])
            Concatenate(&conf, uSTR[indx], ",", 0);
    Concatenate(&conf, DEF_ENDL, "Flags,", 0);
    for (indx = 0; indx < countof(uCSF); indx++)
        if (engc.flgs & uCSF[indx])
            Concatenate(&conf, uSTF[indx], ",", 0);
    Concatenate(&conf, DEF_ENDL, 0);
    rSaveFile(engc.conf, conf, strlen(conf));
    free(engc.lang);
    free(engc.conf);
    free(temp);
    free(conf);
    #undef DEF_ENDL
}
