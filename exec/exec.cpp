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
#define MCT_CAPT mctl_[ 0]
#define MCT_FLTR mctl_[ 1]
#define MCT_EXAC mctl_[ 2]
#define MCT_OGRP mctl_[ 3]
#define MCT_SGRP mctl_[ 4]
#define MCT_SPEC mctl_[ 5]
#define MCT_BADD mctl_[ 6]
#define MCT_SRND mctl_[ 7]
#define MCT_RGPU mctl_[ 8]
#define MCT_BDUP mctl_[ 9]
#define MCT_SELE mctl_[10]
#define MCT_OPTS mctl_[11]
#define MCT_GOGO mctl_[12]
#define MCT_CHAR mctl_[13]

/// /// /// /// /// /// /// /// /// ENGC.OCTL array indices
#define OCT_OPTS octl_[ 0]
#define OCT_UONR octl_[ 1]
#define OCT_ETOP octl_[ 2]
#define OCT_EEFF octl_[ 3]
#define OCT_EINT octl_[ 4]
#define OCT_ESAY octl_[ 5]
#define OCT_ECLR octl_[ 6]
#define OCT_ERCH octl_[ 7]
#define OCT_NRUN octl_[ 8]
#define OCT_TRUN octl_[ 9]
#define OCT_NSCA octl_[10]
#define OCT_TSCA octl_[11]
#define OCT_NDIL octl_[12]
#define OCT_TDIL octl_[13]
#define OCT_NSAY octl_[14]
#define OCT_TSAY octl_[15]
#define OCT_NCDR octl_[16]
#define OCT_TCDR octl_[17]
#define OCT_LCHO octl_[20]
#define OCT_LREL octl_[21]
#define OCT_LRES octl_[22]
#define OCT_LGUI octl_[23]
#define OCT_BCHO octl_[26]
#define OCT_BREL octl_[27]
#define OCT_BRES octl_[28]
#define OCT_BDIR octl_[29]
#define OCT_FREL octl_[31]
#define OCT_FRES octl_[32]

enum {
/** framerate limiter in msec   **/ FRM_WAIT = 40,
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
        case move_all: case move_dh: case move_horz: case move_diag:
        case move_hv:  case move_dv: case move_vert: iid.type = moving; break;
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
class conf_t {
public:
    enum flags_t : uint32_t {
        none        = 0,
        draw        = 1 <<  0,
        show        = 1 <<  1,
        gpu         = 1 <<  2,
        opaque      = 1 <<  3,
        wbgra       = 1 <<  4,
        wpbo        = 1 <<  5,
        wregion     = 1 <<  6,
        update      = 1 <<  7,
        topmost     = 1 <<  8,
        effects     = 1 <<  9,
        interaction = 1 << 10,
        speech      = 1 << 11,
        cspeech     = 1 << 12,
        hover       = 1 << 13,
    };
    class spin_t {
    private:
        int16_t curr_, min_, max_;
    public:
        spin_t() : curr_{}, min_{}, max_{} {};
        spin_t(int16_t curr, int16_t min, int16_t max)
            : curr_{std::clamp(curr, min, max)}, min_{min}, max_{max} {}
        int16_t set(int16_t dist) {
            return curr_ = (dist <= max_) ? (dist >= min_) ? dist : min_ : max_;
        }
        int16_t get() const { return curr_; }
        int16_t move(int16_t dist) { return set(get() + dist); }
    };
    std::string lang;  // name of the language file
    std::string base;  // path to the animation base
    flags_t flgs = {};
    spin_t nrun = spin_t(  5,    0,  1000); // runs between updates
    spin_t nsca = spin_t(100,   25,   300); // base scaling factor
    spin_t ndil = spin_t(100,   10,  1000); // time dilation factor
    spin_t nsay = spin_t( 50,    0,   100); // random speech chance
    spin_t ncdr = spin_t(  0,    0,  1000); // cursor dodge radius
    spin_t spec = spin_t(  0, -100,   100); // group selection
    spin_t rgpu = spin_t(  0,    0, 30000); // random selection
};

/// engine data (client side)
class engine_t : public no_copy_t {
private:
    std::vector<MENU> mspr_; /// per-sprite context menu
    std::vector<MENU> mctx_; /// engine`s main context menu
    std::vector<CTRL> mctl_; /// GUI controls array (main window)
    std::vector<CTRL> octl_; /// GUI controls array (options window)
    std::string cfnm_; /// main configuration file path
    uint64_t tcur_; /// current, dilation-adjusted timestamp
    uint64_t tpre_; /// previous raw timestamp
    float tacc_;  /// partial timestamp accumulator
    T3IV ppos_;   /// mouse pointer position (z = flags)
    T2IV tray_;   /// tray icon dimensions
    T4IV area_;   /// drawing area position and dimensions
    conf_t cdef_; /// default configuration
    conf_t ccur_; /// current configuration
    conf_t cini_; /// initial configuration read at the start
    ENGD *engd_;

    std::unordered_map<library_t::lib_id_t, std::unique_ptr<library_t>> libs_;
    std::vector<std::vector<library_t::lib_id_t>> categories_;

    T2IV get_min_preview_size() {
        auto retn = RUN_FE2C(MCT_SPEC, MSG__GSZ, 0);
        return {{uint16_t(retn), uint16_t(retn >> 16)}};
    }

    T2IV get_avg_font_size() {
        AINF atmp{0, 0, 0, 0, (uint32_t*)"    " /* 4 spaces */};
        auto retn = RUN_FE2C(MCT_CAPT, MSG_WTGD, (intptr_t)&atmp);
        return {{int32_t(0.25f * uint16_t(retn)), uint16_t(retn >> 16)}};
    }

public:
    engine_t(const std::string_view fcnf, const std::string_view base,
            const T2IV tray, const T4IV area);

    void build_library_structure(const std::string &path,
            const std::vector<library_t::input_t> &ins);
};

intptr_t FC2EO(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
/*
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
//*/
    return 0;
}

intptr_t FC2EM(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    INCBIN("../exec/icon.gif", MainIcon);

    switch (ctrl->uuid) {
        case TXT_HEAD: {
/*
            auto engc = (engine_t*)ctrl->data;
            intptr_t retn;
            if (cmsg == MSG_SGIP)
                DisplayPreviews(engc, data, ctrl->fe2c(ctrl, MSG_SGTH, 0),
                               (uint32_t)ctrl->fe2c(ctrl, MSG__GSZ, 0) >> 16);
            else if ((cmsg == MSG_SSID)
                 && ((retn = RearrangePreviews(engc, 8, (uint16_t)data,
                                              (uint16_t)(data >> 16))) >= 0))
                return retn;
//*/
            break;
        }
        case TXT_OGRP: {
/*
            auto engc = (engine_t*)ctrl->data;
            if (cmsg == MSG_LGST) {
                cmsg = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                return (engc->ctgs._[data].flgs & cmsg)? 1 : 0;
            } else if (cmsg == MSG_LSST) {
                intptr_t prev;
                cmsg = RUN_FE2C(engc->MCT_EXAC, MSG_BGST, 0);
                cmsg = (cmsg & FCS_MARK)? 2 : 1;
                prev = (engc->ctgs._[data >> 1].flgs & cmsg)? 1 : 0;
                engc->ctgs._[data >> 1].flgs &= ~cmsg;
                engc->ctgs._[data >> 1].flgs |= (data & 1)? cmsg : 0;
                CategorizePreviews(engc);
                return prev;
            }
//*/
            break;
        }
        case TXT_BADD:
/*
            if (cmsg == MSG_BCLK) {
                auto engc = (engine_t*)ctrl->data;
                long spin;
                data = RUN_FE2C(engc->MCT_SPEC, MSG_NGET, 0);
                for (cmsg = 0; cmsg < engc->libs.size; cmsg++)
                    if (engc->libs._[cmsg].wctx.icnt >= 0) {
                        spin = engc->libs._[cmsg].wctx.icnt;
                        spin = (spin + data > 0)? spin + data : 0;
                        if (engc->libs._[cmsg].spin.fe2c)
                            RUN_FE2C(engc->libs._[cmsg].spin, MSG_NSET, spin);
                        RUN_FC2E(engc->libs._[cmsg].spin, MSG_NSET, spin);
                    }
            }
//*/
            break;

        case TXT_CAPT:
/*
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
//*/
            break;

        case TXT_FLTR:
/*
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_EXAC, MSG__ENB, data);
                CategorizePreviews((ENGC*)ctrl->data);
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_OGRP, MSG__ENB, data);
            }
//*/
            break;

        case TXT_EXAC:
/*
            if (cmsg == MSG_BCLK) {
                ENGC *engc = (ENGC*)ctrl->data;

                CategorizePreviews(engc);
                RUN_FE2C(engc->MCT_OGRP, MSG__TXT,
                        (intptr_t)engc->tran[(data)? TXT_AGRP : TXT_OGRP]);
            }
//*/
            break;

        case TXT_SRND:
/*
            if (cmsg == MSG_BCLK) {
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_RGPU, MSG__ENB, data);
                RUN_FE2C(((ENGC*)ctrl->data)->MCT_BDUP, MSG__ENB, data);
            }
//*/
            break;

        case TXT_BDUP:
            /// nothing goes here
            break;

        case TXT_OPTS:
/*
            if (cmsg == MSG_BCLK)
                RUN_FE2C(((ENGC*)ctrl->data)->OCT_OPTS, MSG__SHW, 1);
//*/
            break;

        case TXT_GOGO: {
/*
            if (cmsg != MSG_BCLK)
                break;

            LINF *libs;
            ENGC *engc = ((ENGC*)ctrl->data);
            AINF igif = {};
            intptr_t icon;
            long ilen, *irnd, *iput;

            irnd = calloc(engc->libs.size, sizeof(*irnd));
            iput = calloc(engc->libs.size, sizeof(*iput));

            /// checking if random choice is enabled
            if ((cmsg = RUN_FE2C(engc->MCT_BDUP, MSG_BGST, 0)) & FCS_ENBL) {
                /// indexing random-capable libraries
                for (ilen = icon = 0; icon < engc->libs.size; icon++)
                    if (engc->libs._[icon].wctx.icnt == 0)
                        iput[ilen++] = icon;
                /// iterating over the requested random sprites count
                for (icon = RUN_FE2C(engc->MCT_RGPU, MSG_NGET, 0);
                    (icon > 0) && ilen; icon--) {
                    irnd[iput[data = RNG_Load(engc->seed) % ilen]]++;
                    if ((~cmsg & FCS_MARK) && (data < --ilen))
                        iput[data] = iput[ilen];
                }
                /// finally, adding the computed random values to ICNTs
                for (icon = 0; icon < engc->libs.size; icon++)
                    engc->libs._[icon].wctx.icnt += irnd[icon];
            }
            /// is there anything selected? let`s find out
            for (icon = 0; icon < engc->libs.size; icon++)
                if (engc->libs._[icon].wctx.icnt > 0)
                    break;
            if (icon >= engc->libs.size) {
                /// [TODO:] do we need to show messages here?
//                rMessage("Nothing selected!", 0, 0);
                free(irnd);
                free(iput);
                break;
            }
            /// counting the number of selected libraries
            for (cmsg = icon = 0; icon < engc->libs.size; icon++)
                if (engc->libs._[icon].wctx.icnt > 0)
                    cmsg++;
            SetProgress(engc, TXT_LOAD, 0, cmsg);

            cEngineCallback(engc->engd, ECB_LOAD, ~0);
            for (data = icon = 0; icon < engc->libs.size; icon++)
                if (engc->libs._[icon].wctx.icnt > 0) {
                    LoadLib(&engc->libs._[icon], engc->engd);
                    SetProgress(engc, TXT_LOAD, ++data, cmsg);
                    RUN_FE2C(engc->MCT_SELE, MSG_PPOS, data);
                }
            cEngineLoadAnimAsync(engc->engd, &igif, (uint8_t*)"/Icon/",
                                 MainIcon, ELA_LOAD, 0);
            cEngineCallback(engc->engd, ECB_LOAD, 0);

            /// [TODO:] adapt for CTR_V_FLTR
            for (libs = engc->libs._, icon = 0; icon < engc->libs.size; icon++)
                if (AppendSpriteArr(&engc->libs._[icon], engc)) {
                    /// revert random ICNT
                    engc->libs._[icon].wctx.icnt -= irnd[icon];
                    if (++libs <= &engc->libs._[icon])
                        CTR_ASSIGN(libs[-1], engc->libs._[icon]);
                }
            free(irnd);
            free(iput);
            CTR_V_MGET(engc->libs, libs - engc->libs._, 1);
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
//*/
            break;
        }
    }
    return 0;
}

void FreeWindow(std::vector<CTRL> &window) {
    long indx = 0;
    while (window[indx].xdim | window[indx].ydim)
        rFreeControl(&window[indx++]);
    window = {};
}

void MakeWindow(std::vector<CTRL> &window) {
    assert(!window.empty() && ((window[0].flgs & FCT_TTTT) == FCT_WNDW));
    rMakeControl(&window[0], nullptr, nullptr); // creating the main window

    long xmax = 0, ymax = 0, xoff = 0, yoff = 0;
    for (size_t indx = 1; indx < window.size(); indx++) {
        window[indx].prev = &window[0];
        rMakeControl(&window[indx], &xoff, &yoff);
        xmax = (xmax > xoff)? xmax : xoff;
        ymax = (ymax > yoff)? ymax : yoff;
    }
    /// resizing and showing the window
    RUN_FE2C(window[0], MSG_WSZC, (uint16_t)xmax | ((uint32_t)ymax << 16));
}

void UpdPreview(intptr_t data, uint64_t time) {
    auto engc = (engine_t*)data;
}

engine_t::engine_t(const std::string_view fcnf, const std::string_view base,
        const T2IV tray, const T4IV area) {
    #define CASE(what) {#what, conf_t::what}
    static std::unordered_map<std::string, conf_t::flags_t> rndrflg = {
        CASE(gpu), CASE(opaque), CASE(draw), CASE(show),
        CASE(wpbo), CASE(wbgra), CASE(wregion),
    };
    static std::unordered_map<std::string, conf_t::flags_t> genflg = {
        CASE(topmost), CASE(speech), CASE(cspeech), CASE(hover),
        CASE(effects), CASE(update), CASE(interaction),
    };
    #undef CASE

    int16_t runs = 0;
    conf_t::flags_t render = conf_t::show | conf_t::draw | conf_t::gpu;
    conf_t::flags_t general = conf_t::hover | conf_t::interaction
            | conf_t::effects | conf_t::speech | conf_t::cspeech;

    tray_ = tray;
    area_ = area;
    cdef_.base = cini_.base = base;
    cdef_.flgs = cini_.flgs = general | render;

    cfnm_ = (!fcnf.empty())
            ? concat_path({std::string(fcnf), DEF_CORE})
            : "";
    if (auto file = (!cfnm_.empty())
            ? rLoadFile(cfnm_.c_str(), nullptr)
            : nullptr) {
        for (token_t text({}, file); !is_empty(text);
                text = next_token(text.second, 0, DEF_CRLF, 0)) {
            if (!text.first.empty() && (text.first.back() == DEF_LFCR))
                text.first.remove_suffix(sizeof(DEF_LFCR));
            auto line = next_token(text.first);
            switch (str_hash(line.first)) {
                default: break;
                case str_hash("Language"):
                    break;
                case str_hash("Content"):
                    line = next_token(line.second, 0);
                    if (line.first.empty()) {
                        cini_.base = cdef_.base;
                        break;
                    }
                    cini_.base = concat_path(
                            {std::string(line.first), "Content", "Ponies"});
                    break;
                case str_hash("RunsTillUpdate"):
                    cini_.nrun.set(process_float(line, cini_.nrun.get()));
                    runs = process_float(line, runs);
                    break;
                case str_hash("BaseScale"):
                    cini_.nsca.set(process_float(line, cini_.nsca.get()));
                    break;
                case str_hash("TimeDilation"):
                    cini_.ndil.set(process_float(line, cini_.ndil.get()));
                    break;
                case str_hash("RandomSpeech"):
                    cini_.nsay.set(process_float(line, cini_.nsay.get()));
                    break;
                case str_hash("CursorDodge"):
                    cini_.ncdr.set(process_float(line, cini_.ncdr.get()));
                    break;
                case str_hash("Render"):
                    render = conf_t::none;
                    while (!is_empty(line))
                        render |= process_map(line, rndrflg, conf_t::none);
                    break;
                case str_hash("Flags"):
                    general = conf_t::none;
                    while (!is_empty(line))
                        general |= process_map(line, genflg, conf_t::none);
                    break;
            }
        }
        free(file);

        ccur_ = cini_;
        ccur_.lang = "";

        cini_.flgs = general | render;
        if (!cini_.nrun.get())
            runs = 0;
        else if (runs >= cini_.nrun.get()) {
            cini_.flgs |= conf_t::update;
            runs = 0;
        }
    }

    /// primary initialization complete, now creating GUI
    ///  0. [ FIRST AND FOREMOST! ] do not forget to edit the appropriate
    ///     *CT_ constants after swapping or adding controls
    ///  1. main window`s "dimensions" are just spaces to leave between
    ///     window edges and actual controls
    const auto here = intptr_t(this);
    mctl_ = {
        {nullptr, here, TXT_CAPT, FSW_SIZE | FCT_WNDW,  1,  1,  1,  1, FC2EM},
        {nullptr, here, TXT_FLTR,            FCT_CBOX,  0,  0, 19,  2, FC2EM},
        {nullptr, here, TXT_EXAC, FCP_VERT | FCT_CBOX,  0,  0, 19,  2, FC2EM},
        {nullptr, here, TXT_OGRP, FCP_VERT | FCT_LIST,  0,  0, 19, 16, FC2EM},
        {nullptr, here, TXT_SGRP, FCP_VERT | FCT_TEXT,  0,  1, 19,  2, FC2EM},
        {nullptr, intptr_t(&ccur_.spec),
                        TXT_SPEC, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2EM},
        {nullptr, here, TXT_BADD, FCP_BOTH | FCT_BUTN,  1, -3,  9,  3, FC2EM},
        {nullptr, here, TXT_SRND, FCP_VERT | FCT_CBOX
                                           | FSX_LEFT,  0,  1, 19,  2, FC2EM},
        {nullptr, intptr_t(&ccur_.rgpu),
                        TXT_RGPU, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2EM},
        {nullptr, here, TXT_BDUP, FCP_BOTH | FCT_CBOX,  1, -3,  9,  3, FC2EM},
        {nullptr, here, TXT_SELE, FCP_VERT | FCT_PBAR,  0,  1, 19,  3, FC2EM},
        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_BUTN,  0,  1,  9,  6, FC2EM},
        {nullptr, here, TXT_GOGO, FCP_BOTH | FCT_BUTN
                                           | FSB_DFLT,  1, -6,  9,  6, FC2EM},
        {nullptr, here, TXT_HEAD, FCP_HORZ | FCT_SBOX,  0,  0, 41, 43, FC2EM},
    };
    octl_ = {
        {nullptr, here, TXT_OPTS,            FCT_WNDW,  1,  1,  1,  1, FC2EO},

        {nullptr, here, TXT_UONR,            FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_ETOP, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_EEFF, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_EINT, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_ESAY, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_ECLR, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},
        {nullptr, here, TXT_ERCH, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2EO},

        {nullptr, intptr_t(&ccur_.nrun),
                        TXT_RUNS,            FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {nullptr, here, TXT_RUNS, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {nullptr, intptr_t(&ccur_.nsca),
                        TXT_SCAL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {nullptr, here, TXT_SCAL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {nullptr, intptr_t(&ccur_.ndil),
                        TXT_TDIL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {nullptr, here, TXT_TDIL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {nullptr, intptr_t(&ccur_.nsay),
                        TXT_RSAY, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {nullptr, here, TXT_RSAY, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},
        {nullptr, intptr_t(&ccur_.ncdr),
                        TXT_PCDR, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2EO},
        {nullptr, here, TXT_PCDR, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2EO},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {nullptr, here, TXT_LGUI, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2EO},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2EO},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {nullptr, here, TXT_BDIR, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2EO},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2EO},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2EO},

        {nullptr, here, TXT_RELO, FCP_VERT | FCT_BUTN, 29,  0, 10,  3, FC2EO},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2EO},
    };
    MakeWindow(mctl_);
    MakeWindow(octl_);
    RUN_FC2E(OCT_LREL, MSG_BCLK, 0); /// relocalize!
    //UpdateOptionControls(&engc, 1);
    RUN_FE2C(MCT_CAPT, MSG__SHW, 1);

    const auto min_preview_width = get_min_preview_size().x;
    const auto avg_font_width = get_avg_font_size().x;
    /// showing the scroll window
    //MCT_CHAR.fc2e = FC2EM; // why do we need this? that was the init value!
    RUN_FE2C(MCT_CHAR, MSG__SHW, 1);

    std::vector<library_t::input_t> ins;
    auto find = rFindMake(cini_.base.c_str());
    while (auto file = rFindFile(find)) {
        ins.emplace_back(library_t::input_t(cini_.base, file));
        free(file);
    }
    build_library_structure(cini_.base, ins);

    /// initialize the rendering engine
    cEngineCallback(0, ECB_INIT, (intptr_t)&engd_);

    rInternalMainLoop(&mctl_[0], FRM_WAIT, UpdPreview, intptr_t(this));

}

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

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    T4IV area{{int32_t(xpos), int32_t(ypos),
               int32_t(xdim - xpos), int32_t(ydim - ypos)}};
    engine_t engc(fcnf, base, T2IV{{int32_t(xico), int32_t(yico)}}, area);



}
