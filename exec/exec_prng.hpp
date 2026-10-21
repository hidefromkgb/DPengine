#ifndef EXEC_PRNG_HPP
#define EXEC_PRNG_HPP

#include <cassert>
#include <cstdint>
#include <vector>

void RNG_Free(uint32_t **seed);
uint32_t *RNG_Make(uint32_t init);
uint32_t RNG_Load(uint32_t *seed);

class weighted_rng_t {
private:
    uint32_t *data_;

public:
    ~weighted_rng_t();
    weighted_rng_t() : weighted_rng_t(std::vector<uint32_t>{}) {}
    // raw DATA_ makes implicit copies double free, and the destructor kills
    // implicit moves; moved-from DATA_ is null, hence the destructor's test:
    // realloc(0, 0) allocates (WHY? Just... WHY?)
    weighted_rng_t(const weighted_rng_t &) = delete;
    weighted_rng_t &operator=(const weighted_rng_t &) = delete;
    weighted_rng_t(weighted_rng_t &&rhs) : data_(rhs.data_) {
        rhs.data_ = nullptr;
    }
    weighted_rng_t &operator=(weighted_rng_t &&rhs) {
        std::swap(data_, rhs.data_);
        return *this;
    }
    size_t size() const;
    uint32_t random_selection(uint32_t seed) const;
    weighted_rng_t(const std::vector<uint32_t> &weights);
};

template <bool allow_empty, typename T>
inline T random_selection(const std::vector<T> &v, uint32_t *seed) {
    if constexpr (!allow_empty) {
        assert(!v.empty());
    } else if (v.empty()) {
        return T{};
    }
    return v[(v.size() > 1) ? uint64_t(RNG_Load(seed)) * v.size() >> 32 : 0];
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
