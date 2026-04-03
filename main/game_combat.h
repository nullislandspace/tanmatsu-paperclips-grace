#pragma once

#include "game_state.h"

// Check if battles should start (per tick)
void check_for_battles(GameState* gs);

// Create a new battle
void create_battle(GameState* gs);

// Update active battles (per tick)
void update_battles(GameState* gs);
