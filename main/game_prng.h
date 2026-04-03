#pragma once

#include "game_state.h"

// Seed the PRNG state using SplitMix64 expansion
void prng_seed(PRNGState* state, uint64_t seed);

// Returns a double in [0, 1) like Math.random()
double game_random(PRNGState* state);

// Returns an integer in [0, max) exclusive
int game_random_int(PRNGState* state, int max);
