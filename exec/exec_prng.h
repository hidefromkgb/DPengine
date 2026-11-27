#ifndef EXEC_PRNG_H
#define EXEC_PRNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void RNG_Free(uint32_t **seed);
uint32_t *RNG_Make(uint32_t init);
uint32_t RNG_Load(uint32_t *seed);
uint32_t RNG_LoadBounded(uint32_t *seed, uint32_t upper);

void RNG_FreeWeighted(uint32_t **rngw);
uint32_t *RNG_MakeWeighted(uint32_t w_size, const uint32_t *weights);
uint32_t RNG_LoadWeighted(const uint32_t *rngw, uint32_t *seed);

#ifdef __cplusplus
}
#endif

#endif // EXEC_PRNG_H
