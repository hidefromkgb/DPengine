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
/** horz + diag + vert          **/ #define BHV_ALLM (BHV_HORM | BHV_DIAM\
                                                               | BHV_VERM)
/** 'mouse-over' state          **/ #define BHV_OVRM (BHV_CTLM | BHV_HORM)
/** 'dragged' state             **/ #define BHV_DRGM (BHV_CTLM | BHV_DIAM)
/** 'sleep' state               **/ #define BHV_SLPM (BHV_CTLM | BHV_VERM)
/** [extractor]                 **/ #define BHV_MMMM (BHV_CTLM | BHV_ALLM)

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

/** do not follow parent        **/ #define EFF_STAY (1 << 30)
/** animation can be looped     **/ #define ANI_LOOP (1 << 31)

/** this sprite is an effect    **/ #define PIF_EFCT (1 <<  0)
/** inactive reserved sprite    **/ #define PIF_IRES (1 <<  1)
/** this sprite is "asleep"     **/ #define PIF_ASLP (1 <<  2)

/// /// /// /// /// /// /// /// /// follow offset type values
/** 'fixed'                     **/ #define FOT_FIXD 0x9A8F97BD
/** 'mirror'                    **/ #define FOT_MIRR 0x304E7075

/// /// /// /// /// /// /// /// /// config file strings
/** 'content'                   **/ #define CNF_BASE 0x6558329A
/** 'language'                  **/ #define CNF_LANG 0x1644959C
/** 'runstillupdate'            **/ #define CNF_RUNS 0x7ECC31BE
/** 'basescale'                 **/ #define CNF_SCAL 0x7A285DE0
/** 'timedilation'              **/ #define CNF_TDIL 0x338D79CF
/** 'randomspeech'              **/ #define CNF_RSAY 0x0C4A8F7D
/** 'cursordodge'               **/ #define CNF_PCDR 0xD00CAB48
/** 'flags'                     **/ #define CNF_FLGS 0x8ACE03CE
/** 'render'                    **/ #define CNF_RNDR 0x3C9F6676
/** 'draw'                      **/ #define CNF_DRAW 0xE7ABD6EE
/** 'show'                      **/ #define CNF_SHOW 0x27D90DCD
/** 'gpu'                       **/ #define CNF_RGPU 0x11927E83
/** 'opaque'                    **/ #define CNF_OPAQ 0xD246CFE1
/** 'wbgra'                     **/ #define CNF_IBGR 0xABF3B1E8
/** 'wpbo'                      **/ #define CNF_IPBO 0x78FE3880
/** 'wregion'                   **/ #define CNF_IRGN 0xDE0DCCBE
/** 'update'                    **/ #define CNF_UONR 0xE4895181
/** 'topmost'                   **/ #define CNF_ETOP 0x0622A23D
/** 'effects'                   **/ #define CNF_EEFF 0xAB1F60DF
/** 'interaction'               **/ #define CNF_EINT 0x3CD837AB
/** 'speech'                    **/ #define CNF_ESAY 0x5E664BA6
/** 'hover'                     **/ #define CNF_ERCH 0x303621E9

/// /// /// /// /// /// /// /// /// client specific flags
/** update the animation base   **/ #define CSF_UONR (1 <<  0)
/** engine window is top-most   **/ #define CSF_ETOP (1 <<  1)
/** behaviour effects are on    **/ #define CSF_EEFF (1 <<  2)
/** interactions are on         **/ #define CSF_EINT (1 <<  3)
/** speech bubbles are on       **/ #define CSF_ESAY (1 <<  4)
/** cursor hover reaction is on **/ #define CSF_ERCH (1 <<  5)

/// /// /// /// /// /// /// /// /// localized text constants
enum {
/** Remove character            **/ TXT_CDEL = 0,
/** Remove all similar          **/ TXT_ADEL,
/** Sleep/pause                 **/ TXT_CSLP,
/** Sleep/pause all similar     **/ TXT_ASLP,
/** Take control: Player 1      **/ TXT_TPL1,
/** Take control: Player 2      **/ TXT_TPL2,
/** More options...             **/ TXT_OPTS,

/** [ Desktop Ponies Engine ]   **/ TXT_HEAD,
/** OS specific options         **/ TXT_SPEC,
/** Disable transparency        **/ TXT_OPAQ,
/** Play animation              **/ TXT_DRAW,
/** Show window                 **/ TXT_SHOW,
/** Exit                        **/ TXT_EXIT,
/** Use GPU for drawing         **/ TXT_RGPU,
/** [ none ]                    **/ TXT_NONE,
/** [ default ]                 **/ TXT_DFLT,

/** Show console                **/ TXT_CONS,
/** Use regions                 **/ TXT_IRGN,
/** Enable BGRA                 **/ TXT_IBGR,
/** Enable pixel buffers        **/ TXT_IPBO,
/** Useless on full opacity!    **/ TXT_UOFO,
/** Useless without GPU!        **/ TXT_UWGL,
/** Cannot initialize GPU!      **/ TXT_CIGL,
/** The animation base <...>    **/ TXT_CTUP,
/** Internet connection failure **/ TXT_INET,
/** Failed to create directory  **/ TXT_FDIR,
/** Update                      **/ TXT_CCUP,

/** Desktop Ponies              **/ TXT_CAPT,
/** Enable filters              **/ TXT_FLTR,
/** Exact matching              **/ TXT_EXAC,
/** [At least one:]             **/ TXT_OGRP,
/** [All at once:]              **/ TXT_AGRP,
/** Random selection:           **/ TXT_SRND,
/** Group selection:            **/ TXT_SGRP,
/** Add                         **/ TXT_BADD,
/** Copies                      **/ TXT_BDUP,
/** Selected:                   **/ TXT_SELE,
/** Loaded:                     **/ TXT_LOAD,
/** Updated:                    **/ TXT_UPTO,
/** GO!                         **/ TXT_GOGO,

/** Update on next run          **/ TXT_UONR,
/** Always on top               **/ TXT_ETOP,
/** Enable effects              **/ TXT_EEFF,
/** Enable interactions         **/ TXT_EINT,
/** Enable speech               **/ TXT_ESAY,
/** React to cursor hover       **/ TXT_ERCH,

/**  runs between updates       **/ TXT_RUNS,
/**  % base scaling factor      **/ TXT_SCAL,
/**  % time dilation factor     **/ TXT_TDIL,
/**  % random speech chance     **/ TXT_RSAY,
/**  pix. cursor dodge radius   **/ TXT_PCDR,

/** Choose...                   **/ TXT_CHOO,
/** Reload                      **/ TXT_RELO,
/** Reset                       **/ TXT_RESE,
/** GUI language: English       **/ TXT_LGUI,
/** Animation base directory:   **/ TXT_BDIR,
/** Moving the animation base   **/ TXT_BMOV,
/** Confirm saving the <...>    **/ TXT_BSAV,
/** On refusal, the source <...>**/ TXT_BDEL,
/** Failed to move the <...>    **/ TXT_BERR,
/** OK                          **/ TXT_BYES,
/** Cancel                      **/ TXT_BNAY,
};

/// /// /// /// /// /// /// /// /// ENGC.MCTL array indices
/**                             **/ #define MCT_CAPT mctl[ 0]
/**                             **/ #define MCT_FLTR mctl[ 1]
/**                             **/ #define MCT_EXAC mctl[ 2]
/**                             **/ #define MCT_OGRP mctl[ 3]
/**                             **/ #define MCT_SGRP mctl[ 4]
/**                             **/ #define MCT_SPEC mctl[ 5]
/**                             **/ #define MCT_BADD mctl[ 6]
/**                             **/ #define MCT_SRND mctl[ 7]
/**                             **/ #define MCT_RGPU mctl[ 8]
/**                             **/ #define MCT_BDUP mctl[ 9]
/**                             **/ #define MCT_SELE mctl[10]
/**                             **/ #define MCT_OPTS mctl[11]
/**                             **/ #define MCT_GOGO mctl[12]
/**                             **/ #define MCT_CHAR mctl[13]

/// /// /// /// /// /// /// /// /// ENGC.OCTL array indices
/**                             **/ #define OCT_OPTS octl[ 0]
/**                             **/ #define OCT_UONR octl[ 1]
/**                             **/ #define OCT_ETOP octl[ 2]
/**                             **/ #define OCT_EEFF octl[ 3]
/**                             **/ #define OCT_EINT octl[ 4]
/**                             **/ #define OCT_ESAY octl[ 5]
/**                             **/ #define OCT_ERCH octl[ 6]
/**                             **/ #define OCT_NRUN octl[ 7]
/**                             **/ #define OCT_TRUN octl[ 8]
/**                             **/ #define OCT_NSCA octl[ 9]
/**                             **/ #define OCT_TSCA octl[10]
/**                             **/ #define OCT_NDIL octl[11]
/**                             **/ #define OCT_TDIL octl[12]
/**                             **/ #define OCT_NSAY octl[13]
/**                             **/ #define OCT_TSAY octl[14]
/**                             **/ #define OCT_NCDR octl[15]
/**                             **/ #define OCT_TCDR octl[16]
/**                             **/ #define OCT_LCHO octl[19]
/**                             **/ #define OCT_LREL octl[20]
/**                             **/ #define OCT_LRES octl[21]
/**                             **/ #define OCT_LGUI octl[22]
/**                             **/ #define OCT_BCHO octl[25]
/**                             **/ #define OCT_BREL octl[26]
/**                             **/ #define OCT_BRES octl[27]
/**                             **/ #define OCT_BDIR octl[28]
/**                             **/ #define OCT_FREL octl[30]
/**                             **/ #define OCT_FRES octl[31]

/// behaviour/effect unit info (write-once, read-only)
typedef struct {
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
typedef struct {
    uint32_t hash, flgs;
    char    *name;
} CTGS;

/// mersenne random number generator info
typedef struct {
    uint32_t size, indx, data[];
} RNGS;

/// parallel execution unit data
typedef struct {
    intptr_t desc;
    char *hash, *file, *hreq;
} PARA;

/// engine data (client side), prototype
typedef struct ENGC ENGC;

/// unit library info (write-once, read-only), opaque outside the module
/// [TODO:] (priority 1) behaviours
/// [TODO:] (priority 2) interactions
/// [TODO:] (priority 3) speech
typedef struct {
    CTRL     pict,  /// image box control to preview the sprite
             capt,  /// character name just below the image box
             spin;  /// spin control to set ICNT
    ENGC    *engc;  /// parent engine
    CTGS    *ctgs;  /// library categories (hashed and sorted)
    BINF    *barr,  /// available behaviours ordered by name hash
            *earr,  /// available effects ordered by parent bhv. name hash
           **bslp,  /// sleep behaviours extracted from BARR
           **bgrp;  /// nonzero-probability BARR elements ordered by bhv. group
    char   **bimp,  /// image paths for behaviours (0 if loaded)
           **eimp,  /// image paths for effects (0 if loaded)
            *path,  /// the folder from which the library was built
            *name;  /// human-readable name (may differ from PATH!)
    long    *ngrp,  /// bounds of behaviour groups in BGRP: [0~~)[G0~~~)[G1...
             nslp,  /// number of sleep behaviours
             prev,  /// preview index in sorted BARR
             ccnt,  /// categories count
             gcnt,  /// behaviour groups count
             zcnt,  /// nonzero probability behaviours count
             bcnt,  /// total behaviours count
             ecnt,  /// total effects count
             icnt;  /// number of on-screen bhv. sprites from the library
} LINF;

/// actual on-screen sprite, opaque outside the module
typedef struct _PICT {
    struct
    _PICT   *next,  /// linked list support for SortByY(), only used there
            *boss;  /// the parent sprite (e.g. follow target or effect base)
    LINF    *ulib;  /// unit library which the sprite belongs to
    T2FV     move,  /// movement direction (or parent offset for effects)
             offs;  /// position of the unit`s lower-left corner
    uint32_t fram,  /// current frame
             flgs,  /// unit-specific flags
             indx,  /// behaviour direction (lowest bit) and index
             ipre;  /// BHV: behaviour before entering a special mode
                    /// EFF: behaviour of the parent
    uint64_t tfrm,  /// BHV: next frame, msec
                    /// EFF: next frame, msec
             tmov,  /// BHV: next movement, msec
                    /// EFF: next respawn, msec (LLONG_MAX if respawned)
             tbhv;  /// BHV: next behaviour, msec
                    ///      flgs::PIF_PAIR is inverted on behaviour change
                    /// EFF: expected deletion, msec
                    ///      prohibit respawn if flgs::PIF_PAIR differs
} PICT;

/// client configuration
typedef struct {
    char    *lang,  /// name of the language file
            *base;  /// path to the animation base
    uint32_t flgs;  /// client-specific flags, e.g. effects +/- (CSF_ prefix)
    int16_t nrun[3],/// \.
            nsca[3],/// |.
            ndil[3],/// |.
            nsay[3],/// |. spinbox limits: [0] = min, [1] = pos, [2] = max
            ncdr[3],/// |.
            spec[3],/// |.
            rgpu[3];/// /
} CONF;

/// engine data (client side)
struct ENGC {
    MENU    *mspr,  /// per-sprite context menu
            *mctx;  /// engine`s main context menu
    CTRL    *mctl,  /// GUI controls array (main window)
            *octl;  /// GUI controls array (options window)
    T4FV    *data;  /// main display sequence passed to the renderer
    ENGD    *engd;  /// rendering engine handle
    LINF    *libs;  /// sprite libraries linked list
    CTGS    *ctgs;  /// categories array
    RNGS    *seed;  /// random number generator seed
    PICT    *pcur,  /// the sprite currently picked
           **parr,  /// on-screen sprite pointers array
           **elem;  /// pointer buffer for SortByY()
    char   **tran,  /// localized text array (ASCIIZ; last item is also 0)
            *cfnm;  /// name of the main configuration file
    double   tacc;  /// partial timestamp accumulator
    uint64_t tcur,  /// current, dilation-adjusted timestamp
             tpre;  /// previous raw timestamp
    uint32_t lcnt,  /// libraries count
             ccnt,  /// categories count
             pcnt,  /// on-screen sprites count (may differ every frame)
             pmax,  /// max. PARR capacity (realloc on exceed)
             ftmp;  /// temporary storage for engine flags
    T3IV     ppos;  /// mouse pointer position (z = flags)
    T2IV     dpos,  /// drawing area position
             dims,  /// drawing area dimensions
             idim;  /// tray icon dimensions
    CONF     cdef,  /// default configuration
             ccur,  /// current configuration
             cini;  /// initial configuration read on the start
};



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



#define Concatenate(retn, ...) _Concatenate(retn, ##__VA_ARGS__, (char*)0)
char *_Concatenate(char **retn, ...) {
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



char *Reslash(char *conv) {
    long iter;

    if (conv)
        for (iter = 0; conv[iter]; iter++)
            if (conv[iter] == '\\')
                conv[iter] = '/';
    return conv;
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



void SetProgress(ENGC *engc, long tran, long frac, long full) {
    char *text;

    text = malloc(32 + strlen(engc->tran[tran]));
    sprintf(text, "%s %ld / %ld", engc->tran[tran], frac, full);
    RUN_FE2C(engc->MCT_SELE, MSG__TXT, (intptr_t)text);
    RUN_FE2C(engc->MCT_SELE, MSG_PLIM, full);
    RUN_FE2C(engc->MCT_SELE, MSG_PPOS, frac);
    free(text);
}



void RecountSelectedLibs(ENGC *engc) {
    long full, frac, indx;

    for (full = frac = indx = 0; indx < engc->lcnt; indx++)
        if (engc->libs[indx].icnt > 0)
            frac++;
        else if (!engc->libs[indx].icnt)
            full++;
    SetProgress(engc, TXT_SELE, frac, frac + full);
}



#define MakeGetQuery(uenc, ...) \
       _MakeGetQuery(uenc, ##__VA_ARGS__, (char*)0)
char *_MakeGetQuery(long uenc, ...) {
    char *indx, *temp, *retn;
    long size;
    va_list list;

    size = 1;
    va_start(list, uenc);
    while ((temp = va_arg(list, char*)))
        size += strlen(temp) * 3 * 6;
    va_end(list);

    va_start(list, uenc);
    temp = retn = realloc(0, size);
    while ((indx = va_arg(list, char*))) {
        for (indx -= (size = -strlen(indx) + 1); size <= 0; size++)
            if (!uenc || (indx[size] == *DEF_DSEP))
                *temp++ = indx[size];
            else {
                temp[2] = "0123456789ABCDEF"[(indx[size] >> 0) & 0xF];
                temp[1] = "0123456789ABCDEF"[(indx[size] >> 4) & 0xF];
                temp[0] = '%';
                temp += 3;
            }
    }
    *temp = 0;
    va_end(list);
    return retn;
}



/** [TODO:] fix little-endian dependency **/
void MakeSHA1(uint32_t *curr, char *text, uint32_t size,
              uint32_t base, uint32_t trim) {
    #define SHA_R(v, b) (((v) << (b)) | ((v) >> (32 - (b))))
    #define SHA_I(i) (data.l[i] = (SHA_R(data.l[i], 24) & 0xFF00FF00)         \
                                | (SHA_R(data.l[i],  8) & 0x00FF00FF))
    #define SHA_U(i) (data.l[i & 15] = SHA_R(data.l[(i + 13) & 0x0F]          \
                                           ^ data.l[(i +  8) & 0x0F]          \
                                           ^ data.l[(i +  2) & 0x0F]          \
                                           ^ data.l[(i +  0) & 0x0F], 1))
    #define SHA_1(n) do {head[0] = ((head[2] & (head[3] ^ head[4])) ^ head[4])\
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)
    #define SHA_2(n) do {head[0] = (head[2] ^ head[3] ^ head[4])              \
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)
    #define SHA_3(n) do {head[0] = (((head[2] | head[3]) & head[4])           \
                                   | (head[2] & head[3]))                     \
                                 + n + head[5] + SHA_R(head[1], 5);           \
                         head[2] = SHA_R(head[2], 30); head--;} while (0)

    uint32_t iter, indx, pack, *head, temp[5];
    #pragma pack(push, 1)
    union {
        uint32_t l[16];
        uint8_t b[64];
    } data;
    #pragma pack(pop)

    pack = size - ((size + 1) & 0x3F) +
                ((((size + 1) & 0x3F) > 0x38)? 0x80 : 0x40);
    trim = (trim)? trim : pack;
    for (iter = base; iter <= trim; iter++) {
        data.b[iter & 0x3F] = (iter <= size)? (iter == size)? 0x80 : text[iter]
                 : (pack - iter < 4)? (size << 3) >> ((~iter << 3) & 0x1F) : 0;
        if ((iter & 0x3F) == 0x3F) {
            head = curr + 5 * 4 * 4 - 1;
            memcpy(temp, curr, sizeof(temp));
            memcpy(head + 1, curr, sizeof(temp));
            for (indx =  0; indx < 16; indx++) SHA_1(0x5A827999 + SHA_I(indx));
            for (indx = 16; indx < 20; indx++) SHA_1(0x5A827999 + SHA_U(indx));
            for (indx = 20; indx < 40; indx++) SHA_2(0x6ED9EBA1 + SHA_U(indx));
            for (indx = 40; indx < 60; indx++) SHA_3(0x8F1BBCDC + SHA_U(indx));
            for (indx = 60; indx < 80; indx++) SHA_2(0xCA62C1D6 + SHA_U(indx));
            curr[0] += temp[0]; curr[1] += temp[1]; curr[2] += temp[2];
            curr[3] += temp[3]; curr[4] += temp[4];
        }
    }
    #undef SHA_3
    #undef SHA_2
    #undef SHA_1
    #undef SHA_U
    #undef SHA_I
    #undef SHA_R
}



void CheckHashAndDownload(intptr_t user, uint64_t zero) {
    PARA *para = (PARA*)user;
    char shas[5 * 4 * 2 + 1], *blob, *load, *text;
    uint32_t curr[5 * 4 * 4 + 4 + 1], retn;
    uint8_t shan[5 * 4];
    long size, flen;

    flen = sprintf(text = calloc(1, 0x100 + strlen(para->file)),
                   "\"%s\"... ", para->file);
    size = 0;
    shan[0] = 1;
    shas[1] = 0;
    shas[0] = '0';
    load = rLoadFile(para->file, &size);
    if (strlen(para->hash) == sizeof(shan) << 1) {
        retn = sprintf(blob = realloc(0, 32), "blob %ld", size);
        curr[0] = 0x67452301; curr[1] = 0xEFCDAB89;
        curr[2] = 0x98BADCFE; curr[3] = 0x10325476;
        curr[4] = 0xC3D2E1F0;
        MakeSHA1(curr, blob, retn + 1 + size, 0, retn);
        MakeSHA1(curr, load - (retn + 1), retn + 1 + size, retn + 1, 0);
        for (retn = 0; retn < sizeof(shan); retn++) {
            /** [TODO:] fix little-endian dependency **/
            shan[retn] = curr[retn >> 2] >> ((~retn & 3) << 3);
            sprintf(shas + (retn << 1), "%02x", shan[retn]);
        }
        for (retn = 0; retn < (sizeof(shan)) << 1; retn++) {
            /** [TODO:] fix little-endian dependency **/
            shan[retn >> 1] = (shan[retn >> 1] << 4) | (shan[retn >> 1] >> 4);
            shan[retn >> 1] ^= ((para->hash[retn] & 0x0F)
                             + ((para->hash[retn] & 0x40)? 9 : 0)) & 0x0F;
        }
        for (retn = 0; retn < sizeof(shan); retn++)
            shan[0] |= shan[retn];
        blob = realloc(blob, 0);
    }
    load = realloc(load, 0);
    if (!shan[0])
        sprintf(text + flen, "already updated");
    else {
        size = rLoadHTTPS(para->desc, para->hreq, &load);
        if (!load)
            sprintf(text + flen, "error!");
        else {
            sprintf(text + flen, "%s vs. %s", shas, para->hash);
            rSaveFile(para->file, load, size);
            load = realloc(load, 0);
        }
        para->hreq = realloc(para->hreq, 0);
    }
    para->file = realloc(para->file, 0);
    para = realloc(para, 0);
    printf("[**WEB**] %s\n", text);
    text = realloc(text, 0);
}



long TryGetFromGithub(ENGC *engc, char *user, char *auth,
                      char *proj, char *bran, char *repo, char *disk) {
    #define GIT_SFIN "\""
    #define GIT_SURL GIT_SFIN "url"  GIT_SFIN ":" GIT_SFIN
    #define GIT_SSHA GIT_SFIN "sha"  GIT_SFIN ":" GIT_SFIN
    #define GIT_STYP GIT_SFIN "type" GIT_SFIN ":" GIT_SFIN
    #define GIT_SPTH GIT_SFIN "path" GIT_SFIN ":" GIT_SFIN
    #define GIT_SRAW "raw.githubusercontent.com"
    #define GIT_SAPI "api.github.com"

    char *path, *file, *text, *temp, *tail;
    long size, rlen, iter, full = 0;
    intptr_t desc, para;
    PARA *tmpl;

    if (engc->lcnt
    || !rMessage(engc->tran[TXT_CTUP], engc->tran[TXT_CCUP],
                 engc->tran[TXT_BYES], engc->tran[TXT_BNAY]))
        return 0;
    text = tail = 0;
    while ((desc = rMakeHTTPS(user, GIT_SAPI))) {
        temp = MakeGetQuery(0, "repos", DEF_DSEP,  auth, DEF_DSEP,
                                  proj, DEF_DSEP, "git", DEF_DSEP,
                               "trees", DEF_DSEP,  bran);
        rLoadHTTPS(desc, temp, &text);
        temp = realloc(temp, 0);
        if (!text)
            break;
        size = sizeof(GIT_SPTH) + 1 + (rlen = strlen(repo));
        memmove(path = realloc(0, size), GIT_SPTH, sizeof(GIT_SPTH) - 1);
        memmove(path + sizeof(GIT_SPTH) - 1, repo, rlen);
        path[size - 2] = *GIT_SFIN;
        path[size - 1] = 0;
        if ((temp = strstr(text, path)) && (temp = strstr(temp, GIT_SURL)))
            tail = strstr(temp += sizeof(GIT_SURL) - 1, GIT_SFIN);
        path = realloc(path, 0);
        if (tail) {
            tail[0] = 0;
            tail = 0;
            if ((temp = strstr(temp, GIT_SAPI))) {
                temp += sizeof(GIT_SAPI); /** no -1: skipping the slash **/
                temp = MakeGetQuery(0, temp, "?recursive=1");
                rLoadHTTPS(desc, temp, &tail);
                temp = realloc(temp, 0);
            }
        }
        text = realloc(text, 0);
        text = (tail)? tail-- : 0;
        rFreeHTTPS(desc);
        break;
    }
    if (text && (desc = rMakeHTTPS(user, GIT_SRAW))) {
        while ((temp = strstr(tail + 1, GIT_SPTH))) {
            if ((tail = strstr(temp + sizeof(GIT_SPTH) - 1, GIT_SFIN))
            &&  (temp = strstr(tail + 1, GIT_STYP))
            &&  (tail = strstr(temp + sizeof(GIT_STYP) - 1, GIT_SFIN))
            &&  (temp = strstr(tail + 1, GIT_SSHA))
            &&  (tail = strstr(temp + sizeof(GIT_SSHA) - 1, GIT_SFIN)))
                full++;
            else
                break;
        }
        tail = text - 1;
        para = rMakeParallel(CheckHashAndDownload, 4);
        for (iter = 0; iter < full; iter++) {
            temp = strstr(tail + 1, GIT_SPTH);
            tail = strstr(temp += sizeof(GIT_SPTH) - 1, GIT_SFIN);
            tail[0] = 0;
            path = temp;

            temp = strstr(tail + 1, GIT_STYP);
            tail = strstr(temp += sizeof(GIT_STYP) - 1, GIT_SFIN);
            tail[0] = 0;
            if (strcmp(temp, "blob"))
                path = 0;

            if (path) {
                file = realloc(strdup(path), 2 + (rlen = strlen(disk))
                                               + (size = strlen(path)));
                file[rlen] = *DEF_DSEP;
                memmove(file, disk, rlen);
                memmove(file + rlen + 1, path, size + 1);
                for (temp = file + 1; (temp = strstr(temp, DEF_DSEP));) {
                    *temp = 0;
                    if (!rMakeDir(file)) {
                        path = Concatenate(0, engc->tran[TXT_FDIR], " ",
                                              GIT_SFIN, file, GIT_SFIN);
                        rMessage(path, engc->tran[TXT_CCUP],
                                 engc->tran[TXT_BYES], 0);
                        file = realloc(file, 0);
                        path = realloc(path, 0);
                        path = 0;
                        break;
                    }
                    *temp++ = *DEF_DSEP;
                }
            }
            temp = strstr(tail + 1, GIT_SSHA);
            tail = strstr(temp += sizeof(GIT_SSHA) - 1, GIT_SFIN);
            tail[0] = 0;
            if (path) {
                *(tmpl = calloc(1, sizeof(*tmpl))) =
                    (PARA){desc, temp, file,
                           MakeGetQuery(1, auth, DEF_DSEP, proj, DEF_DSEP,
                                        bran, DEF_DSEP, repo, DEF_DSEP, path)};
                rLoadParallel(para, (intptr_t)tmpl);
            }
            SetProgress(engc, TXT_UPTO, iter, full);
        }
        rFreeParallel(para);
        text = realloc(text, 0);
        rFreeHTTPS(desc);
    }
    else /// do NOT invert if/else, since conditions have side-effects
        rMessage(engc->tran[TXT_INET], engc->tran[TXT_CCUP],
                 engc->tran[TXT_BYES], 0);
    return 1;
    #undef GIT_SAPI
    #undef GIT_SRAW
    #undef GIT_SPTH
    #undef GIT_STYP
    #undef GIT_SSHA
    #undef GIT_SURL
    #undef GIT_SFIN
}



void FreePRNG(RNGS **seed) {
    free(*seed);
    *seed = 0;
}

RNGS *InitPRNG(uint32_t init) {
    #define RNG_SIZE 624
    RNGS *seed = malloc(sizeof(uint32_t) * (1 + 1 + RNG_SIZE));

    seed->data[0] = init;
    seed->indx = seed->size = RNG_SIZE;
    for (init = 1; init < seed->size; init++)
        seed->data[init] = init + 1812433253 *
            (seed->data[init - 1] ^ (seed->data[init - 1] >> 30));
    return seed;
    #undef RNG_SIZE
}

uint32_t PRNG(RNGS *seed) {
    uint32_t iter, retn;

    if (!seed)
        return 0;
    if (seed->indx >= seed->size)
        for (seed->indx = iter = 0; iter < seed->size; iter++){
            retn = (seed->data[ iter                  ] & 0x80000000)
                 | (seed->data[(iter + 1) % seed->size] & 0x7FFFFFFF);
            seed->data[iter] = seed->data[(iter + 397) % seed->size]
                             ^ (retn >> 1) ^ ((retn & 1)? 0x9908B0DF : 0);
        }
    retn = seed->data[seed->indx++];
    retn ^= (retn >> 11);
    retn ^= (retn <<  7) & 0x9D2C5680;
    retn ^= (retn << 15) & 0xEFC60000;
    retn ^= (retn >> 18);
    return retn;
}



uint32_t HashLine(char *line, long size) {
    uint32_t hash = 0;

    if (!line)
        return hash;
    if (!size)
        size--;
    while (*line && size--)
        hash = 0x11 + 0xFBC5 * hash + *line++;
    return hash;
}



static inline float ClampToBounds(float what, float bmin, float bmax) {
    what = (what > bmin)? what : bmin;
    return (what < bmax)? what : bmax;
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
                                 Dequote(SplitLine(conf, DEF_TSEP, 0)));
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
                   BHV_ALLM | BHV_____ | ANI_LOOP, 0, 0};

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
    /// enforcing (!move && !(flgs & BHV_CTLM)) <=> (flgs == BHV_NONM)
    if (!retn->move && !(retn->flgs & BHV_CTLM))
        retn->flgs = BHV_NONM | (retn->flgs & ~BHV_MMMM);
    if ((retn->flgs & BHV_MMMM) == BHV_NONM)
        retn->move = 0.0;

    /// linked behaviour name..................................................  def = ""
    if (TRY_TEMP(conf))
        retn->link = HashLine(Dequote(temp), 0);

/// [TODO:]
    /// speech said on behaviour start.........................................  def = ""
    if (TRY_TEMP(conf)) {};

/// [TODO:]
    /// speech said on behaviour end...........................................  def = ""
    if (TRY_TEMP(conf)) {};

    /// flag to never execute this behaviour at random.........................  def = False
    if (TRY_TEMP(conf))
        if (HashLine(ToLower(temp, 0), 0) == VAL_TRUE)
            retn->prob = 0;

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
        SET_FLAG(retn->flgs, temp, VAL_FALS, ANI_LOOP);

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
                   ANI_LOOP | EFF_STAY | (EFF_RNDA * 0x1111), 0, 0};

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
        SET_FLAG(retn->flgs, temp, VAL_FALS, ANI_LOOP);
}

void AppendLib(ENGC *engc, char *pcnf, char *base, char *path) {
    char *file, *fptr, *conf, *temp;
    long bcnt, ecnt, ccnt = 0;
    uint32_t hash;
    LINF *libs;
    CTGS *ctgs;

    fptr = Concatenate(0, base, DEF_DSEP, path);
    conf = Concatenate(0, fptr, DEF_DSEP, pcnf);
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
    free(elem->bslp);
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
    long ydim, ymin, ymax, ytmp, iter;
    PICT *temp;

    if (engc->pcnt < 1)
        return;

    ydim = (engc->dims.y << 1) - 1;
    memset(engc->elem, 0, (ydim + 1) * sizeof(*engc->elem));

    ymin = ydim + 1;
    ymax = -1;

    for (iter = 0; iter < engc->pcnt; iter++)
        if ((temp = engc->parr[iter])) {
            ytmp = temp->offs.y;
            if ((ytmp >= 0) && (ytmp < ydim)) {
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
            if ((ytmp < 0) || (temp->offs.y >= ydim)
            || (temp->flgs & PIF_IRES)) /// reserved ones go to the very end
                ytmp = 0;
            temp->next = engc->elem[ytmp];
            engc->elem[ytmp] = temp;
        }
    if ((ymax -= ymin - 1) < 0)
        ymax = 1;
    for (iter = 0; ymax >= 0; ymax--)
        while (engc->elem[ymax]) {
            engc->parr[iter++] = engc->elem[ymax];
            engc->elem[ymax] = engc->elem[ymax]->next;
        }
}



/// SEED = 0 means "keep centering and placement"
void MoveToParent(PICT *pict, RNGS *seed) {
    static float
        xdim[] = {0.0, 0.5, 1.0, 0.0, 0.5, 1.0, 0.0, 0.5, 1.0, -1.0, -1.0},
        ydim[] = {1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, -1.0, -1.0};
    long iter, desc;
    AINF *anim[2];
    T2FV vect[2];

    if (seed) {
        /// parent behaviour animation
        anim[0] = &pict->boss->ulib->barr[pict->boss->indx >> 1].
                                     unit[pict->boss->indx &  1];
        /// effect animation
        anim[1] = &pict->ulib->earr[pict->indx >> 1].unit[pict->indx & 1];
        /// read centering and placement for the current effect
        desc = pict->ulib->earr[pict->indx >> 1].flgs
             >> ((pict->indx & 1) << 3); /// direction was inherited from BOSS
        #define CALC(d) ((d[desc & EFF_AAAA] >= 0.0)? d[desc & EFF_AAAA] \
                       : (float)(PRNG(seed) & 0xFFF) * 0.000244140625)   \
                       * (float)anim[iter]->d       /* ^-[1 / 4096]-^ */
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
    if (pict->flgs & PIF_EFCT)
        return;

    BINF *bhvr = &pict->ulib->barr[pict->indx >> 1];
    float angl;
    long flag;

    if (!(bhvr->flgs & BHV_CTLM) && (bhvr->flgs & BHV_ALLM)) {
        flag = PRNG(engc->seed);
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
                angl = (angl + PRNG(engc->seed) % flag) * DTR_CONV;
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

    pict->indx = (pict->indx & -2) | (PRNG(engc->seed) & 1);
    if (!bhvr->unit[pict->indx & 1].uuid)
        pict->indx ^= 1;
    if (pict->indx & 1)
        pict->move.x = -pict->move.x;
    if (PRNG(engc->seed) & 1)
        pict->move.y = -pict->move.y;
}



/// INDX is not used if FROM is an effect, as all we need is stored there
long SpawnEffect(PICT **retn, PICT *from, RNGS *seed,
                 ulong indx, uint64_t time) {
    BINF *binf;

    if (from->tmov >= LLONG_MAX)
        return 0;  /// no more self-replicating for this effect; exiting

    if (~from->flgs & PIF_EFCT) {
        /// parent = behaviour
        *retn = calloc(1, sizeof(**retn));
        (*retn)->boss = from;
        (*retn)->ulib = from->ulib;
        (*retn)->indx = (indx << 1) | (from->indx & 1);
        (*retn)->flgs |= PIF_EFCT;
    }
    else if (*retn != from) {
        /// parent = effect that needs to be copied to RETN
        *retn = calloc(1, sizeof(**retn));
        **retn = *from;
        from->tmov = LLONG_MAX; /// only one self-replication allowed
    }
    /// ^^^^ along these there is a third way, when an effect gets reused,
    /// and no new effects are created; this keeps sprite count in bounds

    binf = &(*retn)->ulib->earr[(*retn)->indx >> 1];
    (*retn)->fram = -1; /// this means:
    (*retn)->tfrm =  0; /// "update me to 0-th frame ASAP!"
    /// effect respawn time; DMAX = 0 means "do not respawn"
    (*retn)->tmov = (binf->dmax)? time + binf->dmax : LLONG_MAX;
    /// effect ending time; DMIN = 0 means "end when the behaviour ends"
    /// if the effect duration is longer than that of the behaviour, and
    /// also it doesn`t respawn, then trim it
    (*retn)->tbhv = (*retn)->boss->tbhv;
    (*retn)->ipre = (*retn)->boss->indx;
    if (binf->dmin && (binf->dmax || (time + binf->dmin < (*retn)->tbhv)))
        (*retn)->tbhv = time + binf->dmin;
    /// re-adjusting turn, as it may have changed
    (*retn)->indx = ((*retn)->indx & ~1) | ((*retn)->boss->indx & 1);
    MoveToParent(*retn, seed);
    return ~0;
}



void ChooseBehaviour(ENGC *engc, PICT *pict, uint64_t time, uint32_t next) {
    long seed, lbgn, lend;
    LINF *ulib = pict->ulib;
    BINF *binf;
    AINF *anim;

    if (pict->flgs & PIF_EFCT)
        return;

    if (~next)
        pict->indx = next & ~1;
    else {
        if (ulib->barr[pict->indx >> 1].link)
            pict->indx = (ulib->barr[pict->indx >> 1].link - 1) << 1;
        else {
            seed = ulib->barr[pict->indx >> 1].igrp;
            lbgn = (seed)? ulib->ngrp[seed - 1] : 0;
            lend = ulib->ngrp[seed];
            seed = PRNG(engc->seed) % ulib->bgrp[lend - 1]->prob;

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
    binf = &pict->ulib->barr[pict->indx >> 1];
    pict->fram = -1; /// this means:
    pict->tfrm =  0; /// "update me to 0-th frame ASAP!"
    pict->tbhv = time + binf->dmin;
    if (binf->dmax > binf->dmin)
        pict->tbhv += PRNG(engc->seed) % (binf->dmax - binf->dmin);
    if (!time) {
        /// this is the first time this sprite appears; let`s put it somewhere
        anim = &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
        pict->offs.x = PRNG(engc->seed) % (engc->dims.x - anim->xdim);
        pict->offs.y = PRNG(engc->seed) % (engc->dims.y - anim->ydim)
                                                        + anim->ydim;
    }
    ChooseDirection(engc, pict);
    /// now spawning effects for the behaviour, if any
    /// (and only if this is not the first spawn and effects are enabled)
    if (time && (engc->ccur.flgs & CSF_EEFF)) {
        lbgn = pict->ulib->barr[pict->indx >> 1].ieff - 1;
        for (lend = pict->ulib->barr[pict->indx >> 1].neff; lend; lend--)
            SpawnEffect(&engc->parr[engc->pcnt++], pict,
                         engc->seed, lbgn + lend, time);
    }
}

void SpecialBehaviour(ENGC *engc, PICT *pict, uint32_t flgs) {
    uint64_t time = engc->tcur;
    LINF *ulib = pict->ulib;

    if (pict->flgs & PIF_EFCT)
        return;

    switch (flgs & BHV_MMMM) {
        case BHV_OVRM:
        case BHV_OVRM ^ BHV_CTLM:
            time = LLONG_MAX;
            return;

        case BHV_DRGM:
        case BHV_DRGM ^ BHV_CTLM:
            time = LLONG_MAX;
            return;

        case BHV_SLPM:
            if (pict->flgs & PIF_ASLP) {
                pict->indx = pict->ipre;
                pict->flgs &= ~PIF_ASLP;
            }
            else {
                time = LLONG_MAX;
                pict->ipre = pict->indx;
                pict->indx = (ulib->bslp[PRNG(engc->seed) % ulib->nslp]
                             - ulib->barr) << 1;
                pict->flgs |= PIF_ASLP;
            }
            break;
    }
    ChooseBehaviour(engc, pict, time, pict->indx);
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

        /// extracting sleep behaviours
        for (elem->nslp = qmax = turn = indx = 0; indx < elem->bcnt; indx++) {
            if (elem->barr[indx].move < elem->barr[qmax].move)
                qmax = indx;
            if ((elem->barr[indx].flgs & BHV_MMMM) == BHV_SLPM)
                elem->nslp++;
            else if (elem->barr[indx].prob
                 && !elem->barr[indx].trgt && !elem->barr[indx].move)
                turn++;
        }
        elem->bslp = calloc((!elem->nslp)? (!turn)? 1 : turn : elem->nslp,
                              sizeof(*elem->bslp));
        if (elem->nslp)
            for (elem->nslp = indx = 0; indx < elem->bcnt; indx++) {
                if ((elem->barr[indx].flgs & BHV_MMMM) == BHV_SLPM)
                    elem->bslp[elem->nslp++] = &elem->barr[indx];
            }
        else if (turn)
            for (elem->nslp = indx = 0; indx < elem->bcnt; indx++) {
                if (elem->barr[indx].prob
                && !elem->barr[indx].trgt && !elem->barr[indx].move)
                    elem->bslp[elem->nslp++] = &elem->barr[indx];
            }
        else
            elem->bslp[(elem->nslp = 1) - 1] = &elem->barr[qmax];
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
        ChooseBehaviour(engc, engc->parr[engc->pcnt - 1], 0, ~0);
    }
    return elem->icnt + 1;
}



void MMH(MENU *item) {
    uint32_t indx, flgs = 0;
    PICT *pict;
    LINF *ulib;
    ENGC *engc;

    switch (item->uuid) {
        case TXT_CDEL:
            pict = (PICT*)item->data;
            engc = pict->ulib->engc;
            for (indx = 0; indx < engc->pcnt; indx++) {
                if ((engc->parr[indx] == pict)
                || ((engc->parr[indx]->flgs & PIF_EFCT)
                &&  (engc->parr[indx]->boss == pict))) {
                    /// purging the character and all its
                    /// effects, including those reserved.
                    free(engc->parr[indx]);
                    engc->parr[indx] = 0;
                }
                else if (engc->parr[indx]->boss == pict) {
                    /// some character has been connected
                    /// to the one removed; disconnecting.
                    engc->parr[indx]->boss = 0;
                }
            }
            break;

        case TXT_ADEL:
            ulib = ((PICT*)item->data)->ulib;
            engc = ulib->engc;
            for (indx = 0; indx < engc->pcnt; indx++)
                if (engc->parr[indx]->boss
                && (engc->parr[indx]->boss->ulib == ulib)) {
                    /// some character has been connected to
                    /// one of the removed; disconnecting.
                    engc->parr[indx]->boss = 0;
                }
            for (indx = 0; indx < engc->pcnt; indx++)
                if (engc->parr[indx]->ulib == ulib) {
                    /// purging all similar characters and all
                    /// their effects, including those reserved.
                    free(engc->parr[indx]);
                    engc->parr[indx] = 0;
                }
            break;

        case TXT_CSLP:
            pict = (PICT*)item->data;
            SpecialBehaviour(pict->ulib->engc, pict, BHV_SLPM);
            break;

        case TXT_ASLP:
            ulib = ((PICT*)item->data)->ulib;
            engc = ulib->engc;
            for (indx = 0; indx < engc->pcnt; indx++)
                if (engc->parr[indx]->ulib == ulib)
                    SpecialBehaviour(engc, engc->parr[indx], BHV_SLPM);
            break;

        case TXT_TPL1:
            rMessage("Not implemented yet!", "WIP", "OK", 0);
            break;

        case TXT_TPL2:
            rMessage("Not implemented yet!", "WIP", "OK", 0);
            break;

        case TXT_OPTS:
            RUN_FE2C(((ENGC*)item->data)->OCT_OPTS, MSG__SHW, 1);
            break;

        case TXT_RGPU:
            engc = (ENGC*)item->data;
            cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= COM_RGPU;
            else
                flgs &= ~COM_RGPU;
            cEngineCallback(engc->engd, ECB_SFLG, flgs);
            break;

        case TXT_SHOW:
        case TXT_DRAW:
        case TXT_OPAQ:
            engc = (ENGC*)item->data;
            indx = (item->uuid != TXT_OPAQ)? (item->uuid != TXT_DRAW)?
                    COM_SHOW : COM_DRAW : COM_OPAQ;
            cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&flgs);
            if (item->flgs & MFL_VCHK)
                flgs |= indx;
            else
                flgs &= ~indx;
            cEngineCallback(engc->engd, ECB_SFLG, flgs);
            break;

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
        rMessage(engc->tran[TXT_CIGL], 0, engc->tran[TXT_BYES], 0);
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
 1. Effects may count on the fact that non-zero BOSS values are valid
    pointers, as the only way to invalidate them is to delete sprites,
    and sprite deletion process implies zeroing all respective BOSSes.
    Behaviour changing occurs at times that are known at the start,
    so all effects can know when to invalidate themselves.
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

    curr = (engc->tacc += 0.01 * engc->ccur.ndil[1] * (time - engc->tpre));
    engc->tpre = time;
    engc->tacc -= curr;
    engc->tcur += curr;

    /// watch out for reserved-s (they shouldn`t be in [0; ISEL], though)
    if ((attr & UFR_MOUS) && ((isel >= 0) || pict)) {
        if (!pict && ((engc->ppos.z ^ attr) & UFR_LBTN)) {
            pict = engc->pcur = engc->parr[isel];
            if (pict->flgs & PIF_EFCT)
                pict = engc->pcur = pict->boss;
            printf("[GRABBED] %s\n", pict->ulib->name);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
            SpecialBehaviour(engc, pict, BHV_DRGM);
        }
        if (pict && (attr & UFR_LBTN)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict) {
                printf("[DROPPED] %s\n", pict->ulib->name);
                SpecialBehaviour(engc, pict, BHV_DRGM ^ BHV_CTLM);
            }
            pict = engc->pcur = 0;
        }
        if (~attr & engc->ppos.z & UFR_RBTN) {
            if (!pict)
                pict = engc->parr[isel];
            if (pict->flgs & PIF_EFCT)
                pict = pict->boss;
            temp = malloc(32 + strlen(pict->ulib->name));
            sprintf(temp, "[ %s ]", pict->ulib->name);
            free(engc->mspr[0].text);
            engc->mspr[0].text = rConvertUTF8(temp);
            free(temp);
            /// setting data for "remove character", "remove all",
            /// "sleep", "sleep all", "player 1", "player 2"
            engc->mspr[2].data = engc->mspr[3].data =
            engc->mspr[5].data = engc->mspr[6].data =
            engc->mspr[8].data = engc->mspr[9].data = (intptr_t)pict;
            engc->mspr[5].flgs &= ~MFL_VCHK;
            engc->mspr[5].flgs |= (pict->flgs & PIF_ASLP)? MFL_VCHK : 0;
            rOpenContextMenu(engc->mspr);
        }
        if (!pict && (isel >= 0) && engc->parr[isel]) {
            /// we`ve got a simple mouseover situation here
            if (~engc->parr[isel]->flgs & PIF_EFCT) {
                /// effects shall not react to mouseover
                SpecialBehaviour(engc, engc->parr[isel], BHV_OVRM);
            }
        }
        engc->ppos.z = attr;
    }
    for (indx = 0; indx < engc->pcnt; indx++) {
        if (!(pict = engc->parr[indx]))
            continue;
        binf = (pict->flgs & PIF_EFCT)? &pict->ulib->earr[pict->indx >> 1]
                                      : &pict->ulib->barr[pict->indx >> 1];
        anim = &binf->unit[pict->indx & 1];
        if (pict->flgs & PIF_EFCT) {
            /// effect
            if (~binf->flgs & EFF_STAY)
                MoveToParent(pict, 0);   /// follow the parent
            if ((engc->tcur >= pict->tmov)               /// time to wake up
            &&!((pict->ipre ^ pict->boss->indx) & ~1)) { /// same behaviour
                pict->flgs &= ~PIF_IRES; /// drop the reserved state, if any
                if (engc->tcur >= pict->tbhv)
                    elem = indx;         /// PICT expired, let`s edit inplace
                else
                    engc->parr[elem = engc->pcnt++] = 0;
                SpawnEffect(&engc->parr[elem], pict,
                             engc->seed, 0, engc->tcur);
            }
            else if (engc->tcur >= pict->tbhv) {
                /// if (respawn > runtime) and same behaviour
                if (binf->dmin && binf->dmax && (binf->dmax > binf->dmin)
                &&!((pict->ipre ^ pict->boss->indx) & ~1))
                    pict->flgs |= PIF_IRES; /// reserved, waiting for respawn
                else {
                    free(pict);             /// either the run time is up or
                    engc->parr[indx] = 0;   /// parent behaviour has changed
                    continue;
                }
            }
        }
        else if ((engc->tcur >= pict->tbhv)
             || ((pict->fram >= anim->fcnt) && !(binf->flgs & ANI_LOOP))) {
            /// behaviour that needs being changed
            ChooseBehaviour(engc, pict, engc->tcur, ~0);
            binf = &pict->ulib->barr[pict->indx >> 1];
            anim = &binf->unit[pict->indx & 1];
        }
        /// update the current frame, both for effects and behaviours;
        /// has to be precisely in this place, don`t move it elsewhere
        while (engc->tcur >= pict->tfrm) {
            pict->fram =
                (pict->fram + 1 >= anim->fcnt)? (binf->flgs & ANI_LOOP)?
                 0 : pict->fram : pict->fram + 1;
            pict->tfrm = ((pict->tfrm)? pict->tfrm : engc->tcur)
                       + anim->time[pict->fram];
            if (!anim->time[pict->fram] || (~binf->flgs & ANI_LOOP))
                break;
        }
        if (pict->flgs & PIF_EFCT)
            continue;
        /// only behaviours beyond this point!
        if (engc->tcur - pict->tmov >= FRM_WAIT) {
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
            pict->tmov = engc->tcur;
        }
    }
    SortByY(engc);

    /// ELEM is the number of sprites skipped, multiplied by -1
    /// (like those with respawn > runtime staying in reserve)
    for (elem = indx = 0; indx < engc->pcnt; indx++)
        if ((pict = engc->parr[indx])->flgs & PIF_IRES)
            elem--; /// this works only because reserved-s are put among
                    /// the sprites which cannot be selected with mouse
        else {
            anim = (pict->flgs & PIF_EFCT)?
                   &pict->ulib->earr[pict->indx >> 1].unit[pict->indx & 1]
                 : &pict->ulib->barr[pict->indx >> 1].unit[pict->indx & 1];
            engc->data[indx + elem] = (T4FV){{pict->offs.x, pict->offs.y,
                                              pict->fram, anim->uuid}};
        }
    *data = engc->data;
    *size = (engc->pcnt | elem)? engc->pmax : 0;
    return engc->pcnt + elem;
}



void Relocalize(ENGC *engc, char *lang) {
    INCBIN("../exec/loc/en.lang", DefaultLanguage);

    CTRL *retn, *ctls[] = {engc->mctl, engc->octl};
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
    free(engc->ccur.lang);
    engc->ccur.lang = (data)? strdup(lang) : 0;

    MENU *spec,

    /** BE SURE TO UPDATE MSPR INDICES UP THERE BEFORE REARRANGING!!! **/
    mspr[] =
   {{.text = "",                   .flgs = MFL_GRAY},
    {.text = ""},
    {.text = engc->tran[TXT_CDEL], .uuid = TXT_CDEL, .func = MMH},
    {.text = engc->tran[TXT_ADEL], .uuid = TXT_ADEL, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_CSLP], .uuid = TXT_CSLP, .func = MMH,
     .flgs = MFL_CCHK},
    {.text = engc->tran[TXT_ASLP], .uuid = TXT_ASLP, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_TPL1], .uuid = TXT_TPL1, .func = MMH},
    {.text = engc->tran[TXT_TPL2], .uuid = TXT_TPL2, .func = MMH},
    {.text = ""},
    {.text = engc->tran[TXT_OPTS], .uuid = TXT_OPTS, .func = MMH,
     .data = (intptr_t)engc},
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

    FreeMenu(&engc->mspr);
    FreeMenu(&engc->mctx);
    engc->mspr = MenuFromTemplate(mspr);
    engc->mctx = MenuFromTemplate(mctx);
    engc->mctx[2].chld = ((spec = rOSSpecificMenu(engc)))?
                           spec : MenuFromTemplate(none);

    for (size = 0; size < sizeof(ctls) / sizeof(*ctls); size++)
        for (retn = ctls[size]; retn->xdim | retn->ydim; retn++) {
            data = (((retn->flgs & FCT_TTTT) == FCT_WNDW) ||
                    ((retn->flgs & FCT_TTTT) == FCT_TEXT) ||
                    ((retn->flgs & FCT_TTTT) == FCT_BUTN) ||
                    ((retn->flgs & FCT_TTTT) == FCT_CBOX))?
                      engc->tran[retn->uuid] : 0;
            if (((retn->flgs & FCT_TTTT) == FCT_TEXT)
            &&   (retn->flgs & FST_SUNK))
                data = 0;
            if (data)
                retn->fe2c(retn, MSG__TXT, (intptr_t)data);
        }
    RUN_FE2C(engc->MCT_OGRP, MSG__TXT,
            (intptr_t)engc->tran[(RUN_FE2C(engc->MCT_EXAC,
                                           MSG_BGST, 0) & FCS_MARK)?
                                  TXT_AGRP : TXT_OGRP]);
    RUN_FE2C(engc->OCT_LGUI, MSG__TXT, (intptr_t)((engc->ccur.lang)?
             engc->ccur.lang : engc->tran[TXT_DFLT]));
    RUN_FE2C(engc->OCT_BDIR, MSG__TXT, (intptr_t)((engc->ccur.base
          && strcmp(engc->ccur.base, engc->cdef.base))?
             engc->ccur.base : engc->tran[TXT_DFLT]));
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

    flgs = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
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
    RecountSelectedLibs(engc);
    RUN_FE2C(engc->MCT_CHAR, MSG_WSZC, 0);
}



void UpdateOptionControls(ENGC *engc, long main) {
    static uint32_t
        uCSF[] = {CSF_ETOP,        CSF_ERCH,        CSF_EINT,
                  CSF_ESAY,        CSF_EEFF,        CSF_UONR};
    CTRL *uCTX[] = {
           &engc->OCT_ETOP, &engc->OCT_ERCH, &engc->OCT_EINT,
           &engc->OCT_ESAY, &engc->OCT_EEFF, &engc->OCT_UONR,
           &engc->MCT_FLTR, &engc->MCT_EXAC, &engc->MCT_SRND, &engc->MCT_BDUP
    };
    CTRL *uCTN[] = {
           &engc->OCT_NRUN, &engc->OCT_NSCA, &engc->OCT_NDIL,
           &engc->OCT_NSAY, &engc->OCT_NCDR, &engc->MCT_SPEC, &engc->MCT_RGPU
    };
    int16_t *nctl, flag, indx;

    for (indx = countof(uCTX) - ((main)? 1 : 5); indx >= 0; indx--) {
        flag = (indx < countof(uCSF))? !(engc->ccur.flgs & uCSF[indx]) : 1;
        uCTX[indx]->fe2c(uCTX[indx], MSG_BCLK, !flag);
        uCTX[indx]->fc2e(uCTX[indx], MSG_BCLK, !flag);
    }
    for (indx = countof(uCTN) - ((main)? 1 : 3); indx >= 0; indx--) {
        nctl = (int16_t*)uCTN[indx]->data;
        uCTN[indx]->fe2c(uCTN[indx], MSG_NDIM,
                        ((uint32_t)nctl[2] << 16) | (uint16_t)nctl[0]);
        uCTN[indx]->fe2c(uCTN[indx], MSG_NSET, nctl[1]);
    }
}



intptr_t FC2EO(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    static uint32_t uCSF[] = {CSF_UONR, CSF_ETOP, CSF_EEFF,
                              CSF_EINT, CSF_ESAY, CSF_ERCH};
    uint32_t indx = 0;
    char *temp;
    ENGC *engc;

    switch (ctrl->uuid) {
        case TXT_OPTS:
            if (cmsg == MSG_WEND)
                ctrl->fe2c(ctrl, MSG__SHW, 0);
            break;

        case TXT_ERCH: indx++;
        case TXT_ESAY: indx++;
        case TXT_EINT: indx++;
        case TXT_EEFF: indx++;
        case TXT_ETOP: indx++;
        case TXT_UONR:
            if (cmsg != MSG_BCLK)
                break;
            engc = (ENGC*)ctrl->data;
            engc->ccur.flgs = (!data)? engc->ccur.flgs & ~uCSF[indx]
                                     : engc->ccur.flgs |  uCSF[indx];
            if (ctrl->uuid == TXT_ESAY) {
                RUN_FE2C(engc->OCT_NSAY, MSG__ENB, data);
                RUN_FE2C(engc->OCT_TSAY, MSG__ENB, data);
            }
            else if (ctrl->uuid == TXT_ERCH) {
                RUN_FE2C(engc->OCT_NCDR, MSG__ENB, data);
                RUN_FE2C(engc->OCT_TCDR, MSG__ENB, data);
            }
            break;

        case TXT_RUNS:
        case TXT_SCAL:
        case TXT_TDIL:
        case TXT_RSAY:
        case TXT_PCDR:
            if (cmsg == MSG_NSET)
                ((int16_t*)ctrl->data)[1] =
                    ClampToBounds(data, ((int16_t*)ctrl->data)[0],
                                        ((int16_t*)ctrl->data)[2]);
            break;

        case TXT_CHOO:
        case TXT_RELO:
        case TXT_RESE:
            if (cmsg != MSG_BCLK)
                break;
            engc = (ENGC*)ctrl->data;
            if ((ctrl == &engc->OCT_FREL) || (ctrl == &engc->OCT_FRES)) {
                CTRL *lctl, *bctl;

                if (ctrl->uuid == TXT_RELO) {
                    lctl = &engc->OCT_LREL;
                    bctl = &engc->OCT_BREL;
                    engc->ccur = engc->cini;
                    engc->ccur.base = engc->ccur.lang = 0;
                }
                else {
                    lctl = &engc->OCT_LRES;
                    bctl = &engc->OCT_BRES;
                    engc->ccur = engc->cdef;
                    engc->ccur.base = engc->ccur.lang = 0;
                }
                lctl->fc2e(lctl, MSG_BCLK, 0);
                bctl->fc2e(bctl, MSG_BCLK, 0);
                UpdateOptionControls(engc, 0);
                break;
            }
            if ((ctrl == &engc->OCT_LCHO)
            ||  (ctrl == &engc->OCT_LREL) || (ctrl == &engc->OCT_LRES)) {
                if ((temp = (ctrl->uuid == TXT_RELO)? engc->cini.lang : 0))
                    temp = strdup(temp);
                if ((ctrl->uuid != TXT_CHOO)
                ||  (temp = Reslash(rChooseFile(ctrl, "lang",
                                               (engc->ccur.lang)?
                                                engc->ccur.lang : ""))))
                    Relocalize(engc, temp);
            }
            else {
                if ((temp = (ctrl->uuid == TXT_RELO)? engc->cini.base : 0))
                    temp = strdup(temp);
                if ((ctrl->uuid != TXT_CHOO)
                ||  (temp = Reslash(rChooseDir(ctrl, engc->ccur.base)))) {
                    free(engc->ccur.base);
                    engc->ccur.base = (temp)? strdup(temp) : 0;
                    RUN_FE2C(engc->OCT_BDIR, MSG__TXT,
                            (intptr_t)((temp && strcmp(temp, engc->cdef.base))?
                                        temp : engc->tran[TXT_DFLT]));
                }
            }
            free(temp);
            break;
    }
    return 0;
}



intptr_t FC2EM(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    INCBIN("../exec/icon.gif", MainIcon);

    switch (ctrl->uuid) {
        case TXT_HEAD: {
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
                cmsg = RUN_FE2C(((ENGC*)ctrl->data)->MCT_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                return (((ENGC*)ctrl->data)->ctgs[data].flgs & cmsg)? 1 : 0;
            }
            else if (cmsg == MSG_LSST) {
                ENGC *engc = (ENGC*)ctrl->data;
                intptr_t prev;

                cmsg = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
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

                data = RUN_FE2C(engc->MCT_SPEC, MSG_NGET, 0);
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
            if (cmsg == MSG_WEND) {
                ENGC *engc = (ENGC*)ctrl->data;
                char *fptr, *file, *temp;

                /// trying to write the animation base to its new location
                if (engc->cini.base && engc->ccur.base) {
                    fptr = strdup(engc->ccur.base);
                    file = Concatenate(0, engc->cini.base, DEF_DSEP, DEF_FLDR);
                    temp = Concatenate(0, engc->tran[TXT_BSAV],
                                          "\n\n", file, "\n==>\n",
                                          fptr, "\n\n", engc->tran[TXT_BDEL]);
                    if (strcmp(engc->cini.base, engc->ccur.base)) {
                        if (!rMessage(temp, engc->tran[TXT_BMOV],
                                            engc->tran[TXT_BYES],
                                            engc->tran[TXT_BNAY])) {
                            free(fptr);
                            fptr = 0;
                        }
                        if (!rMoveDir(file, fptr)) {
                            free(temp);
                            temp = Concatenate(0, engc->tran[TXT_BERR],
                                                  "\n\n", file, "\n==>\n",
                                                  (fptr)? fptr : "[X]");
                            rMessage(temp, engc->tran[TXT_BMOV],
                                           engc->tran[TXT_BYES], 0);
                        }
                    }
                    free(temp);
                    free(fptr);
                    free(file);
                }
                return 1;
            }
            if ((cmsg == MSG_WSZC) && (((ENGC*)ctrl->data)->mctl))
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_CHAR, cmsg, data);
            break;

        case TXT_FLTR:
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_EXAC, MSG__ENB, data);
                CategorizePreviews(((ENGC*)ctrl->data));
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_OGRP, MSG__ENB, data);
            }
            break;

        case TXT_EXAC:
            if (cmsg == MSG_BCLK) {
                ENGC *engc = (ENGC*)ctrl->data;

                CategorizePreviews(engc);
                RUN_FE2C(engc->MCT_OGRP, MSG__TXT,
                        (intptr_t)engc->tran[(data)? TXT_AGRP : TXT_OGRP]);
            }
            break;

        case TXT_SRND:
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_RGPU, MSG__ENB, data);
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_BDUP, MSG__ENB, data);
            }
            break;

        case TXT_BDUP:
            /// nothing goes here
            break;

        case TXT_OPTS:
            if (cmsg == MSG_BCLK)
                RUN_FE2C(((ENGC*)ctrl->data)->OCT_OPTS, MSG__SHW, 1);
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
            if ((cmsg = RUN_FE2C(engc->MCT_BDUP, MSG_BGST, 0)) & FCS_ENBL) {
                /// indexing random-capable libraries
                for (ilen = icon = 0; icon < engc->lcnt; icon++)
                    if (engc->libs[icon].icnt == 0)
                        iput[ilen++] = icon;
                /// iterating over the requested random sprites count
                for (icon = RUN_FE2C(engc->MCT_RGPU, MSG_NGET, 0);
                    (icon > 0) && ilen; icon--) {
                    irnd[iput[data = PRNG(engc->seed) % ilen]]++;
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
            SetProgress(engc, TXT_LOAD, 0, cmsg);

            cEngineCallback(engc->engd, ECB_LOAD, ~0);
            for (data = icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].icnt > 0) {
                    LoadLib(&engc->libs[icon], engc->engd);
                    SetProgress(engc, TXT_LOAD, ++data, cmsg);
                    RUN_FE2C(engc->MCT_SELE, MSG_PPOS, data);
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
            RUN_FE2C(engc->MCT_CAPT, MSG__SHW, 0);
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
            RecountSelectedLibs(engc);
            RUN_FE2C(engc->MCT_CAPT, MSG__SHW, ~0);
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
                    SetProgress(libs->engc, TXT_SELE,
                                RUN_FE2C(libs->engc->MCT_SELE, MSG_PGET, 0)
                             + ((!libs->icnt && data)? 1 : -1),
                                RUN_FE2C(libs->engc->MCT_SELE, MSG_PGET, 1));
                libs->icnt = data;
            }
            break;
    }
    return 0;
}



void FreeWindow(CTRL **wndw) {
    long indx = 0;

    while ((*wndw)[indx].xdim | (*wndw)[indx].ydim)
        rFreeControl(&(*wndw)[indx++]);
    free(*wndw);
    *wndw = 0;
}

CTRL *MakeWindow(CTRL *tmpl, long size) {
    long indx, xmax, ymax, xoff, yoff;
    CTRL *retn;

    xmax = ymax = xoff = yoff = 0;
    retn = calloc(size + 1, sizeof(*retn));
    for (indx = 0; indx < size; indx++) {
        retn[indx] = tmpl[indx];
        if (indx)
            retn[indx].prev = &retn[0];
        rMakeControl(&retn[indx], &xoff, &yoff);
        xmax = (xmax > xoff)? xmax : xoff;
        ymax = (ymax > yoff)? ymax : yoff;
    }
    /// resizing and showing the window
    RUN_FE2C(retn[0], MSG_WSZC, (uint16_t)xmax | ((uint32_t)ymax << 16));
    return retn;
}



int linfcmp(const void *a, const void *b) {
    return strcmp(((LINF*)a)->name, ((LINF*)b)->name);
}

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    #define DEF_ENDL "\r\n"
    static uint32_t
        uCNR[] = {CNF_RGPU, CNF_SHOW, CNF_IPBO,
                  CNF_IBGR, CNF_OPAQ, CNF_IRGN, CNF_DRAW},
        uCOM[] = {COM_RGPU, COM_SHOW, WIN_IPBO,
                  WIN_IBGR, COM_OPAQ, WIN_IRGN, COM_DRAW},
        uCNF[] = {CNF_ETOP, CNF_ERCH, CNF_EINT,
                  CNF_ESAY, CNF_EEFF, CNF_UONR},
        uCSF[] = {CSF_ETOP, CSF_ERCH, CSF_EINT,
                  CSF_ESAY, CSF_EEFF, CSF_UONR};
    static char
       *uSTR[] = {"GPU",    "Show",   "wPBO",
                  "wBGRA",  "Opaque", "wRegion","Draw"},
       *uSTF[] = {"Topmost","Hover",  "Interaction",
                  "Speech", "Effects","Update"};
    char *file, *fptr, *conf, *temp;
    uint32_t elem, *iter; /// for IF_BIN_FIND and InitPRNG
    uint64_t *fram;
    intptr_t indx;
    int16_t runs = 0;

    ENGC engc = {.tcur = 1, .ftmp = COM_SHOW | COM_DRAW | COM_RGPU};

    engc.cini = engc.cdef = (CONF){0, strdup(base), CSF_EEFF,
    /** runs between updates  **/ {   0,   5,           1000},
    /** base scaling factor   **/ {  25, 100,            300},
    /** time dilation factor  **/ {  10, 100,           1000},
    /** random speech chance  **/ {   0,  50,            100},
    /** cursor dodge radius   **/ {   0,   0,           1000},
    /** group selection       **/ {-100,   0,            100},
    /** random selection      **/ {   0,   0, (int16_t)50000}};
    engc.cdef.base = Reslash(engc.cdef.base);
    engc.cini.base = strdup(engc.cini.base);
    if (fcnf) {
        engc.cfnm = Concatenate(0, fcnf, DEF_CORE);
        fptr = file = rLoadFile(engc.cfnm, 0);
        while ((conf = GetNextLine(&fptr))) {
            switch (DetermineType(&conf)) {
                case CNF_LANG:
                    GET_TEMP(&conf);
                    free(engc.cini.lang);
                    if (!(engc.cini.lang = (temp)?
                          strdup(Reslash(Dequote(temp))) : 0))
                        break;
                    if ((temp = rLoadFile(engc.cini.lang, 0)))
                        free(temp);
                    else {
                        temp = Concatenate(0, fcnf, DEF_DSEP, engc.cini.lang);
                        free(engc.cini.lang);
                        engc.cini.lang = temp;
                    }
                    break;

                case CNF_BASE:
                    GET_TEMP(&conf);
                    free(engc.cini.base);
                    if (!(engc.cini.base = (temp)?
                          strdup(Reslash(Dequote(temp)))
                        : strdup(engc.cdef.base)))
                        break;
                    break;

                case CNF_RUNS:
                    if (TRY_TEMP(&conf))
                        engc.cini.nrun[1] = ClampToBounds(StrToFloat(temp),
                                                          engc.cini.nrun[0],
                                                          engc.cini.nrun[2]);
                    if (TRY_TEMP(&conf))
                        runs = ClampToBounds(StrToFloat(temp),
                                             engc.cini.nrun[0],
                                             engc.cini.nrun[2]) + 1;
                    break;

                case CNF_SCAL:
                    if (TRY_TEMP(&conf))
                        engc.cini.nsca[1] = ClampToBounds(StrToFloat(temp),
                                                          engc.cini.nsca[0],
                                                          engc.cini.nsca[2]);
                    break;

                case CNF_TDIL:
                    if (TRY_TEMP(&conf))
                        engc.cini.ndil[1] = ClampToBounds(StrToFloat(temp),
                                                          engc.cini.ndil[0],
                                                          engc.cini.ndil[2]);
                    break;

                case CNF_RSAY:
                    if (TRY_TEMP(&conf))
                        engc.cini.nsay[1] = ClampToBounds(StrToFloat(temp),
                                                          engc.cini.nsay[0],
                                                          engc.cini.nsay[2]);
                    break;

                case CNF_PCDR:
                    if (TRY_TEMP(&conf))
                        engc.cini.ncdr[1] = ClampToBounds(StrToFloat(temp),
                                                          engc.cini.ncdr[0],
                                                          engc.cini.ncdr[2]);
                    break;

                case CNF_RNDR:
                    engc.ftmp = 0;
                    while (conf) {
                        IF_BIN_FIND(elem, uCNR, &conf)
                            engc.ftmp |= uCOM[elem - 1];
                    }
                    break;

                case CNF_FLGS:
                    engc.cini.flgs = 0;
                    while (conf) {
                        IF_BIN_FIND(elem, uCNF, &conf)
                            engc.cini.flgs |= uCSF[elem - 1];
                    }
                    break;
            }
        }
        free(file);
    }
    if (!engc.cini.nrun[1])
        runs = 0;
    else if (runs >= engc.cini.nrun[1]) {
        engc.cini.flgs |= CSF_UONR;
        runs = 0;
    }
    engc.ccur = engc.cini;
    engc.ccur.flgs &= ~CSF_UONR;
    engc.ccur.base = strdup(engc.ccur.base);
    engc.ccur.lang = 0;

    engc.idim = (T2IV){{xico, yico}};
    engc.dpos = (T2IV){{xpos, ypos}};
    engc.dims = (T2IV){{xdim - engc.dpos.x, ydim - engc.dpos.y}};
    engc.elem = calloc(engc.dims.y << 1, sizeof(*engc.elem));
    indx = (intptr_t)&engc;

    /// primary initialization complete, now creating GUI
    ///  0. [ FIRST AND FOREMOST! ] do not forget to edit the appropriate *CT_
    ///     constants after swapping or adding controls
    ///  1. main window`s "dimensions" are just spaces to leave between window
    ///     edges and actual controls
    CTRL
    mctl[] = {
        {0, indx, TXT_CAPT, FSW_SIZE | FCT_WNDW,  1,  1,  1,  1, FC2EM},
        {0, indx, TXT_FLTR,            FCT_CBOX,  0,  0, 19,  2, FC2EM},
        {0, indx, TXT_EXAC, FCP_VERT | FCT_CBOX,  0,  0, 19,  2, FC2EM},
        {0, indx, TXT_OGRP, FCP_VERT | FCT_LIST,  0,  0, 19, 16, FC2EM},
        {0, indx, TXT_SGRP, FCP_VERT | FCT_TEXT,  0,  1, 19,  2, FC2EM},
        {0, (intptr_t)engc.ccur.spec,
                  TXT_SPEC, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2EM},
        {0, indx, TXT_BADD, FCP_BOTH | FCT_BUTN,  1, -3,  9,  3, FC2EM},
        {0, indx, TXT_SRND, FCP_VERT | FCT_CBOX
                                     | FSX_LEFT,  0,  1, 19,  2, FC2EM},
        {0, (intptr_t)engc.ccur.rgpu,
                  TXT_RGPU, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2EM},
        {0, indx, TXT_BDUP, FCP_BOTH | FCT_CBOX,  1, -3,  9,  3, FC2EM},
        {0, indx, TXT_SELE, FCP_VERT | FCT_PBAR,  0,  1, 19,  3, FC2EM},
        {0, indx, TXT_OPTS, FCP_VERT | FCT_BUTN,  0,  1,  9,  6, FC2EM},
        {0, indx, TXT_GOGO, FCP_BOTH | FCT_BUTN
                                     | FSB_DFLT,  1, -6,  9,  6, FC2EM},
        {0, indx, TXT_HEAD, FCP_HORZ | FCT_SBOX,  0,  0, 41, 43, FC2EM},
    },
    octl[] = {
        {0, indx, TXT_OPTS,            FCT_WNDW,  1,  1,  1,  1, FC2EO},

        {0, indx, TXT_UONR,            FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {0, indx, TXT_ETOP, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {0, indx, TXT_EEFF, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {0, indx, TXT_EINT, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {0, indx, TXT_ESAY, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {0, indx, TXT_ERCH, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},

        {0, (intptr_t)engc.ccur.nrun,
                  TXT_RUNS,            FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {0, indx, TXT_RUNS, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {0, (intptr_t)engc.ccur.nsca,
                  TXT_SCAL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {0, indx, TXT_SCAL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {0, (intptr_t)engc.ccur.ndil,
                  TXT_TDIL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {0, indx, TXT_TDIL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {0, (intptr_t)engc.ccur.nsay,
                  TXT_RSAY, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {0, indx, TXT_RSAY, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {0, (intptr_t)engc.ccur.ncdr,
                  TXT_PCDR, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {0, indx, TXT_PCDR, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},

        {0, indx, TXT_OPTS, FCP_VERT | FCT_TEXT
                                     | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {0, indx, TXT_LGUI, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2EO},
        {0, indx, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2EO},
        {0, indx, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {0, indx, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {0, indx, TXT_DFLT, FCP_VERT | FCT_TEXT
                                     | FST_CNTR,  0,  0, 49,  2, FC2EO},

        {0, indx, TXT_OPTS, FCP_VERT | FCT_TEXT
                                     | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {0, indx, TXT_BDIR, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2EO},
        {0, indx, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2EO},
        {0, indx, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {0, indx, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {0, indx, TXT_DFLT, FCP_VERT | FCT_TEXT
                                     | FST_CNTR,  0,  0, 49,  2, FC2EO},

        {0, indx, TXT_OPTS, FCP_VERT | FCT_TEXT
                                     | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {0, indx, TXT_RELO, FCP_VERT | FCT_BUTN, 29,  0, 10,  3, FC2EO},
        {0, indx, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
    };

    engc.octl = MakeWindow(octl, sizeof(octl) / sizeof(*octl));
    engc.mctl = MakeWindow(mctl, sizeof(mctl) / sizeof(*mctl));
    RUN_FC2E(engc.OCT_LREL, MSG_BCLK, 0); /// relocalize!
    UpdateOptionControls(&engc, 1);
    RUN_FE2C(engc.MCT_CAPT, MSG__SHW, 1);

    fptr = Concatenate(0, engc.cini.base, DEF_DSEP, DEF_FLDR);
    ypos = !!(engc.cini.flgs & CSF_UONR);
    xpos = 0;
    do {
        if (~engc.cini.flgs & CSF_UONR) {
            indx = rFindMake(file = Concatenate(0, fptr, DEF_DSEP, "Ponies"));
            while ((temp = rFindFile(indx))) {
                AppendLib(&engc, DEF_CONF, file, temp);
                free(temp);
            }
            free(file);
        }
        engc.cini.flgs &= ~CSF_UONR;
    } while (!xpos++ && (TryGetFromGithub(&engc, "DPE", "RoosterDragon",
                                          "Desktop-Ponies", "master",
                                          "Content", fptr) || ypos));
    free(fptr);
    /// sort engine`s libraries by name, initialize the rendering engine
    qsort(engc.libs, engc.lcnt, sizeof(*engc.libs), linfcmp);
    cEngineCallback(0, ECB_INIT, (intptr_t)&engc.engd);
    for (indx = 0; indx < engc.lcnt; indx++) {
        LoadLibPreview(&engc.libs[indx], engc.engd);
        SetProgress(&engc, TXT_LOAD, indx, engc.lcnt);
    }
    cEngineCallback(engc.engd, ECB_LOAD, 0);
    cEngineCallback(engc.engd, ECB_LOAD, ~0);

    /// hiding the scroll window
    engc.MCT_CHAR.fc2e = FC2EM;
    RUN_FE2C(engc.MCT_CHAR, MSG__SHW, 0);

    SetProgress(&engc, TXT_SELE, 0, engc.lcnt);
    RUN_FE2C(engc.MCT_SELE, MSG_PPOS, 0);
    for (indx = 0; indx < engc.ccnt; indx++)
        RUN_FE2C(engc.MCT_OGRP, MSG_LADD, (intptr_t)engc.ctgs[indx].name);
    RUN_FE2C(engc.MCT_OGRP, MSG__TXT, (intptr_t)engc.tran[TXT_OGRP]);

    /// getting minimal width for a preview from one of the spin controls
    xico = (uint16_t)RUN_FE2C(engc.MCT_SPEC, MSG__GSZ, 0);
    /// constructing previews
    /// barr[0] should be barr[linf->prev], but barr is still unsorted here
    for (indx = 0; indx < engc.lcnt; indx++) {
        xpos = engc.libs[indx].barr[0].unit[0].xdim;
        xpos = (xico > xpos)? xico : xpos;
        engc.libs[indx].pict =
            (CTRL){&engc.MCT_CHAR, (intptr_t)engc.engd, indx,
                    FCT_IBOX, 0, 0, -xpos,
                    (engc.libs[indx].barr[0].unit[0].ydim)?
                   -(long)engc.libs[indx].barr[0].unit[0].ydim : -1, FC2EI};
        engc.libs[indx].spin =
            (CTRL){&engc.MCT_CHAR, (intptr_t)&engc.libs[indx], indx,
                    FCT_SPIN, 0, 0, -xpos, 3, FC2EI};
        engc.libs[indx].capt =
            (CTRL){&engc.MCT_CHAR, (intptr_t)&engc.libs[indx], indx,
                    FCT_TEXT | FST_CNTR, 0, 0, -xpos, 2, FC2EI};
        rMakeControl(&engc.libs[indx].pict, 0, 0);
        rMakeControl(&engc.libs[indx].spin, 0, 0);
        rMakeControl(&engc.libs[indx].capt, 0, 0);
        RUN_FE2C(engc.libs[indx].spin, MSG_NDIM, 50000 << 16);
        RUN_FE2C(engc.libs[indx].spin, MSG_NSET, 0);
        RUN_FE2C(engc.libs[indx].capt, MSG__TXT,
                (intptr_t)engc.libs[indx].name);
    }
    RUN_FC2E(engc.MCT_FLTR, MSG_BCLK, 0);
    engc.seed = InitPRNG(elem = time(0));

    printf("[((RNG))] seed = 0x%08X\n[[[INI]]] %s\n", elem, engc.cfnm);

    fram = calloc(sizeof(*fram), engc.lcnt * 2 + 1);
    fram[0] = (uintptr_t)&engc;
    rInternalMainLoop(&engc.mctl[0], FRM_WAIT, UpdPreview, (intptr_t)fram);
    free(fram);

    RUN_FE2C(engc.MCT_CHAR, MSG__SHW, 0);
    for (indx = 0; indx < engc.lcnt; indx++) {
        rFreeControl(&engc.libs[indx].pict);
        rFreeControl(&engc.libs[indx].spin);
        rFreeControl(&engc.libs[indx].capt);
    }
    /// discarding windows, menus and translations
    FreeWindow(&engc.mctl);
    FreeWindow(&engc.octl);
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
    Concatenate(&conf, "Content,",
               (engc.ccur.base && strcmp(engc.cdef.base, engc.ccur.base))?
                engc.ccur.base : 0);
    Concatenate(&conf, DEF_ENDL, "Language,", engc.ccur.lang);
    sprintf(temp, "%hd,%hd", engc.ccur.nrun[1], runs);
    Concatenate(&conf, DEF_ENDL, "RunsTillUpdate,", temp);
    sprintf(temp, "%hd", engc.ccur.nsca[1]);
    Concatenate(&conf, DEF_ENDL, "BaseScale,", temp);
    sprintf(temp, "%hd", engc.ccur.ndil[1]);
    Concatenate(&conf, DEF_ENDL, "TimeDilation,", temp);
    sprintf(temp, "%hd", engc.ccur.nsay[1]);
    Concatenate(&conf, DEF_ENDL, "RandomSpeech,", temp);
    sprintf(temp, "%hd", engc.ccur.ncdr[1]);
    Concatenate(&conf, DEF_ENDL, "CursorDodge,", temp);
    Concatenate(&conf, DEF_ENDL, "Render,");
    for (indx = 0; indx < countof(uCOM); indx++)
        if (engc.ftmp & uCOM[indx])
            Concatenate(&conf, uSTR[indx], ",");
    Concatenate(&conf, DEF_ENDL, "Flags,");
    for (indx = 0; indx < countof(uCSF); indx++)
        if (engc.ccur.flgs & uCSF[indx])
            Concatenate(&conf, uSTF[indx], ",");
    Concatenate(&conf, DEF_ENDL);
    rSaveFile(engc.cfnm, conf, strlen(conf));
    FreePRNG(&engc.seed);
    free(engc.cdef.lang);
    free(engc.ccur.lang);
    free(engc.cini.lang);
    free(engc.cdef.base);
    free(engc.ccur.base);
    free(engc.cini.base);
    free(engc.elem);
    free(engc.cfnm);
    free(temp);
    free(conf);
    #undef DEF_ENDL
}
