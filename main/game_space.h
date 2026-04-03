#pragma once

#include "game_state.h"

// Launch a probe (costs unusedClips)
void launch_probe(GameState* gs);

// Raise/lower probe stats
void raise_probe_stat(GameState* gs, int* stat);
void lower_probe_stat(GameState* gs, int* stat);

// Increase probe trust (costs yomi)
void increase_probe_trust(GameState* gs);

// Increase max trust (costs honor)
void increase_max_trust(GameState* gs);

// Per-tick space exploration
void explore_universe(GameState* gs);

// Per-tick probe self-replication
void spawn_probes(GameState* gs);

// Per-tick hazard encounters
void encounter_hazards(GameState* gs);

// Per-tick value drift
void drift(GameState* gs);

// Per-tick probe-spawned infrastructure
void spawn_factories(GameState* gs);
void spawn_harvesters(GameState* gs);
void spawn_wire_drones(GameState* gs);
