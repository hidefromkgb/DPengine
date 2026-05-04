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
    weighted_rng_t(const std::vector<int> &weights) {
        if (weights.empty()) return;
        uint32_t full = 0, over = 0, size = uint32_t(weights.size());
        data_.resize(size);
        for (uint32_t i = size; i > 0; i--) {
            assert(weights[i - 1] > 0);
            data_[i - 1] = {uint32_t(weights[i - 1]), 0, i - 1};
            full += uint32_t(weights[i - 1]);
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
    uint32_t choose(uint32_t seed) const {
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

class library_t;
class speech_t;
class effect_t;
class behaviour_t;

template <typename T>
using ref_vec_t = std::vector<std::reference_wrapper<const T>>;
using lib_vec_t = ref_vec_t<library_t>;
using bhv_vec_t = ref_vec_t<behaviour_t>;

template <typename T>
using ptr_map_t = std::unordered_map<std::string, std::pair<int, T*>>;
using bhv_map_t = ptr_map_t<behaviour_t>;



class no_copy_t {
public:
    no_copy_t() = default;
    no_copy_t(no_copy_t&) = delete;       /// non construction-copyable
    no_copy_t(const no_copy_t&) = delete; /// non construction-copyable
    no_copy_t& operator=(no_copy_t&) = delete;       /// non copyable
    no_copy_t& operator=(const no_copy_t&) = delete; /// non copyable
};

class unit_t : public no_copy_t {
private:
    enum state_flags_t { /// is_ready & !is_primary == this side is a copy
        empty        = 0,
        is_primary   = 1 << 0,
        is_scheduled = 1 << 1,
        is_ready     = 1 << 2,
        from_path    = 1 << 3,
        fake_center  = 1 << 4,
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
private:
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
        __reserved_0 = 12,
        __reserved_1 = 13,
        __reserved_2 = 14,
        __reserved_3 = 15,
    };
    struct {
        gravity_flags_t placement:4;
        gravity_flags_t centering:4;
    } flags_[2];
    bool parent_follow_;
    uint32_t duration_;
    uint32_t respawn_;

public:
    class input_t {
    public:
        std::string name;                                        // NOT USED
        std::string bhv;                                         // USED
        std::string right_image;                                 // NOT USED
        std::string left_image;                                  // NOT USED
        float duration = 5.f;                                    // NOT USED
        float repeat_delay = 0.f;                                // NOT USED
        gravity_flags_t placement_right = gravity_flags_t::any;  // NOT USED
        gravity_flags_t centering_right = gravity_flags_t::any;  // NOT USED
        gravity_flags_t placement_left = gravity_flags_t::any;   // NOT USED
        gravity_flags_t centering_left = gravity_flags_t::any;   // NOT USED
        bool follow = false;                                     // NOT USED
        bool prevent_loop = false;                               // NOT USED

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

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

    effect_t(const input_t &in) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
    }
};

using eff_vec_t = ref_vec_t<effect_t::input_t>;

class speech_t : public unit_t {
private:
    std::string sound_;

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
            name = process_string(line);
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

    speech_t(const input_t &in) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
    }
};

class behaviour_t : public unit_t {
public:
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

    class input_t {
    public:
        std::string name;                                   // USED
        float chance = 0.f;                                 // USED
        float max_duration = 15.f;                          // USED
        float min_duration = 5.f;                           // USED
        float speed = 3.f;                                  // USED
        std::string right_image;                            // USED
        std::string left_image;                             // USED
        movement_flags_t movement = movement_flags_t::all;  // USED
        std::string linked_bhv;                             // USED
        std::string bgn_speech;                             // USED
        std::string end_speech;                             // USED
        bool skip = false;                                  // USED
        T2IV target_xy = {{0, 0}};                          // USED
        std::string follow_target;                          // USED
        bool auto_follow_img = true;                        // USED
        std::string follow_stop_bhv;                        // USED
        std::string follow_mov_bhv;                         // USED
        T2IV right_img_center = {{0, 0}};                   // USED
        T2IV left_img_center = {{0, 0}};                    // USED
        bool prevent_loop = false;                          // USED
        int group = 0;                                      // USED
        bool mirror_target_xy = false;                      // USED

        input_t() = default;
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
            linked_bhv = process_string(line);
            bgn_speech = process_string(line);
            end_speech = process_string(line);
            skip = process_bool(line, skip);
            target_xy.x = process_float(line, target_xy.x);
            target_xy.y = process_float(line, target_xy.y);
            follow_target = process_string(line);
            auto_follow_img = process_bool(line, auto_follow_img);
            follow_stop_bhv = process_string(line);
            follow_mov_bhv = process_string(line);
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
    // are random speeches. this function never returns negatives, as they are
    // only needed for internal reference. END can only contain negatives (end
    // speech from behaviour, if present) or 0; BGN might contain 0, negatives
    // (start speech from behaviour), or positives (pre-filled random speeches
    // taken from library that match the BGN group, if start speech is absent)
    static uint16_t select_speech(uint32_t *seed, uint32_t chance,
            const behaviour_t &prev, const behaviour_t &curr) {
        auto select = [](const std::vector<int16_t> &v, uint32_t *seed) {
            if (v.empty()) return std::remove_reference_t<decltype(v[0])>(0);
            return v[(v.size() > 1) ? RNG_Load(seed) % v.size() : 0];
        };
        auto bgn = select(curr.bgn_speech_idx_, seed);
        auto end = select(prev.end_speech_idx_, seed);
        // priority in DP: 1. start speech; 2. end speech; 3. random speech;
        end = (bgn >= 0) ? (end >= 0) ? bgn : end : bgn;
        // TODO: fix the case when (end > 0) gets replaced with (bgn = 0);
        //       at the time this is hypothetical, but can become relevant
        return (end > 0) ? (RNG_Load(seed) < chance) ? end : 0 : -end;
    }

#ifdef DEV_MODE
    input_t debug_;
#endif // DEV_MODE

private:
    int16_t group_;
    int16_t follow_group_; // select moving/stationary follow images from here
    T2IV duration_; // u = min, v = max
    float movement_speed_;
    movement_flags_t movement_;
    std::vector<int16_t> bgn_speech_idx_; // indices for speeches from library
    std::vector<int16_t> end_speech_idx_;
    const behaviour_t *linked_bhv_;
    const library_t *follow_target_;
    T2IV follow_offset_[2];
    std::vector<std::unique_ptr<effect_t>> effects_;

public:
    //uint16_t group() const { return group_; }
    void set_follow_group(int16_t group) { follow_group_ = group; }
    void set_follow_target(const library_t *target) { follow_target_ = target; }
    void set_linked_behaviour(const behaviour_t *linked) {
        linked_bhv_ = linked;
    }
    behaviour_t(const input_t &in, const std::string &path, const eff_vec_t &ev,
            std::vector<int16_t> bgn_speech, std::vector<int16_t> end_speech)
    : group_(in.group)
    , follow_group_(in.group)
    , duration_{{int32_t(std::min(in.min_duration, in.max_duration) * 1000.f),
                 int32_t(std::max(in.min_duration, in.max_duration) * 1000.f)}}
    , movement_speed_(in.speed * FRM_WAIT / 30.f)
    , movement_(in.movement)
    , bgn_speech_idx_(std::move(bgn_speech))
    , end_speech_idx_(std::move(end_speech))
    , linked_bhv_(nullptr)
    , follow_target_(nullptr) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
        follow_offset_[false] = in.target_xy;
        follow_offset_[true] = (in.mirror_target_xy)
                ? T2IV{{-in.target_xy.x, in.target_xy.y}}
                : in.target_xy;
        add_source(false, concat_path({path, in.right_image}));
        add_source(true, concat_path({path, in.left_image}));
        set_center(false, in.right_img_center);
        set_center(true, in.left_img_center);
        set_loop(!in.prevent_loop);
        for (auto &e : ev) effects_.emplace_back(std::make_unique<effect_t>(e));
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
            targets = std::vector<std::string>(tgt_s.begin(), tgt_s.end());
            target_activation_all
                    = process_map(line, activations, target_activation_all);
            auto bhv_s = process_array(line);
            bhv = std::vector<std::string>(bhv_s.begin(), bhv_s.end());
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

    interaction_t(const input_t &in) {
#ifdef DEV_MODE
        debug_ = in;
#endif // DEV_MODE
    }
};

class library_t : public no_copy_t {
private:
    struct group_t {
        weighted_rng_t nonzero_weights;
        bhv_vec_t nonzero_prob;
        bhv_vec_t stationary;
        bhv_vec_t moving;
        bhv_vec_t mouseover;
        bhv_vec_t dragged;
        bhv_vec_t sleep;

        void append(const group_t &rhs) {
            #define APPEND(name) \
                name.insert(name.end(), rhs.name.begin(), rhs.name.end())
            APPEND(nonzero_prob);
            APPEND(stationary);
            APPEND(moving);
            APPEND(mouseover);
            APPEND(dragged);
            APPEND(sleep);
            #undef APPEND
        }
    };
    CTRL imagebox_; /// image box control to preview the sprite
    CTRL charname_; /// character name just below the image box
    CTRL spinner_;  /// spin control to set ICNT
    std::vector<std::unique_ptr<behaviour_t>> behaviours_;
    std::vector<std::unique_ptr<speech_t>> speeches_;
    // TODO: remap groups sequenitially and make this a vector?
    std::unordered_map<int, group_t> groups_;
    std::string library_path_;
    std::string readable_name_;
    std::string scrollable_name_;
    sprite_bank_t::sprite_t preview_;

    const speech_t *select_speech(uint32_t *seed, uint32_t chance,
            const behaviour_t &prev, const behaviour_t &curr) {
        auto index = behaviour_t::select_speech(seed, chance, prev, curr);
        assert(index <= speeches_.size());
        return (index > 0) ? speeches_[index - 1].get() : nullptr;
    }

    template <typename M, typename I, typename V>
    void append_map(M &m, const I &in, const char *name, const char *ty, V v) {
        auto iter = m.emplace(in.name, typename M::mapped_type(in.group, v));
        if (!iter.second)
            printf("[%s] WARNING, %s name collision: '%s'\n",
                    name, ty, in.name.c_str());
    }

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

    library_t(const std::string &path, const input_t &in, bhv_map_t &bhv_map) {
        std::unordered_map<std::string, std::pair<int, int>> spk_map;
        std::unordered_map<int, std::vector<int>> grp_map;
        std::unordered_map<std::string, eff_vec_t> effects;
        eff_vec_t null;

        library_path_ = path;
        readable_name_ = in.name;

        for (auto &e : in.effects)
            effects[e.bhv].emplace_back(e);
        for (auto &s : in.speeches) {
            speeches_.emplace_back(std::make_unique<speech_t>(s));
            append_map(spk_map, s, in.name.c_str(), "speech",
                    -int(speeches_.size()));
            if (!s.skip) grp_map[s.group].emplace_back(int(speeches_.size()));
        }
        auto i0 = grp_map.find(0);
        for (auto &b : in.behaviours) {
            std::vector<int16_t> b_spk, e_spk;
            auto ig = grp_map.find(b.group);
            auto ib = spk_map.find(b.bgn_speech);
            if (ib != spk_map.end()) { // found behaviour-specific start speech
                b_spk.emplace_back(ib->second.second);
            } else if ((ig != grp_map.end()) || (i0 != grp_map.end())) {
                // no start speech found, substituting it with random speeches;
                // no speech can belong to >1 group, don't look for duplicates
                if (ig != grp_map.end())
                    b_spk.insert(b_spk.end(),
                            ig->second.begin(), ig->second.end());
                if ((i0 != grp_map.end()) && (b.group != 0))
                    b_spk.insert(b_spk.end(),
                            i0->second.begin(), i0->second.end());
            }
            auto ie = spk_map.find(b.end_speech);
            if (ie != spk_map.end()) { // found behaviour-specific end speech
                e_spk.emplace_back(ie->second.second);
            }
            auto e = effects.find(b.name);
            behaviours_.emplace_back(std::make_unique<behaviour_t>(b, path,
                        (e != effects.end()) ? e->second : null, b_spk, e_spk));
            append_map(bhv_map, b, in.name.c_str(), "behaviour",
                    behaviours_.back().get());
        }

        grp_map.clear();
        for (size_t i = 0; i < in.behaviours.size(); i++) {
            auto &b = in.behaviours[i];
            if ((!b.skip) && (b.chance > 0.f)) {
                grp_map[b.group].emplace_back(b.chance * 10000.f);
                groups_[b.group].nonzero_prob.emplace_back(*behaviours_[i]);
            }
            switch (b.movement) {
                case behaviour_t::movement_flags_t::none:
                    groups_[b.group].stationary.emplace_back(*behaviours_[i]);
                    break;
                case behaviour_t::movement_flags_t::mouse:
                    groups_[b.group].mouseover.emplace_back(*behaviours_[i]);
                    break;
                case behaviour_t::movement_flags_t::drag:
                    groups_[b.group].dragged.emplace_back(*behaviours_[i]);
                    break;
                case behaviour_t::movement_flags_t::sleep:
                    groups_[b.group].sleep.emplace_back(*behaviours_[i]);
                    break;
                case behaviour_t::movement_flags_t::all:
                case behaviour_t::movement_flags_t::horz:
                case behaviour_t::movement_flags_t::vert:
                case behaviour_t::movement_flags_t::diag:
                case behaviour_t::movement_flags_t::horz_vert:
                case behaviour_t::movement_flags_t::diag_horz:
                case behaviour_t::movement_flags_t::diag_vert:
                    groups_[b.group].moving.emplace_back(*behaviours_[i]);
                    break;
            }
        }
        // adding random behaviours from group 0 (GroupAny) to all other groups
        if (groups_.find(0) != groups_.end()) {
            for (auto &g : grp_map) {
                if (g.first == 0) continue;
                g.second.insert(
                        g.second.end(), grp_map[0].begin(), grp_map[0].end());
                groups_[g.first].append(groups_[0]);
            }
        }
        // initializing probability maps
        for (auto &g : grp_map) {
            assert(groups_[g.first].nonzero_prob.size() == g.second.size());
            groups_[g.first].nonzero_weights = weighted_rng_t(g.second);
        }
        // creating special groups where follow images are to be taken from,
        // also linking behaviours
        for (size_t i = 0; i < in.behaviours.size(); i++) {
            if (!in.behaviours[i].follow_target.empty()
                    && !in.behaviours[i].auto_follow_img) {
                group_t grp;
                auto is = bhv_map.find(in.behaviours[i].follow_stop_bhv);
                if (is != bhv_map.end())
                    grp.stationary.emplace_back(*is->second.second);

                auto im = bhv_map.find(in.behaviours[i].follow_mov_bhv);
                if (im != bhv_map.end())
                    grp.moving.emplace_back(*im->second.second);

                if (!grp.moving.empty() && !grp.stationary.empty()) {
                    groups_[-int16_t(i)] = std::move(grp);
                    behaviours_[i]->set_follow_group(-int16_t(i));
                } else {
                    printf("[%s] WARNING, invalid custom follow behaviours in "
                           "'%s', reverting to defaults\n",
                            in.name.c_str(), in.behaviours[i].name.c_str());
                }
            }
            auto il = bhv_map.find(in.behaviours[i].linked_bhv);
            if (il != bhv_map.end()) {
                behaviours_[i]->set_linked_behaviour(il->second.second);
            } else if (!in.behaviours[i].linked_bhv.empty()) {
                printf("[%s] WARNING, invalid linked behaviour name in '%s'\n",
                        in.name.c_str(), in.behaviours[i].name.c_str());
            }
        }

        printf("Total behaviours: %lu\n", behaviours_.size());
        for (auto &g : groups_) {
            printf("[%d] %lu + %lu + %lu + %lu + %lu + %lu\n", g.first,
                    g.second.nonzero_prob.size(), g.second.stationary.size(),
                    g.second.moving.size(), g.second.mouseover.size(),
                    g.second.dragged.size(), g.second.sleep.size());
        }
    }
    const std::string &name() const { return readable_name_; }

    void add_interaction() {
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

    std::vector<std::unique_ptr<library_t>> libs_;
    std::vector<lib_vec_t> categories_;

public:
    void build_library_structure(const std::string &path,
            const std::vector<library_t::input_t> &ins);
};

void engine_t::build_library_structure(const std::string &base,
        const std::vector<library_t::input_t> &ins) {
    std::unordered_map<std::string, std::unordered_set<library_t*>> categories;
    std::unordered_map<std::string, std::pair<library_t*, bhv_map_t>> lib_map;

    // load the libraries
    for (size_t i = 0; i < ins.size(); i++) {
        printf("%s\n", ins[i].name.c_str());
        bhv_map_t bhv_map;
        auto path = concat_path({base, ins[i].name});
        libs_.emplace_back(std::make_unique<library_t>(path, ins[i], bhv_map));
        auto il = lib_map.emplace(ascii_to_lower(ins[i].name),
                    std::make_pair(libs_.back().get(), bhv_map));
        assert(il.second); // make sure the library has a unique name in the map
        ((void)il);
        for (auto &c : ins[i].categories)
            categories[c].emplace(libs_.back().get());
    }

    // process follow targets
    for (auto &i : ins)
        for (auto &b : i.behaviours)
            if (!b.follow_target.empty()) {
                auto il = lib_map.find(ascii_to_lower(b.follow_target));
                if (il != lib_map.end()) {
                    auto &bhv_map = lib_map[ascii_to_lower(i.name)].second;
                    bhv_map[b.name].second->set_follow_target(il->second.first);
                } else {
                    printf("[%s] WARNING, invalid follow target name in '%s'\n",
                            i.name.c_str(), b.name.c_str());
                }
            }

    // process interactions
    // TODO: precompute the possible behaviours for each group in each of the
    //       participants and put them in a map keyed by group ID
    //for (size_t i = 0; i < ins.size(); i++)
    //    for (auto &in : ins[i].interactions) {
    //        libs_[i]->add_interaction();
    //    }

    // process categories
    categories_.reserve(categories.size());
    for (auto &c : categories) {
        categories_.emplace_back();
        for (auto &l : c.second) categories_.back().emplace_back(*l);
        using T = lib_vec_t::value_type;
        auto names = [](T &a, T &b) { return a.get().name() < b.get().name(); };
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
            std::vector<library_t::input_t> ins;
            while (!!(file = rFindFile(find))) {
                ins.emplace_back(library_t::input_t(base, file));
                free(file);
            }
            engc.build_library_structure(base, ins);
        }
    }
}
