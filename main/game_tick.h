#pragma once

#include "game_projects.h"
#include "game_state.h"

// Run one game tick (10ms / 100Hz)
// pm is the project manager (lives outside GameState, rebuilt per tick)
void game_tick(GameState* gs, ProjectManager* pm);
