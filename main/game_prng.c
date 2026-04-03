#include "game_prng.h"

// SplitMix64 for seed expansion
static uint64_t splitmix64(uint64_t* state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void prng_seed(PRNGState* state, uint64_t seed) {
    uint64_t sm = seed;
    state->s[0] = splitmix64(&sm);
    state->s[1] = splitmix64(&sm);
    state->s[2] = splitmix64(&sm);
    state->s[3] = splitmix64(&sm);
}

static inline uint64_t rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// xoshiro256** algorithm
static uint64_t xoshiro256ss(PRNGState* state) {
    uint64_t* s      = state->s;
    uint64_t  result = rotl(s[1] * 5, 7) * 9;
    uint64_t  t      = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}

double game_random(PRNGState* state) {
    uint64_t r = xoshiro256ss(state);
    // Convert to [0, 1) double
    return (r >> 11) * 0x1.0p-53;
}

int game_random_int(PRNGState* state, int max) {
    if (max <= 0) return 0;
    return (int)(game_random(state) * max);
}
