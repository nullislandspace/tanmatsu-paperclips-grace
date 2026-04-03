#include "game_state.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_random.h"
#include "game_prng.h"

void game_state_init(GameState* gs) {
    memset(gs, 0, sizeof(GameState));

    // Core / Clip Production
    gs->wire        = 1000;
    gs->clipperCost = 5;
    gs->clippperCost = 5;
    gs->clipperBoost = 1;

    // MegaClippers
    gs->megaClipperCost  = 500;
    gs->megaClipperBoost = 1;

    // Business / Economics
    gs->margin       = 0.25;
    gs->wireCost     = 20;
    gs->wireBasePrice = 20;
    gs->wireSupply   = 1000;
    gs->adCost       = 100;
    gs->demand       = 5;
    gs->demandBoost  = 1;
    gs->marketing    = 1;
    gs->marketingLvl = 1;
    gs->marketingEffectiveness = 1;
    gs->transaction  = 1;
    gs->incomeTrackerLen = 1;  // JS starts with incomeTracker = [0]
    gs->incomeTracker[0] = 0;

    // Computational Resources
    gs->processors    = 1;
    gs->memory        = 1;
    gs->opFadeDelay   = 800;
    gs->trust         = 2;
    gs->nextTrust     = 3000;
    gs->fib1          = 2;
    gs->fib2          = 3;
    gs->creativitySpeed = 1;

    // Quantum Computing - initialize chips with wave seeds
    for (int i = 0; i < NUM_QCHIPS; i++) {
        gs->qChips[i].waveSeed = (i + 1) * 0.1;
        gs->qChips[i].value    = 0;
        gs->qChips[i].active   = 0;
    }
    gs->qChipCost = 10000;
    gs->qFade     = 1;

    // Post-Human / Factories / Drones
    gs->factoryBoost     = 1;
    gs->factoryRate      = 1000000000.0;
    gs->factoryCost      = 100000000.0;
    gs->factoryPowerRate = 200;
    gs->harvesterRate    = 26180337.0;
    gs->harvesterCost    = 1000000.0;
    gs->wireDroneRate    = 16180339.0;
    gs->wireDroneCost    = 1000000.0;
    gs->droneBoost       = 1;
    gs->dronePowerRate   = 1;
    gs->availableMatter  = pow(10, 24) * 6000.0;
    gs->totalMatter      = pow(10, 54) * 30.0;
    gs->foundMatter      = gs->availableMatter;

    // Power System
    gs->farmRate    = 50;
    gs->farmCost    = 10000000.0;
    gs->batterySize = 10000;
    gs->batteryCost = 1000000.0;

    // Swarm Computing
    gs->swarmStatus   = SWARM_NONE;
    gs->giftPeriod    = 125000;
    gs->giftCountdown = 125000;
    gs->entertainCost = 10000;
    gs->synchCost     = 5000;

    // Space / Probes
    gs->probeCost         = pow(10, 17);
    gs->probeTrustCost    = 200;
    gs->maxTrust          = 20;
    gs->maxTrustCost      = 91117.99;
    gs->probeXBaseRate    = 1.75e18;
    gs->probeRepBaseRate  = 0.00005;
    gs->probeHazBaseRate  = 0.01;
    gs->probeDriftBaseRate = 0.000001;
    gs->probeFacBaseRate  = 0.000001;
    gs->probeHarvBaseRate = 0.000002;
    gs->probeWireBaseRate = 0.000002;

    // Combat
    for (int i = 0; i < (int)NUM_BATTLE_NAMES; i++) {
        gs->battleNumbers[i] = 1;
    }
    gs->maxBattles        = 1;
    gs->battleAlarm       = 10;
    gs->outcomeTimer      = 150;
    gs->drifterCombat     = 1.75;
    gs->warTrigger        = 1000000;
    gs->attackSpeed       = 0.2;
    gs->attackSpeedMod    = 0.1;
    gs->battleSpeed       = 0.2;
    gs->battleEndTimer    = 100;
    gs->probeCombatBaseRate = 0.15;
    gs->threnodyCost      = 50000;
    snprintf(gs->threnodyTitle, sizeof(gs->threnodyTitle), "Durenstein 1");

    // Investment Engine
    gs->riskiness          = RISK_MED;
    gs->maxPort            = 5;
    gs->investUpgradeCost  = 100;
    gs->stockGainThreshold = 0.5;

    // Strategy / Tournament
    gs->yomiBoost   = 1;
    gs->tourneyCost = 1000;
    gs->tourneyLvl  = 1;
    gs->pick        = 10;
    gs->hMove       = 1;
    gs->vMove       = 1;
    gs->hMovePrev   = 1;
    gs->vMovePrev   = 1;

    // Flags
    gs->humanFlag        = 1;
    gs->wireBuyerStatus  = 1;
    gs->autoTourneyStatus = 1;
    gs->trustFlag        = 1;

    // Endgame
    gs->driftKingMessageCost = 1;
    gs->bribe                = 1000000;

    // Timing / Misc
    gs->resetFlag = 2;

    // Tanmatsu-specific
    gs->turboClickRate = 3;       // 30Hz default
    gs->lastSaveTimestamp = time(NULL);

    // Seed PRNG from hardware RNG
    uint64_t seed = ((uint64_t)esp_random() << 32) | esp_random();
    prng_seed(&gs->prng, seed);

    // Project uses: most projects start with 1 use
    for (int i = 0; i < NUM_PROJECTS; i++) {
        gs->projectUses[i] = 1;
    }
}

void game_state_reset(GameState* gs) {
    int pu = gs->prestigeU;
    int ps = gs->prestigeS;
    game_state_init(gs);
    gs->prestigeU = pu;
    gs->prestigeS = ps;
}

void display_message(GameState* gs, const char* msg) {
    // Shift messages down
    for (int i = MAX_MESSAGES - 1; i > 0; i--) {
        memcpy(gs->messages[i], gs->messages[i - 1], MAX_MSG_LEN);
    }
    strncpy(gs->messages[0], msg, MAX_MSG_LEN - 1);
    gs->messages[0][MAX_MSG_LEN - 1] = '\0';
    if (gs->messageCount < MAX_MESSAGES) {
        gs->messageCount++;
    }
}
