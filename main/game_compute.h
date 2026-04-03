#pragma once

#include "game_state.h"

// Operations accumulation (per main tick, compFlag==1)
void calculate_operations(GameState* gs);

// Trust from clip milestones (per main tick, humanFlag==1)
void calculate_trust(GameState* gs);

// Add a processor (costs 1 trust or 1 swarmGift)
void add_processor(GameState* gs);

// Add a memory unit (costs 1 trust or 1 swarmGift)
void add_memory(GameState* gs);

// Creativity generation (per main tick)
void calculate_creativity(GameState* gs);

// Quantum chip update (per main tick, qFlag==1)
void quantum_compute(GameState* gs);

// Quantum compute button action
void quantum_compute_action(GameState* gs);
