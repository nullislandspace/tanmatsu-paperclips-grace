#pragma once

#include "game_nbt.h"
#include "game_state.h"

// Write all GameState fields to an NBT writer (root compound must be open).
void game_save_write_state(NbtWriter* w, const GameState* gs);

// Read all GameState fields from an NBT reader.
// gs should be pre-initialized with defaults (game_state_init) before calling.
// Unknown tags are silently skipped. Missing tags keep their default values.
void game_load_read_state(NbtReader* r, GameState* gs);
