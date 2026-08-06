#include <algorithm>
#include <cassert>
#include <charconv>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "exec.h"
#include "zip/zip_load.h"



// TODO: implement a test system? e.g. instead of the screen the characters
//       would print their actions to STDOUT, and if the same PRNG, config,
//       and resolution are set, then everything is deterministic and these
//       output logs can be validated by comparing them to a reference log.



// TODO: content downloading
// TODO: config saving
// TODO: speech bubbles
// TODO: the go button



// FE2C / FC2E helper macros
#define RUN_FE2C(trgt, cmsg, data) (trgt).fe2c(&(trgt), (cmsg), (data))
#define RUN_FC2E(trgt, cmsg, data) (trgt).fc2e(&(trgt), (cmsg), (data))

#define assert_and_discard(a, d) assert(a), ((void)(d))

/* convert degrees to radians  */
#define DTR_CONV (M_PI / 180.0)
/* convert radians to degrees  */
#define RTD_CONV (1.0 / DTR_CONV)

// default comment character
#define DEF_CMNT '\''
// default end-of-line character
#define DEF_CRLF '\n'
// non-default end-of-line character
#define DEF_LFCR '\r'
// default quotation character - inhibits tokenization
#define DEF_QUOT '"'
// default token separator
#define DEF_TSEP ','
// default dir slash (string)
#define DEF_DSEP "/"

enum {
/*  framerate limiter in msec    */ FRM_WAIT = 40,
};
enum {
/// /// /// /// /// /// /// /// /// localized text constants
/*  Remove character             */ TXT_CDEL = 0,
/*  Remove all similar           */ TXT_ADEL,
/*  Sleep / wake up              */ TXT_CSLP,
/*  Sleep / wake up all similar  */ TXT_ASLP,
/*  Take control: Player 1       */ TXT_TPL1,
/*  Take control: Player 2       */ TXT_TPL2,
/*  More options...              */ TXT_OPTS,

/*  [ Desktop Ponies Engine ]    */ TXT_HEAD,
/*  OS specific options          */ TXT_SPEC,
/*  Disable transparency         */ TXT_OPAQ,
/*  Play animation               */ TXT_DRAW,
/*  Show window                  */ TXT_SHOW,
/*  Exit                         */ TXT_EXIT,
/*  Use GPU for drawing          */ TXT_RGPU,
/*  [ none ]                     */ TXT_NONE,
/*  [ default ]                  */ TXT_DFLT,

/*  Show console                 */ TXT_CONS,
/*  Use regions                  */ TXT_IRGN,
/*  Enable BGRA                  */ TXT_IBGR,
/*  Enable pixel buffers         */ TXT_IPBO,
/*  Useless on full opacity!     */ TXT_UOFO,
/*  Useless without GPU!         */ TXT_UWGL,
/*  Cannot initialize GPU!       */ TXT_CIGL,
/*  The animation base <...>     */ TXT_CTUP,
/*  Internet connection failure  */ TXT_INET,
/*  Failed to create directory   */ TXT_FDIR,
/*  Update                       */ TXT_CCUP,

/*  Desktop Ponies               */ TXT_CAPT,
/*  Enable filters               */ TXT_FLTR,
/*  Exact matching               */ TXT_EXAC,
/*  [At least one:]              */ TXT_OGRP,
/*  [All at once:]               */ TXT_AGRP,
/*  Random selection:            */ TXT_SRND,
/*  Group selection:             */ TXT_SGRP,
/*  Add                          */ TXT_BADD,
/*  Copies                       */ TXT_BDUP,
/*  Selected:                    */ TXT_SELE,
/*  Loaded:                      */ TXT_LOAD,
/*  Updated:                     */ TXT_UPTO,
/*  GO!                          */ TXT_GOGO,

/*  Update on next run           */ TXT_UONR,
/*  Always on top                */ TXT_ETOP,
/*  Enable effects               */ TXT_EEFF,
/*  Enable interactions          */ TXT_EINT,
/*  Enable speech                */ TXT_ESAY,
/*  Enable colored speech        */ TXT_ECLR,
/*  React to cursor hover        */ TXT_ERCH,

/*   runs between updates        */ TXT_RUNS,
/*   % base scaling factor       */ TXT_SCAL,
/*   % time dilation factor      */ TXT_TDIL,
/*   % random speech chance      */ TXT_RSAY,
/*   pix. cursor dodge radius    */ TXT_PCDR,

/*  Choose...                    */ TXT_CHOO,
/*  Reload                       */ TXT_RELO,
/*  Reset                        */ TXT_RESE,
/*  GUI language: English        */ TXT_LGUI,
/*  Animation base directory:    */ TXT_BDIR,
/*  Moving the animation base    */ TXT_BMOV,
/*  Confirm saving the <...>     */ TXT_BSAV,
/*  On refusal, the source <...> */ TXT_BDEL,
/*  Failed to move the <...>     */ TXT_BERR,
/*  OK                           */ TXT_BYES,
/*  Cancel                       */ TXT_BNAY,
};

/*
// GEN_FLAGS(): 64-bit flag enum generation machinery
#define GEN_FLAGS(...) \
GEN_FLAGS4(00 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,##__VA_ARGS__) \
GEN_FLAGS4(16                 ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,##__VA_ARGS__) \
GEN_FLAGS4(32                                 ,,,,,,,,,,,,,,,,,##__VA_ARGS__) \
GEN_FLAGS4(48                                                 ,##__VA_ARGS__)
#define GEN_FLAGS4(h, ...) \
GEN_FLAGS3(h+ 0,,,,,,,,,,,,,__VA_ARGS__) GEN_FLAGS3(h+ 4,,,,,,,,,__VA_ARGS__) \
GEN_FLAGS3(h+ 8        ,,,,,__VA_ARGS__) GEN_FLAGS3(h+12        ,__VA_ARGS__)
#define GEN_FLAGS3(h, ...) \
GEN_FLAGS2(h+ 0         ,,,,__VA_ARGS__) GEN_FLAGS2(h+ 1      ,,,__VA_ARGS__) \
GEN_FLAGS2(h+ 2           ,,__VA_ARGS__) GEN_FLAGS2(h+ 3        ,__VA_ARGS__)
#define GEN_FLAGS2(...) GEN_FLAGS10(__VA_ARGS__, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, \
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, \
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, )
#define GEN_FLAGS10(    ___,s1,s2,s3,s4,s5,s6,s7, t0,t1,t2,t3,t4,t5,t6,t7, \
u0,u1,u2,u3,u4,u5,u6,u7, v0,v1,v2,v3,v4,v5,v6,v7, w0,w1,w2,w3,w4,w5,w6,w7, \
x0,x1,x2,x3,x4,x5,x6,x7, y0,y1,y2,y3,y4,y5,y6,y7, z0,z1,z2,z3,z4,z5,z6,z7, \
__,a1,a2,a3,a4,a5,a6,a7, b0,b1,b2,b3,b4,b5,b6,b7, c0,c1,c2,c3,c4,c5,c6,c7, \
d0,d1,d2,d3,d4,d5,d6,d7, e0,e1,e2,e3,e4,e5,e6,e7, f0,f1,f2,f3,f4,f5,f6,f7, \
g0,g1,g2,g3,g4,g5,g6,g7, h0,h1,h2,h3,h4,h5,h6,h7, _, ...) GEN_FLAGS##_(__, ___)
#define GEN_FLAGS1(arg, val) arg = 1uLL << (val),
#define GEN_FLAGS0(arg, val)
/*/
// GEN_FLAGS(): 32-bit flag enum generation machinery
#define GEN_FLAGS(...)           GEN_FLAGS4(00,,,,,,,,,,,,,,,,,##__VA_ARGS__) \
                                 GEN_FLAGS4(16                ,##__VA_ARGS__)
#define GEN_FLAGS4(h, ...) \
GEN_FLAGS3(h+ 0,,,,,,,,,,,,,__VA_ARGS__) GEN_FLAGS3(h+ 4,,,,,,,,,__VA_ARGS__) \
GEN_FLAGS3(h+ 8        ,,,,,__VA_ARGS__) GEN_FLAGS3(h+12        ,__VA_ARGS__)
#define GEN_FLAGS3(h, ...) \
GEN_FLAGS2(h+ 0         ,,,,__VA_ARGS__) GEN_FLAGS2(h+ 1      ,,,__VA_ARGS__) \
GEN_FLAGS2(h+ 2           ,,__VA_ARGS__) GEN_FLAGS2(h+ 3        ,__VA_ARGS__)
#define GEN_FLAGS2(...) GEN_FLAGS10(__VA_ARGS__, \
1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, \
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, )
#define GEN_FLAGS10(    ___,s1,s2,s3,s4,s5,s6,s7, t0,t1,t2,t3,t4,t5,t6,t7, \
u0,u1,u2,u3,u4,u5,u6,u7, v0,v1,v2,v3,v4,v5,v6,v7, __,a1,a2,a3,a4,a5,a6,a7, \
b0,b1,b2,b3,b4,b5,b6,b7, c0,c1,c2,c3,c4,c5,c6,c7, d0,d1,d2,d3,d4,d5,d6,d7, \
_, ...) GEN_FLAGS##_(__, ___)
#define GEN_FLAGS1(arg, val) arg = 1u << (val),
#define GEN_FLAGS0(arg, val)
//*/

// flag enum support
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



// Mersenne random number generator

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
                      ^ (retn >> 1) ^ ((retn & 1) ? 0x9908B0DF : 0);
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
        for (uint32_t i = size; over && (i > over); i--)
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
    static const std::unordered_map<std::string, bool>
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

class unit_t;
class library_t;
class speech_t;
class effect_t;
class behaviour_t;
class interaction_t;
class engine_t;

template <typename T>
using ref_vec_t = std::vector<std::reference_wrapper<const T>>;

enum movement_flags_t {
    GEN_FLAGS(move_none, move_drag, move_sleep, move_mouse,
              move_horz, move_vert, move_diag)
    move_hv = move_horz | move_vert, move_dh = move_diag | move_horz,
    move_dv = move_diag | move_vert, move_all= move_diag | move_horz | move_vert
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
    no_copy_t(no_copy_t&) = delete;       // non construction-copyable
    no_copy_t(const no_copy_t&) = delete; // non construction-copyable
    no_copy_t& operator=(no_copy_t&) = delete;       // non copyable
    no_copy_t& operator=(const no_copy_t&) = delete; // non copyable
};

class sprite_bank_t : public no_copy_t {
public:
    class sprite_t {
    private:
        uint32_t index_;
        library_t *library_;
        uint32_t parent_; // unique identifier of the parent sprite (0 if none)

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
    class input_t;

private:
    enum bhv_type_t : uint32_t { nonzero_prob = 0,
        stationary, moving, mouseover, dragged, sleeping,
        max_ = sleeping, };
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
    size_t preview_id_;
    uint32_t speech_fg_;
    uint32_t speech_bg_;

    inline static bhv_id_t init_bhv_id(movement_flags_t move, int16_t group);

    const speech_t *select_speech(uint32_t *seed, uint32_t chance,
            bhv_id_t prev, bhv_id_t curr) const;

public:
    inline const behaviour_t *get(bhv_id_t id) const;

    static bhv_id_desc_t build_bhv_id_desc(const input_t &in);

    library_t(std::string path, const input_t &in,
            const bhv_id_map_t &bhv_id_map);

    const unit_t &get_preview(ENGD *engd = nullptr);
    void extract_speech_colors(ENGD *engd, intptr_t parallel);
    static void extract_speech_colors_worker(intptr_t data, uint64_t unused);

    const std::string &name() const { return readable_name_; }
};

class unit_t : public no_copy_t {
private:
    // !prepare && !upload = AINF is unprepared, no memory allocations
    //  prepare && !upload = AINF is prepared, ready to upload
    // !prepare &&  upload = AINF is being uploaded
    //  prepare &&  upload = AINF is ready
    enum flags_t : uint32_t {
        GEN_FLAGS(prepare, upload, from_path, fake_center, repeat)
    };
    struct {
        flags_t flags_ = {};
        T2IV center_ = {};
        AINF image_ = {};
    } sides_[2];

    void *allocate(bool left, size_t size) {
        auto data = (uint8_t*)realloc(nullptr, size + sizeof(intptr_t));
        *((intptr_t*)data) = (intptr_t)(&sides_[left].flags_);
        sides_[left].image_.time = (uint32_t*)(data + sizeof(intptr_t));
        return (void*)sides_[left].image_.time;
    }
    static void finalize(void *data) {
        if (!data) return;
        data = ((uint8_t*)data) - sizeof(intptr_t);
        *((flags_t*)(*(intptr_t*)data)) |= prepare | upload;
        data = realloc(data, 0);
    }
    void set_flag(bool left, bool value, flags_t flag) {
        auto &side = sides_[left];
        side.flags_ = (value) ? (side.flags_ | flag) : (side.flags_ & ~flag);
    }
    bool maybe_discard(bool left) {
        if (is_being_uploaded(left)) return false;
        if (is_owner_discardable(left)) {
            assert(sides_[left].image_.time);
            finalize(sides_[left].image_.time);
        }
        sides_[left].flags_ &= ~(prepare | upload);
        sides_[left].image_ = {};
        return true;
    }

protected:
    unit_t(): sides_{} {}
    ~unit_t() {
        const auto discard_r = maybe_discard(false);
        assert_and_discard(discard_r, discard_r);
        const auto discard_l = maybe_discard(true);
        assert_and_discard(discard_l, discard_l);
    }

    void set_center(bool left, const T2IV &center) {
        set_flag(left, (center.x != 0) || (center.y != 0), fake_center);
        sides_[left].center_ = center;
    }
    void set_loop(bool left, bool loop) { set_flag(left, loop, repeat); }

    // return true = AINF prepared successfully
    // return false = AINF busy, could not prepare
    bool prepare_source(bool left, const std::string &name) {
        if (!maybe_discard(left)) return false;
        set_flag(left, true, prepare);
        set_flag(left, true, from_path);
        auto data = (char*)allocate(left, name.size() + 1);
        strncpy(data, name.c_str(), name.size() + 1);
        return true;
    }
    bool prepare_source(bool left, const std::string &name, uint32_t xdim,
            uint32_t ydim, const std::function<void(uint32_t*)> &draw) {
        // the layout is as follows:
        //  -ptr: &flags
        //     0: AINF
        // +AINF: buf
        //  +buf: time
        // +time: name
        if (!maybe_discard(left)) return false;
/*
        set_flag(left, true, prepare);
        set_flag(left, false, from_path);
        auto anim = (AINF*)allocate(left, sizeof(AINF) + name.size() + 1
                                        + sizeof(uint32_t) * (xdim * ydim + 1));
        anim->uuid = (intptr_t)(anim + 1);
        anim->time = ((uint32_t*)anim->uuid) + xdim * ydim;
        anim->xdim = xdim;
        anim->ydim = ydim;
        anim->time[0] = 0; // single frame only for this image type
        strncpy((char*)(anim->time + 1), name.c_str(), name.size() + 1);
        draw((uint32_t*)anim->uuid); // drawing something in the buffer
//*/
        return true;
    }

public:
    bool is_owner_discardable(bool left) const {
        const auto flags = sides_[left].flags_;
        return (flags & prepare) && !(flags & upload);
    }
    bool is_owner_discardable() const {
        return is_owner_discardable(false) && is_owner_discardable(true);
    }

    bool is_being_uploaded(bool left) const {
        const auto flags = sides_[left].flags_;
        return !(flags & prepare) && (flags & upload);
    }
    bool is_being_uploaded() const {
        return is_being_uploaded(false) && is_being_uploaded(true);
    }

    bool is_empty(bool left) const {
        const auto flags = sides_[left].flags_;
        return !(flags & prepare) && !(flags & upload);
    }
    bool is_empty() const { return is_empty(false) && is_empty(true); }

    bool is_ready(bool left) const {
        const auto flags = sides_[left].flags_;
        return (flags & prepare) && (flags & upload);
    }
    bool is_ready() const { return is_ready(false) && is_ready(true); }

    bool schedule_upload(bool left, ENGD *engd) {
        if (!is_owner_discardable(left)) return is_ready(left);
        set_flag(left, false, prepare);
        set_flag(left, true, upload);
        auto &side = sides_[left];
        char *data = (char*)side.image_.time;
        side.image_.time = nullptr;
        if (side.flags_ & from_path) {
            std::string_view temp(data);
            auto pos = temp.find_last_of(DEF_DSEP);
            if ((pos != std::string_view::npos) && (pos > 0))
                pos = temp.find_last_of(DEF_DSEP, pos - 1);
            pos = (pos != std::string_view::npos) ? pos + 1 : 0;
            temp.remove_prefix(pos); // animation hash: last dir + gif name
            cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)temp.data(),
                    data, ELA_DISK, finalize);
        } else if (auto anim = (AINF*)data) {
            cEngineLoadAnimAsync(engd, &side.image_, (uint8_t*)(anim->time + 1),
                    anim, ELA_AINF, finalize);
        }
        return true;
    }
    T2IV dims(bool left) const {
        T2IV retn{{(decltype(retn.x))sides_[left].image_.xdim,
                   (decltype(retn.y))sides_[left].image_.ydim}};
        return retn;
    }
    intptr_t advance(
            bool left, int64_t time, int64_t &old, uint32_t &frame) const {
        if (time <= old) return 0;
        auto &side = sides_[left];
        bool wrap = frame >= (side.image_.fcnt - 1);
        if (!wrap || (side.flags_ & repeat)) {
            if (time > (old += side.image_.time[frame]))
                old = time; // > 1 frame skipped, resetting
            frame = (!wrap) ? frame + 1 : 0;
            return side.image_.uuid;
        } else {
            old = std::numeric_limits<int64_t>::max(); // never expires
            wrap = frame >= side.image_.fcnt;
            frame = side.image_.fcnt - 1;
            return (wrap) ? side.image_.uuid : 0;
        }
    }
};

class effect_t : public unit_t {
protected:
    enum gravity_flags_t : uint8_t { top_left = 0, top, top_right,
        center_left, center, center_right, bottom_left, bottom, bottom_right,
        any, not_center, };
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
            static const std::unordered_map<std::string, gravity_flags_t> p = {
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
            placement_right = process_map(line, p, placement_right);
            centering_right = process_map(line, p, centering_right);
            placement_left = process_map(line, p, placement_left);
            centering_left = process_map(line, p, centering_left);
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
            prepare_source(false, concat_path({path, in.right_image}));
            prepare_source(true, concat_path({path, in.left_image}));
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
        prepare_source(false, in.text, 1, 1, render_text);
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
            static const std::unordered_map<std::string, movement_flags_t> m = {
                {"horizontal_vertical", move_hv  }, {"mouseover", move_mouse},
                {"diagonal_horizontal", move_dh  }, {"dragged",   move_drag },
                {"diagonal_vertical",   move_dv  }, {"sleep",     move_sleep},
                {"horizontal_only",     move_horz}, {"none",      move_none },
                {"vertical_only",       move_vert}, {"all",       move_all  },
                {"diagonal_only",       move_diag},
            };
            static const std::unordered_map<std::string, bool> f = {
                {"false", false}, {"true",   true},
                {"fixed", false}, {"mirror", true},
            };
            token_t line({}, str);
            name = ascii_to_lower(process_string(line));
            chance = process_float(line, chance);
            max_duration = process_float(line, max_duration);
            min_duration = process_float(line, min_duration);
            speed = process_float(line, speed);
            right_image = process_string(line);
            left_image = process_string(line);
            movement = process_map(line, m, movement);
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
            mirror_target_xy = process_map(line, f, mirror_target_xy);
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
            prepare_source(false, concat_path({path, in.right_image}));
            prepare_source(true, concat_path({path, in.left_image}));
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
            static const std::unordered_map<std::string, bool> activations = {
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
                printf("[%s] WARNING, no interaction behaviours with '%s' in "
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
            printf("[%s] WARNING, no interaction behaviours in '%s', "
                   "interaction dropped\n",
                    lib_name.c_str(), in.name.c_str());
    }
};

class library_t::input_t {
public:
    static constexpr char* config_name = (char*)DEF_CONF;

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
            file = (typeof(file))realloc(file, 0);
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

library_t::library_t(std::string path, const input_t &in,
        const bhv_id_map_t &bhv_id_map)
: library_path_(std::move(path))
, readable_name_(in.name) {
    preview_id_ = 0;
    speech_fg_ = 0xFF000000;
    speech_bg_ = 0xFFFFFFFF;

    auto hashable_name = ascii_to_lower(in.name);
    auto &bhv_id_desc = *find_in_map(bhv_id_map, hashable_name);

    // processing the interactions
    for (auto &i : in.interactions) {
        auto it = std::make_unique<interaction_t>(i, hashable_name, bhv_id_map);
        if (!it->is_empty())
            interactions_.emplace_back(std::move(it));
    }
    // distributing effects configs by behaviour name
    std::unordered_map<std::string, eff_vec_t> effects;
    for (auto &e : in.effects) {
        if (find_in_map(bhv_id_desc.m, e.bhv)) {
            effects[e.bhv].emplace_back(e);
        } else {
            printf("[%s] WARNING, unused effect '%s'\n",
                   in.name.c_str(), e.name.c_str());
        }
    }
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
        if (!is != !im)
            printf("[%s] WARNING, inconsistent follow behaviours in '%s'\n",
                   in.name.c_str(), b.name.c_str());

        auto ie = find_in_map(effects, b.name);
        behaviours_.emplace_back(std::make_unique<behaviour_t>(b, iid._,
                    linked_iid._, follow_grp_iid._, str_hash(b.follow_target),
                    library_path_, b_spk, e_spk, (ie) ? *ie : e_null));
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

void library_t::extract_speech_colors_worker(intptr_t data, uint64_t unused) {
    struct hsl_t {float h, s, l;};
    auto hsl_to_rgb = [](const hsl_t &hsl) {
        const float coef = (hsl.l > 0.5f) ? (hsl.s + hsl.l - hsl.s * hsl.l)
                                          : (        hsl.l + hsl.s * hsl.l);
        uint8_t r = 255 * hsl.l, g = 255 * hsl.l, b = 255 * hsl.l;
        if (coef > 0.f) {
            const float mean = 2.f * hsl.l - coef;
            const float frac = 6.f * hsl.h - (int)(6.f * hsl.h);
            const float mid1 = mean + frac * (coef - mean);
            const float mid2 = coef - frac * (coef - mean);
            switch ((int)(6.f * hsl.h) % 6) {
                case 0: r = 255 * coef; g = 255 * mid1; b = 255 * mean; break;
                case 1: r = 255 * mid2; g = 255 * coef; b = 255 * mean; break;
                case 2: r = 255 * mean; g = 255 * coef; b = 255 * mid1; break;
                case 3: r = 255 * mean; g = 255 * mid2; b = 255 * coef; break;
                case 4: r = 255 * mid1; g = 255 * mean; b = 255 * coef; break;
                case 5: r = 255 * coef; g = 255 * mean; b = 255 * mid2; break;
            }
        }
        return b | ((uint32_t)g << 8) | ((uint32_t)r << 16) | 0xFF000000;
    };
    auto rgb_to_hsl = [](uint32_t bgra) {
        const float r = (1.f / 255.f) * (uint8_t)(bgra >> 16);
        const float g = (1.f / 255.f) * (uint8_t)(bgra >> 8);
        const float b = (1.f / 255.f) * (uint8_t)(bgra);
        const float min = std::min(std::min(r, g), b);
        const float max = std::max(std::max(r, g), b);
        hsl_t hsl = {};
        if ((hsl.l = (max + min) * 0.5f) <= 0.f) return hsl;
        const float inv_diff = 1.f / (max - min);
        if ((hsl.s = max - min) > 0.f) {
            hsl.s /= (hsl.l > 0.5f) ? (2.f - max - min) : (max + min);
        } else {
            return hsl;
        }
        if (r == max) {
            hsl.h = (g == min) ? (5.f + (max - b) * inv_diff)
                               : (1.f - (max - g) * inv_diff);
        } else if (g == max) {
            hsl.h = (b == min) ? (1.f + (max - r) * inv_diff)
                               : (3.f - (max - b) * inv_diff);
        } else { // (b == _max) {
            hsl.h = (r == min) ? (3.f + (max - g) * inv_diff)
                               : (5.f - (max - r) * inv_diff);
        }
        hsl.h *= 1.f / 6.f;
        return hsl;
    };
    auto get_luminance = [](uint32_t bgra) {
        const float r = (1.f / 255.f) * (uint8_t)(bgra >> 16);
        const float g = (1.f / 255.f) * (uint8_t)(bgra >> 8);
        const float b = (1.f / 255.f) * (uint8_t)(bgra);
        return (2126 * 255) * std::powf((r + 0.055f) * (1.f / 1.055f), 2.4f)
             + (7152 * 255) * std::powf((g + 0.055f) * (1.f / 1.055f), 2.4f)
             + ( 722 * 255) * std::powf((b + 0.055f) * (1.f / 1.055f), 2.4f);
    };
    auto get_contrast = [&](const hsl_t &dk, const hsl_t &bt) {
        return (500 * 255 + get_luminance(hsl_to_rgb(bt)))
             / (500 * 255 + get_luminance(hsl_to_rgb(dk)));
    };
    auto color_text = [](uint32_t fg, uint32_t bg, const std::string &text) {
        std::string retn = "\033[48;2;" + std::to_string(uint8_t(bg >> 16))
                + ";" + std::to_string(uint8_t(bg >> 8))
                + ";" + std::to_string(uint8_t(bg));
        if (fg != bg)
            retn += ";38;2;" + std::to_string(uint8_t(fg >> 16))
                    + ";" + std::to_string(uint8_t(fg >> 8))
                    + ";" + std::to_string(uint8_t(fg));
        return retn + "m" + text + "\033[0m";
    };

    // reading the inputs, freeing temporary data
    auto lib = (library_t *)(((intptr_t *)data)[0]);
    auto engd = (ENGD *)(((intptr_t *)data)[1]);
    data = (typeof(data))realloc((void *)data, 0);

    // allocating the color buffer and drawing frame #0 of the preview to it
    auto &preview = lib->get_preview();
    auto dims = preview.dims(false);
    size_t preview_size = sizeof(uint32_t) * dims.x * dims.y;
    uint32_t frame = 0;
    int64_t time = 0;
    AINF ainf = {preview.advance(false, 1, time, frame), (uint32_t)dims.x,
        (uint32_t)dims.y, 0, (typeof(ainf.time))realloc(nullptr, preview_size)};
    for (size_t i = dims.x * dims.y; i; ainf.time[--i] = 0) {}
    cEngineCallback(engd, ECB_DRAW, (intptr_t)&ainf);

    // building the color histogram
    std::unordered_map<uint32_t, uint32_t> histogram;
    for (size_t i = dims.x * dims.y; i; histogram[ainf.time[--i]]++) {}
    ainf.time = (typeof(ainf.time))realloc(ainf.time, 0);

    // extracting the most prominent colors from the histogram; end = size - 1
    constexpr size_t end = 2;
    size_t bgn = 0;
    struct {uint32_t clr, idx;} top_clrs[end + 1] = {};
    for (auto &h : histogram)
        if ((h.first & 0xFF000000) && (h.second >= top_clrs[end].idx)) {
            size_t i = end;
            for (; i && (h.second >= top_clrs[i - 1].idx); i--) {}
            for (size_t j = end; j > i; j--)
                top_clrs[j] = top_clrs[j - 1];
            top_clrs[i] = {h.first, h.second};
        }

    // saving the color order for debugging
    std::string colors;
    for (size_t i = bgn; i <= end; i++)
        colors += color_text(top_clrs[i].clr, top_clrs[i].clr, "   ");

    // sorting the colors by luminance
    // more sorting networks: bertdobbelaere.github.io/sorting_networks.html
    for (size_t i = bgn; i <= end; i++)
        top_clrs[i].idx = get_luminance(top_clrs[i].clr);
    #define SORT(v, i, j) if (v[i].idx > v[j].idx) std::swap(v[i], v[j])
    static_assert(end == 2);
    SORT(top_clrs, 0, 1);
    SORT(top_clrs, 0, 2);
    SORT(top_clrs, 1, 2);
    #undef SORT

    // emphasizing the contrast artificially in case it's insufficient
    for (; (bgn < end) && !(top_clrs[bgn].clr & 0xFF000000); bgn++) {}
    auto dk = rgb_to_hsl(top_clrs[bgn].clr); // dark
    auto bt = rgb_to_hsl(top_clrs[end].clr); // bright
    float contrast = -get_contrast(dk, bt);
    constexpr float readable_contrast = 5.f; // W3C WCAG recommends >4.5
    constexpr auto bin_iter = 16; // number of binary search iterations
    if (-contrast < readable_contrast) {
        constexpr float min_l = 0.f, max_l = 1.f;
        const float dk_l = dk.l, bt_l = bt.l, delta_l = bt_l - dk_l;
        const float mid_part = ((max_l - bt_l) < (dk_l - min_l))
                ?       1.f / (1.f + (max_l - bt_l) / (dk_l - min_l))
                : 1.f - 1.f / (1.f + (dk_l - min_l) / (max_l - bt_l));
        for (float min = min_l, max = max_l;
                max - min > (max_l - min_l) / (1 << bin_iter); ) {
            const float mid = 0.5f * (min + max) - delta_l;
            dk.l = std::clamp(dk_l - mid * mid_part, min_l, max_l);
            bt.l = std::clamp(bt_l - mid * mid_part + mid, min_l, max_l);
            contrast = get_contrast(dk, bt);
            ((contrast < readable_contrast) ? min : max) = mid + delta_l;
        }
    }

    // saving the results
    lib->speech_fg_ = hsl_to_rgb(dk);
    lib->speech_bg_ = hsl_to_rgb(bt);

    // printing the debug output
    std::string text(40, '#');
    text.replace(0, lib->readable_name_.size() + 1, lib->readable_name_ + " ");
    char temp[32];
    sprintf(temp, "%06X %06X - ", lib->speech_fg_ & 0xFFFFFF,
            lib->speech_bg_ & 0xFFFFFF);
    text = color_text(lib->speech_fg_, lib->speech_bg_, temp + text + " ");
    sprintf(temp, "%5.2f ", std::fabs(contrast));
    text = color_text((contrast < 0.f) ? lib->speech_fg_ : lib->speech_bg_,
                (contrast < 0.f) ? lib->speech_bg_ : lib->speech_fg_, temp)
         + text + colors;
    printf("%s\n", text.c_str());
}

void library_t::extract_speech_colors(ENGD *engd, intptr_t parallel) {
    if (speeches_.empty()) return;
    auto data = (intptr_t *)realloc(nullptr, sizeof(intptr_t) * 2);
    data[0] = intptr_t(this);
    data[1] = intptr_t(engd);
    rLoadParallel(parallel, intptr_t(data));
}

const unit_t &library_t::get_preview(ENGD *engd) {
    auto &preview = *behaviours_[preview_id_];
    if (engd) preview.schedule_upload(false, engd);
    return preview;
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

// client configuration
class conf_t {
public:
    using lang_map_t = std::unordered_map<int32_t, std::string>;
    enum flags_t : uint32_t {
        GEN_FLAGS(draw, show, gpu, opaque, wbgra, wpbo, wregion, update,
                  topmost, effects, interaction, speech, cspeech, hover,
                  filters, exact, randomsel, copies)
    };
    static constexpr flags_t render = conf_t::show | conf_t::draw | conf_t::gpu;
    static constexpr flags_t general = conf_t::hover | conf_t::interaction
            | conf_t::effects | conf_t::speech | conf_t::cspeech;
    class spin_t {
    private:
        int16_t curr_, min_, max_;
    public:
        spin_t() : curr_{}, min_{}, max_{} {};
        spin_t(int16_t curr, int16_t min, int16_t max)
            : curr_{std::clamp(curr, min, max)}, min_{min}, max_{max} {}
        void set_min(int16_t m) { min_ = m; move(0); }
        void set_max(int16_t m) { max_ = m; move(0); }
        int16_t min() const { return min_; }
        int16_t max() const { return max_; }
        int16_t set(int16_t dist) {
            return curr_ = std::clamp(dist, min_, max_);
        }
        int16_t get() const { return curr_; }
        int16_t move(int16_t dist) { return set(get() + dist); }
    };
    class categories_t {
    private:
        using type_t = uint64_t;
        std::vector<type_t> data_;

        static inline std::pair<size_t, size_t> convert_ctg_id(size_t ctg_id) {
            constexpr auto digits = std::numeric_limits<type_t>::digits;
            return {ctg_id / digits, ctg_id % digits};
        }

    public:
        void add(size_t ctg_id) {
            const auto idx = convert_ctg_id(ctg_id);
            for (size_t i = data_.size(); i <= idx.first; i++)
                data_.emplace_back(0);
            data_[idx.first] |= (type_t(1) << idx.second);
        }
        void remove(size_t ctg_id) {
            const auto idx = convert_ctg_id(ctg_id);
            if (idx.first < data_.size())
                data_[idx.first] &= ~(type_t(1) << idx.second);
        }
        bool match(const categories_t &rhs, bool all_at_once) const {
            bool retn = all_at_once;
            if (all_at_once) {
                size_t size = std::max(data_.size(), rhs.data_.size());
                for (size_t i = 0; retn && (i < size); i++) {
                    auto a = (i < data_.size()) ? data_[i] : type_t(0);
                    auto b = (i < rhs.data_.size()) ? rhs.data_[i] : type_t(0);
                    retn &= (a & b) == b;
                }
            } else {
                size_t size = std::min(data_.size(), rhs.data_.size());
                for (size_t i = 0; !retn && (i < size); i++)
                    retn |= data_[i] & rhs.data_[i];
            }
            return retn;
        }
        categories_t() = default;
        categories_t(size_t ctg_id) { add(ctg_id); }
    };
    std::string base;    // path to the animation base
    std::string lang;    // name of the language file
    lang_map_t lang_map; // localization taken from the language file
    spin_t nrun = spin_t(  5,    0,  1000); // runs between updates
    spin_t nsca = spin_t(100,   25,   300); // base scaling factor
    spin_t ndil = spin_t(100,   10,  1000); // time dilation factor
    spin_t nsay = spin_t( 50,    0,   100); // random speech chance
    spin_t ncdr = spin_t(  0,    0,  1000); // cursor dodge radius
    spin_t spec = spin_t(  0, -100,   100); // group selection
    spin_t rgpu = spin_t(  0,    0, 30000); // random selection
    flags_t flgs = {};
    categories_t ctg_nonex = {};
    categories_t ctg_exact = {};

    static lang_map_t get_lang_map(const std::string_view &file) {
        lang_map_t retn;
        int32_t idx = -1; // first iteration spent on filling token_t
        for (token_t text({}, file); !is_empty(text);
                text = next_token(text.second, 0, DEF_CRLF, 0), idx++)
            if (!text.first.empty()) {
                if (text.first.back() == DEF_LFCR)
                    text.first.remove_suffix(sizeof(DEF_LFCR));
                retn[idx] = text.first;
            }
        return retn;
    }
};

class window_t {
private:
    bool visible_;
    std::vector<CTRL> controls_;

public:
    intptr_t get_id() { return intptr_t(&get(0)); }
    ~window_t() { for (auto &c : controls_) rFreeControl(&c); }

    void toggle_visibility(bool visible) {
        visible_ = visible;
        RUN_FE2C(get_root(), MSG__SHW, visible);
    }
    bool is_visible() const { return visible_; }

protected:
    size_t size() const { return controls_.size(); }
    CTRL &get(int32_t ctl) {
        assert((ctl >= 0) && (size_t(ctl) < controls_.size()));
        return controls_[ctl];
    }
    CTRL &get_root() { return get(0); }
    static int32_t get_type(const CTRL &ctrl) { return ctrl.flgs & FCT_TTTT; }

    window_t(std::vector<CTRL> controls) : visible_(false) {
        controls_ = std::move(controls);
        assert(!controls_.empty() && (get_type(controls_[0]) == FCT_WNDW));
        // creating the main window
        rMakeControl(&controls_[0], nullptr, nullptr);

        long xmax = 0, ymax = 0, xoff = 0, yoff = 0;
        for (size_t indx = 1; indx < controls_.size(); indx++) {
            controls_[indx].prev = &controls_[0];
            rMakeControl(&controls_[indx], &xoff, &yoff);
            xmax = (xmax > xoff) ? xmax : xoff;
            ymax = (ymax > yoff) ? ymax : yoff;
        }
        // resizing and showing the window
        RUN_FE2C(controls_[0], MSG_WSZC,
                (uint16_t)xmax | ((uint32_t)ymax << 16));
    }

    static CTRL *get_parent(CTRL &child) { return child.prev; }

    static CTRL &get_root(CTRL &child) {
        auto root = &child;
        for (auto curr = get_parent(*root);
                curr && (get_type(*root) != FCT_WNDW); curr = get_parent(*root))
            root = curr;
        return *root;
    }

    void set_control_text(const std::string &str, size_t ctl) {
        assert(ctl < size());
        const auto type = get_type(controls_[ctl]);
        if ((type == FCT_WNDW) || (type == FCT_LIST) || (type == FCT_PBAR)
        ||  (type == FCT_BUTN) || (type == FCT_CBOX)
        || ((type == FCT_TEXT) && !(controls_[ctl].flgs & FST_SUNK)))
            RUN_FE2C(controls_[ctl], MSG__TXT, intptr_t(str.c_str()));
    }
};

class conf_window_t : public window_t {
protected:
    const conf_t &def_conf_;
    const conf_t &ini_conf_;
    conf_t &conf_;

    static bool try_update_checkbox(CTRL &c, int &flag) {
        if (get_type(c) != FCT_CBOX) return false;
        auto &flags = ((conf_window_t*)get_root(c).data)->conf_.flgs;
        switch (flag) {
            default:
                if (!c.fe2c) return false;
                flag = !!(flags & conf_t::flags_t(c.data));
                RUN_FE2C(c, MSG_BCLK, flag);
                return true;
            case 0:
                flags &= ~conf_t::flags_t(c.data);
                return true;
            case 1:
                flags |= conf_t::flags_t(c.data);
                return true;
        }
    }

    static bool try_update_spinner(CTRL &c, int16_t val = 0, bool init = true) {
        if (get_type(c) != FCT_SPIN) return false;
        auto spin = (conf_t::spin_t*)c.data;
        if (!init) {
            val = spin->set(val);
        } else if (c.fe2c) {
            bool set_dims = (val == 0);
            val = spin->get();
            if (set_dims)
                RUN_FE2C(c, MSG_NDIM,
                        ((uint32_t)spin->max() << 16) | (uint16_t)spin->min());
            RUN_FE2C(c, MSG_NSET, val);
        } else {
            return false;
        }
        return true;
    }

    static int16_t update_spinner(CTRL &c, int16_t val) {
        if (get_type(c) != FCT_SPIN) return 0;
        auto spin = (conf_t::spin_t*)c.data;
        auto old_empty = !spin->get();
        spin->set(val);
        return old_empty - !spin->get();
    }

    const std::string *get_text_stock(int32_t idx) const {
        if (auto in = find_in_map(conf_.lang_map, idx)) {
            return in;
        } else if (auto id = find_in_map(def_conf_.lang_map, idx)) {
            return id;
        }
        return nullptr;
    }

    void set_control_text_stock(int32_t idx, size_t ctl) {
        set_control_text(*get_text_stock(idx), ctl);
    }

    void relocalize() {
        for (size_t i = 0; i < size(); i++)
            set_control_text_stock(get(i).uuid, i);
        if (auto prev = get_parent(get_root())) // propagate to parent windows
            RUN_FC2E(*prev, MSG__TXT, 0);
    }

    conf_window_t(std::vector<CTRL> controls, conf_t &conf,
            const conf_t &ini_conf, const conf_t &def_conf)
    : window_t(std::move(controls))
    , def_conf_(def_conf)
    , ini_conf_(ini_conf)
    , conf_(conf) {}
};

class main_window_t : public conf_window_t {
private:
    enum elements_t { MCT_CAPT = 0,
        MCT_FLTR, MCT_EXAC, MCT_OGRP, MCT_SGRP, MCT_SPEC, MCT_BADD, MCT_SRND,
        MCT_RGPU, MCT_BDUP, MCT_SELE, MCT_OPTS, MCT_GOGO, MCT_CHAR, };

    class preview_t;
    conf_t::spin_t preview_stats_; // min = 0, curr = selected, max = total
    std::vector<std::unique_ptr<preview_t>> previews_;

    std::vector<CTRL> get_template(intptr_t here, const conf_t &conf);
    static intptr_t FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data);
    static intptr_t FC2EI(CTRL *ctrl, uint32_t cmsg, intptr_t data);
    static void try_update_checkbox(CTRL &c, int flag = -1);
    void display_previews(int32_t y_scroll);
    size_t rearrange_previews(T2IV preview_area);

    T2IV get_scrollbox_metrics() { // u = total height, v = window height
        auto &scrollbox = get(MCT_CHAR);
        return {{(decltype(T2IV::x))(RUN_FE2C(scrollbox, MSG_SGTH, 0)),
                 (decltype(T2IV::y))(RUN_FE2C(scrollbox, MSG__GSZ, 0) >> 16)}};
    }

    void relocalize() {
        conf_window_t::relocalize();
        for (size_t i = 0; i < size(); i++) {
            try_update_checkbox(get(i));
            try_update_spinner(get(i));
        }
        auto text = (conf_.flgs & conf_t::exact) ? TXT_AGRP : TXT_OGRP;
        set_control_text_stock(text, MCT_OGRP);
    }

public:
    main_window_t(conf_t &conf, const conf_t &ini_conf, const conf_t &def_conf)
    : conf_window_t(
            get_template(intptr_t(this), conf), conf, ini_conf, def_conf) {}

    void add_category(const std::string &name) {
        RUN_FE2C(get(MCT_OGRP), MSG_LADD, (intptr_t)name.c_str());
    }

    void set_options_window(intptr_t opt_id) {
        get(MCT_OPTS).data = opt_id;
    }

    T4IV get_min_preview_size() {
        auto sgrp = RUN_FE2C(get(MCT_SGRP), MSG__GSZ, 0);
        auto spec = RUN_FE2C(get(MCT_SPEC), MSG__GSZ, 0);
        return {{uint16_t(spec), uint16_t(spec >> 16),
                uint16_t(sgrp >> 16), uint16_t(spec >> 16)}};
    }

    static T2IV get_string_dims(CTRL &window, const std::string_view &str) {
        AINF atmp{0, 0, 0, 0, (uint32_t*)str.data()};
        auto f = RUN_FE2C(window, MSG_WTGD, (intptr_t)&atmp);
        return {{uint16_t(f), uint16_t(f >> 16)}};
    }

    void set_progress(int32_t text, uint32_t frac, uint32_t full) {
        std::string line = *get_text_stock(text) + " " + std::to_string(frac);
        if (full) line += " / " + std::to_string(full);
        set_control_text(line, MCT_SELE);
        RUN_FE2C(get(MCT_SELE), MSG_PLIM, (full) ? full : 100);
        RUN_FE2C(get(MCT_SELE), MSG_PPOS, (full) ? frac : 0);
    }

    static void update_previews(intptr_t data, uint64_t time);

    void init_preview(ENGD *engd, const unit_t &preview,
            conf_t::categories_t categories, const std::string &name,
            library_t::lib_id_t lib_id);

    void categorize_previews();
    void finalize_previews();

    void main_loop(uint32_t fram) {
        rInternalMainLoop(&get_root(), fram, update_previews, intptr_t(this));
    }
};

void main_window_t::try_update_checkbox(CTRL &c, int flag) {
    if (conf_window_t::try_update_checkbox(c, flag)) {
        if (c.uuid == TXT_SRND) {
            auto w = (main_window_t*)get_root(c).data;
            RUN_FE2C(w->get(MCT_RGPU), MSG__ENB, flag);
            RUN_FE2C(w->get(MCT_BDUP), MSG__ENB, flag);
        } else if (c.uuid == TXT_FLTR) {
            auto w = (main_window_t*)get_root(c).data;
            RUN_FE2C(w->get(MCT_EXAC), MSG__ENB, flag);
            RUN_FE2C(w->get(MCT_OGRP), MSG__ENB, flag);
            w->categorize_previews();
        } else if (c.uuid == TXT_EXAC) {
            auto w = (main_window_t*)get_root(c).data;
            w->set_control_text_stock((flag) ? TXT_AGRP : TXT_OGRP, MCT_OGRP);
            w->categorize_previews();
        }
    }
}

std::vector<CTRL> main_window_t::get_template(
        intptr_t here, const conf_t &conf) {
    return {
        {nullptr, here, TXT_CAPT, FSW_SIZE | FCT_WNDW,  1,  1,  1,  1, FC2E},
        {nullptr, intptr_t(conf_t::filters),
                        TXT_FLTR,            FCT_CBOX,  0,  0, 19,  2, FC2E},
        {nullptr, intptr_t(conf_t::exact),
                        TXT_EXAC, FCP_VERT | FCT_CBOX,  0,  0, 19,  2, FC2E},
        {nullptr, here, TXT_OGRP, FCP_VERT | FCT_LIST,  0,  0, 19, 16, FC2E},
        {nullptr, here, TXT_SGRP, FCP_VERT | FCT_TEXT,  0,  1, 19,  2, FC2E},
        {nullptr, intptr_t(&conf.spec),
                        TXT_SPEC, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
        {nullptr, here, TXT_BADD, FCP_BOTH | FCT_BUTN,  1, -3,  9,  3, FC2E},
        {nullptr, intptr_t(conf_t::randomsel),
             TXT_SRND, FSX_LEFT | FCP_VERT | FCT_CBOX,  0,  1, 19,  2, FC2E},
        {nullptr, intptr_t(&conf.rgpu),
                        TXT_RGPU, FCP_VERT | FCT_SPIN,  0,  0,  9,  3, FC2E},
        {nullptr, intptr_t(conf_t::copies),
                        TXT_BDUP, FCP_BOTH | FCT_CBOX,  1, -3,  9,  3, FC2E},
        {nullptr, here, TXT_SELE, FCP_VERT | FCT_PBAR,  0,  1, 19,  3, FC2E},
        {nullptr,    0, TXT_OPTS, FCP_VERT | FCT_BUTN,  0,  1,  9,  6, FC2E},
        {nullptr, here, TXT_GOGO, FCP_BOTH | FCT_BUTN
                                           | FSB_DFLT,  1, -6,  9,  6, FC2E},
        {nullptr, here, TXT_HEAD, FCP_HORZ | FCT_SBOX,  0,  0, 41, 43, FC2E},
    };
}

intptr_t main_window_t::FC2EI(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (get_type(*ctrl)) {
        case FCT_IBOX:
            if (cmsg == MSG_IFRM)
                cEngineCallback((ENGD*)ctrl->data, ECB_DRAW, data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) {
                auto w = (main_window_t*)get_root(*ctrl).data;
                w->preview_stats_.move(update_spinner(*ctrl, data));
                w->set_progress(TXT_SELE, w->preview_stats_.get(),
                        w->preview_stats_.max());
            }
            break;
    }
    return 0;
}

class options_window_t : public conf_window_t {
private:
    enum elements_t { OCT_OPTS = 0,
        OCT_UONR, OCT_ETOP, OCT_EEFF, OCT_EINT, OCT_ESAY, OCT_ECLR, OCT_ERCH,
        OCT_NRUN, OCT_TRUN, OCT_NSCA, OCT_TSCA, OCT_NDIL, OCT_TDIL, OCT_NSAY,
        OCT_TSAY, OCT_NCDR, OCT_TCDR, OCT_LSEP, OCT_LHDR, OCT_LCHO, OCT_LREL,
        OCT_LRES, OCT_LGUI, OCT_BSEP, OCT_BHDR, OCT_BCHO, OCT_BREL, OCT_BRES,
        OCT_BDIR, OCT_FSEP, OCT_FREL, OCT_FRES, };
    std::vector<CTRL> get_template(
            intptr_t prev, intptr_t here, const conf_t &conf);

    static intptr_t FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data);

    static void try_update_checkbox(CTRL &c, int flag = -1);

    void maybe_set_control_text(const std::string &path, size_t ctl) {
        if (!path.empty()) {
            set_control_text(path, ctl);
        } else {
            set_control_text_stock(TXT_DFLT, ctl);
        }
    }

    void relocalize() {
        conf_window_t::relocalize();
        for (size_t i = 0; i < size(); i++) {
            try_update_checkbox(get(i));
            try_update_spinner(get(i));
        }
        maybe_set_control_text(conf_.lang, OCT_LGUI);
        maybe_set_control_text(conf_.base, OCT_BDIR);
    }

public:
    options_window_t(intptr_t prev, conf_t &conf, const conf_t &ini_conf,
            const conf_t &def_conf)
    : conf_window_t(get_template(prev, intptr_t(this), conf), conf, ini_conf,
            def_conf) {
        relocalize(); // also triggers relocalization of the main window
    }
};

void options_window_t::try_update_checkbox(CTRL &c, int flag) {
    if (conf_window_t::try_update_checkbox(c, flag)) {
        if (c.uuid == TXT_ESAY) {
            auto w = (options_window_t*)get_root(c).data;
            RUN_FE2C(w->get(OCT_ECLR), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_NSAY), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_TSAY), MSG__ENB, flag);
        } else if (c.uuid == TXT_ERCH) {
            auto w = (options_window_t*)get_root(c).data;
            RUN_FE2C(w->get(OCT_NCDR), MSG__ENB, flag);
            RUN_FE2C(w->get(OCT_TCDR), MSG__ENB, flag);
        }
    }
}

std::vector<CTRL> options_window_t::get_template(
        intptr_t prev, intptr_t here, const conf_t &conf) {
    auto prevptr = (CTRL*)prev;
    return {
        {prevptr, here, TXT_OPTS,            FCT_WNDW,  1,  1,  1,  1, FC2E},

        {nullptr, intptr_t(conf_t::update),
                        TXT_UONR,            FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::topmost),
                        TXT_ETOP, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::effects),
                        TXT_EEFF, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::interaction),
                        TXT_EINT, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::speech),
                        TXT_ESAY, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::cspeech),
                        TXT_ECLR, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},
        {nullptr, intptr_t(conf_t::hover),
                        TXT_ERCH, FCP_VERT | FCT_CBOX,  0,  0, 18,  2, FC2E},

        {nullptr, intptr_t(&conf.nrun),
                        TXT_RUNS,            FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_RUNS, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.nsca),
                        TXT_SCAL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_SCAL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.ndil),
                        TXT_TDIL, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_TDIL, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.nsay),
                        TXT_RSAY, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_RSAY, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},
        {nullptr, intptr_t(&conf.ncdr),
                        TXT_PCDR, FCP_VERT | FCT_SPIN, 19,  0,  8,  3, FC2E},
        {nullptr, here, TXT_PCDR, FCP_BOTH | FCT_TEXT,  0, -3, 22,  3, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_LGUI, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_BDIR, FCP_VERT | FCT_TEXT,  0,  0, 18,  3, FC2E},
        {nullptr, here, TXT_CHOO, FCP_BOTH | FCT_BUTN,  1, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RELO, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
        {nullptr, here, TXT_DFLT, FCP_VERT | FCT_TEXT
                                           | FST_CNTR,  0,  0, 49,  2, FC2E},

        {nullptr, here, TXT_OPTS, FCP_VERT | FCT_TEXT
                                           | FST_SUNK,  0,  1, 49, -2, FC2E},

        {nullptr, here, TXT_RELO, FCP_VERT | FCT_BUTN, 29,  0, 10,  3, FC2E},
        {nullptr, here, TXT_RESE, FCP_BOTH | FCT_BUTN,  0, -3, 10,  3, FC2E},
    };
}

intptr_t options_window_t::FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    switch (get_type(*ctrl)) {
        case FCT_WNDW:
            if (cmsg == MSG_WEND) RUN_FE2C(*ctrl, MSG__SHW, 0);
            break;

        case FCT_CBOX:
            if (cmsg == MSG_BCLK) try_update_checkbox(*ctrl, !!data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) try_update_spinner(*ctrl, data, false);
            break;

        case FCT_BUTN: {
            if (cmsg != MSG_BCLK) break;
            auto w = (options_window_t*)ctrl->data;
            if (ctrl == &w->get(OCT_BCHO)) {
                auto path = (!w->conf_.base.empty()) ? w->conf_.base : "";
                auto base = rChooseDir(ctrl, path.c_str());
                if (!base) break;
                w->conf_.base = base;
                base = (typeof(base))realloc(base, 0);
                // TODO: add some checks to verify that the new dir is ok?
            } else if (ctrl == &w->get(OCT_LCHO)) {
                auto path = (!w->conf_.lang.empty()) ? w->conf_.lang : "";
                auto lang = rChooseFile(ctrl, "lang", path.c_str());
                if (!lang) break;
                w->conf_.lang = lang;
                lang = (typeof(lang))realloc(lang, 0);
                long size = 0;
                if (auto file = rLoadFile(w->conf_.lang.c_str(), &size)) {
                    std::string_view str(file, size);
                    w->conf_.lang_map = conf_t::get_lang_map(str);
                    file = (typeof(file))realloc(file, 0);
                } else {
                    w->conf_.lang_map.clear();
                }
            } else if (ctrl == &w->get(OCT_BREL)) {
                w->conf_.base = w->ini_conf_.base;
            } else if (ctrl == &w->get(OCT_BRES)) {
                w->conf_.base = w->def_conf_.base;
            } else if (ctrl == &w->get(OCT_LREL)) {
                w->conf_.lang = w->ini_conf_.lang;
                w->conf_.lang_map = w->ini_conf_.lang_map;
            } else if (ctrl == &w->get(OCT_LRES)) {
                w->conf_.lang = w->def_conf_.lang;
                w->conf_.lang_map = w->def_conf_.lang_map;
            } else if (ctrl == &w->get(OCT_FREL)) {
                w->conf_ = w->ini_conf_;
            } else if (ctrl == &w->get(OCT_FRES)) {
                w->conf_ = w->def_conf_;
            } else {
                assert(false); // no buttons except those above
            }
            w->relocalize();
            break;
        }
    }
    return 0;
}

// engine data (client side)
class engine_t : public no_copy_t {
private:
    std::string cfnm_; // main configuration file path
    conf_t cdef_; // default configuration
    conf_t cini_; // initial configuration read at the start
    conf_t ccur_; // current configuration
    main_window_t mctl_; // main window
    options_window_t octl_; // options window
    T2IV tray_; // tray icon dimensions
    T4IV area_; // drawing area position and dimensions
    T3IV ppos_; // mouse pointer position (z = flags)
    uint64_t tcur_; // current, dilation-adjusted timestamp
    uint64_t tpre_; // previous raw timestamp
    float tacc_; // partial timestamp accumulator
    std::vector<MENU> mspr_; // per-sprite context menu
    std::vector<MENU> mctx_; // main context menu
    ENGD *engd_;

    std::unordered_map<library_t::lib_id_t, std::unique_ptr<library_t>> libs_;

    static conf_t get_def_conf(const std::string_view base) {
        INCBIN("../exec/loc/en.lang", DefLang);
        static const std::string_view def_lang(DefLang, DefLang_end - DefLang);

        conf_t retn;
        retn.base = base;
        retn.flgs = conf_t::general | conf_t::render;
        retn.lang_map = conf_t::get_lang_map(def_lang);
        return retn;
    }

    static conf_t get_ini_conf(const conf_t &def, const std::string &cfnm) {
        #define CASE(what) {#what, conf_t::what}
        static const std::unordered_map<std::string, conf_t::flags_t> rndr = {
            CASE(gpu), CASE(opaque), CASE(draw), CASE(show),
            CASE(wpbo), CASE(wbgra), CASE(wregion),
        };
        static const std::unordered_map<std::string, conf_t::flags_t> gen = {
            CASE(speech), CASE(cspeech), CASE(topmost), CASE(hover),
            CASE(update), CASE(filters), CASE(effects), CASE(exact),
            CASE(copies), CASE(randomsel), CASE(interaction),
        };
        #undef CASE

        auto render = conf_t::render;
        auto general = conf_t::general;
        int16_t runs = 0;
        conf_t retn;
        retn.base = def.base;
        if (auto file = (!cfnm.empty())
                ? rLoadFile(cfnm.c_str(), nullptr)
                : nullptr) {
            for (token_t text({}, file); !is_empty(text);
                    text = next_token(text.second, 0, DEF_CRLF, 0)) {
                if (!text.first.empty() && (text.first.back() == DEF_LFCR))
                    text.first.remove_suffix(sizeof(DEF_LFCR));
                auto line = next_token(text.first);
                switch (str_hash(line.first)) {
                    default: break;
                    case str_hash("Language"): {
                        long size = 0;
                        std::string name(line.second);
                        if (auto file = rLoadFile(name.c_str(), &size)) {
                            retn.lang = std::move(name);
                            retn.lang_map = conf_t::get_lang_map(
                                    std::string_view(file, size));
                            file = (typeof(file))realloc(file, 0);
                        }
                        break;
                    }
                    case str_hash("Content"):
                        line = next_token(line.second, 0);
                        if (line.first.empty()) break;
                        retn.base = line.first;
                        break;
                    case str_hash("RunsTillUpdate"):
                        retn.nrun.set(process_float(line, retn.nrun.get()));
                        runs = process_float(line, runs);
                        break;
                    case str_hash("BaseScale"):
                        retn.nsca.set(process_float(line, retn.nsca.get()));
                        break;
                    case str_hash("TimeDilation"):
                        retn.ndil.set(process_float(line, retn.ndil.get()));
                        break;
                    case str_hash("RandomSpeech"):
                        retn.nsay.set(process_float(line, retn.nsay.get()));
                        break;
                    case str_hash("CursorDodge"):
                        retn.ncdr.set(process_float(line, retn.ncdr.get()));
                        break;
                    case str_hash("Render"):
                        render = {};
                        while (!is_empty(line))
                            render |= process_map(line, rndr, {});
                        break;
                    case str_hash("Flags"):
                        general = {};
                        while (!is_empty(line))
                            general |= process_map(line, gen, {});
                        break;
                }
            }
            file = (typeof(file))realloc(file, 0);

            retn.flgs = conf_t::general | conf_t::render;
            if (retn.nrun.get() && (retn.nrun.get() <= runs))
                retn.flgs |= conf_t::update;
        }
        return retn;
    }

    static conf_t get_cur_conf(const conf_t &ini) { return ini; }

    void build_library_structure(const std::string &base);

public:
    engine_t(const std::string_view fcnf, const std::string_view base,
            const T2IV tray, const T4IV area);

    const library_t &get_library(library_t::lib_id_t lib_id) const {
        auto il = find_in_map(libs_, lib_id);
        assert(il);
        return *il->get();
    }
    void main_loop();
};

engine_t::engine_t(const std::string_view fcnf, const std::string_view base,
        const T2IV tray, const T4IV area)
: cfnm_((!fcnf.empty()) ? concat_path({std::string(fcnf), DEF_CORE}) : "")
, cdef_(get_def_conf(base))
, cini_(get_ini_conf(cdef_, cfnm_))
, ccur_(get_cur_conf(cini_))
, mctl_(ccur_, cini_, cdef_)
, octl_(mctl_.get_id(), ccur_, cini_, cdef_)
, tray_(tray)
, area_(area)
, ppos_{}
, tcur_{}
, tpre_{}
, tacc_{} {
    mctl_.set_options_window(octl_.get_id()); // link options window to main
    cEngineCallback(0, ECB_INIT, (intptr_t)&engd_); // create rendering engine
    build_library_structure(cini_.base); // read configuration, load everything
}

class main_window_t::preview_t : public no_copy_t {
private:
    // !eligible && !visible = doesn't match selection, not visible
    // !eligible &&  visible = doesn't match selection, visible: need to hide
    //  eligible && !visible = matches selection, not visible: need to show
    //  eligible &&  visible = matches selection, visible
    enum flags_t : uint32_t { GEN_FLAGS(finalized, eligible, visible) };
    std::string name_; // padded name
    int32_t name_len_;
    int32_t name_iter_;
    int64_t name_time_;
    library_t::lib_id_t lib_id_;
    conf_t::spin_t count_;
    flags_t flags_;
    T2IV lower_left_;
    T4IV size_;
    conf_t::categories_t categories_;
    const unit_t &unit_;
    uint32_t frame_iter_;
    int64_t frame_time_;
    CTRL imagebox_; // image box control to preview the sprite
    CTRL charname_; // character name just below the image box
    CTRL spinner_;  // spin control to set ICNT

    bool within_scroll(int32_t y_visible, int32_t y_scroll) const {
        auto ymax = lower_left_.y - y_scroll;
        return (ymax >= 0) && (ymax - get_size().y < y_visible);
    }
    std::string_view name_scroll_advance(int64_t time) {
        #define ABS(v) (((v) < 0) ? -(v) : (v))
        constexpr int FRM_TEXT = FRM_WAIT * 1.5f;
        if (!name_len_ || (time <= name_time_ + FRM_TEXT)) return {};
        name_time_ = time + FRM_TEXT;
        const auto true_len = name_.size() - name_len_ * 2;
        name_[true_len + name_len_ + ABS(name_iter_)] = ' ';
        if (++name_iter_ >= name_len_) name_iter_ = -name_iter_;
        name_[true_len + name_len_ + ABS(name_iter_)] = 0;
        return {name_.data() + ABS(name_iter_), true_len + name_len_};
        #undef ABS
    }
    void toggle_visibility(bool visible) {
        if (imagebox_.fe2c) RUN_FE2C(imagebox_, MSG__SHW, visible);
        if (charname_.fe2c) RUN_FE2C(charname_, MSG__SHW, visible);
        if (spinner_.fe2c) RUN_FE2C(spinner_, MSG__SHW, visible);
    }

public:
    ~preview_t() {
        if (imagebox_.fe2c) rFreeControl(&imagebox_);
        if (charname_.fe2c) rFreeControl(&charname_);
        if (spinner_.fe2c) rFreeControl(&spinner_);
    }
    preview_t(CTRL *p, int32_t idx, T4IV size, FCTL fc2e,
            conf_t::categories_t categories, ENGD *engd, const unit_t &unit,
            const std::string &name, library_t::lib_id_t lib_id)
    : name_(std::to_string(idx + 1) + ". " + name)
    , name_len_(0)
    , name_iter_(0)
    , name_time_(0)
    , lib_id_(lib_id)
    , count_(0, 0, 30000)
    , flags_{}
    , lower_left_{}
    , size_(size)
    , categories_(std::move(categories))
    , unit_(unit)
    , frame_iter_(0)
    , frame_time_(0) {
        auto engd_ = intptr_t(engd);
        imagebox_ = {p, engd_, idx, FCT_IBOX, 0, 0, size.x, size.y, fc2e};
        charname_ = {p, engd_, idx, FCT_TEXT | FST_CNTR, 0, 0, size.x, 0, fc2e};
        spinner_ = {p, intptr_t(&count_), idx, FCT_SPIN, 0, 0, size.x, 0, fc2e};
    }
    bool is_eligible() const { return flags_ & eligible; }
    void set_pos(T2IV lower_left) {
        lower_left_ = lower_left;
        flags_ &= ~visible;
        toggle_visibility(false);
    }
    void finalize(float inv_space_width) {
        if ((flags_ & finalized) || !unit_.is_ready(false)) return;

        auto dims = unit_.dims(false);
        size_.x = std::max(size_.x, dims.x);
        size_.y = std::max(size_.y, dims.y);

        constexpr int leeway = 6;
        auto name_size = get_string_dims(get_root(charname_), name_.data());
        if (name_size.x + leeway > size_.x) { // consider scrolling
            auto maybe_name_len
                      = std::ceil(inv_space_width * (name_size.x - size_.x));
            if (maybe_name_len < 8) { // too tight, widen the box instead
                size_.x = name_size.x + leeway;
            } else {
                name_len_ = maybe_name_len + 1;
                std::string padding(name_len_, ' ');
                name_ = padding + name_ + padding;
            }
        }
        imagebox_.xdim = -size_.x;
        imagebox_.ydim = -size_.y;

        charname_.xdim = -size_.x;
        charname_.ydim = -size_.z;

        spinner_.xdim = -size_.x;
        spinner_.ydim = -size_.w;

        flags_ |= finalized;
    }
    void render(int64_t time) {
        if (!imagebox_.fe2c || !charname_.fe2c) return;
        if (auto uuid = unit_.advance(false, time, frame_time_, frame_iter_)) {
            if (!(flags_ & visible)) return;
            RUN_FE2C(imagebox_, MSG_IFRM, (frame_iter_ & 0x3FF) | (uuid << 10));
        }
        auto str = name_scroll_advance(time);
        if (!str.empty() && (flags_ & visible))
            RUN_FE2C(charname_, MSG__TXT, (intptr_t)str.data());
    }
    bool categorize(const conf_t::categories_t &c, bool all_at_once) {
        const bool match = categories_.match(c, all_at_once);
        flags_ = (match) ? (flags_ | eligible) : (flags_ & ~eligible);
        return match;
    }
    void actualize(int32_t y_visible, int32_t y_scroll) {
        if (!(flags_ & finalized)) return;
        const bool e = is_eligible();
        if (e && !within_scroll(y_visible, y_scroll)) return;
        if (e != !(flags_ & visible)) return;
        flags_ ^= visible;
        const bool v = (flags_ & visible);
        if (v) {
            if (!imagebox_.fe2c) rMakeControl(&imagebox_, nullptr, nullptr);
            if (!charname_.fe2c) {
                rMakeControl(&charname_, nullptr, nullptr);
                RUN_FE2C(charname_, MSG__TXT, intptr_t(name_.c_str()));
            }
            if (!spinner_.fe2c) {
                rMakeControl(&spinner_, nullptr, nullptr);
                try_update_spinner(spinner_);
            }
            long pos[2] = {-lower_left_.x, size_.w - lower_left_.y};
            RUN_FE2C(spinner_, MSG__POS, (intptr_t)pos);
            pos[1] += size_.z;
            RUN_FE2C(charname_, MSG__POS, (intptr_t)pos);
            pos[1] += size_.y;
            RUN_FE2C(imagebox_, MSG__POS, (intptr_t)pos);
        }
        toggle_visibility(v);
    }
    T2IV get_size() const { return {{size_.x, size_.y + size_.z + size_.w}}; }
    int16_t count() const { return is_eligible() * count_.get(); }
    int16_t update_spinner_relative(int16_t value) {
        if (!is_eligible()) return 0;
        auto v = conf_window_t::update_spinner(spinner_, count_.get() + value);
        conf_window_t::try_update_spinner(spinner_, 1, true);
        return v;
    }
};

intptr_t main_window_t::FC2E(CTRL *ctrl, uint32_t cmsg, intptr_t data) {
    //INCBIN("../exec/icon.gif", MainIcon);

    switch (get_type(*ctrl)) {
        case FCT_WNDW:
            if ((cmsg == MSG__TXT) && !data) {
                ((main_window_t*)ctrl->data)->relocalize();
            } else if (cmsg == MSG_WSZC) {
                auto w = (main_window_t*)ctrl->data;
                if (size_t(MCT_CHAR) < w->size())
                    RUN_FE2C(w->get(MCT_CHAR), cmsg, data);
            } else if (cmsg == MSG_WEND) {
/*
                auto w = (main_window_t*)ctrl->data;
                char *fptr, *file, *temp;
                // trying to write the animation base to its new location
                if (!w->conf_.base.empty()) {
                    fptr = strdup(engc->ccur.base);
                    file = Concatenate(0, engc->cini.base, DEF_DSEP, DEF_FLDR);
                    temp = Concatenate(0, engc->tran[TXT_BSAV],
                                          "\n\n", file, "\n==>\n",
                                          fptr, "\n\n", engc->tran[TXT_BDEL]);
                    if (strcmp(engc->cini.base, engc->ccur.base)) {
                        if (!rMessage(temp, engc->tran[TXT_BMOV],
                                            engc->tran[TXT_BYES],
                                            engc->tran[TXT_BNAY])) {
                            free_(fptr);
                            fptr = 0;
                        }
                        if (!rMoveDir(file, fptr)) {
                            free_(temp);
                            temp = Concatenate(0, engc->tran[TXT_BERR],
                                                  "\n\n", file, "\n==>\n",
                                                  (fptr) ? fptr : "[X]");
                            rMessage(temp, engc->tran[TXT_BMOV],
                                           engc->tran[TXT_BYES], 0);
                        }
                    }
                    free_(temp);
                    free_(fptr);
                    free_(file);
                }
//*/
                return 1;
            }
            break;

        case FCT_CBOX:
            if (cmsg == MSG_BCLK) try_update_checkbox(*ctrl, !!data);
            break;

        case FCT_SPIN:
            if (cmsg == MSG_NSET) try_update_spinner(*ctrl, data, false);
            break;

        case FCT_LIST:
            if ((cmsg == MSG_LGST) || (cmsg == MSG_LSST)) {
                auto w = (main_window_t*)ctrl->data;
                auto &categories = (w->conf_.flgs & conf_t::exact)
                        ? w->conf_.ctg_exact
                        : w->conf_.ctg_nonex;
                if (cmsg == MSG_LGST) {
                    return categories.match(conf_t::categories_t(data), false);
                } else {
                    bool retn = categories.match(
                            conf_t::categories_t(data >> 1), false);
                    if (data & 1) {
                        categories.add(data >> 1);
                    } else {
                        categories.remove(data >> 1);
                    }
                    w->categorize_previews();
                    return retn;
                }
            }
            break;

        case FCT_SBOX:
            if (cmsg == MSG_SGIP) {
                ((main_window_t*)ctrl->data)->display_previews(data);
            } else if (cmsg == MSG_SSID) {
                return ((main_window_t*)ctrl->data)->rearrange_previews(
                        {{(uint16_t)data, (uint16_t)(data >> 16)}});
            }
            break;

        case FCT_BUTN:
            if (cmsg != MSG_BCLK) break;
            if (ctrl->uuid == TXT_OPTS) {
                if (auto opts = (CTRL*)ctrl->data) RUN_FE2C(*opts, MSG__SHW, 1);
            } else if (ctrl->uuid == TXT_BADD) {
                auto w = (main_window_t*)ctrl->data;
                auto spec = w->conf_.spec.get();
                for (auto &p : w->previews_)
                    if (int total = p->update_spinner_relative(spec)) {
                        w->preview_stats_.move(total);
                        w->set_progress(TXT_SELE, w->preview_stats_.get(),
                                w->preview_stats_.max());
                    }
            } else if (ctrl->uuid == TXT_GOGO) {
/*
                LINF *libs;
                auto engc = (main_window_t*)ctrl->data;
                AINF igif = {};
                intptr_t icon;
                long ilen, *irnd, *iput;

                irnd = calloc(engc->libs.size, sizeof(*irnd));
                iput = calloc(engc->libs.size, sizeof(*iput));

                // checking if random choice is enabled
                if (engc->conf_.flgs & conf_t::randomsel) {
                    // indexing random-capable libraries
                    for (ilen = icon = 0; icon < engc->libs.size; icon++)
                        if (engc->libs._[icon].wctx.icnt == 0)
                            iput[ilen++] = icon;
                    // iterating over the requested random sprites count
                    for (icon = RUN_FE2C(engc->MCT_RGPU, MSG_NGET, 0);
                        (icon > 0) && ilen; icon--) {
                        irnd[iput[data = RNG_Load(engc->seed) % ilen]]++;
                        auto copies = engc->conf_.flgs & conf_t::copies;
                        if (!copies && (data < --ilen))
                            iput[data] = iput[ilen];
                    }
                    // finally, adding the computed random values to ICNTs
                    for (icon = 0; icon < engc->libs.size; icon++)
                        engc->libs._[icon].wctx.icnt += irnd[icon];
                }
                // is there anything selected? let's find out
                for (icon = 0; icon < engc->libs.size; icon++)
                    if (engc->libs._[icon].wctx.icnt > 0)
                        break;
                if (icon >= engc->libs.size) {
                    // [TODO:] do we need to show messages here?
//                    rMessage("Nothing selected!", 0, 0);
                    free_(irnd);
                    free_(iput);
                    break;
                }
                // counting the number of selected libraries
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

                // [TODO:] adapt for CTR_V_FLTR
                for (libs = engc->libs._, icon = 0; icon < engc->libs.size;
                        icon++)
                    if (AppendSpriteArr(&engc->libs._[icon], engc)) {
                        // revert random ICNT
                        engc->libs._[icon].wctx.icnt -= irnd[icon];
                        if (++libs <= &engc->libs._[icon])
                            CTR_ASSIGN(libs[-1], engc->libs._[icon]);
                    }
                free_(irnd);
                free_(iput);
                CTR_V_MGET(engc->libs, libs - engc->libs._, 1);
                igif.fcnt = 0;
                igif.xdim = engc->idim.x;
                igif.ydim = engc->idim.y;
                igif.time = calloc(sizeof(*igif.time), igif.xdim * igif.ydim);
                cEngineCallback(engc->engd, ECB_DRAW, (intptr_t)&igif);
                icon = rMakeTrayIcon(engc->mctx, engc->tran[TXT_HEAD],
                                     igif.time, igif.xdim, igif.ydim);
                free_(igif.time);
                RUN_FE2C(engc->MCT_CAPT, MSG__SHW, 0);
                engc->pcur = engc->povr = 0;
                engc->data = (engc->pmax) ? calloc(engc->pmax,
                                                  sizeof(*engc->data)) : 0;
                cEngineRunMainLoop(engc->engd, engc->dpos.x, engc->dpos.y,
                                   engc->dims.x + engc->dpos.x,
                                   engc->dims.y + engc->dpos.y, engc->ftmp,
                                   FRM_WAIT, (intptr_t)engc, eUpdFrame,
                                   eUpdFlags);
                cEngineCallback(engc->engd, ECB_GFLG, (intptr_t)&engc->ftmp);
                free_(engc->data);

                rFreeTrayIcon(icon);
                for (icon = 0; icon < engc->pcnt; icon++)
                    free_(engc->parr[icon]);
                free_(engc->parr);
                engc->parr = 0;
                engc->pmax = engc->pcnt = 0;

                // finally showing the window
                RecountSelectedLibs(engc);
                RUN_FE2C(engc->MCT_CAPT, MSG__SHW, ~0);
//*/
            }
            break;
    }
    return 0;
}

void main_window_t::init_preview(ENGD *engd, const unit_t &preview,
        conf_t::categories_t categories, const std::string &name,
        library_t::lib_id_t id) {
    previews_.emplace_back(std::make_unique<preview_t>(&get(MCT_CHAR),
                previews_.size(), get_min_preview_size(), FC2EI,
                std::move(categories), engd, preview, name, id));
}

void main_window_t::categorize_previews() {
    int16_t count = 0, total = 0;
    const bool filters = conf_.flgs & conf_t::filters;
    const bool exact = !filters || (conf_.flgs & conf_t::exact);
    const auto &ctg = (filters) ? (exact) ? conf_.ctg_exact : conf_.ctg_nonex
                                : conf_t::categories_t{};
    for (auto &p : previews_)
        if (p->categorize(ctg, exact)) {
            count += !!p->count();
            total++;
        }
    preview_stats_.set_max(total);
    preview_stats_.set(count);
    set_progress(TXT_SELE, preview_stats_.get(), preview_stats_.max());
    RUN_FE2C(get(MCT_CHAR), MSG_WSZC, 0);
}

void main_window_t::finalize_previews() {
    static const std::string_view spaces("    ");
    float inv_space_width = float(spaces.size())
                          / get_string_dims(get_root(), spaces.data()).x;

    for (auto &p : previews_)
        p->finalize(inv_space_width);
    categorize_previews();
}

void main_window_t::display_previews(int32_t y_scroll) {
    auto metrics = get_scrollbox_metrics();
    for (auto &p : previews_)
        p->actualize(metrics.v, y_scroll);
}

size_t main_window_t::rearrange_previews(T2IV preview_area) {
    constexpr int32_t xysp = 8;
    std::vector<T2IV> rows(1, {{}}); // x = last index + 1, y = max row height
    for (auto xmax = xysp; rows.back().x < (decltype(T2IV::x))previews_.size();
            rows.back().x++) {
        if (!previews_[rows.back().x]->is_eligible()) continue;
        auto size = previews_[rows.back().x]->get_size();
        if (rows.back().y && (xmax + size.x + xysp > preview_area.x)) {
            xmax = xysp + size.x + xysp;
            rows.emplace_back(T2IV{{rows.back().x, size.y}});
        } else {
            xmax += size.x + xysp;
            rows.back().y = std::max(rows.back().y, size.y);
        }
    }
    T2IV here = {{xysp, -xysp}};
    for (auto row = 0; row < (decltype(row))rows.size(); row++) {
        here = {{xysp, here.y + xysp + rows[row].y}};
        for (auto p = (row) ? rows[row - 1].x : 0; p < rows[row].x; p++) {
            if (!previews_[p]->is_eligible()) continue;
            previews_[p]->set_pos(here);
            here.x += previews_[p]->get_size().x + xysp;
        }
    }
    return std::max(here.y, preview_area.y);
}

void main_window_t::update_previews(intptr_t data, uint64_t time) {
    auto w = (main_window_t*)data;
    if (!w->is_visible()) return; // window hidden when the engine is active
    for (auto &p : w->previews_)
        p->render(time);
}

void engine_t::main_loop() {
    // waiting for the previews to finish loading
    cEngineCallback(engd_, ECB_LOAD, 0);
    cEngineCallback(engd_, ECB_LOAD, ~0);

    // computing preview sizes, since image sizes are now known
    mctl_.finalize_previews();

    // computing the colors for colored speech, in parallel
    auto parallel = rMakeParallel(library_t::extract_speech_colors_worker, 1);
    for (auto &l : libs_)
        l.second->extract_speech_colors(engd_, parallel);
    rFreeParallel(parallel);

    // starting the GUI loop
    mctl_.main_loop(FRM_WAIT);
}

void engine_t::build_library_structure(const std::string &base) {
    size_t last_category = 0;
    std::vector<std::pair<library_t::lib_id_t, conf_t::categories_t>> ctg;
    std::unordered_map<std::string, size_t> ctg_map;
    library_t::bhv_id_map_t bhv_id_map;

    std::vector<library_t::input_t> ins;
    auto path = concat_path({base, DEF_FLDR, "Ponies"});
    auto find = rFindMake(path.c_str());
    while (auto file = rFindFile(find)) {
        ins.emplace_back(library_t::input_t(path, file));
        file = (typeof(file))realloc(file, 0);
    }

    for (auto &i : ins) {
        // construct the behaviour ID descriptors and extract categories
        auto ib = bhv_id_map.emplace(
                    ascii_to_lower(i.name), library_t::build_bhv_id_desc(i));
        assert_and_discard(ib.second, ib); // make sure the library is unique
        conf_t::categories_t all_categories;
        for (auto &c : i.categories) {
            if (auto ic = find_in_map(ctg_map, c)) {
                all_categories.add(*ic);
            } else {
                all_categories.add(last_category);
                ctg_map[c] = last_category;
                last_category++;
                mctl_.add_category(c);
            }
        }
        ctg.emplace_back(str_hash(i.name), std::move(all_categories));
    }
    for (auto &i : ins) {
        // initialize the libraries
        printf("%s\n", i.name.c_str());
        libs_.emplace(str_hash(i.name), std::make_unique<library_t>(
                    concat_path({path, i.name}), i, bhv_id_map));
        // process follow targets (nothing to do beside report missing ones)
        for (auto &b : i.behaviours)
            if (!b.follow_target.empty())
                if (!find_in_map(bhv_id_map, b.follow_target))
                    printf("[%s] WARNING, invalid follow target name in '%s'\n",
                            i.name.c_str(), b.name.c_str());
    }

    // order the libraries by name
    using ctg_val_t = decltype(ctg)::value_type;
    auto names = [&](ctg_val_t &a, ctg_val_t &b) {
        auto lib_a = find_in_map(libs_, a.first);
        auto lib_b = find_in_map(libs_, b.first);
        return (*lib_a)->name() < (*lib_b)->name();
    };
    std::sort(ctg.begin(), ctg.end(), names);

    // show the main window, initialize the previews
    mctl_.toggle_visibility(true);
    for (size_t lib_idx = 0; lib_idx < ctg.size(); lib_idx++) {
        auto &c = ctg[lib_idx];
        auto &il = *find_in_map(libs_, c.first);
        mctl_.init_preview(engd_, il->get_preview(engd_), std::move(c.second),
                il->name(), c.first);
        mctl_.set_progress(TXT_LOAD, lib_idx + 1, ctg.size());
    }
}

void eProcessMenuItem(MENU *item) {
}

void eExecuteEngine(char *fcnf, char *base, ulong xico, ulong yico,
                    long  xpos, long  ypos, ulong xdim, ulong ydim) {
    T2IV ico{{(decltype(ico.x))xico, (decltype(ico.y))yico}};
    T4IV scr{{(decltype(scr.x))(xpos), (decltype(scr.y))(ypos),
              (decltype(scr.z))(xdim - xpos), (decltype(scr.w))(ydim - ypos)}};
    engine_t engc(fcnf, base, ico, scr);

    engc.main_loop();
}
