#include <stdarg.h>
#include <limits.h>
#include "exec.h"
#include "ctr/std_ctrs.h"
#include "zip/zip_load.h"

/// a macro to count the capacity of static arrays
#define countof(a) (sizeof(a) / sizeof(*(a)))

/// FE2C / FC2E helper macros
#define RUN_FE2C(trgt, cmsg, data) trgt.fe2c(&trgt, cmsg, data)
#define RUN_FC2E(trgt, cmsg, data) trgt.fc2e(&trgt, cmsg, data)

/** convert degrees to radians  **/ #define DTR_CONV (M_PI / 180.0)
/** convert radians to degrees  **/ #define RTD_CONV (1.0 / DTR_CONV)

/** default comment character   **/ #define DEF_CMNT '\''
/** default token separator     **/ #define DEF_TSEP ','
/** default dir slash (string)  **/ #define DEF_DSEP "/"

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
/**                             **/ #define OCT_ECLR octl[ 6]
/**                             **/ #define OCT_ERCH octl[ 7]
/**                             **/ #define OCT_NRUN octl[ 8]
/**                             **/ #define OCT_TRUN octl[ 9]
/**                             **/ #define OCT_NSCA octl[10]
/**                             **/ #define OCT_TSCA octl[11]
/**                             **/ #define OCT_NDIL octl[12]
/**                             **/ #define OCT_TDIL octl[13]
/**                             **/ #define OCT_NSAY octl[14]
/**                             **/ #define OCT_TSAY octl[15]
/**                             **/ #define OCT_NCDR octl[16]
/**                             **/ #define OCT_TCDR octl[17]
/**                             **/ #define OCT_LCHO octl[20]
/**                             **/ #define OCT_LREL octl[21]
/**                             **/ #define OCT_LRES octl[22]
/**                             **/ #define OCT_LGUI octl[23]
/**                             **/ #define OCT_BCHO octl[26]
/**                             **/ #define OCT_BREL octl[27]
/**                             **/ #define OCT_BRES octl[28]
/**                             **/ #define OCT_BDIR octl[29]
/**                             **/ #define OCT_FREL octl[31]
/**                             **/ #define OCT_FRES octl[32]

enum {
/** framerate limiter in msec   **/ FRM_WAIT = 40,
};
enum {
/// /// /// /// /// /// /// /// /// truth values
/** 'true'                      **/ VAL_TRUE = 0x390A9E10,
/** 'false'                     **/ VAL_FALS = 0xD6A90B70,

/// /// /// /// /// /// /// /// /// section value types
/** 'name'                      **/ SVT_NAME = 0x692C5651,
/** 'effect'                    **/ SVT_EFCT = 0x80720D9F,
/** 'behavior'                  **/ SVT_BHVR = 0x532A0FD4,
/** 'behaviorgroup'             **/ SVT_BGRP = 0xA40004B2,
/** 'categories'                **/ SVT_CTGS = 0x21179D08,
/** 'speak'                     **/ SVT_SAYS = 0xF708913D,

/// /// /// /// /// /// /// /// /// behaviour movement types
/** 'none'                      **/ BMT_NONM = 0xF3B3E074,
/** 'horizontal_only'           **/ BMT_HORM = 0x2359740E,
/** 'vertical_only'             **/ BMT_VERM = 0x4753BD0C,
/** 'diagonal_only'             **/ BMT_DIAM = 0xA988F1D9,
/** 'horizontal_vertical'       **/ BMT_HNVM = 0x9D44367E,
/** 'diagonal_horizontal'       **/ BMT_HNDM = 0x590262FB,
/** 'diagonal_vertical'         **/ BMT_DNVM = 0xE2676419,
/** 'all'                       **/ BMT_ALLM = 0x43E72DD0,
/** 'mouseover'                 **/ BMT_OVRM = 0xB73EDCDE,
/** 'dragged'                   **/ BMT_DRGM = 0x4A4CCCE1,
/** 'sleep'                     **/ BMT_SLPM = 0x62B9B962,

/// /// /// /// /// /// /// /// /// effect alignment types
/** 'top_left'                  **/ EMT_TNLA = 0xE73713ED,
/** 'top'                       **/ EMT_TOPA = 0xA47C2B7E,
/** 'top_right'                 **/ EMT_TNRA = 0xECF514DD,
/** 'left'                      **/ EMT_CNLA = 0x7D6BA6E7,
/** 'center'                    **/ EMT_CNTA = 0x4E745BAB,
/** 'right'                     **/ EMT_CNRA = 0x0F854D3F,
/** 'bottom_left'               **/ EMT_BNLA = 0x884F61CE,
/** 'bottom'                    **/ EMT_BTMA = 0x8819E73B,
/** 'bottom_right'              **/ EMT_BNRA = 0xB9049E02,
/** 'any'                       **/ EMT_RNDA = 0x43E92567,
/** 'any-not_center'            **/ EMT_RCLA = 0xD76D8510,

/// /// /// /// /// /// /// /// /// follow offset type values
/** 'fixed'                     **/ FOT_FIXD = 0x9A8F97BD,
/** 'mirror'                    **/ FOT_MIRR = 0x304E7075,

/// /// /// /// /// /// /// /// /// config file strings
/** 'content'                   **/ CNF_BASE = 0x6558329A,
/** 'language'                  **/ CNF_LANG = 0x1644959C,
/** 'runstillupdate'            **/ CNF_RUNS = 0x7ECC31BE,
/** 'basescale'                 **/ CNF_SCAL = 0x7A285DE0,
/** 'timedilation'              **/ CNF_TDIL = 0x338D79CF,
/** 'randomspeech'              **/ CNF_RSAY = 0x0C4A8F7D,
/** 'cursordodge'               **/ CNF_PCDR = 0xD00CAB48,
/** 'flags'                     **/ CNF_FLGS = 0x8ACE03CE,
/** 'render'                    **/ CNF_RNDR = 0x3C9F6676,
/** 'draw'                      **/ CNF_DRAW = 0xE7ABD6EE,
/** 'show'                      **/ CNF_SHOW = 0x27D90DCD,
/** 'gpu'                       **/ CNF_RGPU = 0x11927E83,
/** 'opaque'                    **/ CNF_OPAQ = 0xD246CFE1,
/** 'wbgra'                     **/ CNF_IBGR = 0xABF3B1E8,
/** 'wpbo'                      **/ CNF_IPBO = 0x78FE3880,
/** 'wregion'                   **/ CNF_IRGN = 0xDE0DCCBE,
/** 'update'                    **/ CNF_UONR = 0xE4895181,
/** 'topmost'                   **/ CNF_ETOP = 0x0622A23D,
/** 'effects'                   **/ CNF_EEFF = 0xAB1F60DF,
/** 'interaction'               **/ CNF_EINT = 0x3CD837AB,
/** 'speech'                    **/ CNF_ESAY = 0x5E664BA6,
/** 'cspeech'                   **/ CNF_ECLR = 0x32A64DBA,
/** 'hover'                     **/ CNF_ERCH = 0x303621E9,
};
enum {
/// /// /// /// /// /// /// /// /// behaviour/effect flags
/** no movement at all          **/ BHV_NONM = 0      ,
/** horizontal movement         **/ BHV_HORM = 1 <<  0,
/** diagonal movement           **/ BHV_DIAM = 1 <<  1,
/** vertical movement           **/ BHV_VERM = 1 <<  2,
/** movement control flag       **/ BHV_CTLM = 1 <<  3,

/** horz + vert movement        **/ BHV_HNVM = BHV_HORM | BHV_VERM,
/** horz + diag movement        **/ BHV_HNDM = BHV_HORM | BHV_DIAM,
/** diag + vert movement        **/ BHV_DNVM = BHV_DIAM | BHV_VERM,
/** horz + diag + vert          **/ BHV_ALLM = BHV_HORM | BHV_DIAM | BHV_VERM,
/** 'mouse-over' state          **/ BHV_OVRM = BHV_CTLM | BHV_HORM,
/** 'dragged' state             **/ BHV_DRGM = BHV_CTLM | BHV_DIAM,
/** 'sleep' state               **/ BHV_SLPM = BHV_CTLM | BHV_VERM,
/** [extractor]                 **/ BHV_MMMM = BHV_CTLM | BHV_ALLM,

/** tgt offs has to be mirrored **/ BHV_MIRR = 1 << 28,

/** effect is a speech          **/ EFF_SAYS = 1 << 29,
/** do not follow parent        **/ EFF_STAY = 1 << 30,

/** animation can be looped     **/ ANI_LOOP = 1 << 31,
};
enum {
/// /// /// /// /// /// /// /// /// must stay as-is; these are used as indices
/** top-left alignment          **/ EFF_TNLA = 0x0,
/** top alignment               **/ EFF_TOPA = 0x1,
/** top-right alignment         **/ EFF_TNRA = 0x2,
/** center-left alignment       **/ EFF_CNLA = 0x3,
/** center alignment            **/ EFF_CNTA = 0x4,
/** center-right alignment      **/ EFF_CNRA = 0x5,
/** bottom-left alignment       **/ EFF_BNLA = 0x6,
/** bottom alignment            **/ EFF_BTMA = 0x7,
/** bottom-right alignment      **/ EFF_BNRA = 0x8,
/** random alignment            **/ EFF_RNDA = 0x9,
/** random centerless alignment **/ EFF_RCLA = 0xA,
/** [extractor]                 **/ EFF_AAAA = 0xF,
};
enum {
/// /// /// /// /// /// /// /// /// this sprite is...
/** ...an effect                **/ PIF_EFCT = 1 <<  0,
/** ...inactive, but reserved   **/ PIF_IRES = 1 <<  1,
/** ...stopped when following   **/ PIF_STOP = 1 <<  2,
/** ..."asleep"                 **/ PIF_SLPM = 1 <<  3,
/** ...dragged                  **/ PIF_DRGM = 1 <<  4,
/** ...under cursor             **/ PIF_OVRM = 1 <<  5,
/** ...controlled by Player 1   **/ PIF_TPL1 = 1 <<  6,
/** ...controlled by Player 2   **/ PIF_TPL2 = 1 <<  7,
/** [special modes` extractor]  **/ PIF_SPEC = PIF_SLPM | PIF_DRGM | PIF_OVRM,
/** ["busy" sprites extractor]  **/ PIF_BUSY = PIF_SPEC | PIF_TPL1 | PIF_TPL2,
};
enum {
/// /// /// /// /// /// /// /// /// flags for ChooseBehaviour
/** first spawn of a sprite     **/ CBF_INIT = 1 <<  0,
/** select a pre-set behaviour  **/ CBF_NEXT = 1 <<  1,
/** do not spawn effects        **/ CBF_DNSE = 1 <<  2,
};
enum {
/// /// /// /// /// /// /// /// /// client specific flags
/** update the animation base   **/ CSF_UONR = 1 <<  0,
/** engine window is top-most   **/ CSF_ETOP = 1 <<  1,
/** behaviour effects are on    **/ CSF_EEFF = 1 <<  2,
/** interactions are on         **/ CSF_EINT = 1 <<  3,
/** speech bubbles are on       **/ CSF_ESAY = 1 <<  4,
/** speech bubbles are colored  **/ CSF_ECLR = 1 <<  5,
/** cursor hover reaction is on **/ CSF_ERCH = 1 <<  6,
};
enum {
/// /// /// /// /// /// /// /// /// localized text constants
/** Remove character            **/ TXT_CDEL = 0,
/** Remove all similar          **/ TXT_ADEL,
/** Sleep / wake up             **/ TXT_CSLP,
/** Sleep / wake up all similar **/ TXT_ASLP,
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
/** Enable colored speech       **/ TXT_ECLR,
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

/// engine data (client side), prototype
typedef struct ENGC ENGC;

/// behaviour/effect unit info (write-once, read-only)
typedef struct {
    AINF  unit[2];  /// image pair
    T2IV  cntr[2];  /// image centers
    T2IV  ptgt;     /// follow target relative coords
    long  prob,     /// probability, 0-1000 (may become integral)
          dmin,     /// minimum duration in msec
          dmax,     /// BHV: maximum duration in msec; EFF: respawn in msec
          neff,     /// number of linked effects
          ieff,     /// effect array index of the first linked effect
          igrp;     /// behaviour group index
    float move;     /// movement speed in pixels per frame
    intptr_t trgt;  /// follow target library index (0 if empty)
    uint32_t temp,  /// temporary storage
             name,  /// BHV: name hash; EFF: target index
             flgs,  /// BHV/EFF: flags
             link,  /// linked behaviour index
             bsay,  /// index of the speech said at the beginning
             esay,  /// index of the speech said at the end
          obfm[2];  /// indices for override behaviours for follow mode
                    /// ([0] = static, [1] = moving)
} BINF;

/// library categories (name string and its hash)
typedef struct {
    uint32_t hash, flgs;
    char    *name;
} CTGS;

/// parallel execution unit data
typedef struct {
    intptr_t desc;
    char *hash, *file, *hreq;
} PARA;

/// BINF ordered group
typedef struct {
    BINF   **barr;  /// behaviours / effects ordered by behaviour group
    long    *narr;  /// bounds of behaviour groups: [0~~)[G0~~~)[G1...
} BGRP;

/// unit library info (write-once, read-only), opaque outside the module
/// [TODO:] interactions
typedef struct {
    CTRL     pict,  /// image box control to preview the sprite
             capt,  /// character name just below the image box
             spin;  /// spin control to set ICNT
    BGRP     srnd,  /// random-capable speeches extracted from EARR
             bbhv,  /// non-0 probability BARR elements ordered by bhv. group
             bsta,  /// stationary behaviours extracted from BARR
             bmov,  /// moving behaviours extracted from BARR
             bovr,  /// mouseover behaviours extracted from BARR
             bdrg;  /// drag behaviours extracted from BARR
    CTR_V(CTGS) ctgs;  /// library categories (hashed and sorted)
    CTR_V(BINF) barr,  /// available behaviours ordered by name hash
                earr;  /// available effects ordered by parent bhv. name hash
    CTR_V(BINF*) bslp;  /// sleep behaviours extracted from BARR
    ENGC    *engc;  /// parent engine
    char    *path,  /// the folder from which the library was built
            *name,  /// human-readable name (may differ from PATH!)
            *scrl;  /// index-prepended name adapted for scrolling
    uint32_t fgsc,  /// foreground speech color
             bgsc;  /// background speech color
    long     nsay,  /// template speech ID + 1 (0 if no speech)
             prev,  /// preview index in sorted BARR
             zcnt,  /// nonzero probability behaviours count
             nnam,  /// [ description line length in uint8_t`s               ]
             noff;  /// [ maximum description line offset                    ]
    struct {
    long     ioff,  /// [ current description line offset                    ]
             ifrm,  /// [ index of the frame played in the main menu         ]
             icnt,  /// [ number of behavioral sprites requested by the user ]
             xoff,  /// [ preview's horz. offset; negative if needs changing ]
             ymin,  /// [ preview's minimal vert. offset                     ]
             ymax,  /// [ preview's maximal vert. offset                     ]
             yold,  /// [ preview's previous maximal vert. offset            ]
             show;  /// [ whether the preview needs to be shown              ]
    uint64_t ttxt,  /// [ description line offset timestamp in the main menu ]
             tfrm;  /// [ frame timestamp in the main menu                   ]
    }        wctx;  /// '----------------- WRITABLE CONTEXT -----------------'
} LINF;

/// actual on-screen sprite, opaque outside the module
typedef struct _PICT {
    struct
    _PICT   *next,  /// linked list support for SortBy*(), only used there
            *boss;  /// the parent sprite (e.g. follow target or effect base)
    LINF    *ulib;  /// unit library which the sprite belongs to
    T2FV     move,  /// movement direction (or parent offset for effects)
             offs;  /// position of the unit`s lower-left corner
    uint32_t fram,  /// current frame
             flgs,  /// sprite-specific flags
             indx,  /// behaviour direction (lowest bit) and index
             iovr,  /// override behaviour indices (lo16 = sta, hi16 = mov)
             ipre;  /// BHV: behaviour before entering a special mode
                    /// EFF: behaviour of the parent
    uint64_t tfrm,  /// BHV: next frame, msec
                    /// EFF: next frame, msec
             tmov,  /// BHV: next movement, msec
                    /// EFF: next respawn, msec (LLONG_MAX if respawned)
             tbhv;  /// BHV: next behaviour, msec
                    /// EFF: expected deletion, msec
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
    MENU     *mspr, /// per-sprite context menu
             *mctx; /// engine`s main context menu
    CTRL     *mctl, /// GUI controls array (main window)
             *octl; /// GUI controls array (options window)
    T4FV     *data; /// main display sequence passed to the renderer
    ENGD     *engd; /// rendering engine handle
    LINF     *libs; /// sprite libraries array
    CTR_V(CTGS) ctgs; /// categories array
    PICT     *pcur, /// the sprite currently picked
             *povr, /// the sprite with cursor over it
            **parr, /// on-screen sprite pointers array
            **elem; /// pointer buffer for SortBy*()
    char    **tran, /// localized text array (ASCIIZ; last item is also 0)
             *cfnm; /// name of the main configuration file
    uint32_t *seed, /// random number generator seed
             *blgp, /// boundaries of library groups in sorted PARR
              lcnt, /// libraries count
              pcnt, /// on-screen sprites count (may differ every frame)
              pmax, /// max. PARR capacity (realloc on exceed)
              ftmp; /// temporary storage for engine flags
    uint64_t  tcur, /// current, dilation-adjusted timestamp
              tpre; /// previous raw timestamp
    float     tacc; /// partial timestamp accumulator
    T3IV      ppos; /// mouse pointer position (z = flags)
    T2IV      dpos, /// drawing area position
              dims, /// drawing area dimensions
              idim; /// tray icon dimensions
    CONF      cdef, /// default configuration
              ccur, /// current configuration
              cini; /// initial configuration read at the start
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



char *ExtractLastDirs(char *path, long dcnt) {
    long iter;

    if (path && ((iter = strlen(path)) > 0)) {
        while (--iter)
            if ((path[iter] == '/') && !--dcnt) {
                ++iter;
                break;
            }
        path += iter;
    }
    return path;
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
            return !0;
        case '\xC2':
            return !!(line[1] == '\xA0');
        case '\xE1':
            return !!((line[1] == '\x9A') && (line[2] == '\x80'));
        case '\xE3':
            return !!((line[1] == '\x80') && (line[2] == '\x80'));
        case '\xEF':
            return !!((line[1] == '\xBB') && (line[2] == '\xBF'));
        case '\xE2':
            switch (line[1]) {
                case '\x81': return !!(line[2] == '\x9F');
                case '\x80':
                    switch (line[2]) {
                        case '\x80': case '\x81': case '\x82': case '\x83':
                        case '\x84': case '\x85': case '\x86': case '\x87':
                        case '\x88': case '\x89': case '\x8A': case '\x8B':
                        case '\xAF': return !0;
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
            *tail = (**tail)? SkipCharUTF8(*tail) : 0;
            if (*temp) {
                if (*temp != tsep)
                    temp = SkipCharUTF8(temp);
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

    sprintf(text = malloc(32 + strlen(engc->tran[tran])),
           (full)? "%s %ld / %ld" : "%s %ld", engc->tran[tran], frac, full);
    RUN_FE2C(engc->MCT_SELE, MSG__TXT, (intptr_t)text);
    RUN_FE2C(engc->MCT_SELE, MSG_PLIM, (full)? full : 100);
    RUN_FE2C(engc->MCT_SELE, MSG_PPOS, (full)? frac : 0);
    free(text);
}



void RecountSelectedLibs(ENGC *engc) {
    long full, frac, indx;

    for (full = frac = indx = 0; indx < engc->lcnt; indx++)
        if (engc->libs[indx].wctx.icnt > 0)
            frac++;
        else if (!engc->libs[indx].wctx.icnt)
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
        curr[0] = 0x67452301; curr[1] = 0xEFCDAB89; curr[2] = 0x98BADCFE;
        curr[3] = 0x10325476; curr[4] = 0xC3D2E1F0;
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
        size = rLoadHTTPS(para->desc, para->hreq, &load, 0, 0);
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



void ZIPCallback(char *name, char *data, long size, void *user) {
    if ((name = strstr(name, DEF_FLDR))
    && ((name += sizeof(DEF_FLDR) - 1)[0] == '/')) {
        name = Concatenate(0, (char*)user, name);
        if (!size)
            rMakeDir(name, 0);
        else
            rSaveFile(name, data, size);
        free(name);
    }
}

void LoadCallback(long size, intptr_t user) {
    SetProgress((ENGC*)user, TXT_LOAD, size >> 10, 0);
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

    char *temp, *file = 0, *path = 0, *text = 0, *tail = 0;
    long size, rlen, iter = 0, full = 0;
    intptr_t para, desc = 0;
    PARA *tmpl;

    if (engc->lcnt || !rMessage(engc->tran[TXT_CTUP], engc->tran[TXT_CCUP],
                                engc->tran[TXT_BYES], engc->tran[TXT_BNAY]))
        return 0;
    if ((desc = rMakeHTTPS(user, GIT_SAPI)) && (iter = rMakeDir(disk, 1))) {
        /// no target directory present, need to populate it first
        SetProgress(engc, TXT_LOAD, 0, 0);
        temp = MakeGetQuery(0, "repos", DEF_DSEP,      auth, DEF_DSEP,
                                  proj, DEF_DSEP, "zipball", DEF_DSEP, bran);
        size = rLoadHTTPS(desc, temp, &path, LoadCallback, (intptr_t)engc);
        temp = realloc(temp, 0);
        if (path) {
            ZIP_Load(path, size, disk, ZIPCallback);
            temp = realloc(path, 0);
        }
    }
    rFreeHTTPS(desc);
    if (iter || (!path && !desc && !iter)) {
        if (!path && (!desc || iter))
            rMessage(engc->tran[TXT_INET], engc->tran[TXT_CCUP],
                     engc->tran[TXT_BYES], 0);
        return 1;
    }
    if ((desc = rMakeHTTPS(user, GIT_SAPI))) {
        temp = MakeGetQuery(0, "repos", DEF_DSEP,  auth, DEF_DSEP,
                                  proj, DEF_DSEP, "git", DEF_DSEP,
                               "trees", DEF_DSEP,  bran);
        rLoadHTTPS(desc, temp, &text, 0, 0);
        temp = realloc(temp, 0);
        if (text) {
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
                    rLoadHTTPS(desc, temp, &tail, 0, 0);
                    temp = realloc(temp, 0);
                }
            }
            text = realloc(text, 0);
            text = (tail)? tail-- : 0;
        }
    }
    rFreeHTTPS(desc);
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
                    if (!rMakeDir(file, 0)) {
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



/// Mersenne random number generator

void RNG_Free(uint32_t **seed) {
    free(*seed);
    *seed = 0;
}

uint32_t *RNG_Make(uint32_t init) {
    const uint32_t size = 624;
    uint32_t *seed;

    (seed = calloc(sizeof(*seed), size + 1))[1] = init;
    for (init = 1; init < size; init++)
        seed[init + 1] = init + (seed[init] ^ (seed[init] >> 30)) * 1812433253;
    return seed;
}

uint32_t RNG_Load(uint32_t *seed) {
    const uint32_t size = 624;
    uint32_t retn, iter;

    if (!seed)
        return 0;

    seed[0] = (iter = seed[0] + 1) % size;
    retn = (seed[iter] & 0x80000000) | (seed[seed[0] + 1] & 0x7FFFFFFF);
    retn = seed[iter] = seed[(iter + 396) % size + 1]
                      ^ (retn >> 1) ^ ((retn & 1)? 0x9908B0DF : 0);
    retn ^= (retn >> 11);
    retn ^= (retn <<  7) & 0x9D2C5680;
    retn ^= (retn << 15) & 0xEFC60000;
    return retn ^ (retn >> 18);
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
    return (what < bmax)? (what > bmin)? what : bmin : bmax;
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
    int64_t retn = (int64_t)(*(BINF**)a)->igrp - (int64_t)(*(BINF**)b)->igrp;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int namecmp(const void *a, const void *b) {
    int64_t retn = (int64_t)((BINF*)a)->name - (int64_t)((BINF*)b)->name;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int esaycmp(const void *a, const void *b) {
    int64_t retn = (int64_t)((BINF*)a)->esay - (int64_t)((BINF*)b)->esay;
    return (retn)? (retn < 0)? -1 : 1 : 0;
}

int bsaycmp(const void *a, const void *b) {
    int64_t retn = (int64_t)((BINF*)a)->bsay - (int64_t)((BINF*)b)->bsay;
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

void MakeSpritePair(BINF *retn, char *path, char **conf) {
    for (long iter = 0; iter <= 1; iter++)
        retn->unit[iter].time = (uint32_t*)Concatenate(
            0, path, DEF_DSEP, Dequote(SplitLine(conf, DEF_TSEP, 0)));
}

void ParseBehaviour(BINF *retn, char *path, char **conf) {
    static uint32_t
        uBMT[] = {BMT_HORM, BMT_ALLM, BMT_VERM, BMT_DRGM, BMT_HNDM, BMT_SLPM,
                  BMT_HNVM, BMT_DIAM, BMT_OVRM, BMT_DNVM, BMT_NONM},
        uBHV[] = {BHV_HORM, BHV_ALLM, BHV_VERM, BHV_DRGM, BHV_HNDM, BHV_SLPM,
                  BHV_HNVM, BHV_DIAM, BHV_OVRM, BHV_DNVM, BHV_NONM};
    uint32_t elem, *iter;
    char *temp;

    /// defaults          (neff, ieff, igrp)---v--v--v
    *retn = (BINF){{}, {}, {}, 0, 5000, 15000, 0, 0, 0, 0.1 * FRM_WAIT, 0, 0, 0,
                   BHV_ALLM | ANI_LOOP, 0, 0, 0, {}};

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
    MakeSpritePair(retn, path, conf);

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

    /// speech said on behaviour start.........................................  def = ""
    if (TRY_TEMP(conf)) { /// for the meaning of '\r' see ParseSpeech
        (temp = Dequote(temp))[-1] = '\r';
        retn->bsay = (*temp)? HashLine(ToLower(temp - 1, 0), 0) : 0;
    }
    /// speech said on behaviour end...........................................  def = ""
    if (TRY_TEMP(conf)) { /// for the meaning of '\r' see ParseSpeech
        (temp = Dequote(temp))[-1] = '\r';
        retn->esay = (*temp)? HashLine(ToLower(temp - 1, 0), 0) : 0;
    }
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
    if (TRY_TEMP(conf)) {
        temp = Dequote(temp);
        /// has to be converted into global library index
        /// as soon as the library array is properly sorted
        if (temp && *temp)
            retn->trgt = (intptr_t)strdup(temp);
    }
    /// automatically determine the images to use when following something.....  def = True
    elem = 0;
    if (TRY_TEMP(conf))
        SET_FLAG(elem, temp, VAL_FALS, 1);

    /// static behaviour for follow mode.......................................  def = ""
    if (TRY_TEMP(conf))
        retn->obfm[0] = (elem)? HashLine(Dequote(temp), 0) : 0;

    /// moving behaviour for follow mode.......................................  def = ""
    if (TRY_TEMP(conf))
        retn->obfm[1] = (elem)? HashLine(Dequote(temp), 0) : 0;

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

void ParseEffect(BINF *retn, char *path, char **conf) {
    static uint32_t
        uEMT[] = {EMT_CNRA, EMT_RNDA, EMT_CNTA, EMT_CNLA, EMT_BTMA, EMT_BNLA,
                  EMT_TOPA, EMT_BNRA, EMT_RCLA, EMT_TNLA, EMT_TNRA},
        uEFF[] = {EFF_CNRA, EFF_RNDA, EFF_CNTA, EFF_CNLA, EFF_BTMA, EFF_BNLA,
                  EFF_TOPA, EFF_BNRA, EFF_RCLA, EFF_TNLA, EFF_TNRA};
    uint32_t elem, *iter;
    char *temp;

    /// defaults      (neff, ieff, igrp)---v--v--v
    *retn = (BINF){{}, {}, {}, 0, 5000, 0, 0, 0, 0, 0.0, 0, 0, 0,
                   ANI_LOOP | EFF_STAY | (EFF_RNDA * 0x1111), 0, 0, 0, {}};

    /// effect name (skipped intentionally).................................... !def
    if (TRY_TEMP(conf)) {};

    /// behaviour name......................................................... !def
    retn->name = HashLine(Dequote(GET_TEMP(conf)), 0);

    /// right-sided image...................................................... !def
    /// left-sided image....................................................... !def
    MakeSpritePair(retn, path, conf);

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

    /// flag to follow parent..................................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, EFF_STAY);

    /// flag to prevent animation looping......................................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->flgs, temp, VAL_FALS, ANI_LOOP);
}

void ParseSpeech(BINF *retn, char *path, char **conf) {
    /// speeches are nothing more than additional effect sprites!
    char *temp;

    /// defaults               v---(prob)   v--v--v---(neff, ieff, igrp)
    *retn = (BINF){{}, {}, {}, 1, 0, 10000, 0, 0, 0, 0.0, 0, 0, 0,
                  (EFF_TOPA << 0) | (EFF_BTMA << 4) | EFF_SAYS
                | (EFF_TOPA << 8) | (EFF_BTMA << 12), 0, 0, 0, {}};

    /// speech name............................................................ !def
    /// N.B.: we prepend speech names with '\r' so that they
    ///       don`t occasionally match some behaviour names!
    (temp = Dequote(GET_TEMP(conf)))[-1] = '\r';
    retn->name = retn->bsay = retn->esay = HashLine(ToLower(temp - 1, 0), 0);

    /// speech text............................................................ !def
    if ((*(*conf)++ == '"') && (temp = SplitLine(conf, '"', 0))) {
        retn->unit[0].time = (uint32_t*)Concatenate(0, temp);
        retn->unit[1].time = 0;
        (*conf) += (**conf == DEF_TSEP)? 1 : 0;
        /// DMAX is 10000 by default and does not change, whereas DMIN
        /// depends on the text length; in DP it equals (L / 15) seconds
        retn->dmin = (1000.0 / 15.0) * strlen((char*)retn->unit[0].time);
        retn->dmin = (retn->dmin)? retn->dmin : FRM_WAIT;
    }
    else {
        retn->unit[0].time = retn->unit[1].time = 0;
        return;
    }
    /// sound files............................................................  def = ""
    if ((*(*conf)++ == '{') && (temp = SplitLine(conf, '}', 0))) {
        /// [TODO:] stop ignoring sounds
        (*conf) += (**conf == DEF_TSEP)? 1 : 0;
    }
    /// flag to never execute this speech at random............................  def = False
    if (TRY_TEMP(conf))
        SET_FLAG(retn->prob, temp, VAL_FALS, retn->prob);

    /// behaviour group index..................................................  def = 0
    if (TRY_TEMP(conf)) {
        retn->igrp = StrToFloat(temp);
        retn->igrp = (retn->igrp > 0)? retn->igrp : 0;
    }
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
        CTR_ASSIGN(engc->libs[engc->lcnt - 1]);
        libs = &engc->libs[engc->lcnt - 1];
        libs->engc = engc;
        libs->path = fptr;

        fptr = file;
        bcnt = ecnt = 0;
        while ((conf = SplitLine(&fptr, '\n', 1)))
            switch (DetermineType(&conf)) {
                case SVT_SAYS:
                case SVT_EFCT:
                    ecnt++;
                    break;

                case SVT_BHVR:
                    bcnt++;
                    break;
            }

        CTR_V_MGET(libs->earr, ecnt);
        CTR_V_MGET(libs->barr, bcnt);
        if (!libs->barr.size) {
            /// no behaviours found, the library is broken; stopping
            engc->lcnt--;
            free(libs->path);
            free(file);
            return;
        }
        fptr = file;
        libs->nsay = bcnt = ecnt = 0;
        libs->name = strdup(path);          /// DP does exactly the same thing
        while ((conf = GetNextLine(&fptr)))
            switch (DetermineType(&conf)) {
                case SVT_NAME:              /// not this, but the one above
//                    libs->name = strdup(GET_TEMP(&conf));
                    break;

                case SVT_SAYS:
                    ParseSpeech(&libs->earr._[ecnt], libs->path, &conf);
                    libs->nsay++;
                    ecnt++;
                    break;

                case SVT_EFCT:
                    ParseEffect(&libs->earr._[ecnt], libs->path, &conf);
                    ecnt++;
                    break;

                case SVT_BHVR:
                    ParseBehaviour(&libs->barr._[bcnt], libs->path, &conf);
                    bcnt++;
                    break;

                case SVT_BGRP:
                    /// doesn`t help much, skipping
                    break;

                case SVT_CTGS:
                    while ((temp = ToLower(Dequote(GET_TEMP(&conf)), 0))) {
                        *temp = toupper(*temp); /// capitalizing first letter
                        CTR_V_PUSH(libs->ctgs,
                                  ((CTGS){HashLine(temp, 0), 0, temp}), 8);
                    }
                    break;
            }
        if (!libs->name)
            libs->name = strdup(path);
        if (libs->ctgs.size) {
            /// sorting categories, removing duplicates, truncating the memory
            CTR_V_SORT(libs->ctgs, ctgscmp);
            for (bcnt = ccnt = 1; ccnt < libs->ctgs.size; ccnt++)
                if (libs->ctgs._[ccnt - 1].hash != libs->ctgs._[ccnt].hash)
                    if (bcnt++ != ccnt)
                        libs->ctgs._[bcnt - 1] = libs->ctgs._[ccnt];
            if (CTR_V_CGET(libs->ctgs) > bcnt)
                CTR_V_MGET(libs->ctgs, bcnt, 1);
            /// now looking for categories previously unknown
            for (hash = ccnt = 0; ccnt < libs->ctgs.size; ccnt++)
                if ((ctgs = bsearch(&libs->ctgs._[ccnt], engc->ctgs._,
                                     engc->ctgs.size, sizeof(*engc->ctgs._),
                                     ctgscmp)))
                    libs->ctgs._[ccnt] = (CTGS){0, 0, ctgs->name};
                else
                    hash++;
            /// some categories need to be added to the global category base
            if (hash)
                CTR_V_MGET(engc->ctgs, engc->ctgs.size + hash);
            ctgs = engc->ctgs._ + engc->ctgs.size - hash;
            for (ccnt = 0; ccnt < libs->ctgs.size; ccnt++)
                if (!libs->ctgs._[ccnt].hash)
                    libs->ctgs._[ccnt].hash =
                        HashLine(libs->ctgs._[ccnt].name, 0);
                else {
                    libs->ctgs._[ccnt].name = strdup(libs->ctgs._[ccnt].name);
                    *ctgs++ = libs->ctgs._[ccnt];
                }
            if (hash)
                CTR_V_SORT(engc->ctgs, ctgscmp);
        }
        free(file);
        conf = 0;
    }
    else
        free(fptr);
    free(conf);
}



void FreeBhvGroup(BGRP *bgrp) {
    free(bgrp->barr);
    free(bgrp->narr);
}

void FreeLib(LINF *elem) {
    for (auto narr = &elem->barr; narr->size; narr = &elem->earr) {
        for (long iter = 0; iter < narr->size * 2; iter++)
            if (!narr->_[iter >> 1].unit[iter & 1].fcnt)
                free(narr->_[iter >> 1].unit[iter & 1].time);
        CTR_V_MGET(*narr);
    }
    CTR_V_MGET(elem->bslp);
    CTR_V_MGET(elem->ctgs); /// nothing to free inside CTGS:
                            /// category names were copied from the engine
    free(elem->path);
    free(elem->name);
    free(elem->scrl);
    FreeBhvGroup(&elem->srnd);
    FreeBhvGroup(&elem->bbhv);
    FreeBhvGroup(&elem->bsta);
    FreeBhvGroup(&elem->bmov);
    FreeBhvGroup(&elem->bovr);
    FreeBhvGroup(&elem->bdrg);
}



void LoadLib(LINF *elem, ENGD *engd) {
    long iter, indx, xdim, ydim, lsrc, ldst;
    char clrs[32];
    uint32_t *bptr;
    uint8_t *name;
    CTR_V(BINF) *narr;
    AINF *temp;

    if (elem->bslp.size || (elem->wctx.icnt <= 0))
        return;
    for (indx = 0; indx <= 1; indx++) {
        narr = (indx)? (typeof(narr))&elem->earr : (typeof(narr))&elem->barr;
        for (iter = 0; iter < narr->size * 2; iter++) {
            auto curr = &narr->_[iter >> 1].unit[iter & 1];

            if (!curr->fcnt) {
                curr->fcnt = 1;
                if (!(narr->_[iter >> 1].flgs & EFF_SAYS)) {
                    name = (uint8_t*)ExtractLastDirs((char*)curr->time, 2);
                    cEngineLoadAnimAsync(engd, curr, name, curr->time,
                                         ELA_DISK, free);
                }
                else {
                    sprintf(clrs, "#%08X / #%08X: ", elem->fgsc, elem->bgsc);
                    name = (uint8_t*)Concatenate(0, clrs, curr->time);
                    ydim = strlen((char*)name) + 1;
                    temp = realloc(curr->time, sizeof(*temp));
                    temp->time = (uint32_t*)(name + 23);
                    temp->xdim = elem->barr._[0].unit[0].xdim << 1;
                    temp->ydim = elem->barr._[0].unit[0].ydim >> 1;
                    xdim = RUN_FE2C(elem->engc->MCT_CAPT,
                                    MSG_WTGD, (intptr_t)temp);
                    /// don`t forget to allocate horizontal space for
                    /// the pointing arrow which is 12 pixels wide!
                    temp->ydim = 4 + 13 + ((uint32_t)xdim >> 16);
                    temp->xdim = 8 +  8 + ((uint32_t)xdim & 0xFFFF);
                    temp->xdim = (temp->xdim > 64)? temp->xdim : 64;
                    xdim = 1 + temp->xdim * temp->ydim;
                    temp = realloc(temp, sizeof(*temp) + ydim
                                       + sizeof(*bptr) * xdim);
                    bptr = (uint32_t*)(temp + 1) + xdim;
                    strncpy((char*)bptr, (char*)name, ydim);
                    free(name);
                    name = (uint8_t*)bptr;
                    temp->uuid = (intptr_t)(bptr = (uint32_t*)(temp + 1) + 1);
                    temp->time = (uint32_t*)(name + 23);
                    /// body
                    for (ydim = temp->ydim - 8 - 1; ydim >= 0; ydim--)
                        for (lsrc = temp->xdim * ydim,
                             xdim = 0; xdim < temp->xdim; xdim++)
                            bptr[lsrc + xdim] =
                                ((ydim < 2) || (ydim >= temp->ydim - 10)
                              || (xdim < 2) || (xdim >= temp->xdim - 2))?
                                    elem->fgsc : elem->bgsc;
                    /// edges
                    for (ydim = 0; ydim < 4; ydim++)
                        for (lsrc = temp->xdim * ydim * 2,
                             ldst = temp->xdim * (temp->ydim - 10) - lsrc,
                             xdim = 0; xdim < 4; xdim++)
                            /** upper right **/
                            bptr[lsrc - xdim * 2 - 2 + temp->xdim * 2] =
                            bptr[lsrc - xdim * 2 - 1 + temp->xdim * 2] =
                            bptr[lsrc - xdim * 2 - 2 + temp->xdim] =
                            bptr[lsrc - xdim * 2 - 1 + temp->xdim] =
                            /** lower right **/
                            bptr[ldst - xdim * 2 - 2 + temp->xdim * 2] =
                            bptr[ldst - xdim * 2 - 1 + temp->xdim * 2] =
                            bptr[ldst - xdim * 2 - 2 + temp->xdim] =
                            bptr[ldst - xdim * 2 - 1 + temp->xdim] =
                            /** upper left **/
                            bptr[lsrc + xdim * 2 + 1 + temp->xdim] =
                            bptr[lsrc + xdim * 2 + 0 + temp->xdim] =
                            bptr[lsrc + xdim * 2 + 1] =
                            bptr[lsrc + xdim * 2 + 0] =
                            /** lower left **/
                            bptr[ldst + xdim * 2 + 1 + temp->xdim] =
                            bptr[ldst + xdim * 2 + 0 + temp->xdim] =
                            bptr[ldst + xdim * 2 + 1] =
                            bptr[ldst + xdim * 2 + 0] =
                                ((xdim < 2) || (ydim < 2))? (xdim * ydim > 1)?
                                  elem->fgsc : 0 : elem->bgsc;
                    /// arrow
                    for (ydim = temp->ydim - 8; ydim < temp->ydim; ydim++)
                        for (lsrc = temp->xdim * ydim,
                             xdim = 0; xdim < temp->xdim; xdim++)
                            bptr[lsrc + xdim] = 0;
                    for (ydim = 0; ydim < 10; ydim++)
                        for (lsrc =  (temp->ydim - ydim) * temp->xdim
                                  - ((temp->xdim * 0xAAAB) >> 17),
                             xdim = -2; xdim < 10; xdim++)
                            bptr[lsrc + xdim] =
                                ((xdim >= 0) && ((xdim & -2) != (ydim & -2)))?
                                                ((xdim & -2) >  (ydim & -2))?
                                                0 : elem->bgsc : elem->fgsc;
                    /// text
                    temp->xdim |= 0x08080000; /// right, left
                    temp->ydim |= 0x0C040000; /// bottom, top
                    temp->fcnt = elem->fgsc;
                    elem->engc->mctl[0].fe2c(elem->engc->mctl,
                                             MSG_WTDA, (intptr_t)temp);
                    temp->xdim &= 0xFFFF;
                    temp->ydim &= 0xFFFF;
                    *(temp->time = (uint32_t*)(temp + 1)) = 0;
                    temp->fcnt = 1;
                    cEngineLoadAnimAsync(engd, curr, name, temp,
                                         ELA_AINF, free);
                }
            }
        }
    }
}



void SortByLib(ENGC *engc) {
    long iter, indx;

    if (engc->pcnt < 1)
        return;

    memset(engc->elem, 0, (engc->lcnt + 1) * sizeof(*engc->elem));
    memset(engc->blgp, 0, (engc->lcnt + 1) * sizeof(*engc->blgp));

    for (iter = engc->pcnt - 1; iter >= 0; iter--) {
        if (!engc->parr[iter])
            engc->pcnt--;
        else {
            indx = (~engc->parr[iter]->flgs & PIF_EFCT)?
                     engc->parr[iter]->ulib - engc->libs + 1 : 0;
            engc->parr[iter]->next = engc->elem[indx];
            engc->elem[indx] = engc->parr[iter];
            engc->blgp[indx]++;
        }
    }
    for (iter = indx = 0; indx <= engc->lcnt; indx++) {
        while (engc->elem[indx]) {
            engc->parr[iter++] = engc->elem[indx];
            engc->elem[indx] = engc->elem[indx]->next;
        }
        engc->blgp[indx] += (indx)? engc->blgp[indx - 1] : 0;
    }
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



void FollowParent(ENGC *engc, PICT *pict, T2IV *move) {
    BINF *barr, *parr, *oarr;
    T2IV dest;
    long iter;
    float dist;

    /// pict->boss and pict->iovr are guaranteed to be nonzero here
    if (move) {
        dest.x = pict->offs.x + move->x;
        dest.y = pict->offs.y + move->y;
        dist = (move->x | move->y)? 1.0 : 0.0;
    }
    else {
        parr = &pict->ulib->barr._[pict->indx >> 1];
        barr = &pict->boss->ulib->barr._[pict->boss->indx >> 1];
        for (iter = 0; iter <= 1; iter++) {
            oarr = &pict->ulib->barr._[(pict->iovr >> (iter << 4)) & 0xFFFF];
            dest.x = ClampToBounds(pict->boss->offs.x
                                 + barr->cntr[pict->boss->indx & 1].x
                              - (((parr->flgs & BHV_MIRR)
                               && (pict->boss->indx & 1))?
                                   parr->ptgt.x : -parr->ptgt.x)
                                 - oarr->cntr[pict->indx & 1].x, 0,
                                   engc->dims.x
                                 - oarr->unit[pict->indx & 1].xdim);
            dest.y = ClampToBounds(pict->boss->offs.y
                                 - barr->cntr[pict->boss->indx & 1].y
                                 + parr->ptgt.y + oarr->cntr[pict->indx & 1].y,
                                   oarr->unit[pict->indx & 1].ydim,
                                   engc->dims.y);
            dist = (dest.x - pict->offs.x) * (dest.x - pict->offs.x)
                 + (dest.y - pict->offs.y) * (dest.y - pict->offs.y);
            if (dist == 0.0)
                break;
        }
        if (dist > parr->move * parr->move)
            dist = parr->move / sqrt(dist);
        else {
            pict->offs.x = dest.x;
            pict->offs.y = dest.y;
        }
    }
    iter = (dist == 0.0) ^ !!(pict->flgs & PIF_STOP);
    pict->flgs = (dist == 0.0)? (pict->flgs |  PIF_STOP)
                              : (pict->flgs & ~PIF_STOP);
    pict->move = (T2FV){{dist * (dest.x - pict->offs.x),
                         dist * (dest.y - pict->offs.y)}};
    if (pict->move.x != 0.0)
        pict->indx = (pict->indx & -2) | ((pict->move.x < 0.0)? 1 : 0);
    if (iter) {
        pict->fram = -1; /// this means:
        pict->tfrm =  0; /// "update me to 0-th frame ASAP!"
    }
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
        anim[0] = &pict->boss->ulib->barr._[pict->boss->indx >> 1].
                                       unit[pict->boss->indx &  1];
        /// effect animation
        anim[1] = &pict->ulib->earr._[pict->indx >> 1].unit[pict->indx & 1];
        /// read centering and placement for the current effect
        desc = pict->ulib->earr._[pict->indx >> 1].flgs
             >> ((pict->indx & 1) << 3); /// direction was inherited from BOSS
        #define CALC(d) ((d[desc & EFF_AAAA] >= 0.0)? d[desc & EFF_AAAA]   \
                       : (float)(RNG_Load(seed) & 0xFFF) * 0.000244140625) \
                       * (float)anim[iter]->d           /* ^-[1 / 4096]-^ */
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



/// INDX is not used if FROM is an effect, as all we need is stored there
long SpawnEffect(PICT **retn, PICT *from, uint32_t *seed,
                 ulong indx, uint64_t time, uint32_t make) {
    BINF *binf;

    if (from->tmov >= LLONG_MAX)
        return 0;  /// no more self-replicating for this effect; exiting

    if (~from->flgs & PIF_EFCT) {
        /// parent = behaviour
        if (make)
            *retn = calloc(1, sizeof(**retn));
        (*retn)->boss = from;
        (*retn)->ulib = from->ulib;
        (*retn)->indx = (indx << 1) | (from->indx & 1);
        (*retn)->flgs |= PIF_EFCT;
    }
    else if (*retn != from) {
        /// parent = effect that needs to be copied to RETN
        if (make)
            *retn = calloc(1, sizeof(**retn));
        **retn = *from;
        from->tmov = LLONG_MAX; /// only one self-replication allowed
    }
    /// ^^^^ along these there is a third way, when an effect gets reused,
    /// and no new effects are created; this keeps sprite count in bounds

    binf = &(*retn)->ulib->earr._[(*retn)->indx >> 1];
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



void ChooseBehaviour(ENGC *engc, PICT *pict, uint32_t next, uint32_t flgs) {
    long seed, lbgn, lend, prev;
    LINF *ulib = pict->ulib;
    BINF *binf, *oinf;
    float angl;

    if (pict->flgs & PIF_EFCT)
        return;

    prev = (pict->boss || (pict->flgs & (PIF_TPL1 | PIF_TPL2)))?
         (((pict->iovr >> ((pict->flgs & PIF_STOP)? 0 : 16)) & 0xFFFF) << 1)
         + (pict->indx & 1) : pict->indx;
    pict->boss = 0;
    if (flgs & CBF_NEXT)
        pict->indx = next;
    else {
        if (ulib->barr._[pict->indx >> 1].link)
            pict->indx = (ulib->barr._[pict->indx >> 1].link - 1) << 1;
        else {
            seed = ulib->barr._[pict->indx >> 1].igrp;
            lbgn = (seed)? ulib->bbhv.narr[seed - 1] : 0;
            lend = ulib->bbhv.narr[seed];
            seed = RNG_Load(engc->seed) % ulib->bbhv.barr[lend - 1]->prob;

            /// nonexact binary search: bsearch() won`t help here
            ///  0th -----,  1st -,  2nd ----,   <  these are the elements
            /// [0;       5)[5;   7)[7;     10)  <  this is the very array
            ///       |      |             `---- SEED = 9: 2nd element
            ///       |      `---- SEED = 5: 1st element
            ///       `---- SEED = 3: 0th element
            while (lbgn < lend)
                if (ulib->bbhv.barr[(lend + lbgn) >> 1]->prob <= seed)
                    lbgn = (lend + lbgn + 2) >> 1;
                else
                    lend = (lend + lbgn + 0) >> 1;
            pict->indx = (ulib->bbhv.barr[lbgn] - ulib->barr._) << 1;
        }
    }
    binf = &pict->ulib->barr._[pict->indx >> 1];
    if (flgs & CBF_INIT) {
        /// this is the first time this sprite appears; let`s put it somewhere
        lbgn = binf->cntr[pict->indx & 1].x << 1;
        lend = binf->cntr[pict->indx & 1].y << 1;
        pict->offs.x = RNG_Load(engc->seed) % (engc->dims.x - lbgn);
        pict->offs.y = RNG_Load(engc->seed) % (engc->dims.y - lend) + lend;
    }
    else {
        /// re-center the upcoming sprite based on the previous center
        pict->offs.x += pict->ulib->barr._[prev >> 1].cntr[prev & 1].x
                     -  binf->cntr[pict->indx & 1].x;
        pict->offs.y -= pict->ulib->barr._[prev >> 1].cntr[prev & 1].y
                     -  binf->cntr[pict->indx & 1].y;
    }
    pict->fram = -1; /// this means:
    pict->tfrm =  0; /// "update me to 0-th frame ASAP!"
    pict->tbhv = (pict->flgs & PIF_BUSY)? LLONG_MAX : engc->tcur + binf->dmin;
    if (binf->dmax > binf->dmin)
        pict->tbhv += RNG_Load(engc->seed) % (binf->dmax - binf->dmin);
    /// now choosing sprite direction
    if ((pict->flgs & (PIF_TPL1 | PIF_TPL2))
    ||  (binf->trgt && !(binf->flgs & BHV_CTLM)
    &&  (prev = engc->blgp[binf->trgt] - engc->blgp[binf->trgt - 1]))) {
        if (!(flgs & CBF_INIT) && !(pict->flgs & (PIF_TPL1 | PIF_TPL2)))
            pict->boss = engc->parr[RNG_Load(engc->seed) % prev
                                  + engc->blgp[binf->trgt - 1]];
        for (pict->iovr = prev = 0; prev <= 1; prev++) {
            if (binf->obfm[prev])
                oinf = &pict->ulib->barr._[binf->obfm[prev] - 1];
            else {
                seed = (prev)? pict->ulib->bmov.narr[binf->igrp]
                     - ((binf->igrp)? pict->ulib->bmov.narr[binf->igrp - 1] : 0)
                     :         pict->ulib->bsta.narr[binf->igrp]
                     - ((binf->igrp)? pict->ulib->bsta.narr[binf->igrp - 1] : 0);
                oinf = (prev)? pict->ulib->bmov.barr[RNG_Load(engc->seed) % seed]
                             : pict->ulib->bsta.barr[RNG_Load(engc->seed) % seed];
            }
            pict->iovr |= (oinf - pict->ulib->barr._) << (prev << 4);
        }
        pict->flgs = ((pict->move.x == 0.0) && (pict->move.y == 0.0))?
                      (pict->flgs | PIF_STOP) : (pict->flgs & ~PIF_STOP);
        prev = pict->iovr >> ((pict->flgs & PIF_STOP)? 0 : 16);
        oinf = &pict->ulib->barr._[prev & 0xFFFF];
        /// re-center again (there`s no better way, since the override)
        pict->offs.x += binf->cntr[pict->indx & 1].x
                     -  oinf->cntr[pict->indx & 1].x;
        pict->offs.y -= binf->cntr[pict->indx & 1].y
                     -  oinf->cntr[pict->indx & 1].y;
        /// do not spawn effects on override
        flgs |= CBF_DNSE;
    }
    else if (!(binf->flgs & BHV_CTLM) && (binf->flgs & BHV_ALLM)) {
        prev = RNG_Load(engc->seed);
        switch (binf->flgs & BHV_ALLM) {
            /// horizontal + diagonal + vertical movement
            case BHV_ALLM:
                prev %= 3;
                prev = (prev)? (prev != 1)? BHV_HORM : BHV_DIAM : BHV_VERM;
                break;

            /// horizontal + vertical movement
            case BHV_HNVM:
                prev = (prev & 1)? BHV_HORM : BHV_VERM;
                break;

            /// horizontal + diagonal movement
            case BHV_HNDM:
                prev = (prev & 1)? BHV_HORM : BHV_DIAM;
                break;

            /// diagonal + vertical movement
            case BHV_DNVM:
                prev = (prev & 1)? BHV_DIAM : BHV_VERM;
                break;

            /// separate movement
            default:
                prev = binf->flgs;
                break;
        }
        switch (prev) {
            case BHV_DIAM:
                prev = (((binf->flgs & BHV_HNVM) == BHV_HNVM) ||
                        !(binf->flgs & BHV_HNVM))? 61 : 31;
                angl =  ((binf->flgs & BHV_HNVM) == BHV_VERM)? 45.0 : 15.0;
                angl = (angl + RNG_Load(engc->seed) % prev) * DTR_CONV;
                break;

            case BHV_VERM:
                angl = 0.5 * M_PI;
                break;

            default:
                angl = 0.0;
                break;
        }
        pict->indx = (pict->indx & -2) | (RNG_Load(engc->seed) & 1);
        pict->move = (T2FV){{cos(angl) * binf->move, sin(angl) * binf->move}};
        if (pict->indx & 1)
            pict->move.x = -pict->move.x;
        if (RNG_Load(engc->seed) & 1)
            pict->move.y = -pict->move.y;
    }
    else {
        if (!(binf->flgs & BHV_CTLM))
            pict->indx = (pict->indx & -2) | (RNG_Load(engc->seed) & 1);
        pict->move = (T2FV){{0.0, 0.0}};
    }
    /// now spawning the behaviour effects, if any (and if allowed to)
    if ((~flgs & CBF_DNSE) && (engc->ccur.flgs & CSF_EEFF))
        for (lend = binf->neff; lend; lend--)
            SpawnEffect(&engc->parr[engc->pcnt++], pict,
                         engc->seed, lend + binf->ieff - 1,
                        (pict->tbhv >= LLONG_MAX)? LLONG_MAX : engc->tcur, 1);
}

long RandByGrp(uint32_t *seed, PICT *pict, BGRP *bgrp) {
    uint32_t indx, flgs;

    indx = bgrp->narr[flgs = pict->ulib->barr._[pict->indx >> 1].igrp];
    indx -= (flgs = (flgs)? bgrp->narr[flgs - 1] : 0);
    return bgrp->barr[flgs + RNG_Load(seed) % indx] - pict->ulib->barr._;
}

long SpecialBehaviour(ENGC *engc, PICT *pict, uint32_t mode) {
    uint32_t flgs = 0, indx = 0;
    LINF *ulib = pict->ulib;
    PICT temp;
    AINF anim;

    if ((pict->flgs & PIF_EFCT) || (mode & (mode - 1)))
        return 0;

    if (pict->flgs & mode) {
        indx = pict->ipre >> 1;
        pict->flgs &= ~mode;
        flgs = CBF_NEXT;
        if (pict->flgs & PIF_SLPM)
            return 0;
    }
    else {
        switch (mode) {
            case PIF_DRGM:
            case PIF_OVRM:
                indx = RandByGrp(engc->seed, pict,
                                (mode == PIF_OVRM)? &ulib->bovr : &ulib->bdrg);
                break;

            case PIF_SLPM:
                /// compatible with RandByGrp() if all sleeping IGRP equal zero !!!
                /// [TODO:] deduplicate
                indx = ulib->bslp._[RNG_Load(engc->seed) % ulib->bslp.size] - ulib->barr._;
                break;

            case PIF_TPL1:
            case PIF_TPL2:
                indx = pict->indx >> 1;
                break;
        }
        if (mode == PIF_OVRM) {
            temp = *pict;
            ChooseBehaviour(engc, &temp, (indx << 1) | (temp.indx & 1),
                            CBF_NEXT | CBF_DNSE);
            anim = (AINF){temp.ulib->barr._[temp.indx >> 1].
                                       unit[temp.indx &  1].uuid,
                          engc->ppos.x - temp.offs.x,
                          engc->ppos.y - temp.offs.y, 0};
            cEngineCallback(engc->engd, ECB_TEST, (intptr_t)&anim);
            if (!anim.fcnt)
                return 0;
        }
        flgs = CBF_NEXT;
        pict->flgs |= mode;
        pict->ipre = pict->indx;
    }
    /// do not duplicate effects if the behaviour stays the same
    if (indx == (pict->indx >> 1))
        flgs |= CBF_DNSE;
    ChooseBehaviour(engc, pict, (indx << 1) | (pict->indx & 1), flgs);
    return 1;
}



long Truncate(CTR_V(BINF) *base) {
    BINF *root = base->_, *iter = root - 1;
    long indx;

    for (indx = 0; indx < base->size; indx++)
        if ((root[indx].unit[0].uuid | root[indx].unit[1].uuid)
        &&  (++iter != &root[indx]))
            *iter = root[indx];
    if ((indx = iter - root + 1) < base->size)
        CTR_V_MGET(*base, indx, 1);
    return base->size;
}



void ExtractStaMov(LINF *elem, BGRP *bgrp, long *fill, long gcnt, long move) {
    long indx;

    bgrp->narr = calloc(gcnt, sizeof(*bgrp->narr));
    for (indx = 0; indx < gcnt; indx++)
        fill[indx] = 0;
    for (indx = 0; indx < elem->barr.size; indx++)
        if ((elem->barr._[indx].move == 0.0) == !move)
            fill[elem->barr._[indx].igrp]++;
    bgrp->narr[0] = fill[0] = (fill[0])? fill[0] : 1;
    for (indx = 1; indx < gcnt; indx++)
        bgrp->narr[indx] = bgrp->narr[indx - 1] + ((fill[indx])? fill[indx] : fill[0]);
    bgrp->barr = calloc(bgrp->narr[gcnt - 1], sizeof(*bgrp->barr));
    for (indx = 0; indx < elem->barr.size; indx++)
        if ((elem->barr._[indx].move == 0.0) == !move)
            bgrp->barr[bgrp->narr[elem->barr._[indx].igrp] - fill[elem->barr._[indx].igrp]--] = &elem->barr._[indx];
    /// if there are absolutely no elements in group 0, assign the preview:
    /// it can not get any worse now, anyway
    if (!bgrp->barr[0])
        bgrp->barr[0] = &elem->barr._[elem->prev];
    for (--gcnt; gcnt; gcnt--)
        if (!bgrp->barr[bgrp->narr[gcnt - 1]])
            for (indx = 0; indx < bgrp->narr[0]; indx++)
                bgrp->barr[bgrp->narr[gcnt - 1] + indx] = bgrp->barr[indx];
}



/// return values:
///  -1: got animations pending upload
///   0: no behaviours found, library freed
///   1: all good, 0 sprites on the screen
///  >1: all good, (return) - 1 sprites on the screen
long AppendSpriteArr(LINF *elem, ENGC *engc) {
    long indx, turn, next, qmax, gcnt, bcnt, *fill;
    int32_t *cgrp, *bgrp, *ggrp;
    BINF temp, *iter;

    if (!elem->bslp.size) {  /// not even 1 sleep behaviour? pending init
        for (indx = elem->barr.size * 2, turn = 0; indx && !turn;
             --indx, turn |= !elem->barr._[indx >> 1].unit[indx & 1].fcnt);
        for (indx = elem->earr.size * 2;           indx && !turn;
             --indx, turn |= !elem->earr._[indx >> 1].unit[indx & 1].fcnt);

        /// skip the lib if it`s got animations pending upload
        if (turn)
            return -1;

        /// saving preview name hash
        temp.name = elem->barr._[elem->prev].name;

        /// shortening all behaviours and effects to save memory
        if (!Truncate(&elem->barr)) {
            /// freeing the library in case it`s got no behaviours
            FreeLib(elem);
            CTR_ASSIGN(*elem);
            return 0;
        }
        Truncate(&elem->earr);
        CTR_V_SORT(elem->earr, namecmp);

        if (elem->nsay) {
            for (indx = elem->earr.size - 1; indx >= 0; indx--)
                if (elem->earr._[indx].flgs & EFF_SAYS) {
                    elem->nsay = indx + 1;
                    break;
                }
            /// [TODO:] deduplicate BSAY and ESAY loops right below!

            /// resolving speeches said on behaviour start
            CTR_V_SORT(elem->barr, bsaycmp);
            for (indx = elem->barr.size - 1; indx >= 0; indx--)
                elem->barr._[indx].temp = 0;
            for (indx = elem->earr.size - 1; indx >= 0; indx--)
                if ((elem->earr._[indx].flgs & EFF_SAYS)
                &&  (iter = bsearch(&elem->earr._[indx], elem->barr._,
                                     elem->barr.size, sizeof(*elem->barr._),
                                     bsaycmp))) {
                    for (turn = iter - elem->barr._ + 1;
                        (turn < elem->barr.size) && (elem->barr._[turn].bsay == iter->bsay); turn++)
                        elem->barr._[turn].temp = indx + 1;
                    for (turn = iter - elem->barr._ - 1;
                        (turn >= 0) && (elem->barr._[turn].bsay == iter->bsay); turn--)
                        elem->barr._[turn].temp = indx + 1;
                    iter->temp = indx + 1;
                }
            for (indx = elem->barr.size - 1; indx >= 0; indx--)
                elem->barr._[indx].bsay = elem->barr._[indx].temp;
            /// resolving speeches said on behaviour end
            CTR_V_SORT(elem->barr, esaycmp);
            for (indx = elem->earr.size - 1; indx >= 0; indx--)
                elem->barr._[indx].temp = 0;
            for (indx = elem->earr.size - 1; indx >= 0; indx--)
                if ((elem->earr._[indx].flgs & EFF_SAYS)
                &&  (iter = bsearch(&elem->earr._[indx], elem->barr._,
                                     elem->barr.size, sizeof(*elem->barr._),
                                     esaycmp))) {
                    for (turn = iter - elem->barr._ + 1;
                        (turn < elem->barr.size) && (elem->barr._[turn].esay == iter->esay); turn++)
                        elem->barr._[turn].temp = indx + 1;
                    for (turn = iter - elem->barr._ - 1;
                        (turn >= 0) && (elem->barr._[turn].esay == iter->esay); turn--)
                        elem->barr._[turn].temp = indx + 1;
                    iter->temp = indx + 1;
                }
            for (indx = elem->barr.size - 1; indx >= 0; indx--)
                elem->barr._[indx].esay = elem->barr._[indx].temp;
        }
        /// determining the effect count and min. index for every behaviour
        /// (note that some behaviours may have no effects)
        CTR_V_SORT(elem->barr, namecmp);
        for (indx = (long)elem->earr.size - 1; indx >= 0; indx--)
            if (!(elem->earr._[indx].flgs & EFF_SAYS)
            &&   (iter = bsearch(&elem->earr._[indx], elem->barr._,
                                  elem->barr.size, sizeof(*elem->barr._),
                                  namecmp))) {
                iter->ieff = indx;
                iter->neff++;
            }
        /// retrieving the new preview position
        if ((iter = bsearch(&temp, elem->barr._, elem->barr.size,
                             sizeof(*elem->barr._), namecmp)))
            elem->prev = iter - elem->barr._;
        /// iterating over all behaviours
        for (indx = 0; indx < elem->barr.size; indx++) {
            /// resolving the linked behaviour, if any
            if ((temp.name = elem->barr._[indx].link)) {
                if ((iter = bsearch(&temp, elem->barr._, elem->barr.size,
                                     sizeof(*elem->barr._), namecmp)))
                    elem->barr._[indx].link = iter - elem->barr._ + 1;
                else
                    elem->barr._[indx].link = 0;
            }
            /// resolving behaviour overrides, if any
            for (turn = 0; turn <= 1; turn++)
                if ((temp.name = elem->barr._[indx].obfm[turn])) {
                    if ((iter = bsearch(&temp, elem->barr._, elem->barr.size,
                                         sizeof(*elem->barr._), namecmp)))
                        elem->barr._[indx].obfm[turn] = iter - elem->barr._ + 1;
                    else {
                        /// no such behaviour, need to select automatically
                        elem->barr._[indx].obfm[turn] = 0;
                    }
                }
            /// resolving image centers if unset (only possible here!)
            for (iter = &elem->barr._[indx], turn = 0; turn <= 1; turn++)
                if (!(iter->cntr[turn].x | iter->cntr[turn].y))
                    iter->cntr[turn] = (T2IV){{iter->unit[turn].xdim >> 1,
                                               iter->unit[turn].ydim >> 1}};
                else
                    iter->cntr[turn].y = (long)iter->unit[turn].ydim
                                       -       iter->cntr[turn].y;
        }

        /// [TODO:] deduplicate

        /// populating SRND::BARR with random speeches, sorting by group number
        elem->srnd.barr = malloc(elem->earr.size * sizeof(*elem->srnd.barr));
        for (turn = indx = 0; indx < elem->earr.size; indx++)
            if ((elem->earr._[indx].flgs & EFF_SAYS) && elem->earr._[indx].prob)
                elem->srnd.barr[turn++] = &elem->earr._[indx];
        qsort(elem->srnd.barr, turn, sizeof(*elem->srnd.barr), igrpcmp);

        /// populating BBHV::BARR with all behaviours, sorting by group number
        elem->bbhv.barr = malloc(elem->barr.size * sizeof(*elem->bbhv.barr));
        for (indx = 0; indx < elem->barr.size; indx++)
            elem->bbhv.barr[indx] = &elem->barr._[indx];
        qsort(elem->bbhv.barr, elem->barr.size, sizeof(*elem->bbhv.barr), igrpcmp);

        if (!turn) { /// turn = random speech count
            free(elem->srnd.barr);
            elem->srnd.barr = 0;
        }
        else {
            ggrp = malloc(elem->earr.size * sizeof(*ggrp));
            ggrp[0] = elem->srnd.barr[0]->igrp;
            for (gcnt = indx = 1; indx < turn; indx++)
                if (igrpcmp(&elem->srnd.barr[indx],
                            &elem->srnd.barr[indx - 1]))
                    ggrp[gcnt++] = elem->srnd.barr[indx]->igrp;

            bgrp = malloc(elem->barr.size * sizeof(*bgrp));
            bgrp[0] = elem->bbhv.barr[0]->igrp;
            for (bcnt = indx = 1; indx < elem->barr.size; indx++)
                if (igrpcmp(&elem->bbhv.barr[indx],
                            &elem->bbhv.barr[indx - 1]))
                    bgrp[bcnt++] = elem->bbhv.barr[indx]->igrp;

            cgrp = malloc((gcnt + bcnt) * sizeof(*cgrp));
            for (indx = 0; indx < gcnt; indx++)
                cgrp[indx] = ggrp[indx];
            for (indx = 0; indx < bcnt; indx++)
                cgrp[gcnt + indx] = bgrp[indx];
            qsort(cgrp, gcnt + bcnt, sizeof(*cgrp), uintcmp);
            for (qmax = next = indx = 0; indx < gcnt + bcnt; indx++)
                if (!indx || (cgrp[indx] != cgrp[indx - 1])) {
                    switch (0x10 * !bsearch(&cgrp[indx], ggrp, gcnt,
                                             sizeof(*ggrp), uintcmp)
                                 + !bsearch(&cgrp[indx], bgrp, bcnt,
                                             sizeof(*bgrp), uintcmp)) {
                        case 0x01: cgrp[qmax++] = (cgrp[indx])?
                                                   INT32_MAX : next++; break;
                        case 0x00: cgrp[qmax++] =              next++; break;
                        case 0x10:                             next++; break;
                    }
                }
            free(bgrp);
            free(ggrp);

            elem->srnd.barr[0]->igrp = cgrp[0];
            for (gcnt = 0, indx = 1; indx < turn; indx++) {
                if (igrpcmp(&elem->srnd.barr[indx],
                            &elem->srnd.barr[indx - 1]))
                    gcnt++;
                elem->srnd.barr[indx]->igrp = cgrp[gcnt];
            }
            free(cgrp);
            qsort(elem->srnd.barr, turn, sizeof(*elem->srnd.barr), igrpcmp);
            for (indx = 0; indx < turn; indx++)
                if (elem->srnd.barr[indx]->igrp >= INT32_MAX)
                    break;
            if (indx < turn) {
                turn = indx;
                elem->srnd.barr =
                    realloc(elem->srnd.barr, indx * sizeof(*elem->srnd.barr));
            }
            if (!turn) {
                free(elem->srnd.barr);
                elem->srnd.barr = 0;
            }
            else {
                /// NEXT has been set in the switch, and
                /// equals the last normalized group index
                elem->srnd.narr = calloc(next, sizeof(*elem->srnd.narr));
                for (indx = 0; indx < turn; indx++)
                    elem->srnd.narr[elem->srnd.barr[indx]->igrp]++;
                for (indx = 1; indx < next; indx++)
                    elem->srnd.narr[indx] += elem->srnd.narr[indx - 1];
            }
        }

        /// counting groups + normalizing their indices to [0; GCNT) interval
        for (gcnt = turn = indx = 0; indx < elem->barr.size; indx++) {
            if (elem->bbhv.barr[indx]->igrp != turn) {
                turn = elem->bbhv.barr[indx]->igrp;
                gcnt++;
            }
            elem->bbhv.barr[indx]->igrp = gcnt;
        }
        gcnt++;

        if (!elem->srnd.narr) /// if no random speech, fill all groups with 0s
            elem->srnd.narr = calloc(gcnt, sizeof(*elem->srnd.narr));

        /// populating BBHV::BARR with nonzero-probability behaviours
        for (elem->zcnt = indx = 0; indx < elem->barr.size; indx++)
            if (elem->barr._[indx].prob)
                elem->bbhv.barr[elem->zcnt++] = &elem->barr._[indx];
        elem->bbhv.barr = realloc(elem->bbhv.barr, elem->zcnt * sizeof(*elem->bbhv.barr));
        /// sorting BBHV::BARR by normalized group index
        qsort(elem->bbhv.barr, elem->zcnt, sizeof(*elem->bbhv.barr), igrpcmp);

        /// filling BBHV::NARR with behaviour group boundaries
        elem->bbhv.narr = calloc(gcnt, sizeof(*elem->bbhv.narr));
        for (turn = indx = 0; indx < elem->zcnt; indx++)
            elem->bbhv.narr[elem->bbhv.barr[indx]->igrp] = indx + 1;
        /// adjusting groups with no members of nonzero probability
        for (indx = 1; indx < gcnt; indx++)
            if (!elem->bbhv.narr[indx])
                elem->bbhv.narr[indx] = elem->bbhv.narr[indx - 1];
        /// turning probabilities into integral probabilities
        for (turn = indx = 0; indx < gcnt; indx++) {
            for (++turn; turn < elem->bbhv.narr[indx]; ++turn)
                elem->bbhv.barr[turn]->prob += elem->bbhv.barr[turn - 1]->prob;
            turn = elem->bbhv.narr[indx];
        }
        fill = calloc(gcnt, sizeof(*fill));

        /// extracting stationary and moving behaviours
        ExtractStaMov(elem, &elem->bsta, fill, gcnt, 0);
        ExtractStaMov(elem, &elem->bmov, fill, gcnt, 1);

        /// extracting sleep behaviours
        for (next = qmax = turn = indx = 0; indx < elem->barr.size; indx++) {
            if (elem->barr._[indx].move < elem->barr._[qmax].move)
                qmax = indx; /// QMAX := index of the slowest moving sprite
            if ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_SLPM)
                next++;
            else if (elem->barr._[indx].prob
                 && !elem->barr._[indx].trgt && !elem->barr._[indx].move)
                turn++;
        }
        CTR_V_MGET(elem->bslp, (!next) ? (!turn) ? 1 : turn : next);
        if (next)
            for (next = indx = 0; indx < elem->barr.size; indx++) {
                if ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_SLPM)
                    elem->bslp._[next++] = &elem->barr._[indx];
            }
        else if (turn)
            for (next = indx = 0; indx < elem->barr.size; indx++) {
                if (elem->barr._[indx].prob
                && !elem->barr._[indx].trgt && !elem->barr._[indx].move)
                    elem->bslp._[next++] = &elem->barr._[indx];
            }
        else
            elem->bslp._[0] = &elem->barr._[qmax];

        /// extracting mouseover behaviours
        elem->bovr.narr = calloc(gcnt, sizeof(*elem->bovr.narr));
        for (indx = 0; indx < gcnt; indx++)
            fill[indx] = 0;
        for (qmax = indx = 0; indx < elem->barr.size; indx++) {
            if (elem->barr._[indx].move < elem->barr._[qmax].move)
                qmax = indx;
            if ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_OVRM)
                elem->bovr.narr[elem->barr._[indx].igrp]++;
            else if (elem->barr._[indx].prob
                 && !elem->barr._[indx].trgt && !elem->barr._[indx].move)
                fill[elem->barr._[indx].igrp]--;
        }
        for (indx = 0; indx < gcnt; indx++) {
            if (elem->bovr.narr[indx] > 0)
                fill[indx] = elem->bovr.narr[indx];
            else
                elem->bovr.narr[indx] = -fill[indx];
            if (!elem->bovr.narr[indx])
                elem->bovr.narr[indx] = fill[indx] =
                    (elem->bovr.narr[0])? elem->bovr.narr[0] : 1;
            elem->bovr.narr[indx] += (indx)? elem->bovr.narr[indx - 1] : 0;
            fill[indx] = (fill[indx] > 0)?
                          elem->bovr.narr[indx] : -elem->bovr.narr[indx];
        }
        elem->bovr.barr = calloc(elem->bovr.narr[gcnt - 1], sizeof(*elem->bovr.barr));
        elem->bovr.barr[0] = &elem->barr._[qmax];
        for (indx = 0; indx < elem->barr.size; indx++) {
            qmax = fill[elem->barr._[indx].igrp];
            if (((qmax > 0) &&
                ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_OVRM))
            ||  ((qmax < 0) && elem->barr._[indx].prob &&
                 !elem->barr._[indx].trgt && !elem->barr._[indx].move)) {
                elem->bovr.barr[((qmax > 0)? qmax : -qmax) - 1] = &elem->barr._[indx];
                fill[elem->barr._[indx].igrp] = qmax - ((qmax > 0)? 1 : -1);
            }
        }
        qmax = 0;
        for (indx = elem->bovr.narr[0]; indx < elem->bovr.narr[gcnt - 1]; indx++)
            if (!elem->bovr.barr[indx]) {
                elem->bovr.barr[indx] = elem->bovr.barr[qmax];
                qmax = (qmax + 1) % elem->bovr.narr[0];
            }

        /// extracting 'dragged' behaviours (uses mouseovers!)
        elem->bdrg.narr = calloc(gcnt, sizeof(*elem->bdrg.narr));
        for (indx = 0; indx < gcnt; indx++)
            fill[indx] = 0;
        for (indx = 0; indx < elem->barr.size; indx++)
            if ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_DRGM)
                elem->bdrg.narr[elem->barr._[indx].igrp]++;
            else if ((elem->barr._[indx].flgs & BHV_MMMM) == BHV_SLPM)
                fill[elem->barr._[indx].igrp]--;
        for (indx = 0; indx < gcnt; indx++) {
            if (elem->bdrg.narr[indx] > 0)
                fill[indx] = elem->bdrg.narr[indx];
            else
                elem->bdrg.narr[indx] = -fill[indx];
            if (!elem->bdrg.narr[indx])
                elem->bdrg.narr[indx] =
                    elem->bovr.narr[indx] - ((indx)? elem->bovr.narr[indx - 1] : 0);
            elem->bdrg.narr[indx] += (indx)? elem->bdrg.narr[indx - 1] : 0;
        }
        elem->bdrg.barr = calloc(elem->bdrg.narr[gcnt - 1], sizeof(*elem->bdrg.barr));
        for (indx = 0; indx < gcnt; indx++)
            if (!fill[indx]) {
                qmax = (indx)? elem->bdrg.narr[indx - 1] : 0;
                turn = (indx)? elem->bovr.narr[indx - 1] : 0;
                for (; qmax < elem->bdrg.narr[indx];)
                    elem->bdrg.barr[qmax++] = elem->bovr.barr[turn++];
            }
        for (indx = 0; indx < elem->barr.size; indx++)
            if ((fill[qmax = elem->barr._[indx].igrp] > 0)
            &&  (elem->barr._[indx].flgs & BHV_MMMM) == BHV_DRGM)
                elem->bdrg.barr[elem->bdrg.narr[qmax] - --fill[qmax] - 1] =
                    &elem->barr._[indx];
            else if ((fill[qmax] < 0)
                 &&  (elem->barr._[indx].flgs & BHV_MMMM) == BHV_SLPM)
                elem->bdrg.barr[elem->bdrg.narr[qmax] + ++fill[qmax] - 1] =
                    &elem->barr._[indx];

        /// fixing one-sided effects and speeches, if any
        for (indx = 0; indx < elem->earr.size; indx++)
            if (!elem->earr._[indx].unit[1].time)
                elem->earr._[indx].unit[1] = elem->earr._[indx].unit[0];

        free(fill);
    }

    if (elem->wctx.icnt <= 0)
        return 1;

    /// now computing Q, the maximum number of effect spawns per behaviour
    /// (Q = 1 when repeat delay D = 0; otherwise, Q = ceil(((E)? E : B) / D)
    /// where E is effect duration and B is maximum behaviour duration)
    /// note that E / D is # of effects created per effect expiration window
    for (qmax = (elem->nsay)? 1 : 0, indx = 0; indx < elem->barr.size; indx++)
        for (turn = 0; turn < elem->barr._[indx].neff; turn++)
            if (!(iter = &elem->earr._[elem->barr._[indx].ieff + turn])->dmax)
                qmax += 2; /// > 1, as ChooseBehaviour() runs before SortByY()
            else
                qmax += 2 + ((iter->dmin)? iter->dmin : elem->barr._[indx].dmax)
                          / iter->dmax;
    engc->pmax += elem->wctx.icnt * (qmax + 1); /// +1 for the sprite itself
    engc->parr = realloc(engc->parr, engc->pmax * sizeof(*engc->parr));
    /// now spawning sprites to the screen and emptying ICNT
    for (indx = 0; indx < elem->wctx.icnt; indx++) {
        engc->pcnt++;
        engc->parr[engc->pcnt - 1] = calloc(1, sizeof(**engc->parr));
        engc->parr[engc->pcnt - 1]->ulib = elem;
        ChooseBehaviour(engc, engc->parr[engc->pcnt - 1],
                        0, CBF_INIT | CBF_DNSE);
        if (elem->nsay) { /// creating a speech template, if needed
            SpawnEffect(&engc->parr[engc->pcnt], engc->parr[engc->pcnt - 1],
                         engc->seed, elem->nsay - 1, engc->tcur, 1);
            engc->parr[engc->pcnt++]->flgs |= PIF_IRES;
        }
    }
    return elem->wctx.icnt + 1;
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
            if (engc->povr == pict)
                engc->povr = 0;
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
            if (engc->povr && (engc->povr->ulib == ulib))
                engc->povr = 0;
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

        case TXT_TPL1:
            pict = (PICT*)item->data;
            SpecialBehaviour(pict->ulib->engc, pict, PIF_TPL1);
            break;

        case TXT_TPL2:
            pict = (PICT*)item->data;
            SpecialBehaviour(pict->ulib->engc, pict, PIF_TPL2);
            break;

        case TXT_CSLP:
            pict = (PICT*)item->data;
            SpecialBehaviour(pict->ulib->engc, pict, PIF_SLPM);
            break;

        case TXT_ASLP:
            ulib = ((PICT*)item->data)->ulib;
            engc = ulib->engc;
            for (indx = 0; indx < engc->pcnt; indx++)
                if (engc->parr[indx]->ulib == ulib)
                    SpecialBehaviour(engc, engc->parr[indx], PIF_SLPM);
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
 4. Speeches are created as "reserved" and never ever go away, they just
    wait in silence till the time comes for a random speech or for some new
    behaviour to start (if the new one has a speech attached, it plays,
    otherwise it is the exit speech from the previous one that gets played)
 **/
uint32_t eUpdFrame(ENGD *engd, T4FV **data, uint32_t *size,
                   uint64_t time, intptr_t user, uint32_t attr,
                   int32_t xptr, int32_t yptr, int32_t isel) {
    ENGC *engc = (ENGC*)user;
    BINF *binf, *btmp;
    LINF *ltmp;
    PICT *pict;
    AINF *anim;
    T2IV  bmov;

    char *temp;
    long  indx, elem, tran;
    uint64_t curr;

//    cEngineCallback(engd, ECB_LOAD, ~0);
//    /// here you can add new sprites!
//    cEngineCallback(engd, ECB_LOAD, 0);

    curr = (engc->tacc += 0.01 * engc->ccur.ndil[1] * (time - engc->tpre));
    pict = engc->pcur;
    engc->tpre = time;
    engc->tacc -= curr;
    engc->tcur += curr;

    /// watch out for reserved-s (they shouldn`t be in [0; ISEL], though)
    if ((attr & UFR_MOUS) && ((isel >= 0) || pict)) {
        if (!pict && ((engc->ppos.z ^ attr) & UFR_LBTN)
        && (pict = engc->pcur = engc->parr[isel])) {
            if (pict->flgs & PIF_EFCT)
                pict = engc->pcur = pict->boss;
            printf("[GRABBED] %s\n", pict->ulib->name);
            /// 'sleeping' sprites shall ignore the dragged state
            if (~pict->flgs & PIF_SLPM)
                SpecialBehaviour(engc, pict, PIF_DRGM);
            engc->ppos.x = xptr - pict->offs.x;
            engc->ppos.y = yptr - pict->offs.y;
        }
        if (pict && (attr & UFR_LBTN)) {
            pict->offs.x = xptr - engc->ppos.x;
            pict->offs.y = yptr - engc->ppos.y;
        }
        else {
            if (pict) {
                printf("[DROPPED] %s\n", pict->ulib->name);
                /// 'sleeping' sprites shall ignore the dragged state
                if (~pict->flgs & PIF_SLPM)
                    SpecialBehaviour(engc, pict, PIF_DRGM);
            }
            else {
                engc->ppos.x = xptr;
                engc->ppos.y = yptr;
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
            engc->mspr[5].flgs |= (pict->flgs & PIF_SLPM)? MFL_VCHK : 0;
            engc->mspr[8].flgs &= ~MFL_VCHK;
            engc->mspr[8].flgs |= (pict->flgs & PIF_TPL1)? MFL_VCHK : 0;
            engc->mspr[9].flgs &= ~MFL_VCHK;
            engc->mspr[9].flgs |= (pict->flgs & PIF_TPL2)? MFL_VCHK : 0;
            rOpenContextMenu(engc->mspr);
        }
    }
    if ((engc->ccur.flgs & CSF_ERCH) && (attr & UFR_MOUS) && !pict) {
        if (engc->povr && ((isel < 0) || (engc->parr[isel] != engc->povr))) {
            /// mouseover finish
            SpecialBehaviour(engc, engc->povr, PIF_OVRM);
            printf("[UNHOVER] %s\n", engc->povr->ulib->name);
            engc->povr = 0;
        }
        if (!engc->povr && (isel >= 0) && engc->parr[isel]
        && !(engc->parr[isel]->flgs & (PIF_EFCT | PIF_SLPM
                                     | PIF_TPL1 | PIF_TPL2))) {
            /// effects, 'sleeping' and controlled sprites shall ignore mouse
            if (SpecialBehaviour(engc, engc->parr[isel], PIF_OVRM)) {
                /// the mouseover actually affected the sprite; reacting
                engc->povr = engc->parr[isel];
                printf("[HOVERED] %s\n", engc->povr->ulib->name);
            }
        }
    }
    SortByLib(engc);
    for (indx = 0; indx < engc->pcnt; indx++) {
        if (!(pict = engc->parr[indx]))
            continue;
        binf = (pict->flgs & PIF_EFCT)? &pict->ulib->earr._[pict->indx >> 1]
                                      : &pict->ulib->barr._[pict->indx >> 1];
        anim = &binf->unit[pict->indx & 1];
        if (pict->flgs & PIF_EFCT) {
            /// effect
            if (engc->povr && (pict->boss == engc->povr)
            && (pict->ipre != engc->povr->indx) && (~binf->flgs & EFF_SAYS)) {
                /// on mouseover, previously bound effects are to be purged
                /// (only the speech sprite has to survive that)
                free(pict);
                engc->parr[indx] = 0;
                continue;
            }
            if (~binf->flgs & EFF_STAY)
                MoveToParent(pict, 0);   /// teleport next to the parent
            if (~binf->flgs & EFF_SAYS) { /// this is not a speech, right?..
                if ((engc->tcur >= pict->tmov)               /// restoring!
                &&!((pict->ipre ^ pict->boss->indx) & ~1)) { /// same behaviour
                    pict->flgs &= ~PIF_IRES; /// drop the reserved state, if any
                    if (engc->tcur >= pict->tbhv)
                        elem = indx;         /// PICT expired, let`s edit inplace
                    else
                        engc->parr[elem = engc->pcnt++] = 0;
                    /// 1 here has no effect when from == to!
                    SpawnEffect(&engc->parr[elem], pict, engc->seed,
                                 0, engc->tcur, 1);
                }
                else if (engc->tcur >= pict->tbhv) { /// expired!
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
            else if (engc->ccur.flgs & CSF_ESAY) { /// ...wrong, this is a speech!
                if ((pict->ipre ^ pict->boss->indx) & ~1) {
                    /// the parent behaviour has changed; updating
                    btmp = pict->boss->ulib->barr._;
                    if (!(elem = btmp[pict->boss->indx >> 1].bsay))
                        elem = btmp[pict->ipre >> 1].esay;
                    if (elem) { /// do not spawn new speech but reuse old
                        pict->flgs &= ~PIF_IRES;
                        SpawnEffect(&engc->parr[indx], pict->boss, engc->seed,
                                     elem - 1, engc->tcur, 0);
                    }
                }
                if (engc->tcur >= pict->tbhv) { /// expired!
                    /// cooldown is in, generating a random speech in advance;
                    /// if either group 0 is empty or RNG tells so, we choose
                    /// the group corresponding to the parent`s current one
                    ltmp = pict->boss->ulib;
                    elem = (!ltmp->srnd.narr[0] || (RNG_Load(engc->seed) & 1))?
                             ltmp->barr._[pict->boss->indx >> 1].igrp : 0;
                    tran = (elem)? ltmp->srnd.narr[elem - 1] : 0;
                    if (!(elem = ltmp->srnd.narr[elem] - tran)) {
                        elem = ltmp->srnd.narr[0];
                        tran = 0; /// falling back to 0th group
                    }
                    if ((RNG_Load(engc->seed)
                      % (engc->ccur.nsay[2] - engc->ccur.nsay[0])
                      >= engc->ccur.nsay[1] - engc->ccur.nsay[0]) || !elem)
                        /// no speech available, will try again after cooldown
                        pict->tmov = pict->tbhv = engc->tcur + binf->dmax;
                    else {
                        /// it`s possible to choose a random speech; choosing
                        tran += RNG_Load(engc->seed) % elem;
                        SpawnEffect(&engc->parr[indx], pict->boss, engc->seed,
                                     ltmp->srnd.barr[tran] - ltmp->earr._,
                                     engc->tcur + binf->dmax, 0);
                        pict->tmov = engc->tcur + binf->dmax;
                    }
                    pict->flgs |= PIF_IRES; /// reserving till it`s time
                }
                else if ((engc->tcur >= pict->tmov)) { /// restoring!
                    pict->flgs &= ~PIF_IRES;
                    pict->tmov = LLONG_MAX;
                }
            }
            else /// disable the sprite if speeches are disallowed
                pict->flgs |= PIF_IRES;
        }
        else if ((engc->tcur >= pict->tbhv)
             || ((pict->fram >= anim->fcnt) && !(binf->flgs & ANI_LOOP))) {
            /// behaviour that needs being changed
            ChooseBehaviour(engc, pict, 0, 0);
            binf = &pict->ulib->barr._[pict->indx >> 1];
        }
        /// correcting BINF in case it has overrides
        /// (either got them from ChooseBehaviour() or from the start)
        if (binf->trgt || (pict->flgs & (PIF_TPL1 | PIF_TPL2))) {
            bmov = (T2IV){{}};
            elem = 0.125 * curr;
            if (pict->flgs & PIF_TPL1) {
                bmov.x += ((attr & UFR_PL1D)? elem : 0)
                       -  ((attr & UFR_PL1A)? elem : 0);
                bmov.y += ((attr & UFR_PL1S)? elem : 0)
                       -  ((attr & UFR_PL1W)? elem : 0);
            }
            if (pict->flgs & PIF_TPL2) {
                bmov.x += ((attr & UFR_PL2D)? elem : 0)
                       -  ((attr & UFR_PL2A)? elem : 0);
                bmov.y += ((attr & UFR_PL2S)? elem : 0)
                       -  ((attr & UFR_PL2W)? elem : 0);
            }
            if ((pict->boss || ((engc->ppos.z ^ attr) & UFR_KEYB))
            && (~pict->flgs & PIF_EFCT))
                FollowParent(engc, pict,
                            ((engc->ppos.z ^ attr) & UFR_KEYB)? &bmov : 0);
            elem = pict->iovr >> ((pict->flgs & PIF_STOP)? 0 : 16);
            binf = &pict->ulib->barr._[elem & 0xFFFF];
        }
        anim = &binf->unit[pict->indx & 1];
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
                pict->offs.x += binf->cntr[ pict->indx & 1].x
                             -  binf->cntr[~pict->indx & 1].x;
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
    engc->ppos.z = attr;

    /// ELEM is the number of sprites skipped, multiplied by -1
    /// (like those with respawn > runtime staying in reserve)
    for (elem = indx = 0; indx < engc->pcnt; indx++)
        if ((pict = engc->parr[indx])->flgs & PIF_IRES)
            elem--; /// this works only because reserved-s are put among
                    /// the sprites which cannot be selected with mouse
        else {
            if (pict->flgs & PIF_EFCT)
                binf = &pict->ulib->earr._[pict->indx >> 1];
            else {
                attr = (pict->boss || (pict->flgs & (PIF_TPL1 | PIF_TPL2)))?
                       (pict->iovr >> ((pict->flgs & PIF_STOP)? 0 : 16))
                     : (pict->indx >> 1);
                binf = &pict->ulib->barr._[attr & 0xFFFF];
            }
            engc->data[indx + elem] =
                (T4FV){{pict->offs.x, pict->offs.y, pict->fram,
                        binf->unit[pict->indx & 1].uuid}};
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
    {.text = engc->tran[TXT_TPL1], .uuid = TXT_TPL1, .func = MMH,
     .flgs = MFL_CCHK},
    {.text = engc->tran[TXT_TPL2], .uuid = TXT_TPL2, .func = MMH,
     .flgs = MFL_CCHK},
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
            (intptr_t)engc->tran[(RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0)
                                & FCS_MARK)? TXT_AGRP : TXT_OGRP]);
    RUN_FE2C(engc->OCT_LGUI, MSG__TXT, (intptr_t)((engc->ccur.lang)?
             engc->ccur.lang : engc->tran[TXT_DFLT]));
    RUN_FE2C(engc->OCT_BDIR, MSG__TXT, (intptr_t)((engc->ccur.base
          && strcmp(engc->ccur.base, engc->cdef.base))?
             engc->ccur.base : engc->tran[TXT_DFLT]));
}



void CreatePreview(LINF *ulib) {
    rMakeControl(&ulib->pict, 0, 0);
    rMakeControl(&ulib->spin, 0, 0);
    rMakeControl(&ulib->capt, 0, 0);
    RUN_FE2C(ulib->spin, MSG_NDIM, 30000 << 16);
    RUN_FE2C(ulib->spin, MSG_NSET,
            (ulib->wctx.icnt < 0)? -ulib->wctx.icnt - 1 : ulib->wctx.icnt);
    RUN_FE2C(ulib->capt, MSG__TXT, (intptr_t)ulib->scrl);
}



void UpdPreview(intptr_t data, uint64_t time) {
    enum {FRM_TEXT = 6}; /// text advancment multiplier of FRM_WAIT
    ENGC *engc = (ENGC*)data;
    LINF *ulib;
    AINF *anim;
    char *name;
    long  iter;

    if (engc->parr)
        return; /// previews are hidden when the engine is active

    for (iter = data = 0; data < engc->lcnt; data++) {
        ulib = &engc->libs[data];
        if (!(anim = ulib->barr._[ulib->prev].unit)->fcnt || !ulib->capt.fe2c)
            continue; /// this preview has been loaded incorrectly, skipping
        if (ulib->noff && (time > ulib->wctx.ttxt)) { /// update text!
            if (++ulib->wctx.ioff >= ulib->noff)
                ulib->wctx.ioff = -ulib->wctx.ioff;
            name = &ulib->scrl[(ulib->wctx.ioff < 0)? -ulib->wctx.ioff
                                                    :  ulib->wctx.ioff];
            name[ulib->nnam + ulib->noff] = '\0';
            if (time > (ulib->wctx.ttxt += FRM_WAIT * FRM_TEXT))
                ulib->wctx.ttxt = time + FRM_WAIT * (iter++ % FRM_TEXT);
            /// ^-- this is done to split text updates into several batches,
            ///     thus lowering the number of simultaneous updates per tick
            RUN_FE2C(ulib->capt, MSG__TXT, (intptr_t)name);
            if (-ulib->wctx.ioff != ulib->noff)
                name[ulib->nnam + ulib->noff] = ' ';
        }
        if (time > ulib->wctx.tfrm) { /// update animation!
            if (++ulib->wctx.ifrm >= anim->fcnt)
                ulib->wctx.ifrm = 0;
            if (time > (ulib->wctx.tfrm += anim->time[ulib->wctx.ifrm]))
                ulib->wctx.tfrm = time; /// > 1 frame skipped, resetting
            RUN_FE2C(ulib->pict, MSG_IFRM,
                    (ulib->wctx.ifrm & 0x3FF) | (anim->uuid << 10));
        }
    }
}



void CategorizePreviews(ENGC *engc) {
    long iter, indx, flgs;
    CTGS temp, *elem;

    flgs = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
    flgs = (flgs & FCS_ENBL)? (flgs & FCS_MARK)? 2 : 1 : 0;
    for (elem = 0, indx = 0; indx < engc->lcnt; indx++) {
        for (iter = 0; iter < engc->ctgs.size; iter++)
            if (engc->ctgs._[iter].flgs & flgs) {
                temp.hash = engc->ctgs._[iter].hash;
                elem = bsearch(&temp, engc->libs[indx].ctgs._,
                                engc->libs[indx].ctgs.size,
                                sizeof(*engc->libs->ctgs._), ctgscmp);
                if (!!elem ^ (flgs - 1))
                    break;
            }
        engc->libs[indx].wctx.icnt =
            ((!elem & !!flgs) ^ (engc->libs[indx].wctx.icnt < 0))?
            -engc->libs[indx].wctx.icnt - 1 : engc->libs[indx].wctx.icnt;
    }
    RecountSelectedLibs(engc);
    RUN_FE2C(engc->MCT_CHAR, MSG_WSZC, 0);
}



void DisplayPreviews(ENGC *engc, long yoff, long ydim, long ywnd) {
    long iter = 0, show = -1, ycap, yspi, apos[2];
    LINF *ulib = engc->libs;

    if (!engc->lcnt || !ulib->capt.fe2c)
        return;

    ycap = (uint16_t)(RUN_FE2C(ulib->capt, MSG__GSZ, 0) >> 16);
    yspi = (uint16_t)(RUN_FE2C(ulib->spin, MSG__GSZ, 0) >> 16);
    for (; iter < engc->lcnt; show = -1, ulib = engc->libs + ++iter) {
        apos[0] = ulib->wctx.yold + ulib->wctx.ymin - ulib->wctx.ymax;
        if ((ulib->wctx.ymax >= yoff) && (ulib->wctx.ymin <= yoff + ywnd)) {
            ulib->wctx.yold = ulib->wctx.ymax;
            if (ulib->wctx.show == (ulib->wctx.icnt < 0))
                show = !ulib->wctx.show;
            if ((ulib->wctx.icnt >= 0) && (ulib->wctx.xoff < 0)) {
                if (!ulib->capt.fe2c)
                    CreatePreview(ulib);
                apos[0] = ulib->wctx.xoff;
                apos[1] = yspi - ulib->wctx.ymax;
                RUN_FE2C(ulib->spin, MSG__POS, (intptr_t)apos);
                apos[1] += ycap;
                RUN_FE2C(ulib->capt, MSG__POS, (intptr_t)apos);
                apos[1] -= ulib->pict.ydim;
                RUN_FE2C(ulib->pict, MSG__POS, (intptr_t)apos);
                ulib->wctx.xoff = -ulib->wctx.xoff;
                show = 1;
            }
        }
        else if ((ulib->wctx.yold >= yoff) && (apos[0] <= yoff + ywnd)) {
            ulib->wctx.yold = ulib->wctx.ymax;
            show = 0;
        }
        if (ulib->capt.fe2c && (show >= 0)) {
            RUN_FE2C(ulib->pict, MSG__SHW, show);
            RUN_FE2C(ulib->capt, MSG__SHW, show);
            RUN_FE2C(ulib->spin, MSG__SHW, show);
            ulib->wctx.show = show;
        }
    }
}



intptr_t RearrangePreviews(ENGC *engc, long skip, long xdim, long ydim) {
    long xinc, yinc, ymax, ycap, yspi, iter, line;
    LINF *ulib = engc->libs;

    if (!engc->lcnt || !ulib->capt.fe2c)
        return -1;

    xinc = skip;
    line = yinc = ymax = 0;
    ycap = (uint16_t)(RUN_FE2C(ulib->capt, MSG__GSZ, 0) >> 16);
    yspi = (uint16_t)(RUN_FE2C(ulib->spin, MSG__GSZ, 0) >> 16);
    for (iter = 0; iter <= engc->lcnt; ulib = engc->libs + ++iter) {
        if ((iter < engc->lcnt) && (ulib->wctx.icnt < 0))
            continue; /// skipping disabled libraries
        if ((iter < engc->lcnt) && (xinc + skip - ulib->pict.xdim <= xdim)) {
            xinc += skip - ulib->pict.xdim;
            ymax = (-ulib->pict.ydim > ymax)? -ulib->pict.ydim : ymax;
            continue;
        }
        /// if we are here, either the line is full or the array ended
        for (xinc = skip, ulib = engc->libs + line;
             line < iter; ulib = engc->libs + ++line) {
            if (ulib->wctx.icnt >= 0) {
                ulib->wctx.ymax = yinc + ymax + ycap + yspi;
                ulib->wctx.ymin = yinc;
                ulib->wctx.xoff = -xinc;
                xinc += skip - ulib->pict.xdim;
            }
        }
        yinc += ymax + ycap + yspi + skip;
        if (iter < engc->lcnt) {
            xinc = skip + skip - ulib->pict.xdim;
            ymax = -ulib->pict.ydim;
        }
    }
    return (yinc - skip > ydim)? yinc - skip : ydim;
}



void UpdateOptionControls(ENGC *engc, long main) {
    static uint32_t
        uCSF[] = {CSF_ETOP,        CSF_ERCH,        CSF_EINT,
                  CSF_ESAY,        CSF_ECLR,        CSF_EEFF,        CSF_UONR};
    CTRL *uCTX[] = {
           &engc->OCT_ETOP, &engc->OCT_ERCH, &engc->OCT_EINT,
           &engc->OCT_ESAY, &engc->OCT_ECLR, &engc->OCT_EEFF, &engc->OCT_UONR,
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
        flag = nctl[1];
        /// this call may alter nctl[1], so its value has to be saved
        uCTN[indx]->fe2c(uCTN[indx], MSG_NDIM,
                        ((uint32_t)nctl[2] << 16) | (uint16_t)nctl[0]);
        uCTN[indx]->fe2c(uCTN[indx], MSG_NSET, flag);
    }
}



intptr_t FC2EO(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    static uint32_t uCSF[] = {CSF_UONR, CSF_ETOP, CSF_EEFF,
                              CSF_EINT, CSF_ECLR, CSF_ESAY, CSF_ERCH};
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
        case TXT_ECLR: indx++;
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
                RUN_FE2C(engc->OCT_ECLR, MSG__ENB, data);
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
            ENGC *engc = (ENGC*)ctrl->data;
            intptr_t retn;

            if (cmsg == MSG_SGIP)
                DisplayPreviews(engc, data, ctrl->fe2c(ctrl, MSG_SGTH, 0),
                               (uint32_t)ctrl->fe2c(ctrl, MSG__GSZ, 0) >> 16);
            else if ((cmsg == MSG_SSID)
                 && ((retn = RearrangePreviews(engc, 8, (uint16_t)data,
                                              (uint16_t)(data >> 16))) >= 0))
                return retn;
            break;
        }
        case TXT_OGRP:
            if (cmsg == MSG_LGST) {
                cmsg = RUN_FE2C(((ENGC*)ctrl->data)->MCT_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                return (((ENGC*)ctrl->data)->ctgs._[data].flgs & cmsg)? 1 : 0;
            }
            else if (cmsg == MSG_LSST) {
                ENGC *engc = (ENGC*)ctrl->data;
                intptr_t prev;

                cmsg = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                prev = (engc->ctgs._[data >> 1].flgs & cmsg)? 1 : 0;
                engc->ctgs._[data >> 1].flgs &= ~cmsg;
                engc->ctgs._[data >> 1].flgs |= (data & 1)? cmsg : 0;
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
                    if (engc->libs[cmsg].wctx.icnt >= 0) {
                        spin = engc->libs[cmsg].wctx.icnt;
                        spin = (spin + data > 0)? spin + data : 0;
                        if (engc->libs[cmsg].spin.fe2c)
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
                CategorizePreviews((ENGC*)ctrl->data);
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
                    if (engc->libs[icon].wctx.icnt == 0)
                        iput[ilen++] = icon;
                /// iterating over the requested random sprites count
                for (icon = RUN_FE2C(engc->MCT_RGPU, MSG_NGET, 0);
                    (icon > 0) && ilen; icon--) {
                    irnd[iput[data = RNG_Load(engc->seed) % ilen]]++;
                    if ((~cmsg & FCS_MARK) && (data < --ilen))
                        iput[data] = iput[ilen];
                }
                /// finally, adding the computed random values to ICNTs
                for (icon = 0; icon < engc->lcnt; icon++)
                    engc->libs[icon].wctx.icnt += irnd[icon];
            }
            /// is there anything selected? let`s find out
            for (icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].wctx.icnt > 0)
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
                if (engc->libs[icon].wctx.icnt > 0)
                    cmsg++;
            SetProgress(engc, TXT_LOAD, 0, cmsg);

            cEngineCallback(engc->engd, ECB_LOAD, ~0);
            for (data = icon = 0; icon < engc->lcnt; icon++)
                if (engc->libs[icon].wctx.icnt > 0) {
                    LoadLib(&engc->libs[icon], engc->engd);
                    SetProgress(engc, TXT_LOAD, ++data, cmsg);
                    RUN_FE2C(engc->MCT_SELE, MSG_PPOS, data);
                }
            cEngineLoadAnimAsync(engc->engd, &igif, (uint8_t*)"/Icon/",
                                 MainIcon, ELA_LOAD, 0);
            cEngineCallback(engc->engd, ECB_LOAD, 0);

            for (libs = engc->libs, icon = 0; icon < engc->lcnt; icon++)
                if (AppendSpriteArr(&engc->libs[icon], engc)) {
                    /// revert random ICNT
                    engc->libs[icon].wctx.icnt -= irnd[icon];
                    if (++libs <= &engc->libs[icon])
                        CTR_ASSIGN(libs[-1], engc->libs[icon]);
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
            engc->pcur = engc->povr = 0;
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

                cmsg = !libs->wctx.icnt;
                libs->wctx.icnt = data;
                if (cmsg == !!data)
                    SetProgress(libs->engc, TXT_SELE,
                                RUN_FE2C(libs->engc->MCT_SELE, MSG_PGET, 0)
                             + ((cmsg && data)? 1 : -1),
                                RUN_FE2C(libs->engc->MCT_SELE, MSG_PGET, 1));
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



void RGB2HSL(uint32_t bgra, float *_hsl) {
    #define min(a, b) ((a < b)? a : b)
    #define max(a, b) ((a > b)? a : b)
    float rrrr = (float)((uint8_t)(bgra >> 16)) / 255,
          gggg = (float)((uint8_t)(bgra >>  8)) / 255,
          bbbb = (float)((uint8_t)(bgra      )) / 255,
          _min = min(min(rrrr, gggg), bbbb),
          _max = max(max(rrrr, gggg), bbbb), mean;
    #undef min
    #undef max
    _hsl[0] = _hsl[1] = _hsl[2] = 0.0;
    if ((_hsl[2] = (_min + _max) * 0.5) <= 0.0)
        return;
    if ((_hsl[1] = mean = _max - _min) > 0.0)
        _hsl[1] /= (_hsl[2] <= 0.5)? (_max + _min) : (2.0 - _max - _min);
    else
        return;
    if (rrrr == _max)
        _hsl[0] = (gggg == _min)? (5.0 + (_max - bbbb) / mean)
                                : (1.0 - (_max - gggg) / mean);
    else if (gggg == _max)
        _hsl[0] = (bbbb == _min)? (1.0 + (_max - rrrr) / mean)
                                : (3.0 - (_max - bbbb) / mean);
    else // (bbbb == _max)
        _hsl[0] = (rrrr == _min)? (3.0 + (_max - gggg) / mean)
                                : (5.0 - (_max - rrrr) / mean);
    _hsl[0] /= 6.0;
}

uint32_t HSL2RGB(float *_hsl) {
    uint8_t rrrr = 255 * _hsl[2], gggg = 255 * _hsl[2], bbbb = 255 * _hsl[2];
    float coef, mean, fraq, mid1, mid2;

    coef = (_hsl[2] <= 0.5)? (          _hsl[2] + _hsl[1] * _hsl[2])
                           : (_hsl[1] + _hsl[2] - _hsl[1] * _hsl[2]);
    if (coef > 0) {
        mean = 2.0 * _hsl[2] - coef;
        fraq = (coef - mean) * (6 * _hsl[0] - (int)(6 * _hsl[0]));
        mid1 = mean + fraq;
        mid2 = coef - fraq;
        switch ((int)(6 * _hsl[0])) {
        case 0: rrrr = 255 * coef; gggg = 255 * mid1; bbbb = 255 * mean; break;
        case 1: rrrr = 255 * mid2; gggg = 255 * coef; bbbb = 255 * mean; break;
        case 2: rrrr = 255 * mean; gggg = 255 * coef; bbbb = 255 * mid1; break;
        case 3: rrrr = 255 * mean; gggg = 255 * mid2; bbbb = 255 * coef; break;
        case 4: rrrr = 255 * mid1; gggg = 255 * mean; bbbb = 255 * coef; break;
        case 5: rrrr = 255 * coef; gggg = 255 * mean; bbbb = 255 * mid2; break;
        }
    }
    return bbbb | ((uint32_t)gggg << 8) | ((uint32_t)rrrr << 16) | 0xFF000000;
}

int li64cmp(const void *a, const void *b) {
    int64_t retn = *(int64_t*)a - *(int64_t*)b;
    return (retn >= 0)? (retn > 0)? 1 : 0 : -1;
}

int bgracmp(const void *a, const void *b) {
    return *(int32_t*)a - *(int32_t*)b;
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
        uCNF[] = {CNF_ETOP, CNF_ERCH, CNF_ECLR,
                  CNF_EINT, CNF_ESAY, CNF_EEFF, CNF_UONR},
        uCSF[] = {CSF_ETOP, CSF_ERCH, CSF_ECLR,
                  CSF_EINT, CSF_ESAY, CSF_EEFF, CSF_UONR};
    static char
       *uSTR[] = {"GPU",    "Show",   "wPBO",
                  "wBGRA",  "Opaque", "wRegion","Draw"},
       *uSTF[] = {"Topmost","Hover",  "CSpeech",
              "Interaction","Speech", "Effects","Update"};
    const float ldif = 0.25;
    float mean, _hsl[2][3];
    char *file, *fptr, *conf, *temp;
    uint32_t elem, *iter; /// for IF_BIN_FIND and RNG_Make
    uint64_t clrs[3], tclr;
    intptr_t indx;
    int16_t runs = 0;
    LINF *ulib;
    AINF atmp;

    ENGC engc = {.tcur = 1, .ftmp = COM_SHOW | COM_DRAW | COM_RGPU};

    engc.cini = engc.cdef = (CONF){0, strdup(base), CSF_EEFF | CSF_EINT
                                       | CSF_ESAY | CSF_ECLR | CSF_ERCH,
    /** runs between updates  **/ {   0,   5,  1000},
    /** base scaling factor   **/ {  25, 100,   300},
    /** time dilation factor  **/ {  10, 100,  1000},
    /** random speech chance  **/ {   0,  50,   100},
    /** cursor dodge radius   **/ {   0,   0,  1000},
    /** group selection       **/ {-100,   0,   100},
    /** random selection      **/ {   0,   0, 30000}};
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

                /** [TODO:] deduplicate **/
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
    engc.elem = calloc(0x4000, sizeof(*engc.elem));
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
        {0, indx, TXT_ECLR, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
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

    /// getting minimal width for a preview from one of the spin controls
    xico = (uint16_t)RUN_FE2C(engc.MCT_SPEC, MSG__GSZ, 0);
    /// getting the width of a single space character
    atmp.xdim = atmp.ydim = 0;
    atmp.time = (uint32_t*)" ";
    yico = (uint16_t)RUN_FE2C(engc.MCT_CAPT, MSG_WTGD, (intptr_t)&atmp);
    /// showing the scroll window
    engc.MCT_CHAR.fc2e = FC2EM;
    RUN_FE2C(engc.MCT_CHAR, MSG__SHW, 1);

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
    engc.blgp = calloc(engc.lcnt + 1, sizeof(*engc.blgp));
    /// sort engine`s libraries by name, initialize the rendering engine
    qsort(engc.libs, engc.lcnt, sizeof(*engc.libs), linfcmp);
    cEngineCallback(0, ECB_INIT, (intptr_t)&engc.engd);

    for (indx = 0; indx < engc.ctgs.size; indx++)
        RUN_FE2C(engc.MCT_OGRP, MSG_LADD, (intptr_t)engc.ctgs._[indx].name);
    RUN_FE2C(engc.MCT_OGRP, MSG__TXT, (intptr_t)engc.tran[TXT_OGRP]);

    /// now constructing previews
    for (ulib = engc.libs, indx = 0; indx < engc.lcnt;
         ulib = engc.libs + ++indx) {
        if (!ulib->barr._[0].unit[0].fcnt) {
            ulib->barr._[0].unit[0].fcnt = 1;
            temp = ExtractLastDirs((char*)ulib->barr._[0].unit[0].time, 2);
            cEngineLoadAnimAsync(engc.engd, &ulib->barr._[0].unit[0],
                                (uint8_t*)temp, ulib->barr._[0].unit[0].time,
                                 ELA_DISK, free);
        }
        /// and resolving follow targets for all behaviours in this library
        for (elem = 0; elem < ulib->barr.size; elem++) {
            LINF *iter, temp;

            if ((temp.name = (char*)ulib->barr._[elem].trgt)
            &&  (iter = bsearch(&temp, engc.libs, engc.lcnt,
                                sizeof(*engc.libs), linfcmp)))
                ulib->barr._[elem].trgt = iter - engc.libs + 1;
            else
                ulib->barr._[elem].trgt = 0;
            free(temp.name);
        }
        /// initial library preparations complete, reporting progress
        SetProgress(&engc, TXT_LOAD, indx, engc.lcnt);
    }
    cEngineCallback(engc.engd, ECB_LOAD, 0);
    cEngineCallback(engc.engd, ECB_LOAD, ~0);

    /// still constructing previews
    for (ulib = engc.libs, indx = 0; indx < engc.lcnt;
         ulib = engc.libs + ++indx) {
        xpos = ulib->barr._[0].unit[0].xdim;
        xpos = (xico > xpos)? xico : xpos;
        ulib->pict = (CTRL){&engc.MCT_CHAR, (intptr_t)engc.engd, indx,
                             FCT_IBOX, 0, 0, -xpos,
                             (ulib->barr._[0].unit[0].ydim)?
                            -(long)ulib->barr._[0].unit[0].ydim : -1, FC2EI};
        ulib->spin = (CTRL){&engc.MCT_CHAR, (intptr_t)ulib, indx,
                             FCT_SPIN, 0, 0, -xpos, 3, FC2EI};
        ulib->capt = (CTRL){&engc.MCT_CHAR, (intptr_t)ulib, indx,
                             FCT_TEXT | FST_CNTR, 0, 0, -xpos, 2, FC2EI};
        /// calculating library name offsets
        ulib->scrl = calloc(1, ulib->nnam = strlen(ulib->name) + 16);
        sprintf(ulib->scrl, "%d. %s", (int)indx + 1, ulib->name);
        atmp.xdim = atmp.ydim = 0;
        atmp.time = (uint32_t*)ulib->scrl;
        ypos = (uint16_t)RUN_FE2C(engc.MCT_CAPT, MSG_WTGD, (intptr_t)&atmp);
        ulib->noff = 0;
        if (xpos < ypos) {
            /// name width greater than the name field width, need to scroll
            ulib->nnam = strlen(ulib->scrl);
            ulib->noff = ceil((float)(ypos - xpos) / (float)yico) + 2;
            for ((file = malloc(ulib->noff + 1))[ypos = ulib->noff] = 0;
                  ypos; file[--ypos] = ' ');
            fptr = Concatenate(0, file, ulib->scrl, file);
            free(ulib->scrl);
            free(file);
            ulib->scrl = fptr;
        }
        if (!indx)
            CreatePreview(ulib); /// need that preview for others to work

        ulib->fgsc = 0xFF000000;
        ulib->bgsc = 0xFFFFFFFF;
        /// calculating speech colors if there are speeches in
        /// the library and the "colored speech" flag is set
        if (ulib->nsay && (engc.ccur.flgs & CSF_ECLR)) {
            atmp.fcnt = 0;
            atmp.xdim = ulib->barr._[0].unit[0].xdim;
            atmp.ydim = ulib->barr._[0].unit[0].ydim;
            atmp.uuid = ulib->barr._[0].unit[0].uuid;
            atmp.time =
                calloc(xpos = atmp.xdim * atmp.ydim, sizeof(*atmp.time));
            cEngineCallback(engc.engd, ECB_DRAW, (intptr_t)&atmp);
            qsort(atmp.time, xpos, sizeof(*atmp.time), bgracmp);
            for (tclr = clrs[0] = clrs[1] = clrs[2] = 0; xpos >= 0; xpos--)
                if (xpos && ((tclr & 0xFFFFFFFFUL) == atmp.time[xpos - 1]))
                    tclr += 0x100000000UL;
                else {
                    if ((tclr & 0xFF000000)
                    && ((tclr >> 32) >= (clrs[2] >> 32))) {
                        if ((tclr >> 32) < (clrs[1] >> 32))
                            clrs[2] = tclr;
                        else if ((tclr >> 32) < (clrs[0] >> 32)) {
                            clrs[2] = clrs[1];
                            clrs[1] = tclr;
                        }
                        else {
                            clrs[2] = clrs[1];
                            clrs[1] = clrs[0];
                            clrs[0] = tclr;
                        }
                    }
                    tclr = (xpos)? atmp.time[xpos - 1] | 0x100000000UL : 0;
                }
            free(atmp.time);
            #define LUMA(c) (299 * (uint8_t)(c >> 16) + /* -R /G |B */ \
                             587 * (uint8_t)(c >> 8) + 114 * (uint8_t)c)
            clrs[0] = ((uint64_t)LUMA(clrs[0]) << 32) + (uint32_t)clrs[0];
            clrs[1] = ((uint64_t)LUMA(clrs[1]) << 32) + (uint32_t)clrs[1];
            clrs[2] = ((uint64_t)LUMA(clrs[2]) << 32) + (uint32_t)clrs[2];
            #undef LUMA
            qsort(clrs, 3, sizeof(*clrs), li64cmp);
            RGB2HSL(clrs[0], _hsl[0]);
            RGB2HSL(clrs[2], _hsl[1]);
            if (_hsl[1][2] - _hsl[0][2] < 2 * ldif) {
                mean = 0.5 * (_hsl[1][2] + _hsl[0][2]);
                if (mean + ldif > 1)
                    _hsl[0][2] = mean - 2 * ldif;
                else if (mean - ldif < 0)
                    _hsl[1][2] = mean + 2 * ldif;
                else {
                    _hsl[0][2] = mean - ldif;
                    _hsl[1][2] = mean + ldif;
                }
            }
            ulib->fgsc = HSL2RGB(_hsl[0]);
            ulib->bgsc = HSL2RGB(_hsl[1]);
        }
    }
    RUN_FC2E(engc.MCT_FLTR, MSG_BCLK, 0);
    CategorizePreviews(&engc);
    engc.seed = RNG_Make(elem = time(0));

    printf("[((RNG))] seed = 0x%08X\n[[[INI]]] %s\n", elem, engc.cfnm);

    rInternalMainLoop(&engc.mctl[0], FRM_WAIT, UpdPreview, (intptr_t)&engc);

    RUN_FE2C(engc.MCT_CHAR, MSG__SHW, 0);

    for (ulib = &engc.libs[indx = 0]; indx < engc.lcnt;
         ulib = &engc.libs[++indx]) {
        rFreeControl(&ulib->pict);
        rFreeControl(&ulib->spin);
        rFreeControl(&ulib->capt);
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

    for (unsigned iter = 0; iter < engc.ctgs.size; iter++)
        free(engc.ctgs._[iter].name);
    CTR_V_MGET(engc.ctgs);

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
    RNG_Free(&engc.seed);
    free(engc.cdef.lang);
    free(engc.ccur.lang);
    free(engc.cini.lang);
    free(engc.cdef.base);
    free(engc.ccur.base);
    free(engc.cini.base);
    free(engc.blgp);
    free(engc.elem);
    free(engc.cfnm);
    free(temp);
    free(conf);
    #undef DEF_ENDL
}
