#ifndef EXEC_COMMON_HPP
#define EXEC_COMMON_HPP

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "exec.h"



using lib_id_t = size_t;
using bhv_id_t = uint32_t;

enum {
/*  framerate limiter in msec    */ FRM_WAIT = 40,
};

#define assert_and_discard(a, d) assert(a), ((void)(d))

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

template <typename K, typename V>
inline const V *find_in_map(const std::unordered_map<K, V> &map, const K &key) {
    auto iter = map.find(key);
    return (iter != map.end()) ? &iter->second : nullptr;
}

template <typename T>
using ref_vec_t = std::vector<std::reference_wrapper<const T>>;

class no_copy_t {
public:
    no_copy_t() = default;
    no_copy_t(no_copy_t&) = delete;       // non construction-copyable
    no_copy_t(const no_copy_t&) = delete; // non construction-copyable
    no_copy_t& operator=(no_copy_t&) = delete;       // non copyable
    no_copy_t& operator=(const no_copy_t&) = delete; // non copyable
};

// client configuration
class conf_t {
public:
    enum flags_t : uint32_t {
        GEN_FLAGS(draw, show, gpu, opaque, wbgra, wpbo, wregion,
                  update, topmost, effects, interaction, speech,
                  cspeech, hover, filters, exact, randomsel, copies)
        render_mask = draw | show | gpu | opaque | wbgra | wpbo | wregion,
        general_mask = update | topmost | effects | interaction | speech
                     | cspeech | hover | filters | exact | randomsel | copies,
    };
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
    using lang_map_t = std::unordered_map<int32_t, std::string>;
    std::string base; // path to the animation base
    std::string lang; // name of the language file
    lang_map_t lang_map; // localization taken from the language file
    spin_t nrun; // runs between updates
    spin_t nsca; // base scaling factor
    spin_t ndil; // time dilation factor
    spin_t nsay; // random speech chance
    spin_t ncdr; // cursor dodge radius
    spin_t spec; // group selection
    spin_t rgpu; // random selection
    flags_t flgs = {};
    categories_t ctg_nonex = {};
    categories_t ctg_exact = {};

    static lang_map_t get_lang_map(const std::string &file_name);
};

#endif // EXEC_COMMON_HPP
