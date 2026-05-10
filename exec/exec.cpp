#include <algorithm>
#include <cassert>
#include <charconv>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "exec.h"
#include "zip/zip_load.h"



// TODO: check which *::input_t fields are still left unprocessed, and fix!

// TODO: implement a test system? e.g. instead of the screen the characters
//       would print their actions to STDOUT, and if the same PRNG, config,
//       and resolution are set, then everything is deterministic and these
//       output logs can be validated by comparing them to a reference log.



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



/// Mersenne random number generator

void RNG_Free(uint32_t **seed) {
    *seed = (uint32_t*)realloc(*seed, 0);
    *seed = 0;
}

uint32_t *RNG_Make(uint32_t init) {
    const uint32_t size = 624;
    uint32_t *seed;

    (seed = (uint32_t*)realloc(0, sizeof(*seed) * (size + 1)))[1] = init;
    for (seed[0] = 0, init = 1; init < size; init++)
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

class weighted_rng_t {
// this is an RNG helper that given a uniform distribution can choose an index
// within an array of items with different relative probabilities, in O(NlogN)
// preparation time (once, at init) and O(1) per choice; uses the Alias Method
// (see https://en.wikipedia.org/wiki/Alias_method for more information)
private:
    struct item_t { uint32_t prob, alias, index; };
    std::vector<item_t> data_;

public:
    weighted_rng_t() = default;
    weighted_rng_t(const std::vector<uint32_t> &weights) {
        if (weights.empty()) return;
        uint32_t full = 0, over = 0, size = uint32_t(weights.size());
        data_.resize(size);
        for (uint32_t i = size; i > 0; i--) {
            data_[i - 1] = {weights[i - 1], 0, i - 1};
            full += weights[i - 1];
        }
        auto rev_prob = [](item_t &a, item_t &b) { return a.prob > b.prob; };
        std::sort(data_.begin(), data_.end(), rev_prob);
        for (uint32_t i = size; i > 0; i--) {
            data_[i - 1].prob *= size;
            if (!over && (data_[i - 1].prob > full)) over = i;
        }
        for (uint32_t i = size; over && (i > over); --i)
            if (data_[i - 1].prob != full) {
                data_[i - 1].alias = over - 1;
                data_[over - 1].prob -= full - data_[i - 1].prob;
                if (data_[over - 1].prob <= full) over--;
            }
        data_[0].alias = size;
    }
    size_t size() const { return data_.size(); }

    uint32_t random_selection(uint32_t seed) const {
        if (data_.empty()) return 0;
        uint32_t i = seed % data_[0].alias; // == seed % size
        bool alias = (seed / data_[0].alias) % data_[0].prob >= data_[i].prob;
        return data_[(alias) ? data_[i].alias : i].index;
    }
};



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

template <typename K, typename V>
inline const V *find_in_map(const std::unordered_map<K, V> &map, const K &key) {
    auto iter = map.find(key);
    return (iter != map.end()) ? &iter->second : nullptr;
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
    auto it = find_in_map(map, ascii_to_lower(line.first));
    return (it) ? *it : def;
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

class library_t;
class speech_t;
class effect_t;
class behaviour_t;
class interaction_t;

template <typename T>
using ref_vec_t = std::vector<std::reference_wrapper<const T>>;

using bhv_map_t = std::unordered_map<std::string, behaviour_t*>;

enum movement_flags_t {
    move_none  = (1 << 0),
    move_mouse = (1 << 1),
    move_drag  = (1 << 2),
    move_sleep = (1 << 3),
    move_horz  = (1 << 4),
    move_vert  = (1 << 5),
    move_diag  = (1 << 6),
    move_hv    = move_horz | move_vert,
    move_dh    = move_diag | move_horz,
    move_dv    = move_diag | move_vert,
    move_all   = move_diag | move_horz | move_vert,
};

template <bool allow_empty, typename T>
inline T random_selection(const std::vector<T> &v, uint32_t *seed) {
    if constexpr (!allow_empty) {
        assert(!v.empty());
    } else if (v.empty()) {
        return T{};
    }
    return v[(v.size() > 1) ? RNG_Load(seed) % v.size() : 0];
};

template <bool allow_empty, typename T>
inline T weighted_random_selection(
        const std::vector<T> &v, const weighted_rng_t &weight, uint32_t *seed) {
    assert(weight.size() == v.size());
    if constexpr (!allow_empty) {
        assert(!v.empty());
    } else if (v.empty()) {
        return T{};
    }
    return v[(v.size() > 1) ? weight.random_selection(RNG_Load(seed)) : 0];
};



class no_copy_t {
public:
    no_copy_t() = default;
    no_copy_t(no_copy_t&) = delete;       /// non construction-copyable
    no_copy_t(const no_copy_t&) = delete; /// non construction-copyable
    no_copy_t& operator=(no_copy_t&) = delete;       /// non copyable
    no_copy_t& operator=(const no_copy_t&) = delete; /// non copyable
};

class sprite_bank_t : public no_copy_t {
public:
    class sprite_t {
    private:
        uint32_t index_;
        library_t *library_;
        uint32_t parent_; /// unique identifier of the parent sprite (0 if none)

    public:
        void select(const behaviour_t &b, bool left) {
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

class library_t : public no_copy_t {
public:
    using lib_id_t = size_t;
    using bhv_id_t = uint32_t;
    struct bhv_id_desc_t {
        std::unordered_map<std::string, library_t::bhv_id_t> m;
        std::vector<library_t::bhv_id_t> v;
    };
    using bhv_id_map_t = std::unordered_map<std::string, bhv_id_desc_t>;

private:
    enum bhv_type_t : uint32_t {
        nonzero_prob = 0,
        stationary = 1,
        moving = 2,
        mouseover = 3,
        dragged = 4,
        sleeping = 5,
        max_ = 5 // needs to equal the largest index this enum has
    };
    // each behaviour within a library is uniquely identified by 3 numbers:
    // - its 'native' group (the one it got from the config file)
    // - its type (see library_t::group_t)
    // - its index within the type array
    union bhv_id_internal_t {
        bhv_id_t _;
        struct {
            uint32_t index:15;
            bhv_type_t type:3;
            int32_t group:14;
        };
    };
    struct group_t {
        weighted_rng_t nonzero_weights;
        std::array<ref_vec_t<behaviour_t>, bhv_type_t::max_ + 1> bhv;

        void append(const group_t &rhs) {
            for (size_t i = 0; i <= bhv_type_t::max_; i++)
                bhv[i].insert(
                        bhv[i].end(), rhs.bhv[i].begin(), rhs.bhv[i].end());
        }
    };

    std::string library_path_;
    std::string readable_name_;
    std::vector<std::unique_ptr<interaction_t>> interactions_;
    std::vector<std::unique_ptr<behaviour_t>> behaviours_;
    std::vector<std::unique_ptr<speech_t>> speeches_;
    // TODO: remap groups sequenitially and make this a vector?
    std::unordered_map<int, group_t> groups_;
    sprite_bank_t::sprite_t preview_;
    CTRL imagebox_; /// image box control to preview the sprite
    CTRL charname_; /// character name just below the image box
    CTRL spinner_;  /// spin control to set ICNT

    inline static bhv_id_t init_bhv_id(movement_flags_t move, int16_t group);

    const speech_t *select_speech(uint32_t *seed, uint32_t chance,
            bhv_id_t prev, bhv_id_t curr) const;

public:
    class input_t;

    inline const behaviour_t *get(bhv_id_t id) const;

    static bhv_id_desc_t build_bhv_id_desc(const input_t &in);

    library_t(const std::string &path, const input_t &in,
            const bhv_id_map_t &bhv_id_map);
    const std::string &name() const { return readable_name_; }
};

class unit_t : public no_copy_t {
private:
    enum flags_t : uint32_t { /// is_ready & !is_primary == this side is a copy
        empty        = 0,
        is_primary   = 1 << 0,
        is_ready     = 1 << 1,
        is_scheduled = 1 << 2,
        from_path    = 1 << 3,
        fake_center  = 1 << 4,
        repeat       = 1 << 5,
    };
    struct {
        flags_t flags_ = {};
        AINF image_ = {};
        T2IV center_ = {};
    } sides_[2];

    static void finalize(void *data) {
        data = ((uint8_t*)data) - sizeof(intptr_t);
        *((flags_t*)data) &= ~is_scheduled;
        *((flags_t*)data) |= is_ready;
        free(data);
    }
    void set_flag(bool left, bool value, flags_t flag) {
        auto &side = sides_[left];
        if (value)
            side.flags_ |= flag;
        else
            side.flags_ &= ~flag;
    }
    void remove(bool left) {
        auto &side = sides_[left];
        assert(!(side.flags_ & is_scheduled));
        if (!(side.flags_ & is_ready) && side.image_.time)
            finalize(side.image_.time);
        side.image_ = {};
        side.flags_ = {};
    }
    void add_source(bool left) {
        remove(left);
        set_flag(left, true, is_primary);
    }

protected:
    unit_t(): sides_{} {}
    ~unit_t() { remove(false); remove(true); }

    void set_center(bool left, const T2IV &center) {
        sides_[left].center_ = center;
        set_flag(left, (center.x != 0) || (center.y != 0), fake_center);
    }
    void set_primary(bool left, bool what) {
        set_flag(left, what, is_primary);
    }
    void set_loop(bool left, bool loop) {
        set_flag(left, loop, repeat);
    }
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
                std::string_view temp((char*)side.image_.time);
                auto pos = temp.find_last_of(DEF_DSEP);
                if ((pos != std::string_view::npos) && (pos > 0))
                    pos = temp.find_last_of(DEF_DSEP, pos - 1);
                if (pos == std::string_view::npos) pos = 0;
                temp.remove_prefix(pos); // animation hash: last dir + gif name
                cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)temp.data(),
                                     side.image_.time, ELA_DISK, finalize);
            }
            else if (auto anim = (AINF*)side.image_.time) {
                auto name = (uint8_t*)(anim->time + 1);
                cEngineLoadAnimAsync(engd, &side.image_, name, anim,
                                     ELA_AINF, finalize);
            }
        }
    }
    T2IV dims(bool left) {
        return {{(int32_t)sides_[left].image_.xdim,
                 (int32_t)sides_[left].image_.ydim}};
    }
    uint32_t next_frame(bool left, uint32_t curr) {
        return (curr + 1 >= sides_[left].image_.fcnt)
                ? (sides_[left].flags_ & repeat) ? 0 : curr
                : (curr + 1);
    }
    bool is_empty() {
        // TODO: is this correct? there can be and will be full copies of anims
        return !(sides_[0].flags_ & is_primary)
            && !(sides_[1].flags_ & is_primary);
    }
};

class effect_t : public unit_t {
protected:
    enum gravity_flags_t : uint8_t {
        top_left     = 0,
        top          = 1,
        top_right    = 2,
        center_left  = 3,
        center       = 4,
        center_right = 5,
        bottom_left  = 6,
        bottom       = 7,
        bottom_right = 8,
        any          = 9,
        not_center   = 10,
    };
    const struct {
        gravity_flags_t placement:4;
        gravity_flags_t centering:4;
    } gravity_flags_[2];
    const T2IV duration_; // u = duration, v = repeat_delay
    const bool follow_;
    std::vector<T2IV> gravity_[2]; // can't be made const: size unknown at init

    inline T2IV select_gravity(bool left, uint32_t *seed) const {
        return random_selection<false>(gravity_[left], seed);
    }

public:
    class input_t {
    public:
        std::string name;                                        // NOT USED
        std::string bhv;                                         // USED
        std::string right_image;                                 // USED
        std::string left_image;                                  // USED
        float duration = 5.f;                                    // USED
        float repeat_delay = 0.f;                                // USED
        gravity_flags_t placement_right = gravity_flags_t::any;  // USED
        gravity_flags_t centering_right = gravity_flags_t::any;  // USED
        gravity_flags_t placement_left = gravity_flags_t::any;   // USED
        gravity_flags_t centering_left = gravity_flags_t::any;   // USED
        bool follow = false;                                     // USED
        bool prevent_loop = false;                               // USED

        input_t() = default;
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
            bhv = ascii_to_lower(process_string(line));
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

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

    effect_t(const input_t &in, const std::string &path)
    : gravity_flags_{{in.placement_right, in.centering_right},
                     {in.placement_left, in.centering_left}}
    , duration_{{(decltype(duration_.x))(1000.f * in.duration),
                 (decltype(duration_.y))(1000.f * in.repeat_delay)}}
    , follow_(in.follow) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        if (!path.empty()) {
            add_source(false, concat_path({path, in.right_image}));
            add_source(true, concat_path({path, in.left_image}));
        }
        set_loop(false, !in.prevent_loop);
        set_loop(true, !in.prevent_loop);
    }

    // TODO: check if everything is correct with both horz and vert axes
    void finalize(T2IV bhv_right_size, T2IV bhv_left_size) {
        auto process_side = [](gravity_flags_t geff, gravity_flags_t gbhv,
                T2IV eff, T2IV bhv) {
            // depends on the order of elements in gravity_flags_t!
            static const std::array<std::vector<gravity_flags_t>, 9 + 2> i = {{
                {top_left    }, {top         }, {top_right   }, {center_left },
                {center      }, {center_right}, {bottom_left }, {bottom      },
                {bottom_right},
                // any
                {top_left    , top         , top_right   , center_left ,
                 center      , center_right, bottom_left , bottom      ,
                 bottom_right},
                // not_center
                {top_left    , top         , top_right   , center_left ,
                 center_right, bottom_left , bottom      , bottom_right},
            }};
            // depends on the order of elements in gravity_flags_t!
            static const std::array<T2IV, 9> g = {{
                {{0, 2}}, {{1, 2}}, {{2, 2}}, {{0, 1}}, {{1, 1}},
                {{2, 1}}, {{0, 0}}, {{1, 0}}, {{2, 0}},
            }};
            std::vector<T2IV> retn;
            for (auto &b : i[gbhv])
                for (auto &e : i[geff])
                    retn.emplace_back(T2IV{{
                                (g[b].x * bhv.x - g[e].x * eff.x) / 2,
                                (g[b].y * bhv.y - g[e].y * eff.y) / 2}});
            return retn;
        };
        gravity_[false] = process_side(gravity_flags_[false].centering,
                gravity_flags_[false].placement, dims(false), bhv_right_size);
        gravity_[true] = process_side(gravity_flags_[true].centering,
                gravity_flags_[true].placement, dims(true), bhv_left_size);
    }
};

using eff_vec_t = ref_vec_t<effect_t::input_t>;

class speech_t : public effect_t {
public:
    class input_t {
    public:
        std::string name;   // NOT USED
        std::string text;   // NOT USED
        std::string sound;  // NOT USED
        bool skip = false;  // USED
        int group = 0;      // USED

        input_t() = default;
        input_t(const std::string_view &str) {
            token_t line({}, str);
            name = ascii_to_lower(process_string(line));
            text = process_string(line);
            auto sound_files = process_array(line);
            if (!sound_files.empty()) sound = sound_files[0];
            skip = process_bool(line, skip);
            group = process_float(line, group);
        }
        bool validate() const { return !text.empty(); }
    };

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const std::string sound_;

    static effect_t::input_t convert_input(const speech_t::input_t &in) {
        effect_t::input_t retn;
        retn.name = in.name;
        retn.bhv = "group = " + std::to_string(in.group) + ", skip = "
                 + std::to_string(in.skip) + ", sound = " + in.sound;
        retn.right_image = retn.left_image = in.text;
        retn.duration = 0.5f + 0.065f * in.text.size();
        // fixed value taken from DP codebase - only applies to random speech:
        retn.repeat_delay = 10.f;
        retn.placement_right = retn.placement_left = gravity_flags_t::top;
        retn.centering_right = retn.centering_left = gravity_flags_t::bottom;
        retn.follow = true;
        retn.prevent_loop = true;
        return retn;
    }

    static void render_text(uint32_t *data) {
    }

public:
    speech_t(const input_t &in)
    : effect_t(convert_input(in), std::string())
    , sound_(in.sound) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        // TODO: call this one properly!
        add_source(false, in.text, 1, 1, render_text);
    }
};

class behaviour_t : public unit_t {
public:
    class input_t {
    public:
        std::string name;                      // USED
        float chance = 0.f;                    // USED
        float max_duration = 15.f;             // USED
        float min_duration = 5.f;              // USED
        float speed = 3.f;                     // USED
        std::string right_image;               // USED
        std::string left_image;                // USED
        movement_flags_t movement = move_all;  // USED
        std::string linked_bhv;                // USED
        std::string bgn_speech;                // USED
        std::string end_speech;                // USED
        bool skip = false;                     // USED
        T2IV target_xy = {{0, 0}};             // USED
        std::string follow_target;             // USED
        bool auto_follow_img = true;           // USED
        std::string follow_stop_bhv;           // USED
        std::string follow_mov_bhv;            // USED
        T2IV right_img_center = {{0, 0}};      // USED
        T2IV left_img_center = {{0, 0}};       // USED
        bool prevent_loop = false;             // USED
        int group = 0;                         // USED
        bool mirror_target_xy = false;         // USED

        input_t() = default;
        input_t(const std::string_view &str) {
            static std::unordered_map<std::string, bool>
                followflg = { {"false", false}, {"true",   true},
                              {"fixed", false}, {"mirror", true}, };
            static std::unordered_map<std::string, movement_flags_t> moveflg = {
                {"horizontal_vertical", move_hv  }, {"mouseover", move_mouse},
                {"diagonal_horizontal", move_dh  }, {"dragged",   move_drag },
                {"diagonal_vertical",   move_dv  }, {"sleep",     move_sleep},
                {"horizontal_only",     move_horz}, {"none",      move_none },
                {"vertical_only",       move_vert}, {"all",       move_all  },
                {"diagonal_only",       move_diag},
            };
            token_t line({}, str);
            name = ascii_to_lower(process_string(line));
            chance = process_float(line, chance);
            max_duration = process_float(line, max_duration);
            min_duration = process_float(line, min_duration);
            speed = process_float(line, speed);
            right_image = process_string(line);
            left_image = process_string(line);
            movement = process_map(line, moveflg, movement);
            linked_bhv = ascii_to_lower(process_string(line));
            bgn_speech = ascii_to_lower(process_string(line));
            end_speech = ascii_to_lower(process_string(line));
            skip = process_bool(line, skip);
            target_xy.x = process_float(line, target_xy.x);
            target_xy.y = process_float(line, target_xy.y);
            follow_target = ascii_to_lower(process_string(line));
            auto_follow_img = process_bool(line, auto_follow_img);
            follow_stop_bhv = ascii_to_lower(process_string(line));
            follow_mov_bhv = ascii_to_lower(process_string(line));
            right_img_center = process_quoted_int_pair(line, right_img_center);
            left_img_center = process_quoted_int_pair(line, left_img_center);
            prevent_loop = process_bool(line, prevent_loop);
            group = process_float(line, group);
            mirror_target_xy = process_map(line, followflg, mirror_target_xy);
        }
        bool validate() const {
            bool okay = !name.empty();
            okay &= !right_image.empty();
            okay &= !left_image.empty();
            return okay;
        }
    };

    // this is the whole speech switching logic from Desktop Ponies, trust me.
    // 0 means no speech, negatives are behaviour-specific speeches, positives
    // are random speeches. END can only yield negatives (end speech from bhv,
    // if present) or 0; BGN might yield 0, negatives (start speech from bhv),
    // or positives (pre-filled random speeches taken from library that match
    // the BGN group, if start speech is absent)
    static int16_t select_speech(uint32_t *seed, uint32_t chance,
            const behaviour_t &prev, const behaviour_t &curr) {
        auto bgn = random_selection<true>(curr.bgn_speech_idx_, seed);
        auto end = random_selection<true>(prev.end_speech_idx_, seed);
        // priority in DP: 1. start speech; 2. end speech; 3. random speech;
        end = (bgn >= 0) ? (end >= 0) ? bgn : end : bgn;
        // TODO: fix the case when (end > 0) gets replaced with (bgn = 0);
        //       at the time this is hypothetical, but can become relevant
        return ((end < 0) || (RNG_Load(seed) < chance)) ? end : 0;
    }

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    const library_t::bhv_id_t id_;
    const library_t::bhv_id_t linked_id_;
    // follow_grp_id_ is only needed for its group info: have to
    // pick moving/stationary follow images from this very group
    const library_t::bhv_id_t follow_grp_id_;
    const T2IV duration_; // u = min, v = max
    const float movement_speed_;
    const movement_flags_t movement_;
    const std::vector<int16_t> bgn_speech_idx_; // speech indices from library
    const std::vector<int16_t> end_speech_idx_;
    const library_t::lib_id_t follow_tgt_;
    const T2IV follow_offset_[2];
    const std::vector<std::unique_ptr<effect_t>> effects_;

    std::vector<std::unique_ptr<effect_t>> process_effects(
            const std::string &path, const eff_vec_t &eff) {
        std::vector<std::unique_ptr<effect_t>> retn;
        for (auto &e : eff)
            retn.emplace_back(std::make_unique<effect_t>(e, path));
        return retn;
    }

public:
    library_t::bhv_id_t id() const { return id_; }
    library_t::bhv_id_t linked_id() const { return linked_id_; }
    library_t::bhv_id_t follow_grp_id() const { return follow_grp_id_; }

    behaviour_t(const input_t &in, library_t::bhv_id_t id,
            library_t::bhv_id_t linked_id, library_t::bhv_id_t follow_grp_id,
            library_t::lib_id_t follow_tgt, const std::string &path,
            std::vector<int16_t> bgn_speech, std::vector<int16_t> end_speech,
            const eff_vec_t &eff)
    : id_(id)
    , linked_id_(linked_id)
    , follow_grp_id_(follow_grp_id)
    , duration_{{(decltype(duration_.x))(1000.f *
                                std::min(in.min_duration, in.max_duration)),
                 (decltype(duration_.y))(1000.f *
                                std::max(in.min_duration, in.max_duration))}}
    , movement_speed_(in.speed * FRM_WAIT / 30.f)
    , movement_(in.movement)
    , bgn_speech_idx_(std::move(bgn_speech))
    , end_speech_idx_(std::move(end_speech))
    , follow_tgt_(follow_tgt)
    , follow_offset_{in.target_xy, (in.mirror_target_xy)
                                       ? T2IV{{-in.target_xy.x, in.target_xy.y}}
                                       : in.target_xy}
    , effects_(process_effects(path, eff)) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        if (!path.empty()) {
            add_source(false, concat_path({path, in.right_image}));
            add_source(true, concat_path({path, in.left_image}));
        }
        set_center(false, in.right_img_center);
        set_center(true, in.left_img_center);
        set_loop(false, !in.prevent_loop);
        set_loop(true, !in.prevent_loop);
    }
};

class interaction_t : public no_copy_t {
private:
    bool all_;
    uint16_t proximity_;
    uint32_t chance_;
    const T2IV duration_; // u = duration (INT_MAX), v = reactivation delay
    std::vector<library_t::bhv_id_t> initiator_;
    std::unordered_map<library_t::lib_id_t, std::vector<library_t::bhv_id_t>>
        targets_;

public:
    class input_t {
    public:
        std::string name;                    // NOT USED
        float chance = 0.f;                  // NOT USED
        float proximity = 125.f;             // NOT USED
        std::vector<std::string> targets;    // NOT USED
        bool target_activation_all = false;  // NOT USED
        std::vector<std::string> bhv;        // NOT USED
        float reactivation_delay = 60.f;     // NOT USED

        input_t() = default;
        input_t(const std::string_view &str) {
            // TODO: is 'any' really equivalent to 'one'? check if there are
            //       relevant interactions where 'any' means '>1'
            static std::unordered_map<std::string, bool> activations = {
                {"true", true}, {"false",  false}, {"any", false},
                {"all",  true}, {"random", false}, {"one", false},
            };
            token_t line({}, str);
            name = process_string(line);
            chance = process_float(line, chance);
            proximity = process_float(line, proximity);
            auto tgt_s = process_array(line);
            for (auto &t : tgt_s) targets.emplace_back(ascii_to_lower(t));
            target_activation_all
                    = process_map(line, activations, target_activation_all);
            auto bhv_s = process_array(line);
            for (auto &b : bhv_s) bhv.emplace_back(ascii_to_lower(b));
            reactivation_delay = process_float(line, reactivation_delay);
        }
        bool validate() const {
            bool okay = !targets.empty();
            okay &= !bhv.empty();
            for (auto &t : targets) okay &= !t.empty();
            for (auto &b : bhv) okay &= !b.empty();
            return okay;
        }
    };

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

    bool is_empty() const { return initiator_.empty() || targets_.empty(); }

    interaction_t(const input_t &in, const std::string &lib_name,
            const library_t::bhv_id_map_t &bhv_map)
    : all_(in.target_activation_all)
    , proximity_(in.proximity)
    , chance_(double(uint32_t(~0)) * std::clamp(in.chance, 0.f, 1.f))
    , duration_{{std::numeric_limits<decltype(duration_.x)>::max(),
                (decltype(duration_.y))(1000.f * in.reactivation_delay)}} {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        auto append_bhv = [&](const std::string &lib_id, bool target) {
            auto &retn = (target) ? targets_[str_hash(lib_id)] : initiator_;
            if (auto bhv_id_desc = find_in_map(bhv_map, lib_id))
                for (auto &b : in.bhv)
                    if (auto bhv_id = find_in_map(bhv_id_desc->m, b))
                        retn.emplace_back(*bhv_id);
            if (retn.empty()) {
                printf("[%s] WARNING: no interaction behaviours with '%s' in "
                       "'%s', target dropped\n",
                        lib_name.c_str(), lib_id.c_str(), in.name.c_str());
                return false;
            }
            return true;
        };
        if (append_bhv(lib_name, false)) {
            for (auto &t : in.targets) append_bhv(t, true);
            for (auto it = targets_.begin(); it != targets_.end(); )
                it = (it->second.empty()) ? targets_.erase(it) : std::next(it);
        }
        if (is_empty())
            printf("[%s] WARNING: no interaction behaviours in '%s', "
                   "interaction dropped\n",
                    lib_name.c_str(), in.name.c_str());
    }
};

class library_t::input_t {
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

library_t::bhv_id_desc_t library_t::build_bhv_id_desc(const input_t &in) {
    std::unordered_map<int, std::array<int, bhv_type_t::max_ + 1>> bhv_grp;
    bhv_id_desc_t retn;
    for (auto &b : in.behaviours) {
        bhv_id_internal_t iid{init_bhv_id(b.movement, b.group)};
        if (!find_in_map(bhv_grp, iid.group)) bhv_grp[iid.group] = {};
        iid.index += bhv_grp[iid.group][iid.type]++;
        retn.v.emplace_back(iid._);
        if (!b.name.empty() && !find_in_map(retn.m, b.name)) {
            retn.m.emplace(b.name, iid._);
        } else {
            printf("[%s] WARNING, behaviour name collision: '%s'\n",
                    in.name.c_str(), b.name.c_str());
        }
    }
    return retn;
}

library_t::library_t(const std::string &path, const input_t &in,
        const bhv_id_map_t &bhv_id_map)
: library_path_(path)
, readable_name_(in.name) {
    auto hashable_name = ascii_to_lower(in.name);

    // processing the interactions
    for (auto &i : in.interactions) {
        auto it = std::make_unique<interaction_t>(i, hashable_name, bhv_id_map);
        if (!it->is_empty())
            interactions_.emplace_back(std::move(it));
    }
    // distributing effects configs by group
    std::unordered_map<std::string, eff_vec_t> effects;
    for (auto &e : in.effects)
        effects[e.bhv].emplace_back(e);

    std::unordered_map<std::string, int> spk_bhv_map;
    std::unordered_map<int, std::vector<int>> spk_rnd_map;
    std::unordered_map<std::string, behaviour_t*> bhv_map;
    eff_vec_t e_null;

    // distributing behaviour-linked (< 0) and random (> 0) speeches
    for (auto &s : in.speeches) {
        auto dejavu = find_in_map(spk_bhv_map, s.name);
        if (dejavu)
            printf("[%s] WARNING, speech name collision: '%s'%s\n",
                    in.name.c_str(), s.name.c_str(),
                    (s.skip) ? ", non-selectable (dropped)" : ", selectable");
        if (!dejavu || !s.skip) {
            speeches_.emplace_back(std::make_unique<speech_t>(s));
            spk_bhv_map.emplace(s.name, -int(speeches_.size()));
            if (!s.skip)
                spk_rnd_map[s.group].emplace_back(int(speeches_.size()));
        }
    }
    auto i0 = find_in_map(spk_rnd_map, 0); // random speeches from group 0

    // creating the behaviours
    auto &bhv_id_desc = *find_in_map(bhv_id_map, hashable_name);
    for (size_t i = 0; i < in.behaviours.size(); i++) {
        std::vector<int16_t> b_spk, e_spk;
        auto &b = in.behaviours[i];
        auto ig = find_in_map(spk_rnd_map, b.group);
        if (auto ib = find_in_map(spk_bhv_map, b.bgn_speech)) {
            b_spk.emplace_back(*ib); // found behaviour-specific start speech
        } else if (ig || i0) {
            // no start speech found, substituting it with random speeches;
            // no speech can belong to >1 group, don't look for duplicates
            if (ig) b_spk.insert(b_spk.end(), ig->begin(), ig->end());
            if (i0 && (b.group != 0))
                b_spk.insert(b_spk.end(), i0->begin(), i0->end());
        }
        if (auto ie = find_in_map(spk_bhv_map, b.end_speech))
            e_spk.emplace_back(*ie); // found behaviour-specific end speech

        bhv_id_internal_t iid{bhv_id_desc.v[i]};
        bhv_id_internal_t linked_iid = {};
        if (auto il = find_in_map(bhv_id_desc.m, b.linked_bhv)) {
            linked_iid._ = *il;
        } else if (!b.linked_bhv.empty()) {
            printf("[%s] WARNING, invalid linked behaviour name in '%s'\n",
                    in.name.c_str(), b.name.c_str());
        }
        auto is = find_in_map(bhv_id_desc.m, b.follow_stop_bhv);
        auto im = find_in_map(bhv_id_desc.m, b.follow_mov_bhv);
        bhv_id_internal_t follow_grp_iid = {};
        follow_grp_iid.group = (is && im) ? -int16_t(i) : iid.group;

        auto ie = find_in_map(effects, b.name);
        behaviours_.emplace_back(std::make_unique<behaviour_t>(b, iid._,
                    linked_iid._, follow_grp_iid._, str_hash(b.follow_target),
                    path, b_spk, e_spk, (ie) ? *ie : e_null));
        groups_[iid.group].bhv[iid.type].emplace_back(*behaviours_.back());
        bhv_map.emplace(b.name, behaviours_.back().get());
        assert(iid.index == groups_[iid.group].bhv[iid.type].size());
    }

    std::unordered_map<int, std::vector<uint32_t>> prob_map;
    for (size_t i = 0; i < in.behaviours.size(); i++) {
        auto &b = in.behaviours[i];
        if ((!b.skip) && (b.chance > 0.f)) {
            prob_map[b.group].emplace_back(
                    10000.f * std::clamp(b.chance, 0.f, 1.f));
            groups_[b.group].bhv[nonzero_prob].emplace_back(*behaviours_[i]);
        }
    }
    // adding behaviours from group 0 (GroupAny) to all other groups
    if (auto ig = find_in_map(groups_, 0))
        for (auto &p : prob_map)
            if (p.first != 0) {
                p.second.insert(
                        p.second.end(), prob_map[0].begin(), prob_map[0].end());
                groups_[p.first].append(*ig);
            }
    // initializing probabilities
    for (auto &p : prob_map) {
        assert(groups_[p.first].bhv[nonzero_prob].size() == p.second.size());
        groups_[p.first].nonzero_weights = weighted_rng_t(p.second);
    }
    // creating special groups where follow images are to be taken from
    for (size_t i = 0; i < in.behaviours.size(); i++)
        if (!in.behaviours[i].follow_target.empty()
                && !in.behaviours[i].auto_follow_img) {
            group_t grp;
            if (auto is = find_in_map(
                        bhv_map, in.behaviours[i].follow_stop_bhv))
                grp.bhv[stationary].emplace_back(**is);
            if (auto im = find_in_map(
                        bhv_map, in.behaviours[i].follow_mov_bhv))
                grp.bhv[moving].emplace_back(**im);

            if (!grp.bhv[moving].empty() && !grp.bhv[stationary].empty()) {
                groups_[-int16_t(i)] = std::move(grp);
            } else {
                printf("[%s] WARNING, invalid custom follow behaviours in "
                       "'%s', reverting to defaults\n",
                        in.name.c_str(), in.behaviours[i].name.c_str());
            }
        }

    printf("Total behaviours: %lu\n", behaviours_.size());
    for (auto &g : groups_) {
        printf("[%d] %lu + %lu + %lu + %lu + %lu + %lu\n", g.first,
            g.second.bhv[nonzero_prob].size(), g.second.bhv[stationary].size(),
            g.second.bhv[moving].size(), g.second.bhv[mouseover].size(),
            g.second.bhv[dragged].size(), g.second.bhv[sleeping].size());
    }
}

library_t::bhv_id_t library_t::init_bhv_id(movement_flags_t move, int16_t grp) {
    bhv_id_internal_t iid;
    iid.index = 1;
    switch (move) {
        case move_none:  iid.type = stationary; break;
        case move_mouse: iid.type = mouseover;  break;
        case move_drag:  iid.type = dragged;    break;
        case move_sleep: iid.type = sleeping;   break;
        case move_all: case move_dv: case move_horz: case move_diag:
        case move_hv:  case move_dh: case move_vert: iid.type = moving; break;
    }
    iid.group = grp;
    return iid._;
}

const behaviour_t *library_t::get(bhv_id_t id) const {
    bhv_id_internal_t iid = {id};
    if (!iid.index) return nullptr;
    auto ig = find_in_map(groups_, iid.group);
    return (ig && (iid.index <= ig->bhv[iid.type].size()))
            ? &ig->bhv[iid.type][iid.index - 1].get()
            : nullptr;
}

const speech_t *library_t::select_speech(uint32_t *seed, uint32_t chance,
        bhv_id_t prev, bhv_id_t curr) const {
    auto b_prev = get(prev), b_curr = get(curr);
    if (!b_prev || !b_curr) return nullptr;
    auto index = behaviour_t::select_speech(seed, chance, *b_prev, *b_curr);
    assert(std::abs(index) <= speeches_.size());
    return (index) ? speeches_[std::abs(index) - 1].get() : nullptr;
}

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
class engine_t : public no_copy_t {
private:
    MENU     *mspr_, /// per-sprite context menu
             *mctx_; /// engine`s main context menu
    CTRL     *mctl_, /// GUI controls array (main window)
             *octl_; /// GUI controls array (options window)
    uint64_t  tcur_, /// current, dilation-adjusted timestamp
              tpre_; /// previous raw timestamp
    float     tacc_; /// partial timestamp accumulator
    T3IV      ppos_; /// mouse pointer position (z = flags)
    T2IV      dpos_, /// drawing area position
              dims_, /// drawing area dimensions
              idim_; /// tray icon dimensions
    CONF      cdef_, /// default configuration
              ccur_, /// current configuration
              cini_; /// initial configuration read at the start

    std::unordered_map<library_t::lib_id_t, std::unique_ptr<library_t>> libs_;
    std::vector<std::vector<library_t::lib_id_t>> categories_;

public:
    void build_library_structure(const std::string &path,
            const std::vector<library_t::input_t> &ins);
};

void engine_t::build_library_structure(const std::string &base,
        const std::vector<library_t::input_t> &ins) {
    std::unordered_map<std::string, std::unordered_set<library_t::lib_id_t>>
        categories;
    library_t::bhv_id_map_t bhv_id_map;

    // construct the behaviour ID descriptors and draft categories for everyone
    for (size_t i = 0; i < ins.size(); i++) {
        auto ib = bhv_id_map.emplace(ascii_to_lower(ins[i].name),
                    library_t::build_bhv_id_desc(ins[i]));
        assert(ib.second); // make sure the library has a unique name
        for (auto &c : ins[i].categories)
            categories[c].emplace(str_hash(ins[i].name));
    }

    // process follow targets (nothing to do beside report missing ones)
    for (auto &i : ins)
        for (auto &b : i.behaviours)
            if (!b.follow_target.empty())
                if (!find_in_map(bhv_id_map, b.follow_target))
                    printf("[%s] WARNING, invalid follow target name in '%s'\n",
                            i.name.c_str(), b.name.c_str());

    // load the libraries
    for (size_t i = 0; i < ins.size(); i++) {
        printf("%s\n", ins[i].name.c_str());
        auto path = concat_path({base, ins[i].name});
        libs_.emplace(str_hash(ins[i].name),
                std::make_unique<library_t>(path, ins[i], bhv_id_map));
    }

    // process categories
    categories_.reserve(categories.size());
    for (auto &c : categories) {
        categories_.emplace_back();
        for (auto l : c.second) categories_.back().emplace_back(l);
        auto names = [&](library_t::lib_id_t &a, library_t::lib_id_t &b) {
            auto lib_a = find_in_map(libs_, a);
            auto lib_b = find_in_map(libs_, b);
            return (*lib_a)->name() < (*lib_b)->name();
        };
        std::sort(categories_.back().begin(), categories_.back().end(), names);
        // TODO: associate names/indices with the list control
    }
}

void eProcessMenuItem(MENU *item) {
}

// TODO: what is 'base'?
void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    engine_t engc;
    if (fcnf) {
        auto config = concat_path({fcnf, DEF_CORE});
        if (auto file = rLoadFile(config.c_str(), nullptr)) {
            std::string base;
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
            std::vector<library_t::input_t> ins;
            auto find = rFindMake(base.c_str());
            while (!!(file = rFindFile(find))) {
                ins.emplace_back(library_t::input_t(base, file));
                free(file);
            }
            engc.build_library_structure(base, ins);
        }
    }
}
