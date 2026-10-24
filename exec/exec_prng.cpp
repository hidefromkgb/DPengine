#include <cstdlib>

#include "exec_prng.hpp"



/// Mersenne random number generator

void RNG_Free(uint32_t **seed) {
    if (!seed || !*seed) return;
    free(*seed);
    *seed = 0;
}

uint32_t *RNG_Make(uint32_t init) {
    const uint32_t size = 624;
    uint32_t *seed;

    (seed = (uint32_t*)realloc(NULL, sizeof(*seed) * (size + 1)))[1] = init;
    for (seed[0] = 0, init = 1; init < size; init++)
        seed[init + 1] = init + (seed[init] ^ (seed[init] >> 30)) * 1812433253;
    return seed;
}

uint32_t RNG_Load(uint32_t *seed) {
    const uint32_t size = 624, shift = size - 396 - 1;
    uint32_t retn, iter;

    iter = seed[0] + 1;
    seed[0] = (iter >= size) ? iter - size : iter;
    retn = (seed[iter] & 0x80000000) | (seed[seed[0] + 1] & 0x7FFFFFFF);
    seed[iter] = seed[(iter <= shift) ? (iter + size - shift) : (iter - shift)]
               ^ (retn >> 1) ^ ((retn & 1) ? 0x9908B0DF : 0);
    retn = seed[iter] ^ (seed[iter] >> 11);
    retn ^= (retn <<  7) & 0x9D2C5680;
    retn ^= (retn << 15) & 0xEFC60000;
    return retn ^ (retn >> 18);
}

// yields 0 if UPPER is 0 (who would want it anyway?), otherwise [0; UPPER - 1]
uint32_t RNG_LoadBounded(uint32_t *seed, uint32_t upper) {
    return (upper > 1) ? (uint64_t)RNG_Load(seed) * upper >> 32 : 0;
}



// this is an RNG helper that given a uniform distribution can choose an index
// within an array of items with potentially non-equal probabilities - in O(N)
// preparation time (once, at initialization) and O(1) per draw, via the Alias
// Method; for more information see https://en.wikipedia.org/wiki/Alias_method

static void RNG_FreeWeighted(uint32_t **rngw) { // identical to RNG_Free()
    if (!rngw || !*rngw) return;
    free(*rngw);
    *rngw = 0;
}

static uint32_t *RNG_MakeWeighted(uint32_t w_size, const uint32_t *weights) {
    if (w_size && !weights) return NULL;
    // slot count == item count: item #i owns slot #i, occupying a fraction of
    // it that equals THRESHOLD / (1 << 32) and reserving the rest for #ALIAS,
    // an item that is too probable for just one slot and may spill over into
    // many, not only #i; an unsplit slot #k has ALIAS == #k: this slot always
    // yields k (either directly or via ALIAS), and thus its THRESHOLD value
    // can be anything - conceptually it's (1 << 32), which exceeds UINT32_MAX.
    // THRESHOLD = RNGW[i * 2 + 1], ALIAS = RNGW[i * 2 + 2], N = RNGW[0];
    // slot to become #i has INDEX == i - until then partitioning shuffles the
    // order, ALIAS is also INDEX of a slot (same logic as above), PROBABILITY
    // is the item's weight * item count N, and the capacity of every slot is
    // the sum of all weights, making total capacity equal sum * N - enough to
    // fit all PROBABILITY values with no gap by redistributing over the slots.
    struct RNG_PREP { uint32_t index, alias; uint64_t probability; } *prep;
    uint64_t full = 0;
    uint32_t donor = 0;
    uint32_t *rngw = (uint32_t*)realloc(NULL, sizeof(*rngw) * (w_size * 2 + 1));
    for (uint32_t i = 0; i < w_size; full += weights[i++]) {}
    *rngw = w_size;
    if (!full || (full > UINT32_MAX)) { // fallback to uniform distribution
        assert(!(full > UINT32_MAX)); // signal the overflow in debug mode
        for (uint32_t i = 0; i < w_size; i++) {
            rngw[i * 2 + 2] = i;
            rngw[i * 2 + 1] = 0;
        }
        return rngw;
    }
    prep = (struct RNG_PREP*)realloc(NULL, sizeof(*prep) * w_size);
    for (uint32_t i = 0; i < w_size; i++)
        prep[i] = (struct RNG_PREP){i, i, (uint64_t)weights[i] * w_size};
    // donors first, acceptors last - no further sorting, for it doesn't matter
    for (uint32_t acceptor = w_size; donor < acceptor; )
        if (prep[donor].probability > full) {
            donor++;
        } else if (prep[--acceptor].probability > full) {
            struct RNG_PREP swap = prep[donor];
            prep[donor++] = prep[acceptor];
            prep[acceptor] = swap;
        }
    for (uint32_t acceptor = w_size; donor && (donor < acceptor); acceptor--) {
        if (prep[acceptor - 1].probability == full) continue;
        prep[acceptor - 1].alias = prep[donor - 1].index;
        // if a donor has less extra probability than its acceptor can take, it
        // still gives enough to fill the acceptor exactly to capacity: now the
        // former donor becomes an acceptor, and will get probability from some
        // other donor; the process distributes everything and never underflows
        prep[donor - 1].probability -= full - prep[acceptor - 1].probability;
        if (prep[donor - 1].probability <= full) donor--;
    }
    for (uint32_t i = 0; i < w_size; i++) {
        assert(prep[i].alias != prep[i].index || prep[i].probability == full);
        rngw[prep[i].index * 2 + 2] = prep[i].alias;
        rngw[prep[i].index * 2 + 1] = (prep[i].probability << 32) / full;
    }      // ^--- PROBABILITY == FULL yields (1 << 32), it overflows to 0, but
    free(prep); // it's ok (see top comment)
    return rngw; // TODO: maybe allow FULL > UINT32_MAX and just rescale it?
}

static uint32_t RNG_LoadWeighted(const uint32_t *rngw, uint32_t *seed) {
    // no NULL check for RNGW; we die like men.
    // one 32x32->64 multiplication splits the seed: the high half picks a slot
    // and the low half is weighed against THRESHOLD to pick one of its 2 items
    if (rngw[0] <= 1) return 0;
    uint64_t both = (uint64_t)RNG_Load(seed) * rngw[0]; // == RNG_Load() * size
    uint32_t slot = (uint32_t)(both >> 32);
    return ((uint32_t)both >= rngw[slot * 2 + 1]) ? rngw[slot * 2 + 2] : slot;
}



weighted_rng_t::weighted_rng_t(const std::vector<uint32_t> &weights)
: data_(RNG_MakeWeighted(weights.size(), weights.data())) {}

weighted_rng_t::~weighted_rng_t() { RNG_FreeWeighted(&data_); }

size_t weighted_rng_t::size() const { return (data_) ? data_[0] : 0; }

uint32_t weighted_rng_t::random_selection(uint32_t *seed) const {
    return RNG_LoadWeighted(data_, seed);
}
