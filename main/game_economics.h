#pragma once

#include "game_state.h"

// Core clip production - consumes wire, produces clips
void clip_click(GameState* gs, double amount);

// Buy wire spool
void buy_wire(GameState* gs);

// Adjust wire price (called per slow tick)
void adjust_wire_price(GameState* gs);

// Raise/lower clip price
void raise_price(GameState* gs);
void lower_price(GameState* gs);

// Update demand curve (called per main tick, humanFlag==1 only)
void update_demand(GameState* gs);

// Attempt sales (called per slow tick, humanFlag==1 only)
void sales_tick(GameState* gs);

// Sell N clips
void sell_clips(GameState* gs, double number);

// Buy an AutoClipper
void make_clipper(GameState* gs);

// Buy a MegaClipper
void make_mega_clipper(GameState* gs);

// Buy marketing level
void buy_marketing(GameState* gs);

// Revenue tracking (called every 1 second = every 100 ticks)
void calculate_rev(GameState* gs);

// Clip rate measurement (called every 100 ticks)
void update_clip_rate(GameState* gs);

// Milestone progression checks
void milestone_check(GameState* gs);
