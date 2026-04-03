#pragma once

#include "game_projects.h"
#include "game_state.h"
#include "pax_gfx.h"

#define SAVE_SLOT_COUNT 5
#define SAVE_SLOT_AUTO  0

// Save game state to a slot (0 = autosave). Stamps lastSaveTimestamp.
int game_save(GameState* gs, int slot);

// Load game state from a slot.
// Validates magic/version and repairs projectUses for triggered-but-unpurchased projects.
int game_load(GameState* gs, int slot);

// Check if a save slot exists
int game_save_exists(int slot);

// Save prestige data (persists across resets)
int game_save_prestige(const GameState* gs);

// Load prestige data
int game_load_prestige(GameState* gs);

// Create the save directory if it doesn't exist
void game_save_init(void);

// Peek at a save slot's metadata without loading. Returns 0 on success.
typedef struct {
    double clips;
    double ticks;
    time_t timestamp;
    int    humanFlag;
    int    spaceFlag;
    int    dismantle;
} SaveSlotInfo;
int game_save_peek(int slot, SaveSlotInfo* info);

// --- Auto-save task (runs on Core 0 at low priority) ---

// Start the auto-save background task
void game_save_task_start(void);

// Request an auto-save. Called from Core 1 when timer expires.
// The actual write happens asynchronously on the save task.
void game_save_request(GameState* gs);

// Check and clear the pending-save flag (called by save task)
// Returns pointer to the GameState snapshot to save, or NULL if nothing pending.
GameState* game_save_get_pending(void);

// --- Offline progress / replay ---

// Replay a given number of ticks with a progress bar.
// label = display text (e.g. "Catching up..." or "Fast Forward...").
// elapsed_sec = simulated time (for display only).
// ticks = number of game_tick() calls to run.
// Returns the number of ticks actually replayed (may be less if dismantling starts).
int game_replay_with_progress(GameState* gs, ProjectManager* pm,
                              pax_buf_t* fb, void (*blit_fn)(void),
                              const char* label, double elapsed_sec, int ticks);

// Replay offline ticks after loading a save.
// Shows a progress bar on fb, blits via blit_fn.
// Returns the number of ticks replayed.
int game_save_replay_offline(GameState* gs, ProjectManager* pm,
                             pax_buf_t* fb, void (*blit_fn)(void));

// Full load sequence: load from slot, replay offline progress (with progress bar),
// then repair project uses so they're ready for a fresh ProjectManager.
// Returns 0 on success.
int game_load_full(GameState* gs, int slot, pax_buf_t* fb, void (*blit_fn)(void));
