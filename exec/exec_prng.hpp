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
