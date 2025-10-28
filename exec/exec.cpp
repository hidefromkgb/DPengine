#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include "exec.h"
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

/// flag enum support!
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator~ (T  a) { return (T)(~(uint32_t)(a)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator| (T  a, T b) { return (T)((uint32_t )(a) |  (uint32_t)(b)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator& (T  a, T b) { return (T)((uint32_t )(a) &  (uint32_t)(b)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator^ (T  a, T b) { return (T)((uint32_t )(a) ^  (uint32_t)(b)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator|=(T &a, T b) { return (T)((uint32_t&)(a) |= (uint32_t)(b)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator&=(T &a, T b) { return (T)((uint32_t&)(a) &= (uint32_t)(b)); }
template<class T> constexpr typename std::enable_if<std::is_enum_v<T>, T>::type
operator^=(T &a, T b) { return (T)((uint32_t&)(a) ^= (uint32_t)(b)); }



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
    while ((temp = va_arg(list, __typeof__(temp))))
        size += strlen(temp);
    va_end(list);

    if (!retn)
        head = (char*)calloc(1, size);
    else {
        head = *retn;
        head = (char*)realloc(head, size += (head)? strlen(head) : 0);
        if (!*retn)
            *head = 0;
        *retn = head;
    }

    va_start(list, retn);
    while ((temp = va_arg(list, __typeof__(temp))))
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

size_t HashLine(char *line) { return std::hash<std::string>{}(line); }



class nocopy {
public:
    nocopy() = default;
    nocopy(nocopy&) = delete;                  /// non construction-copyable
    nocopy(const nocopy&) = delete;            /// non construction-copyable
    nocopy& operator=(nocopy&) = delete;       /// non copyable
    nocopy& operator=(const nocopy&) = delete; /// non copyable
};

class library;

class unit : public nocopy {
private:
    enum state_flags { /// is_ready & !is_primary == this side is a copy
        empty        = 0,
        is_primary   = 1 << 0,
        is_scheduled = 1 << 1,
        is_ready     = 1 << 2,
        from_path    = 1 << 4,
        fake_center  = 1 << 5,
    };
    state_flags flags_[2];
    AINF image_[2];
    T2IV center_[2];
    bool loop_;
    ENGD &engd_;
    library &library_;

    static void finalize(void *data) {
        data = ((uint8_t*)data) - sizeof(intptr_t);
        *((state_flags*)data) &= ~is_scheduled;
        *((state_flags*)data) |= is_ready;
        free(data);
    }
    void remove(bool left) {
        assert(!(flags_[left] & is_scheduled));
        if (!(flags_[left] & is_ready) && image_[left].time)
            finalize(image_[left].time);
        image_[left] = (AINF){};
        flags_[left] = empty;
    }
    void add_source(bool left) {
        remove(left);
        flags_[left] |= is_primary;
    }

protected:
    void set_center(bool left, const T2IV &center) {
        center_[left] = center;
        if ((center.x != 0) || (center.y != 0))
            flags_[left] |= fake_center;
        else
            flags_[left] &= ~fake_center;
    }
    void set_primary(bool left, bool what) {
        if (what)
            flags_[left] |= is_primary;
        else
            flags_[left] &= ~is_primary;
    }

public:
    unit(ENGD &engd, library &l): flags_{empty, empty}, image_{}, center_{},
        loop_(true), engd_(engd), library_(l) {}
    ~unit() { remove(false); remove(true); }

    void add_source(bool left, const std::string &name) {
        add_source(left);
        /// here we prepare a structure that contains the string above byte 0,
        /// and a pointer to the corresponding flags below byte 0
        char *retn = (char*)malloc(name.size() + 1 + sizeof(intptr_t));
        strncpy(retn + sizeof(intptr_t), name.c_str(), name.size() + 1);
        *((intptr_t*)retn) = (intptr_t)(&flags_[left]);
        image_[left].time = (uint32_t*)(retn + sizeof(intptr_t));
        flags_[left] |= from_path;
    }
    void add_source(bool left, const std::string &name, uint32_t xdim,
                    uint32_t ydim, const std::function<void(uint32_t*)> &draw) {
        add_source(left);
        /// the layout is as follows:
        ///  -ptr: &flags
        ///     0: AINF
        /// +AINF: buf
        ///  +buf: time
        /// +time: name
        auto data = malloc(sizeof(intptr_t) + sizeof(AINF) + name.size() + 1
                         + sizeof(uint32_t) * (xdim * ydim + 1));
        *((intptr_t*)data) = (intptr_t)(&flags_[left]);
        auto anim = (AINF*)(((intptr_t*)data) + 1);
        anim->uuid = (intptr_t)(anim + 1);
        anim->time = ((uint32_t*)anim->uuid) + xdim * ydim;
        anim->xdim = xdim;
        anim->ydim = ydim;
        anim->time[0] = 0; /// single frame only for this image type
        strncpy((char*)(anim->time + 1), name.c_str(), name.size() + 1);
        image_[left].time = (uint32_t*)anim;
        draw((uint32_t*)anim->uuid); /// drawing something in the buffer
    }
    void schedule_load(bool left) {
        if (!(flags_[left] & is_ready) && (flags_[left] & is_primary)) {
            flags_[left] |= is_scheduled;
            if (flags_[left] & from_path) {
                auto temp = ExtractLastDirs((char*)image_[left].time, 2);
                cEngineLoadAnimAsync(&engd_, &image_[left], (uint8_t*)temp,
                                     image_[left].time, ELA_DISK, finalize);
            }
            else if (auto anim = (AINF*)image_[left].time) {
                auto name = (uint8_t*)(anim->time + 1);
                cEngineLoadAnimAsync(&engd_, &image_[left], name, anim,
                                     ELA_AINF, finalize);
            }
        }
    }
    T2IV dims(bool left) {
        return (T2IV){(int32_t)image_[left].xdim, (int32_t)image_[left].ydim};
    }
    uint32_t next_frame(bool left, uint32_t curr) {
        return (curr + 1 >= image_[left].fcnt) ? (loop_) ? 0 : curr : curr + 1;
    }
    bool is_empty() {
        return !(flags_[0] & is_primary) && !(flags_[1] & is_primary);
    }
    library &get_library() { return library_; }
    void set_loop(bool loop) { loop_ = loop; }
};

class effect : public unit {
protected:
    enum gravity_flags {
        top_left     = 1,
        top          = 2,
        top_right    = 3,
        center_left  = 4,
        center       = 5,
        center_right = 6,
        bottom_left  = 7,
        bottom       = 8,
        bottom_right = 9,
        any          = 10,
        not_center   = 11,
    };
    struct {
        gravity_flags placement:4;
        gravity_flags centering:4;
    } flags_[2];
    bool skip_; /// no such flag in effects, it is here for speeches
    bool parent_follow_;
    uint32_t duration_;
    uint32_t respawn_;
    size_t parent_name_;

public:
    effect(ENGD &engd, library &l):
        unit(engd, l), flags_{{any, any}, {any, any}}, skip_(true),
        parent_follow_(false), duration_(5000), respawn_(0), parent_name_(0) {}
    effect(ENGD &engd, library &l, const std::string &path, char **conf):
        effect(engd, l) {
        static std::unordered_map<std::string, gravity_flags> gravity = {
            {"any",            any        }, {"any-not_center", not_center  },
            {"top_left",       top_left   }, {"top_right",      top_right   },
            {"top",            top        }, {"bottom",         bottom      },
            {"bottom_left",    bottom_left}, {"bottom_right",   bottom_right},
            {"left",           center_left}, {"right",          center_right},
            {"center",         center     },
        };
        static std::unordered_map<std::string, bool>
            booleans = { {"false", false}, {"true", true}, };
        auto GET_TEMP = [](char **conf) { return SplitLine(conf, DEF_TSEP, 0); };
        char *temp;

        /// effect name (skipped intentionally)................................ !def
        if ((temp = GET_TEMP(conf)) && *temp) {};

        /// behaviour name..................................................... !def
        parent_name_ = HashLine(ToLower(Dequote(GET_TEMP(conf)), 0));

        /// right-sided image.................................................. !def
        /// left-sided image................................................... !def
        for (unsigned iter = 0; iter <= 1; iter++) {
            auto name = path + DEF_DSEP + Dequote(SplitLine(conf, DEF_TSEP, 0));
            add_source(iter, name);
        }

        /// duration in sec....................................................  def = 5
        if ((temp = GET_TEMP(conf)) && *temp)
            duration_ = StrToFloat(temp) * 1000.0;

        /// respawn in sec.....................................................  def = 0 (no respawn)
        if ((temp = GET_TEMP(conf)) && *temp)
            respawn_ = StrToFloat(temp) * 1000.0;

        /// possible right placements..........................................  def = Any
        ToLower(temp = GET_TEMP(conf), 0);
        if (gravity.find(temp) != gravity.end())
            flags_[0].placement = gravity[temp];

        /// possible right centerings..........................................  def = Any
        ToLower(temp = GET_TEMP(conf), 0);
        if (gravity.find(temp) != gravity.end())
            flags_[0].centering = gravity[temp];

        /// possible left placements...........................................  def = Any
        ToLower(temp = GET_TEMP(conf), 0);
        if (gravity.find(temp) != gravity.end())
            flags_[1].placement = gravity[temp];

        /// possible left centerings...........................................  def = Any
        ToLower(temp = GET_TEMP(conf), 0);
        if (gravity.find(temp) != gravity.end())
            flags_[1].centering = gravity[temp];

        /// flag to follow parent..............................................  def = False
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            parent_follow_ = booleans[ToLower(temp, 0)];

        /// flag to prevent animation looping..................................  def = False
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            set_loop(!booleans[ToLower(temp, 0)]);
    }
};

class speech : public effect {
private:
    std::string text_;
    uint32_t group_;
    size_t name_;

public:
    speech(ENGD &engd, library &l):
        effect(engd, l), text_(), group_(0), name_(0) {
        flags_[0] = flags_[1] = {bottom, top};
        respawn_ = 10000;
        skip_ = false;
    }
    speech(ENGD &engd, library &l, const std::string &path, char **conf):
        speech(engd, l) {
        static std::unordered_map<std::string, bool>
            booleans = { {"false", false}, {"true", true}, };
        auto GET_TEMP = [](char **conf) { return SplitLine(conf, DEF_TSEP, 0); };
        char *temp;

        /// speech name........................................................ !def
        name_ = HashLine(ToLower(Dequote(GET_TEMP(conf)), 0));

        /// speech text........................................................ !def
        if ((*(*conf)++ != '"') || !(temp = SplitLine(conf, '"', 0)))
            return;
        else {
            text_ = temp;
            (*conf) += (**conf == DEF_TSEP)? 1 : 0;
            /// respawn_ is 10000 by default and does not change, but duration_
            /// depends on the text length; in DP it equals (L / 15) seconds
            duration_ = std::max((uint32_t)FRM_WAIT,
                                 (uint32_t)text_.size() * 1000 / 15);
            set_primary(false, true);
        }
        /// sound files........................................................  def = ""
        if ((*(*conf)++ == '{') && (temp = SplitLine(conf, '}', 0))) {
            /// [TODO:] stop ignoring sounds
            (*conf) += (**conf == DEF_TSEP)? 1 : 0;
        }
        /// flag to never execute this speech at random........................  def = False
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            skip_ = booleans[temp];

        /// behaviour group index..............................................  def = 0
        if ((temp = GET_TEMP(conf)) && *temp)
            group_ = std::max(0.f, StrToFloat(temp));
    }
};

class behaviour : public unit {
private:
    enum movement_flags {
        none      = (1 << 0),
        mouse     = (1 << 1),
        drag      = (1 << 2),
        sleep     = (1 << 3),
        horz      = (1 << 4),
        vert      = (1 << 5),
        diag      = (1 << 6),
        horz_vert = horz | vert,
        diag_horz = diag | horz,
        diag_vert = diag | vert,
        all       = diag | horz | vert,
    };
    enum state_flags {
        empty                 = 0,
        has_linked_beh        = (1 << 0),
        has_start_speech      = (1 << 1),
        has_end_speech        = (1 << 2),
        has_target            = (1 << 3),
        manual_follow_images  = (1 << 4),
        has_follow_static_beh = (1 << 5),
        has_follow_moving_beh = (1 << 6),
        mirror_target_offsets = (1 << 7),
    };
    size_t name_;
    uint32_t probability_;
    uint32_t max_duration_;
    uint32_t min_duration_;
    float movement_speed_;
    movement_flags movement_;
    std::vector<effect> effects_;
    size_t linked_beh_name_;
    size_t start_speech_name_;
    size_t end_speech_name_;
    T2IV follow_target_coords_;
    size_t follow_target_name_;
    size_t follow_static_beh_;
    size_t follow_moving_beh_;
    uint32_t group_;
    state_flags flags_;

public:
    behaviour(ENGD &engd, library &l):
        unit(engd, l), name_(0), probability_(0), max_duration_(15000),
        min_duration_(5000), movement_speed_(0.1 * FRM_WAIT), movement_(all),
        linked_beh_name_(0), start_speech_name_(0), end_speech_name_(0),
        follow_target_coords_{}, follow_target_name_(0), follow_static_beh_(0),
        follow_moving_beh_(0), group_(0), flags_(empty) {}
    behaviour(ENGD &engd, library &l, const std::string &path, char **conf):
        behaviour(engd, l) {
        static std::unordered_map<std::string, movement_flags> movement = {
            {"horizontal_vertical", horz_vert}, {"horizontal_only",horz},
            {"diagonal_horizontal", diag_horz}, {"vertical_only",  vert},
            {"diagonal_vertical",   diag_vert}, {"diagonal_only",  diag},
            {"mouseover",           mouse    }, {"dragged",        drag},
            {"sleep",               sleep    }, {"all",            all },
            {"none",                none     },
        };
        static std::unordered_map<std::string, bool>
            booleans = { {"false", false}, {"true",   true},
                         {"fixed", false}, {"mirror", true}, };
        auto GET_TEMP = [](char **conf) { return SplitLine(conf, DEF_TSEP, 0); };
        char *temp;

        /// behaviour name..................................................... !def
        name_ = HashLine(ToLower(Dequote(GET_TEMP(conf)), 0));

        /// probability of this behaviour......................................  def = 0
        if ((temp = GET_TEMP(conf)) && *temp)
            probability_ = std::clamp(StrToFloat(temp) * 1000.0, 0.0, 1000.0);

        /// maximum duration in sec............................................  def = 15
        if ((temp = GET_TEMP(conf)) && *temp)
            max_duration_ = StrToFloat(temp) * 1000.0;

        /// minimum duration in sec............................................  def = 5
        if ((temp = GET_TEMP(conf)) && *temp)
            min_duration_ = StrToFloat(temp) * 1000.0;

        /// movement speed (*100/3 for pix/sec)................................  def = 3
        if ((temp = GET_TEMP(conf)) && *temp)
            movement_speed_ = StrToFloat(temp) * FRM_WAIT * 0.1 / 3.0;

        /// right-sided image.................................................. !def
        /// left-sided image................................................... !def
        for (unsigned iter = 0; iter <= 1; iter++) {
            auto name = path + DEF_DSEP + Dequote(SplitLine(conf, DEF_TSEP, 0));
            add_source(iter, name);
        }

        /// possible movement directions.......................................  def = All
        if (ToLower(temp = GET_TEMP(conf), 0) && (movement.find(temp) != movement.end()))
            movement_ = movement[temp];
        if (!(movement_ & all)) /// zeroing the speed if the movement is static
            movement_speed_ = 0.0;

        /// linked behaviour name..............................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_linked_beh;
            linked_beh_name_ = HashLine(ToLower(Dequote(temp), 0));
        }
        /// speech said on behaviour start.....................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_start_speech;
            start_speech_name_ = HashLine(ToLower(Dequote(temp), 0));
        }
        /// speech said on behaviour end.......................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_end_speech;
            end_speech_name_ = HashLine(ToLower(Dequote(temp), 0));
        }
        /// flag to never execute this behaviour at random.....................  def = False
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            probability_ *= !booleans[temp];

        /// X target to follow.................................................  def = 0
        if ((temp = GET_TEMP(conf)) && *temp)
            follow_target_coords_.x = StrToFloat(temp);

        /// Y target to follow.................................................  def = 0
        if ((temp = GET_TEMP(conf)) && *temp)
            follow_target_coords_.y = StrToFloat(temp);

        /// name of the target.................................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_target;
            follow_target_name_ = HashLine(ToLower(Dequote(temp), 0));
        }
        /// automatically determine the images to use when following...........  def = True
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            flags_ |= (booleans[temp]) ? empty : manual_follow_images;

        /// static behaviour for follow mode...................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_follow_static_beh;
            follow_static_beh_ = HashLine(Dequote(temp));
        }
        /// moving behaviour for follow mode...................................  def = ""
        if ((temp = GET_TEMP(conf)) && *temp) {
            flags_ |= has_follow_moving_beh;
            follow_moving_beh_ = HashLine(Dequote(temp));
        }
        /// right image center (natural center if "0,0").......................  def = "0,0"
        if ((temp = GET_TEMP(conf)) && *temp) {
            T2IV center;
            center.x = StrToFloat(Dequote(temp));
            center.y = StrToFloat(Dequote(GET_TEMP(conf)));
            set_center(false, center);
        }
        /// left image center (natural center if "0,0")........................  def = "0,0"
        if ((temp = GET_TEMP(conf)) && *temp) {
            T2IV center;
            center.x = StrToFloat(Dequote(temp));
            center.y = StrToFloat(Dequote(GET_TEMP(conf)));
            set_center(true, center);
        }
        /// flag to prevent animation looping..................................  def = False
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            set_loop(!booleans[temp]);

        /// behaviour group index..............................................  def = 0
        if ((temp = GET_TEMP(conf)) && *temp)
            group_ = std::max(0.f, StrToFloat(temp));

        /// whether target offset shall be mirrored............................  def = Fixed
        if (ToLower(temp = GET_TEMP(conf), 0) && (booleans.find(temp) != booleans.end()))
            flags_ |= (booleans[temp]) ? mirror_target_offsets : empty;
    }
};

class spritebank : public nocopy {
public:
    class sprite {
    private:
        uint32_t index_;
        library *library_;
        uint32_t parent_; /// unique identifier of the parent sprite (0 if none)

    public:
        void select(behaviour &b, bool left) {
            library_ = &b.get_library();
//            index_ = library_->locate(b);
        }
        sprite() {}
    };

    void step() {}

private:
    std::vector<sprite> sprites_;
    std::vector<T4FV> output_;
};

class library : public nocopy {
private:
    struct groups {
        std::vector<speech*> random_speeches;
        std::vector<behaviour*> nonzero_prob;
        std::vector<behaviour*> stationary;
        std::vector<behaviour*> moving;
        std::vector<behaviour*> mouseover;
        std::vector<behaviour*> dragged;
        std::vector<behaviour*> sleep;
    };
    ENGC &engc_;    /// parent engine
    CTRL imagebox_; /// image box control to preview the sprite
    CTRL charname_; /// character name just below the image box
    CTRL spinner_;  /// spin control to set ICNT
    std::vector<behaviour> behaviours_;
    std::vector<effect> effects_;
    std::vector<speech> speeches_;
    std::unordered_map<uint32_t, groups> groups_;
    std::vector<std::string> categories_;
    std::string library_path_;
    std::string readable_name_;
    std::string scrollable_name_;
    uint32_t speech_foreground_;
    uint32_t speech_background_;
    spritebank::sprite preview_;

public:
    library(ENGC &engc, const std::string &base, const std::string &path):
        engc_(engc), library_path_(base + path), readable_name_(path) {
        if (auto file = rLoadFile((library_path_ + name).c_str(), 0)) {
            auto fptr = file;

            while ((conf = GetNextLine(&fptr)))
                switch (DetermineType(&conf)) {
                    case SVT_NAME: /// weird, but DP does not use this field
//                        readable_name_ = GET_TEMP(&conf);
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
                CTR_V_SORT(libs->ctgs, ctgscmp, 0, libs->ctgs.size);
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
                    CTR_V_SORT(engc->ctgs, ctgscmp, 0, engc->ctgs.size);
            }
            free(file);



            if (!libs->barr.size) {
                /// no behaviours found, the library is broken; stopping
                free(file);
                free(libs->path);
                CTR_V_MGET(engc->libs, engc->libs.size - 1);
                return;
            }
        }
    }
};

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
/*
/// engine data (client side)
struct ENGC {
    MENU     *mspr, /// per-sprite context menu
             *mctx; /// engine`s main context menu
    CTRL     *mctl, /// GUI controls array (main window)
             *octl; /// GUI controls array (options window)
    T4FV     *data; /// main display sequence passed to the renderer
    LVEC      libs; /// sprite libraries array
    CVEC      ctgs; /// categories array
    ENGD     *engd; /// rendering engine handle
    PICT     *pcur, /// the sprite currently picked
             *povr, /// the sprite with cursor over it
            **parr, /// on-screen sprite pointers array
            **elem; /// pointer buffer for SortBy*()
    char    **tran, /// localized text array (ASCIIZ; last item is also 0)
             *cfnm; /// name of the main configuration file
    uint32_t *seed, /// random number generator seed
             *blgp, /// boundaries of library groups in sorted PARR
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
*/


void  eProcessMenuItem(MENU *item) {
}

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
}
