#include "exec_prng.hpp"

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
    const uint32_t size = 624, shift = size - 396 - 1;
    uint32_t retn, iter;

    if (!seed)
        return 0;

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

