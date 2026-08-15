#ifndef EXEC_PRNG_HPP
#define EXEC_PRNG_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

// Mersenne random number generator
void RNG_Free(uint32_t **seed);
uint32_t *RNG_Make(uint32_t init);
uint32_t RNG_Load(uint32_t *seed);

class weighted_rng_t {
// this is an RNG helper that given a uniform distribution can choose an index
// within an array of items with different relative probabilities, in O(NlogN)
// preparation time (once, at init) and O(1) per choice; uses the Alias Method
// (see https://en.wikipedia.org/wiki/Alias_method for more information)
private:
    // there are exactly as many slots as items, and the item owning a slot
    // has the same index, which is what makes a slot index directly usable
    // by the caller; the capacity of a slot, however, may be split between
    // its owner and one other item. THRESH is the part belonging to the
    // owner, scaled to 1 << 32; ALIAS is the index of the slot owned by the
    // item that occupies the rest of the capacity, and that is always an
    // item too probable to fit into a single slot. Otherwise, when the slot
    // is not split, ALIAS is the slot's own index
    uint32_t size_;
    struct slot_t { uint32_t thresh, alias; } *data_;

public:
    ~weighted_rng_t() { if (data_) data_ = (typeof(data_))realloc(data_, 0); }
    weighted_rng_t() : weighted_rng_t(std::vector<uint32_t>{}) {}
    // DATA_ being a raw pointer makes the compiler-generated copies a double
    // free, and a user-declared destructor suppresses the implicit moves, so
    // both have to be spelled out. The moved-from object is left with a null
    // DATA_, which is exactly why the destructor above has to test it: unlike
    // realloc(p, 0), which frees, realloc(0, 0) allocates (WHY? Just... WHY?)
    weighted_rng_t(const weighted_rng_t &) = delete;
    weighted_rng_t &operator=(const weighted_rng_t &) = delete;
    weighted_rng_t(weighted_rng_t &&rhs) : size_(rhs.size_), data_(rhs.data_) {
        rhs.size_ = 0;
        rhs.data_ = nullptr;
    }
    weighted_rng_t &operator=(weighted_rng_t &&rhs) {
        auto size = size_;
        auto data = data_;
        size_ = rhs.size_;
        data_ = rhs.data_;
        rhs.size_ = size;
        rhs.data_ = data;
        return *this;
    }
    weighted_rng_t(const std::vector<uint32_t> &weights)
    : size_(weights.size()) {
        // PROB is the current weight times SIZE, which turns FULL (the sum of
        // all weights) into the exact probability capacity of a slot; INDEX is
        // the item's original position stored to unpermute the sort, and ALIAS
        // set to SIZE means "no alias yet"; PROB and FULL are uint64_t for the
        // sole purpose of lifting the range restrictions on the weights as
        // much as possible: uint32_t would demand that the scaled sum fit into
        // 32 bits, i.e. SIZE times worse
        struct prep_t { uint64_t prob; uint32_t index, alias; };

        uint64_t full = 0;
        uint32_t over = 0, size = (size_ > 1u) ? size_ : 1u;
        for (uint32_t i = 0; i < size_; full += weights[i++]) {}
        data_ = (typeof(data_))realloc(nullptr, sizeof(*data_) * size);
        if (!full || (full > UINT32_MAX)) { // fallback to uniform distribution
            assert(!(full > UINT32_MAX)); // signal the overflow in debug mode
            for (uint32_t i = size; i > 0; i--) data_[i - 1] = {0, i - 1};
            return;
        }
        std::vector<prep_t> prep(size);
        for (uint32_t i = size; i > 0; i--)
            prep[i - 1] = {(uint64_t)weights[i - 1] * size, i - 1, size};
        auto rev_prob = [](prep_t &a, prep_t &b) { return a.prob > b.prob; };
        std::sort(prep.begin(), prep.end(), rev_prob);
        for (uint32_t i = size; (i > 0) && !over; i--)
            if (prep[i - 1].prob > full) over = i;
        // the first OVER slots hold more probability than they are allowed and
        // the rest have room to spare; walk both groups towards each other and
        // fill the latter from the former. N.B.: PROB adds up to SIZE*FULL at
        // all times so the surplus in [0, OVER) always equals the room left in
        // [OVER, i) and both reach 0 at the same time, which is why no slot is
        // ever left both underfilled and aliasless; the assert below should
        // always hold, it is there to catch a future edit that can break this
        for (uint32_t i = size; over && (i > over); i--)
            if (prep[i - 1].prob != full) {
                prep[i - 1].alias = over - 1;
                prep[over - 1].prob -= full - prep[i - 1].prob;
                if (prep[over - 1].prob <= full) over--;
            }
        for (uint32_t i = 0; i < size; i++) {
            // a slot filled to the brim needs no alias, and then its threshold
            // does not matter at all, as both outcomes of the comparison yield
            // the same index, so it is ok for the scaled FULL to equal 1 << 32
            // and get truncated down to 0 here
            assert((prep[i].alias < size) || (prep[i].prob == full));
            data_[prep[i].index] = {(uint32_t)((prep[i].prob << 32) / full),
                    (prep[i].alias < size) ? prep[prep[i].alias].index
                                           : prep[i].index};
        }
    }
    size_t size() const { return size_; }

    uint32_t random_selection(uint32_t seed) const {
        // a single 32x32->64 multiplication splits the seed into 2 independent
        // parts: the high half picks the slot, the low half is then weighed
        // against THRESH to pick one of the 2 items sharing that slot
        uint64_t both = (uint64_t)seed * size_;
        uint32_t slot = (uint32_t)(both >> 32);
        return ((uint32_t)both < data_[slot].thresh) ? slot : data_[slot].alias;
    }
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

#endif // EXEC_PRNG_HPP
