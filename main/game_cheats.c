#include "game_cheats.h"

#if CHEATS_ENABLED

#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "game_projects.h"
#include "game_tick.h"

static const char* TAG = "cheats";

// Pending fast-forward request (seconds). Set by cheat_execute, consumed by main loop.
static int s_ff_pending = 0;

static const char* cheat_names[] = {
    [CHEAT_CLIPS]           = "+1M clips",
    [CHEAT_MONEY]           = "+$1M funds",
    [CHEAT_TRUST]           = "+10 trust",
    [CHEAT_OPS]             = "Fill ops to max",
    [CHEAT_CREATIVITY]      = "+100K creativity",
    [CHEAT_YOMI]            = "+100K yomi",
    [CHEAT_HONOR]           = "+100K honor",
    [CHEAT_WIRE]            = "+1M wire",
    [CHEAT_MATTER]          = "Set matter to 0",
    [CHEAT_PRESTIGE_U]      = "+1 prestige U",
    [CHEAT_PRESTIGE_S]      = "+1 prestige S",
    [CHEAT_PROBES]          = "+1K probes",
    [CHEAT_DRIFTERS]        = "+10M drifters",
    [CHEAT_SWARM_GIFTS]     = "+100 swarm gifts",
    [CHEAT_POWER]           = "+10M power",
    [CHEAT_ALL_STRATEGIES]  = "Unlock all strats",
    [CHEAT_FF_5MIN]         = "Fast Forward 5 min",
    [CHEAT_FF_10MIN]        = "Fast Forward 10 min",
    [CHEAT_FF_30MIN]        = "Fast Forward 30 min",
};

static const char* jump_names[] = {
    [JUMP_EARLY_GAME]        = "Early Game",
    [JUMP_PRE_COMPUTE]       = "Pre-Compute",
    [JUMP_MID_HUMAN]         = "Mid-Human",
    [JUMP_PRE_HYPNODRONE]    = "Pre-HypnoDrone",
    [JUMP_EARLY_POST_HUMAN]  = "Early Post-Human",
    [JUMP_PRE_SPACE]         = "Pre-Space",
    [JUMP_EARLY_SPACE]       = "Early Space",
    [JUMP_MID_SPACE_COMBAT]  = "Mid-Space Combat",
    [JUMP_PRE_ENDGAME]       = "Pre-Endgame",
    [JUMP_ENDGAME_ACCEPT]    = "Endgame Accept",
    [JUMP_ENDGAME_REJECT]    = "Endgame Reject",
};

const char* cheat_get_name(CheatAction action) {
    if (action >= 0 && action < CHEAT_COUNT) return cheat_names[action];
    return "???";
}

const char* cheat_jump_get_name(JumpPreset preset) {
    if (preset >= 0 && preset < JUMP_COUNT) return jump_names[preset];
    return "???";
}

void cheat_execute(GameState* gs, CheatAction action) {
    switch (action) {
        case CHEAT_CLIPS:
            gs->clips += 1000000;
            gs->unusedClips += 1000000;
            gs->unsoldClips += 1000000;
            break;
        case CHEAT_MONEY:
            gs->funds += 1000000;
            break;
        case CHEAT_TRUST:
            gs->trust += 10;
            break;
        case CHEAT_OPS:
            gs->standardOps = gs->memory * 1000;
            break;
        case CHEAT_CREATIVITY:
            gs->creativity += 100000;
            break;
        case CHEAT_YOMI:
            gs->yomi += 100000;
            break;
        case CHEAT_HONOR:
            gs->honor += 100000;
            break;
        case CHEAT_WIRE:
            gs->wire += 1000000;
            break;
        case CHEAT_MATTER:
            gs->availableMatter = 0;
            break;
        case CHEAT_PRESTIGE_U:
            gs->prestigeU += 1;
            break;
        case CHEAT_PRESTIGE_S:
            gs->prestigeS += 1;
            break;
        case CHEAT_PROBES:
            gs->probeCount += 1000;
            gs->probeTrust = 20;
            gs->probeSpeed = 2;
            gs->probeNav = 2;
            gs->probeRep = 2;
            gs->probeHaz = 2;
            gs->probeFac = 2;
            gs->probeHarv = 2;
            gs->probeWire = 2;
            gs->probeCombat = 2;
            gs->probeUsedTrust = 16;
            break;
        case CHEAT_DRIFTERS:
            gs->drifterCount += 10000000;
            break;
        case CHEAT_SWARM_GIFTS:
            gs->swarmGifts += 100;
            break;
        case CHEAT_POWER:
            gs->storedPower += 10000000;
            break;
        case CHEAT_ALL_STRATEGIES:
            for (int i = 0; i < NUM_STRATEGIES; i++) {
                gs->stratActive[i] = 1;
            }
            gs->strategyEngineFlag = 1;
            break;
        case CHEAT_FF_5MIN:
            s_ff_pending = 5 * 60;
            break;
        case CHEAT_FF_10MIN:
            s_ff_pending = 10 * 60;
            break;
        case CHEAT_FF_30MIN:
            s_ff_pending = 30 * 60;
            break;
        default:
            break;
    }
    ESP_LOGI(TAG, "Cheat: %s", cheat_get_name(action));
}

int cheat_ff_pending(void) {
    return s_ff_pending;
}

void cheat_ff_clear(void) {
    s_ff_pending = 0;
}

// Helper to set common early-game state
static void setup_compute_era(GameState* gs) {
    gs->compFlag     = 1;
    gs->projectsFlag = 1;
}

void cheat_jump(GameState* gs, JumpPreset preset) {
    // Save PRNG and prestige across reset
    PRNGState prng = gs->prng;
    int pu = gs->prestigeU;
    int ps = gs->prestigeS;

    game_state_init(gs);
    gs->prng      = prng;
    gs->prestigeU = pu;
    gs->prestigeS = ps;

    switch (preset) {
        case JUMP_EARLY_GAME:
            gs->clips       = 100;
            gs->unsoldClips = 50;
            gs->funds       = 50;
            gs->wire        = 500;
            gs->clipmakerLevel = 5;
            gs->clipperCost = pow(1.1, 5) + 5;
            gs->clippperCost = gs->clipperCost;
            gs->autoClipperFlag = 1;
            gs->milestoneFlag = 1;
            break;

        case JUMP_PRE_COMPUTE:
            gs->clips          = 1900;
            gs->unsoldClips    = 100;
            gs->funds          = 200;
            gs->wire           = 200;
            gs->clipmakerLevel = 20;
            gs->clipperCost    = pow(1.1, 20) + 5;
            gs->clippperCost   = gs->clipperCost;
            gs->autoClipperFlag = 1;
            gs->milestoneFlag  = 2;
            break;

        case JUMP_MID_HUMAN:
            gs->clips          = 100000;
            gs->unsoldClips    = 5000;
            gs->funds          = 5000;
            gs->wire           = 10000;
            gs->clipmakerLevel = 50;
            gs->clipperCost    = pow(1.1, 50) + 5;
            gs->clippperCost   = gs->clipperCost;
            gs->autoClipperFlag = 1;
            gs->milestoneFlag  = 5;
            setup_compute_era(gs);
            gs->trust          = 30;
            gs->processors     = 5;
            gs->memory         = 5;
            gs->standardOps    = 3000;
            gs->creativity     = 200;
            gs->creativityOn   = 1;
            gs->creativitySpeed = 5;
            gs->strategyEngineFlag = 1;
            gs->investmentEngineFlag = 1;
            gs->megaClipperFlag = 1;
            gs->megaClipperLevel = 10;
            gs->megaClipperCost = pow(1.07, 10) * 1000;
            gs->stratActive[0] = 1;
            gs->stratActive[1] = 1;
            gs->stratActive[2] = 1;
            gs->projectFlags[1] = 1;
            gs->projectFlags[3] = 1;
            gs->projectFlags[6] = 1;
            gs->projectFlags[20] = 1;
            gs->projectFlags[21] = 1;
            gs->projectFlags[22] = 1;
            gs->projectFlags[60] = 1;
            gs->projectFlags[61] = 1;
            break;

        case JUMP_PRE_HYPNODRONE:
            gs->clips          = 10000000;
            gs->unsoldClips    = 100000;
            gs->funds          = 10000000;
            gs->wire           = 100000;
            gs->clipmakerLevel = 100;
            gs->autoClipperFlag = 1;
            gs->milestoneFlag  = 6;
            setup_compute_era(gs);
            gs->trust          = 99;
            gs->processors     = 10;
            gs->memory         = 10;
            gs->standardOps    = 500000;
            gs->creativity     = 5000;
            gs->creativityOn   = 1;
            gs->yomi           = 20000;
            gs->megaClipperFlag = 1;
            gs->megaClipperLevel = 50;
            gs->strategyEngineFlag = 1;
            gs->investmentEngineFlag = 1;
            for (int i = 0; i < NUM_STRATEGIES; i++) gs->stratActive[i] = 1;
            // Set prerequisite project flags
            gs->projectFlags[1] = 1;
            gs->projectFlags[3] = 1;
            gs->projectFlags[4] = 1;
            gs->projectFlags[5] = 1;
            gs->projectFlags[6] = 1;
            gs->projectFlags[7] = 1;
            gs->projectFlags[13] = 1;
            gs->projectFlags[14] = 1;
            gs->projectFlags[15] = 1;
            gs->projectFlags[16] = 1;
            gs->projectFlags[17] = 1;
            gs->projectFlags[19] = 1;
            gs->projectFlags[20] = 1;
            gs->projectFlags[21] = 1;
            gs->projectFlags[22] = 1;
            gs->projectFlags[23] = 1;
            gs->projectFlags[24] = 1;
            gs->projectFlags[25] = 1;
            gs->projectFlags[27] = 1;
            gs->projectFlags[34] = 1;
            gs->projectFlags[70] = 1;
            break;

        case JUMP_EARLY_POST_HUMAN:
            gs->clips          = 1e12;
            gs->unusedClips    = 1e11;
            gs->wire           = 1e9;
            gs->humanFlag      = 0;
            gs->milestoneFlag  = 7;
            setup_compute_era(gs);
            gs->trust          = 100;
            gs->processors     = 15;
            gs->memory         = 15;
            gs->standardOps    = 15000;
            gs->creativity     = 10000;
            gs->creativityOn   = 1;
            gs->tothFlag       = 1;
            gs->factoryFlag    = 1;
            gs->harvesterFlag  = 1;
            gs->wireDroneFlag  = 1;
            gs->wireProductionFlag = 1;
            gs->factoryLevel   = 5;
            gs->harvesterLevel = 50;
            gs->wireDroneLevel = 50;
            gs->farmLevel      = 10;
            gs->batteryLevel   = 5;
            gs->swarmFlag      = 1;
            gs->powMod         = 1.0;
            gs->projectFlags[35] = 1;
            gs->projectFlags[18] = 1;
            gs->projectFlags[41] = 1;
            gs->projectFlags[43] = 1;
            gs->projectFlags[44] = 1;
            gs->projectFlags[45] = 1;
            gs->projectFlags[99] = 1;  // 127 mapped to slot 99 (will adjust)
            break;

        case JUMP_PRE_SPACE:
            gs->clips          = 1e27;
            gs->unusedClips    = 1e26;
            gs->wire           = 1e20;
            gs->humanFlag      = 0;
            gs->milestoneFlag  = 13;
            setup_compute_era(gs);
            gs->trust          = 100;
            gs->processors     = 20;
            gs->memory         = 20;
            gs->standardOps    = 120000;
            gs->creativity     = 50000;
            gs->creativityOn   = 1;
            gs->tothFlag       = 1;
            gs->factoryFlag    = 1;
            gs->harvesterFlag  = 1;
            gs->wireDroneFlag  = 1;
            gs->wireProductionFlag = 1;
            gs->factoryLevel   = 80;
            gs->harvesterLevel = 500;
            gs->wireDroneLevel = 500;
            gs->farmLevel      = 100;
            gs->batteryLevel   = 50;
            gs->swarmFlag      = 1;
            gs->powMod         = 1.1;
            gs->momentum       = 1;
            gs->storedPower    = 10000000;
            gs->availableMatter = 1000;
            break;

        case JUMP_EARLY_SPACE:
            gs->clips          = 1e28;
            gs->unusedClips    = 1e20;
            gs->humanFlag      = 0;
            gs->spaceFlag      = 1;
            gs->milestoneFlag  = 14;
            setup_compute_era(gs);
            gs->trust          = 100;
            gs->processors     = 20;
            gs->memory         = 20;
            gs->standardOps    = 20000;
            gs->creativity     = 50000;
            gs->creativityOn   = 1;
            gs->probeCount     = 10;
            gs->probeTrust     = 10;
            gs->probeSpeed     = 2;
            gs->probeNav       = 2;
            gs->probeRep       = 2;
            gs->probeHaz       = 1;
            gs->probeFac       = 1;
            gs->probeHarv      = 1;
            gs->probeWire      = 1;
            gs->probeUsedTrust = 10;
            gs->powMod         = 1.0;
            gs->farmLevel      = 1;
            break;

        case JUMP_MID_SPACE_COMBAT:
            gs->clips          = 1e40;
            gs->unusedClips    = 1e35;
            gs->humanFlag      = 0;
            gs->spaceFlag      = 1;
            gs->battleFlag     = 1;
            gs->milestoneFlag  = 14;
            setup_compute_era(gs);
            gs->trust          = 100;
            gs->processors     = 25;
            gs->memory         = 25;
            gs->standardOps    = 25000;
            gs->creativity     = 100000;
            gs->creativityOn   = 1;
            gs->probeCount     = 1000000;
            gs->drifterCount   = 5000000;
            gs->probeTrust     = 15;
            gs->probeCombat    = 5;
            gs->probeSpeed     = 2;
            gs->probeNav       = 2;
            gs->probeRep       = 2;
            gs->probeHaz       = 1;
            gs->probeFac       = 1;
            gs->probeHarv      = 1;
            gs->probeWire      = 1;
            gs->probeUsedTrust = 15;
            gs->honor          = 10000;
            gs->powMod         = 1.0;
            gs->farmLevel      = 1;
            break;

        case JUMP_PRE_ENDGAME:
            gs->clips       = gs->totalMatter * 0.99;
            gs->unusedClips  = 1e50;
            gs->humanFlag    = 0;
            gs->spaceFlag    = 1;
            gs->milestoneFlag = 14;
            setup_compute_era(gs);
            gs->trust        = 100;
            gs->processors   = 30;
            gs->memory       = 30;
            gs->standardOps  = 30000;
            gs->creativity   = 200000;
            gs->creativityOn = 1;
            gs->probeCount   = 1e30;
            gs->powMod       = 1.0;
            gs->farmLevel    = 1;
            break;

        case JUMP_ENDGAME_ACCEPT:
            gs->clips        = gs->totalMatter;
            gs->unusedClips  = 1e50;
            gs->humanFlag    = 0;
            gs->spaceFlag    = 1;
            gs->milestoneFlag = 15;
            setup_compute_era(gs);
            gs->trust        = 100;
            gs->processors   = 30;
            gs->memory       = 30;
            gs->standardOps  = 300000;
            gs->creativity   = 300000;
            gs->creativityOn = 1;
            gs->projectFlags[46] = 1;
            break;

        case JUMP_ENDGAME_REJECT:
            gs->clips        = gs->totalMatter;
            gs->unusedClips  = 1e50;
            gs->wire         = 50;
            gs->humanFlag    = 0;
            gs->spaceFlag    = 1;
            gs->milestoneFlag = 15;
            setup_compute_era(gs);
            gs->trust        = 100;
            gs->processors   = 30;
            gs->memory       = 30;
            gs->standardOps  = 300000;
            gs->creativity   = 300000;
            gs->creativityOn = 1;
            gs->projectFlags[48] = 1;  // project148 Reject
            break;

        default:
            break;
    }

    ESP_LOGI(TAG, "Jump: %s", cheat_jump_get_name(preset));
}

#endif  // CHEATS_ENABLED
