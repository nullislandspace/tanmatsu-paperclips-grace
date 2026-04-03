#pragma once

#include "game_state.h"

// Start a new tournament (costs ops)
void new_tourney(GameState* gs);

// Run one sub-round per call (called per tick from game_tick)
void run_tourney(GameState* gs);

// Auto-tourney check (called per main tick)
void auto_tourney_tick(GameState* gs);

// Tournament display state (read by UI, written by game_tournament.c)
extern int g_tourney_hStrat;    // Horizontal strategy index (-1 = none)
extern int g_tourney_vStrat;    // Vertical strategy index (-1 = none)
extern int g_tourney_lastCell;  // Last hit cell: 0=AA 1=AB 2=BA 3=BB (-1=none)
extern int g_tourney_cellTimer; // Ticks remaining for cell highlight
