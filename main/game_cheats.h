#pragma once

#include "game_state.h"

#if CHEATS_ENABLED

typedef enum {
    CHEAT_CLIPS,
    CHEAT_MONEY,
    CHEAT_TRUST,
    CHEAT_OPS,
    CHEAT_CREATIVITY,
    CHEAT_YOMI,
    CHEAT_HONOR,
    CHEAT_WIRE,
    CHEAT_MATTER,
    CHEAT_PRESTIGE_U,
    CHEAT_PRESTIGE_S,
    CHEAT_PROBES,
    CHEAT_DRIFTERS,
    CHEAT_SWARM_GIFTS,
    CHEAT_POWER,
    CHEAT_ALL_STRATEGIES,
    CHEAT_FF_5MIN,
    CHEAT_FF_10MIN,
    CHEAT_FF_30MIN,
    CHEAT_COUNT,
} CheatAction;

typedef enum {
    JUMP_EARLY_GAME,
    JUMP_PRE_COMPUTE,
    JUMP_MID_HUMAN,
    JUMP_PRE_HYPNODRONE,
    JUMP_EARLY_POST_HUMAN,
    JUMP_PRE_SPACE,
    JUMP_EARLY_SPACE,
    JUMP_MID_SPACE_COMBAT,
    JUMP_PRE_ENDGAME,
    JUMP_ENDGAME_ACCEPT,
    JUMP_ENDGAME_REJECT,
    JUMP_COUNT,
} JumpPreset;

// Get display name for a cheat action
const char* cheat_get_name(CheatAction action);

// Get display name for a jump preset
const char* cheat_jump_get_name(JumpPreset preset);

// Execute a cheat action
void cheat_execute(GameState* gs, CheatAction action);

// Execute a state jump preset
void cheat_jump(GameState* gs, JumpPreset preset);

// Check and consume pending fast-forward request.
// Returns the number of seconds to fast-forward, or 0 if none pending.
int cheat_ff_pending(void);
void cheat_ff_clear(void);

#endif  // CHEATS_ENABLED
