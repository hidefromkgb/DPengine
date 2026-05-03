#include <algorithm>
#include <cassert>
#include <charconv>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "exec.h"
#include "zip/zip_load.h"



/// FE2C / FC2E helper macros
#define RUN_FE2C(trgt, cmsg, data) trgt.fe2c(&trgt, cmsg, data)
#define RUN_FC2E(trgt, cmsg, data) trgt.fc2e(&trgt, cmsg, data)

/** convert degrees to radians  **/
#define DTR_CONV (M_PI / 180.0)
/** convert radians to degrees  **/
#define RTD_CONV (1.0 / DTR_CONV)

/// default comment character
#define DEF_CMNT '\''
/// default end-of-line character
#define DEF_CRLF '\n'
/// non-default end-of-line character
#define DEF_LFCR '\r'
/// default quotation character - inhibits tokenization
#define DEF_QUOT '"'
/// default token separator
#define DEF_TSEP ','
/// default dir slash (string)
#define DEF_DSEP "/"

/// /// /// /// /// /// /// /// /// ENGC.MCTL array indices
#define MCT_CAPT mctl[ 0]
#define MCT_FLTR mctl[ 1]
#define MCT_EXAC mctl[ 2]
#define MCT_OGRP mctl[ 3]
#define MCT_SGRP mctl[ 4]
#define MCT_SPEC mctl[ 5]
#define MCT_BADD mctl[ 6]
#define MCT_SRND mctl[ 7]
#define MCT_RGPU mctl[ 8]
#define MCT_BDUP mctl[ 9]
#define MCT_SELE mctl[10]
#define MCT_OPTS mctl[11]
#define MCT_GOGO mctl[12]
#define MCT_CHAR mctl[13]

/// /// /// /// /// /// /// /// /// ENGC.OCTL array indices
#define OCT_OPTS octl[ 0]
#define OCT_UONR octl[ 1]
#define OCT_ETOP octl[ 2]
#define OCT_EEFF octl[ 3]
#define OCT_EINT octl[ 4]
#define OCT_ESAY octl[ 5]
#define OCT_ECLR octl[ 6]
#define OCT_ERCH octl[ 7]
#define OCT_NRUN octl[ 8]
#define OCT_TRUN octl[ 9]
#define OCT_NSCA octl[10]
#define OCT_TSCA octl[11]
#define OCT_NDIL octl[12]
#define OCT_TDIL octl[13]
#define OCT_NSAY octl[14]
#define OCT_TSAY octl[15]
#define OCT_NCDR octl[16]
#define OCT_TCDR octl[17]
#define OCT_LCHO octl[20]
#define OCT_LREL octl[21]
#define OCT_LRES octl[22]
#define OCT_LGUI octl[23]
#define OCT_BCHO octl[26]
#define OCT_BREL octl[27]
#define OCT_BRES octl[28]
#define OCT_BDIR octl[29]
#define OCT_FREL octl[31]
#define OCT_FRES octl[32]

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



static inline void utf8_skip_char(std::string_view &str) {
    static const char skip[] = {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6};
    str.remove_prefix((str[0] & 0x80) ? skip[(str[0] >> 2) & 0x0F] : 1);
}

bool utf8_is_wspace(const std::string_view &str) {
    const auto size = str.size();
    if (size == 0) return false;
    switch (str[0]) {
        case '\x09': case '\x20': return true;
        case '\xC2':
            return (size > 1) && (str[1] == '\xA0');
        case '\xE1':
            return (size > 2) && (str[1] == '\x9A') && (str[2] == '\x80');
        case '\xE3':
            return (size > 2) && (str[1] == '\x80') && (str[2] == '\x80');
        case '\xEF':
            return (size > 2) && (str[1] == '\xBB') && (str[2] == '\xBF');
        case '\xE2':
            if (size <= 2) return false;
            switch (str[1]) {
                case '\x81': return str[2] == '\x9F';
                case '\x80':
                    switch (str[2]) {
                        case '\x80': case '\x81': case '\x82': case '\x83':
                        case '\x84': case '\x85': case '\x86': case '\x87':
                        case '\x88': case '\x89': case '\x8A': case '\x8B':
                        case '\xAF': return true;
                    }
            }
    }
    return false;
}

std::string concat_path(const std::initializer_list<const std::string> &list) {
    std::string retn;
    for (auto &str : list) retn += (!retn.empty()) ? DEF_DSEP + str : str;
    return retn;
}

static inline constexpr char ascii_to_lower(char c) {
    return ((c >= 'A') && (c <= 'Z')) ? c + ('a' - 'A') : c;
}

static inline constexpr char ascii_to_upper(char c) {
    return ((c >= 'a') && (c <= 'z')) ? c - ('a' - 'A') : c;
}

std::string ascii_to_lower(const std::string_view &str) {
    std::string retn(str);
    for (auto &c : retn) c = ascii_to_lower(c);
    return retn;
}

std::string ascii_to_upper(const std::string_view &str) {
    std::string retn(str);
    for (auto &c : retn) c = ascii_to_upper(c);
    return retn;
}

size_t constexpr str_hash(const std::string_view &str, bool ascii2low = true) {
    size_t retn = (str.size()) ? 14695981039346656037uLL : 0uLL;
    const size_t mult = 1099511628211uLL;
    if (ascii2low)
        for (auto c : str) retn = (retn ^ (size_t)ascii_to_lower(c)) * mult;
    else
        for (auto c : str) retn = (retn ^ (size_t)c) * mult;
    return retn;
}

using token_t = std::pair<std::string_view, std::string_view>;
bool is_empty(const token_t &t) { return t.first.empty() && t.second.empty(); }

token_t next_token(const std::string_view &str,
        char c = DEF_CMNT, char s = DEF_TSEP, char q = DEF_QUOT) {
    assert(s != q);
    auto sep = s;
    auto iter = str;
    while (utf8_is_wspace(iter)) utf8_skip_char(iter);
    // abort if token begins with comment char
    if (iter.empty() || (iter[0] == c)) return {};
    // look for the next quote instead of separator if token begins with quote
    if (iter[0] == q) {
        utf8_skip_char(iter);
        sep = q;
    }
    auto found = iter.find(sep);
    if (found == std::string_view::npos) return {iter, {}};
    std::string_view token((found > 0) ? iter.data() : nullptr, found);
    iter.remove_prefix(found + sizeof(sep));
    if (sep != s) {
        found = iter.find(s);
        if (found == std::string_view::npos) iter = {};
        else iter.remove_prefix(found + sizeof(s));
        //if (found > 0) printf("'%s': tokens delimited past quotes!\n");
    }
    return {token, iter};
}

template <typename T>
T process_map(token_t &line, const std::unordered_map<std::string, T> &map,
        T def, char s = DEF_TSEP, char q = DEF_QUOT) {
    line = next_token(line.second, 0, s, q);
    auto it = map.find(ascii_to_lower(line.first));
    return (it != map.end()) ? it->second : def;
}

bool process_bool(
        token_t &line, bool def, char s = DEF_TSEP, char q = DEF_QUOT) {
    static std::unordered_map<std::string, bool>
        map = { {"false", false}, {"true", true}, };
    return process_map(line, map, def, s, q);
}

float process_float(
        token_t &line, float def, char s = DEF_TSEP, char q = DEF_QUOT) {
    line = next_token(line.second, 0, s, q);
    std::from_chars(
            line.first.data(), line.first.data() + line.first.size(), def);
    return def;
}

std::vector<std::string_view> process_array(token_t &line,
        char s = DEF_TSEP, char q = DEF_QUOT, char bgn = '{', char end = '}') {
    std::vector<std::string_view> retn;
    line = next_token(line.second, 0, bgn, 0);
    if (line.second.empty()) { // no array beginning detected, array = {token}
        line = next_token(line.first, 0, s, q);
        return {line.first};
    }
    line = next_token(line.second, 0, end, 0);
    for (token_t iter({}, line.first); !is_empty(iter);
            iter = next_token(iter.second, 0, s, q))
        if (!iter.first.empty()) retn.emplace_back(iter.first);
    line = next_token(line.second, 0, s, q);
    return retn;
}

T2IV process_quoted_int_pair(token_t &line,
        T2IV def, char s = DEF_TSEP, char q = DEF_QUOT) {
    auto pair = process_array(line, s, 0, q, q);
    if (pair.size() > 0)
        std::from_chars(pair[0].data(), pair[0].data() + pair[0].size(), def.x);
    if (pair.size() > 1)
        std::from_chars(pair[1].data(), pair[1].data() + pair[1].size(), def.y);
    return def;
}

std::string process_string(
        token_t &line, char s = DEF_TSEP, char q = DEF_QUOT) {
    line = next_token(line.second, 0, s, q);
    return std::string(line.first);
}

class no_copy_t {
public:
    no_copy_t() = default;
    no_copy_t(no_copy_t&) = delete;       /// non construction-copyable
    no_copy_t(const no_copy_t&) = delete; /// non construction-copyable
    no_copy_t& operator=(no_copy_t&) = delete;       /// non copyable
    no_copy_t& operator=(const no_copy_t&) = delete; /// non copyable
};

class library_t;
class speech_t;
class effect_t;
class behaviour_t;

class unit_t : public no_copy_t {
private:
    enum state_flags_t { /// is_ready & !is_primary == this side is a copy
        empty        = 0,
        is_primary   = 1 << 0,
        is_scheduled = 1 << 1,
        is_ready     = 1 << 2,
        from_path    = 1 << 4,
        fake_center  = 1 << 5,
    };
    struct {
        state_flags_t flags_;
        AINF image_;
        T2IV center_;
    } sides_[2];
    bool loop_;

    static void finalize(void *data) {
        data = ((uint8_t*)data) - sizeof(intptr_t);
        *((state_flags_t*)data) &= ~is_scheduled;
        *((state_flags_t*)data) |= is_ready;
        free(data);
    }
    void remove(bool left) {
        auto &side = sides_[left];
        assert(!(side.flags_ & is_scheduled));
        if (!(side.flags_ & is_ready) && side.image_.time)
            finalize(side.image_.time);
        side.image_ = (AINF){};
        side.flags_ = empty;
    }
    void add_source(bool left) {
        remove(left);
        auto &side = sides_[left];
        side.flags_ |= is_primary;
    }

protected:
    void set_center(bool left, const T2IV &center) {
        auto &side = sides_[left];
        side.center_ = center;
        if ((center.x != 0) || (center.y != 0))
            side.flags_ |= fake_center;
        else
            side.flags_ &= ~fake_center;
    }
    void set_primary(bool left, bool what) {
        auto &side = sides_[left];
        if (what)
            side.flags_ |= is_primary;
        else
            side.flags_ &= ~is_primary;
    }

public:
    unit_t(): sides_(), loop_(true) {}
    ~unit_t() { remove(false); remove(true); }

    void add_source(bool left, const std::string &name) {
        add_source(left);
        /// here we prepare a structure that contains the string above byte 0,
        /// and a pointer to the corresponding flags below byte 0
        auto &side = sides_[left];
        char *retn = (char*)malloc(name.size() + 1 + sizeof(intptr_t));
        strncpy(retn + sizeof(intptr_t), name.c_str(), name.size() + 1);
        *((intptr_t*)retn) = (intptr_t)(&side.flags_);
        side.image_.time = (uint32_t*)(retn + sizeof(intptr_t));
        side.flags_ |= from_path;
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
        auto &side = sides_[left];
        *((intptr_t*)data) = (intptr_t)(&side.flags_);
        auto anim = (AINF*)(((intptr_t*)data) + 1);
        anim->uuid = (intptr_t)(anim + 1);
        anim->time = ((uint32_t*)anim->uuid) + xdim * ydim;
        anim->xdim = xdim;
        anim->ydim = ydim;
        anim->time[0] = 0; /// single frame only for this image type
        strncpy((char*)(anim->time + 1), name.c_str(), name.size() + 1);
        side.image_.time = (uint32_t*)anim;
        draw((uint32_t*)anim->uuid); /// drawing something in the buffer
    }
    void schedule_load(ENGD *engd, bool left) {
        auto &side = sides_[left];
        if (!(side.flags_ & is_ready) && (side.flags_ & is_primary)) {
            side.flags_ |= is_scheduled;
            if (side.flags_ & from_path) {
                //auto temp = ExtractLastDirs((char*)side.image_.time, 2);
                //cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)temp,
                //                     side.image_.time, ELA_DISK, finalize);
            }
            else if (auto anim = (AINF*)side.image_.time) {
                auto name = (uint8_t*)(anim->time + 1);
                cEngineLoadAnimAsync(engd, &side.image_, name, anim,
                                     ELA_AINF, finalize);
            }
        }
    }
    T2IV dims(bool left) {
        auto &side = sides_[left];
        return (T2IV){{(int32_t)side.image_.xdim, (int32_t)side.image_.ydim}};
    }
    uint32_t next_frame(bool left, uint32_t curr) {
        auto &side = sides_[left];
        return (curr + 1 >= side.image_.fcnt) ? (loop_) ? 0 : curr : curr + 1;
    }
    bool is_empty() {
        return !(sides_[0].flags_ & is_primary)
            && !(sides_[1].flags_ & is_primary);
    }
    void set_loop(bool loop) { loop_ = loop; }
};

class effect_t : public unit_t {
protected:
    enum gravity_flags_t {
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
        gravity_flags_t placement:4;
        gravity_flags_t centering:4;
    } flags_[2];
    std::string parent_name_;
    bool skip_; /// no such flag in effects, it is here for speeches
    bool parent_follow_;
    uint32_t duration_;
    uint32_t respawn_;

public:
    class input_t {
    public:
        std::string name;
        std::string bhv;
        std::string right_image;
        std::string left_image;
        float duration = 5.f;
        float repeat_delay = 0.f;
        gravity_flags_t placement_right = gravity_flags_t::any;
        gravity_flags_t centering_right = gravity_flags_t::any;
        gravity_flags_t placement_left = gravity_flags_t::any;
        gravity_flags_t centering_left = gravity_flags_t::any;
        bool follow = false;
        bool prevent_loop = false;

        input_t(const std::string_view &str) {
            static std::unordered_map<std::string, gravity_flags_t> gravity = {
                {"any",         any        }, {"any-not_center", not_center  },
                {"top_left",    top_left   }, {"top_right",      top_right   },
                {"top",         top        }, {"bottom",         bottom      },
                {"bottom_left", bottom_left}, {"bottom_right",   bottom_right},
                {"left",        center_left}, {"right",          center_right},
                {"center",      center     },
            };
            token_t line({}, str);
            name = process_string(line);
            bhv = process_string(line);
            right_image = process_string(line);
            left_image = process_string(line);
            duration = process_float(line, duration);
            repeat_delay = process_float(line, repeat_delay);
            placement_right = process_map(line, gravity, placement_right);
            centering_right = process_map(line, gravity, centering_right);
            placement_left = process_map(line, gravity, placement_left);
            centering_left = process_map(line, gravity, centering_left);
            follow = process_bool(line, follow);
            prevent_loop = process_bool(line, prevent_loop);
        }
        bool validate() const {
            bool okay = !name.empty();
            okay &= !bhv.empty();
            okay &= !right_image.empty();
            okay &= !left_image.empty();
            return okay;
        }
    };
    /*
    effect_t():
        flags_{{any, any}, {any, any}}, parent_name_(), skip_(true),
        parent_follow_(false), duration_(5000), respawn_(0) {}
    */
    effect_t(const input_t &input) {
    }
};

class speech_t : public unit_t {
private:
    std::string text_;
    uint32_t group_;
    size_t name_;

public:
    class input_t {
    public:
        std::string name;
        std::string text;
        std::string sound_file;
        bool skip = false;
        int group = 0;

        input_t(const std::string_view &str) {
            token_t line({}, str);
            name = process_string(line);
            text = process_string(line);
            auto sound_files = process_array(line);
            if (!sound_files.empty()) sound_file = sound_files[0];
            skip = process_bool(line, skip);
            group = process_float(line, group);
        }
        bool validate() const { return !text.empty(); }
    };
    /*
    speech_t(): text_(), group_(0), name_(0) {
        flags_[0] = flags_[1] = {bottom, top};
        respawn_ = 10000;
        skip_ = false;
    }
    //*/
    speech_t(const input_t &input) {
    }
};

class behaviour_t : public unit_t {
private:
    enum movement_flags_t {
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
    enum state_flags_t {
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
    uint32_t probability_;
    uint32_t max_duration_;
    uint32_t min_duration_;
    float movement_speed_;
    movement_flags_t movement_;
    std::vector<effect_t> effects_;
    size_t linked_beh_name_;
    size_t start_speech_name_;
    size_t end_speech_name_;
    T2IV follow_target_coords_;
    size_t follow_target_name_;
    size_t follow_static_beh_;
    size_t follow_moving_beh_;
    uint32_t group_;
    state_flags_t flags_;

public:
    class input_t {
    public:
        std::string name;
        float chance = 0.f;
        float max_duration = 15.f;
        float min_duration = 5.f;
        float speed = 3.f;
        std::string right_image;
        std::string left_image;
        movement_flags_t movement = movement_flags_t::all;
        std::string linked_bhv_str; behaviour_t *linked_bhv = nullptr;
        std::string bgn_speech_str; speech_t *bgn_speech = nullptr;
        std::string end_speech_str; speech_t *end_speech = nullptr;
        bool skip = false;
        T2IV target_xy = {{0, 0}};
        std::string follow_target_str; library_t *follow_target = nullptr;
        bool auto_follow_img = true;
        std::string follow_stop_bhv_str; behaviour_t *follow_stop_bhv = nullptr;
        std::string follow_mov_bhv_str; behaviour_t *follow_mov_bhv = nullptr;
        T2IV right_img_center = {{0, 0}};
        T2IV left_img_center = {{0, 0}};
        bool prevent_loop = false;
        int group = 0;
        bool follow_offset_type_mirror = false;

        input_t(const std::string_view &str) {
            static std::unordered_map<std::string, bool>
                followflg = { {"false", false}, {"true",   true},
                              {"fixed", false}, {"mirror", true}, };
            static std::unordered_map<std::string, movement_flags_t> moveflg = {
                {"horizontal_vertical", horz_vert}, {"horizontal_only",horz},
                {"diagonal_horizontal", diag_horz}, {"vertical_only",  vert},
                {"diagonal_vertical",   diag_vert}, {"diagonal_only",  diag},
                {"mouseover",           mouse    }, {"dragged",        drag},
                {"sleep",               sleep    }, {"all",            all },
                {"none",                none     },
            };
            token_t line({}, str);
            name = process_string(line);
            chance = process_float(line, chance);
            max_duration = process_float(line, max_duration);
            min_duration = process_float(line, min_duration);
            speed = process_float(line, speed);
            right_image = process_string(line);
            left_image = process_string(line);
            movement = process_map(line, moveflg, movement);
            linked_bhv_str = process_string(line);
            bgn_speech_str = process_string(line);
            end_speech_str = process_string(line);
            skip = process_bool(line, skip);
            target_xy.x = process_float(line, target_xy.x);
            target_xy.y = process_float(line, target_xy.y);
            follow_target_str = process_string(line);
            auto_follow_img = process_bool(line, auto_follow_img);
            follow_stop_bhv_str = process_string(line);
            follow_mov_bhv_str = process_string(line);
            right_img_center = process_quoted_int_pair(line, right_img_center);
            left_img_center = process_quoted_int_pair(line, left_img_center);
            prevent_loop = process_bool(line, prevent_loop);
            group = process_float(line, group);
            follow_offset_type_mirror
                    = process_map(line, followflg, follow_offset_type_mirror);
        }
        bool validate() const {
            bool okay = !name.empty();
            okay &= !right_image.empty();
            okay &= !left_image.empty();
            return okay;
        }
    };
    /*
    behaviour_t(library_t &l):
        unit_t(l), probability_(0), max_duration_(15000),
        min_duration_(5000), movement_speed_(0.1 * FRM_WAIT), movement_(all),
        linked_beh_name_(0), start_speech_name_(0), end_speech_name_(0),
        follow_target_coords_{}, follow_target_name_(0), follow_static_beh_(0),
        follow_moving_beh_(0), group_(0), flags_(empty) {}
    //*/
    behaviour_t(const input_t &input) {
    }
};

class sprite_bank_t : public no_copy_t {
public:
    class sprite_t {
    private:
        uint32_t index_;
        library_t *library_;
        uint32_t parent_; /// unique identifier of the parent sprite (0 if none)

    public:
        void select(behaviour_t &b, bool left) {
//            library_ = &b.get_library();
//            index_ = library_->locate(b);
        }
        sprite_t() {}
    };

    void step() {}

private:
    std::vector<sprite_t> sprites_;
    std::vector<T4FV> output_;
};

class interaction_t : public no_copy_t {
private:
public:
    class input_t {
    public:
        std::string name;
        float chance = 0.f;
        float proximity = 125.f;
        std::vector<std::string> targets_str; std::vector<library_t*> targets;
        bool target_activation_all = false;
        std::vector<std::string> bhv;
        float reactivation_delay = 60.f;

        input_t(const std::string_view &str) {
            static std::unordered_map<std::string, bool> activations = {
                {"true", true}, {"false",  false}, {"any", false},
                {"all",  true}, {"random", false}, {"one", false},
            };
            token_t line({}, str);
            name = process_string(line);
            chance = process_float(line, chance);
            proximity = process_float(line, proximity);
            auto tgt_s = process_array(line);
            targets_str = std::vector<std::string>(tgt_s.begin(), tgt_s.end());
            target_activation_all
                    = process_map(line, activations, target_activation_all);
            auto bhv_s = process_array(line);
            bhv = std::vector<std::string>(bhv_s.begin(), bhv_s.end());
            reactivation_delay = process_float(line, reactivation_delay);
        }
        bool validate() const {
            bool okay = !targets_str.empty();
            okay &= !bhv.empty();
            for (auto &t : targets_str) okay &= !t.empty();
            for (auto &b : bhv) okay &= !b.empty();
            return okay;
        }
    };
    interaction_t(const input_t &input) {
    }
};

class library_t : public no_copy_t {
private:
    struct groups_t {
        std::vector<const speech_t*> random_speeches;
        std::vector<const behaviour_t*> nonzero_prob;
        std::vector<const behaviour_t*> stationary;
        std::vector<const behaviour_t*> moving;
        std::vector<const behaviour_t*> mouseover;
        std::vector<const behaviour_t*> dragged;
        std::vector<const behaviour_t*> sleep;
    };
    ENGC &engc_;    /// parent engine
    CTRL imagebox_; /// image box control to preview the sprite
    CTRL charname_; /// character name just below the image box
    CTRL spinner_;  /// spin control to set ICNT
    std::unordered_map<std::string, behaviour_t> behaviours_;
    std::vector<effect_t> effects_;
    std::vector<speech_t> speeches_;
    std::unordered_map<uint32_t, groups_t> groups_;
    std::vector<std::string> categories_;
    std::string library_path_;
    std::string readable_name_;
    std::string scrollable_name_;
    uint32_t speech_foreground_;
    uint32_t speech_background_;
    sprite_bank_t::sprite_t preview_;

public:
    class input_t {
    public:
        static constexpr char* config_name = (char*)"pony.ini";

        std::string name;
        std::vector<std::string> categories;
        std::vector<speech_t::input_t> speeches;
        std::vector<effect_t::input_t> effects;
        std::vector<behaviour_t::input_t> behaviours;
        std::vector<interaction_t::input_t> interactions;

        input_t(const std::string &base, const std::string &dir) {
            auto config = concat_path(
                    {base, dir, library_t::input_t::config_name});
            if (auto file = rLoadFile(config.c_str(), nullptr)) {
                name = dir;
                for (token_t text({}, file); !is_empty(text);
                        text = next_token(text.second, 0, DEF_CRLF, 0)) {
                    if (!text.first.empty() && (text.first.back() == DEF_LFCR))
                        text.first.remove_suffix(sizeof(DEF_LFCR));
                    auto line = next_token(text.first);
                    switch (str_hash(line.first)) {
                        //case str_hash("Name"):
                        //case str_hash("BehaviorGroup"):
                        default: break;
                        case str_hash("Categories"):
                            for (line.first = {}; !is_empty(line);
                                    line = next_token(line.second, 0))
                                if (!line.first.empty()) {
                                    auto category = ascii_to_lower(line.first);
                                    category[0] = ascii_to_upper(category[0]);
                                    categories.emplace_back(category);
                                }
                            break;
                        case str_hash("Behavior"):
                            behaviours.emplace_back(line.second);
                            break;
                        case str_hash("Effect"):
                            effects.emplace_back(line.second);
                            break;
                        case str_hash("Speak"):
                            speeches.emplace_back(line.second);
                            break;
                        case str_hash("Interaction"):
                            interactions.emplace_back(line.second);
                            break;
                    }
                }
                free(file);
            }
        }
    };
    library_t(ENGC &engc, const std::string &base, const std::string &path):
        engc_(engc), library_path_(base + path), readable_name_(path) {
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
    std::vector<library_t::input_t> libs;
    if (fcnf) {
        std::string base;
        auto config = concat_path({fcnf, DEF_CORE});
        if (auto file = rLoadFile(config.c_str(), nullptr)) {
            for (token_t text({}, file); !is_empty(text);
                    text = next_token(text.second, 0, DEF_CRLF, 0)) {
                if (!text.first.empty() && (text.first.back() == DEF_LFCR))
                    text.first.remove_suffix(sizeof(DEF_LFCR));
                auto line = next_token(text.first);
                switch (str_hash(line.first)) {
                    default: break;
                    case str_hash("Content"):
                        line = next_token(line.second, 0);
                        base = concat_path(
                                {std::string(line.first), "Content", "Ponies"});
                        break;
                }
            }
            free(file);
            auto find = rFindMake(base.c_str());
            while (!!(file = rFindFile(find))) {
                libs.emplace_back(library_t::input_t(base, file));
                free(file);
            }
        }
    }
    for (auto &l : libs) {
        printf("%s\n", l.name.c_str());
    }
}
