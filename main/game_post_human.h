#pragma once

#include "game_state.h"

// Build a factory (costs unusedClips)
void make_factory(GameState* gs);
// Build a harvester drone
void make_harvester(GameState* gs);
// Build a wire drone
void make_wire_drone(GameState* gs);
// Build a solar farm
void make_farm(GameState* gs);
// Build a battery tower
void make_battery(GameState* gs);

// Reboot (disassemble and refund) infrastructure
void reboot_factory(GameState* gs);
void reboot_harvester(GameState* gs);
void reboot_wire_drone(GameState* gs);
void reboot_farm(GameState* gs);
void reboot_battery(GameState* gs);

// Per-tick matter acquisition (harvester drones)
void acquire_matter(GameState* gs);
// Per-tick wire processing (wire drones)
void process_matter(GameState* gs);

// Per-tick power supply/demand update
void update_power(GameState* gs);

// Per-tick swarm computing update
void update_swarm(GameState* gs);

// Player actions for swarm
void entertain_swarm(GameState* gs);
void synch_swarm(GameState* gs);
