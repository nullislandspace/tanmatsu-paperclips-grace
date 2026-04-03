#pragma once

#include "game_projects.h"
#include "game_state.h"
#include "pax_gfx.h"
#include "ui_core.h"

// Render the full game UI
void ui_render(pax_buf_t* fb, const GameState* gs, const UIState* ui, const ProjectManager* pm);

// Update LEDs based on game state
void ui_update_leds(const GameState* gs, const ProjectManager* pm);

// Render the AI takeover animation frame
// Returns true while animation is still running
bool ui_render_takeover(pax_buf_t* fb, UIState* ui);
