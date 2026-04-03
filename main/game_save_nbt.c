#include "game_save_nbt.h"

#include <string.h>

#include "game_projects.h"

// ---------------------------------------------------------------------------
// Project slot <-> name mapping
// ---------------------------------------------------------------------------

typedef struct {
    int         slot;
    const char* name;
} ProjSlotName;

// This table must match the PROJ_* defines in game_projects.h.
// Order does not matter — lookup is by name (load) or slot (save).
static const ProjSlotName proj_slot_names[] = {
    // Original game projects
    { PROJ_IMPROVED_AUTOCLIPPERS,  "PROJ_IMPROVED_AUTOCLIPPERS" },
    { PROJ_EVEN_BETTER_AUTO,       "PROJ_EVEN_BETTER_AUTO" },
    { PROJ_OPTIMIZED_AUTO,         "PROJ_OPTIMIZED_AUTO" },
    { PROJ_HADWIGER_CLIP,          "PROJ_HADWIGER_CLIP" },
    { PROJ_MEGACLIPPERS,           "PROJ_MEGACLIPPERS" },
    { PROJ_IMPROVED_MEGA,          "PROJ_IMPROVED_MEGA" },
    { PROJ_EVEN_BETTER_MEGA,       "PROJ_EVEN_BETTER_MEGA" },
    { PROJ_OPTIMIZED_MEGA,         "PROJ_OPTIMIZED_MEGA" },
    { PROJ_IMPROVED_WIRE,          "PROJ_IMPROVED_WIRE" },
    { PROJ_OPTIMIZED_WIRE,         "PROJ_OPTIMIZED_WIRE" },
    { PROJ_MICROLATTICE,           "PROJ_MICROLATTICE" },
    { PROJ_SPECTRAL_FROTH,         "PROJ_SPECTRAL_FROTH" },
    { PROJ_QUANTUM_FOAM,           "PROJ_QUANTUM_FOAM" },
    { PROJ_NEW_SLOGAN,             "PROJ_NEW_SLOGAN" },
    { PROJ_CATCHY_JINGLE,          "PROJ_CATCHY_JINGLE" },
    { PROJ_HYPNO_HARMONICS,        "PROJ_HYPNO_HARMONICS" },
    { PROJ_HYPNODRONES,            "PROJ_HYPNODRONES" },
    { PROJ_CREATIVITY,             "PROJ_CREATIVITY" },
    { PROJ_LIMERICK,               "PROJ_LIMERICK" },
    { PROJ_LEXICAL_PROCESSING,     "PROJ_LEXICAL_PROCESSING" },
    { PROJ_COMBINATORY_HARMONICS,  "PROJ_COMBINATORY_HARMONICS" },
    { PROJ_HADWIGER_PROBLEM,       "PROJ_HADWIGER_PROBLEM" },
    { PROJ_TOTH_SAUSAGE,           "PROJ_TOTH_SAUSAGE" },
    { PROJ_DONKEY_SPACE,           "PROJ_DONKEY_SPACE" },
    { PROJ_CEV,                    "PROJ_CEV" },
    { PROJ_CURE_CANCER,            "PROJ_CURE_CANCER" },
    { PROJ_WORLD_PEACE,            "PROJ_WORLD_PEACE" },
    { PROJ_GLOBAL_WARMING,         "PROJ_GLOBAL_WARMING" },
    { PROJ_MALE_BALDNESS,          "PROJ_MALE_BALDNESS" },
    { PROJ_TOKEN_GOODWILL,         "PROJ_TOKEN_GOODWILL" },
    { PROJ_TOKEN_GOODWILL_B,       "PROJ_TOKEN_GOODWILL_B" },
    { PROJ_STRATEGIC_MODELING,     "PROJ_STRATEGIC_MODELING" },
    { PROJ_STRATEGY_A100,          "PROJ_STRATEGY_A100" },
    { PROJ_STRATEGY_B100,          "PROJ_STRATEGY_B100" },
    { PROJ_STRATEGY_GREEDY,        "PROJ_STRATEGY_GREEDY" },
    { PROJ_STRATEGY_GENEROUS,      "PROJ_STRATEGY_GENEROUS" },
    { PROJ_STRATEGY_MINIMAX,       "PROJ_STRATEGY_MINIMAX" },
    { PROJ_STRATEGY_TFT,           "PROJ_STRATEGY_TFT" },
    { PROJ_STRATEGY_BEAT_LAST,     "PROJ_STRATEGY_BEAT_LAST" },
    { PROJ_AUTOTOURNEY,            "PROJ_AUTOTOURNEY" },
    { PROJ_THEORY_OF_MIND,         "PROJ_THEORY_OF_MIND" },
    { PROJ_STRATEGIC_ATTACHMENT,   "PROJ_STRATEGIC_ATTACHMENT" },
    { PROJ_ALGORITHMIC_TRADING,    "PROJ_ALGORITHMIC_TRADING" },
    { PROJ_HOSTILE_TAKEOVER,       "PROJ_HOSTILE_TAKEOVER" },
    { PROJ_FULL_MONOPOLY,          "PROJ_FULL_MONOPOLY" },
    { PROJ_BEG_FOR_WIRE,           "PROJ_BEG_FOR_WIRE" },
    { PROJ_WIREBUYER,              "PROJ_WIREBUYER" },
    { PROJ_REVTRACKER,             "PROJ_REVTRACKER" },
    { PROJ_QUANTUM_COMPUTING,      "PROJ_QUANTUM_COMPUTING" },
    { PROJ_PHOTONIC_CHIP,          "PROJ_PHOTONIC_CHIP" },
    { PROJ_LIMERICK_CONT,          "PROJ_LIMERICK_CONT" },
    { PROJ_QUANTUM_TEMPORAL,       "PROJ_QUANTUM_TEMPORAL" },
    { PROJ_XAVIER_REINIT,          "PROJ_XAVIER_REINIT" },
    { PROJ_RELEASE_HYPNODRONES,    "PROJ_RELEASE_HYPNODRONES" },
    { PROJ_TOTH_TUBULE,            "PROJ_TOTH_TUBULE" },
    { PROJ_POWER_GRID,             "PROJ_POWER_GRID" },
    { PROJ_NANOSCALE_WIRE,         "PROJ_NANOSCALE_WIRE" },
    { PROJ_HARVESTER_DRONES,       "PROJ_HARVESTER_DRONES" },
    { PROJ_WIRE_DRONES,            "PROJ_WIRE_DRONES" },
    { PROJ_CLIP_FACTORIES,         "PROJ_CLIP_FACTORIES" },
    { PROJ_UPGRADED_FACTORIES,     "PROJ_UPGRADED_FACTORIES" },
    { PROJ_HYPERSPEED_FACTORIES,   "PROJ_HYPERSPEED_FACTORIES" },
    { PROJ_SUPPLY_CHAIN,           "PROJ_SUPPLY_CHAIN" },
    { PROJ_DRONE_COLLISION,        "PROJ_DRONE_COLLISION" },
    { PROJ_DRONE_ALIGNMENT,        "PROJ_DRONE_ALIGNMENT" },
    { PROJ_DRONE_COHESION,         "PROJ_DRONE_COHESION" },
    { PROJ_MOMENTUM,               "PROJ_MOMENTUM" },
    { PROJ_SWARM_COMPUTING,        "PROJ_SWARM_COMPUTING" },
    { PROJ_SPACE_EXPLORATION,      "PROJ_SPACE_EXPLORATION" },
    { PROJ_COMBAT,                 "PROJ_COMBAT" },
    { PROJ_NAME_BATTLES,           "PROJ_NAME_BATTLES" },
    { PROJ_OODA_LOOP,              "PROJ_OODA_LOOP" },
    { PROJ_ELLIPTIC_HULL,          "PROJ_ELLIPTIC_HULL" },
    { PROJ_REBOOT_SWARM,           "PROJ_REBOOT_SWARM" },
    { PROJ_MONUMENT,               "PROJ_MONUMENT" },
    { PROJ_THRENODY,               "PROJ_THRENODY" },
    { PROJ_GLORY,                  "PROJ_GLORY" },
    { PROJ_MEMORY_RELEASE,         "PROJ_MEMORY_RELEASE" },
    { PROJ_EMPEROR_MSG0,           "PROJ_EMPEROR_MSG0" },
    { PROJ_EMPEROR_MSG1,           "PROJ_EMPEROR_MSG1" },
    { PROJ_EMPEROR_MSG2,           "PROJ_EMPEROR_MSG2" },
    { PROJ_EMPEROR_MSG3,           "PROJ_EMPEROR_MSG3" },
    { PROJ_EMPEROR_MSG4,           "PROJ_EMPEROR_MSG4" },
    { PROJ_EMPEROR_MSG5,           "PROJ_EMPEROR_MSG5" },
    { PROJ_EMPEROR_MSG6,           "PROJ_EMPEROR_MSG6" },
    { PROJ_ACCEPT,                 "PROJ_ACCEPT" },
    { PROJ_REJECT,                 "PROJ_REJECT" },
    { PROJ_UNIVERSE_NEXT_DOOR,     "PROJ_UNIVERSE_NEXT_DOOR" },
    { PROJ_UNIVERSE_WITHIN,        "PROJ_UNIVERSE_WITHIN" },
    { PROJ_DISMANTLE_PROBES,       "PROJ_DISMANTLE_PROBES" },
    { PROJ_DISMANTLE_SWARM,        "PROJ_DISMANTLE_SWARM" },
    { PROJ_DISMANTLE_FACTORIES,    "PROJ_DISMANTLE_FACTORIES" },
    { PROJ_DISMANTLE_STRATEGY,     "PROJ_DISMANTLE_STRATEGY" },
    { PROJ_DISMANTLE_QUANTUM,      "PROJ_DISMANTLE_QUANTUM" },
    { PROJ_DISMANTLE_PROCESSORS,   "PROJ_DISMANTLE_PROCESSORS" },
    { PROJ_DISMANTLE_MEMORY,       "PROJ_DISMANTLE_MEMORY" },
    // Tanmatsu-specific
    { PROJ_RAPID_KEYPRESSING,      "PROJ_RAPID_KEYPRESSING" },
    { PROJ_RAPID_KEYPRESSING_2,    "PROJ_RAPID_KEYPRESSING_2" },
    { PROJ_RAPID_KEYPRESSING_3,    "PROJ_RAPID_KEYPRESSING_3" },
    { PROJ_RAPID_KEYPRESSING_4,    "PROJ_RAPID_KEYPRESSING_4" },
    { PROJ_RAPID_KEYPRESSING_5,    "PROJ_RAPID_KEYPRESSING_5" },
    { PROJ_OFFLINE_1,              "PROJ_OFFLINE_1" },
    { PROJ_OFFLINE_2,              "PROJ_OFFLINE_2" },
    { PROJ_OFFLINE_3,              "PROJ_OFFLINE_3" },
    { PROJ_OFFLINE_4,              "PROJ_OFFLINE_4" },
    { PROJ_OFFLINE_5,              "PROJ_OFFLINE_5" },
};

#define NUM_PROJ_SLOT_NAMES (sizeof(proj_slot_names) / sizeof(proj_slot_names[0]))

static int proj_name_to_slot(const char* name) {
    for (int i = 0; i < (int)NUM_PROJ_SLOT_NAMES; i++) {
        if (strcmp(proj_slot_names[i].name, name) == 0) {
            return proj_slot_names[i].slot;
        }
    }
    return -1;
}


// ---------------------------------------------------------------------------
// Write helpers (macros to reduce boilerplate)
// ---------------------------------------------------------------------------

#define W_D(field)   nbt_write_double(w, #field, gs->field)
#define W_I(field)   nbt_write_int32(w, #field, gs->field)
#define W_I64(field) nbt_write_int64(w, #field, (int64_t)gs->field)
#define W_S(field)   nbt_write_string(w, #field, gs->field)

void game_save_write_state(NbtWriter* w, const GameState* gs) {
    // -- Peek header (fields needed by game_save_peek, written first for fast access) --
    nbt_write_compound(w, "peek");
    W_D(clips);
    W_D(ticks);
    W_I64(lastSaveTimestamp);
    W_I(humanFlag);
    W_I(spaceFlag);
    W_I(dismantle);
    nbt_write_end(w);

    // -- Core / Clip Production --
    // (clips is in peek compound)
    W_D(unusedClips);
    W_D(wire);
    W_D(nanoWire);
    W_D(clipRate);
    W_D(clipRateTemp);
    W_D(prevClips);
    W_D(clipRateTracker);
    W_D(clipmakerRate);
    W_D(clipmakerLevel);
    W_D(clipperCost);
    W_D(clippperCost);
    W_D(clipperBoost);
    W_D(unsoldClips);
    W_I(finalClips);

    // -- MegaClippers --
    W_D(megaClipperLevel);
    W_D(megaClipperCost);
    W_D(megaClipperBoost);

    // -- Business / Economics --
    W_D(funds);
    W_D(margin);
    W_D(wireCost);
    W_D(wireBasePrice);
    W_D(wirePriceCounter);
    W_D(wirePriceTimer);
    W_D(wireSupply);
    W_D(wirePurchase);
    W_D(adCost);
    W_D(demand);
    W_D(demandBoost);
    W_D(marketing);
    W_D(marketingLvl);
    W_D(marketingEffectiveness);
    W_D(clipsSold);
    W_D(avgRev);
    W_D(income);

    nbt_write_compound(w, "incomeTracker");
    for (int i = 0; i < MAX_INCOME_TRACK; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        nbt_write_double(w, key, gs->incomeTracker[i]);
    }
    nbt_write_end(w);

    W_I(incomeTrackerLen);
    W_D(transaction);
    W_D(bankroll);

    // -- Computational Resources --
    W_I(processors);
    W_I(memory);
    W_D(standardOps);
    W_D(tempOps);
    W_D(operations);
    W_D(opFade);
    W_D(opFadeTimer);
    W_D(opFadeDelay);
    W_D(trust);
    W_D(nextTrust);
    W_D(fib1);
    W_D(fib2);
    W_D(creativity);
    W_I(creativityOn);
    W_D(creativitySpeed);
    W_D(creativityCounter);
    W_I(boostLvl);

    // -- Quantum Computing --
    for (int i = 0; i < NUM_QCHIPS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "qChip%d", i);
        nbt_write_compound(w, key);
        nbt_write_double(w, "waveSeed", gs->qChips[i].waveSeed);
        nbt_write_double(w, "value",    gs->qChips[i].value);
        nbt_write_int32(w,  "active",   gs->qChips[i].active);
        nbt_write_end(w);
    }
    W_D(qClock);
    W_D(qChipCost);
    W_I(nextQchip);
    W_D(qFade);

    // -- Post-Human / Factories / Drones --
    W_D(factoryLevel);
    W_D(factoryBoost);
    W_D(factoryRate);
    W_D(factoryCost);
    W_D(factoryBill);
    W_D(factoryPowerRate);
    W_D(harvesterLevel);
    W_D(harvesterRate);
    W_D(harvesterCost);
    W_D(harvesterBill);
    W_D(wireDroneLevel);
    W_D(wireDroneRate);
    W_D(wireDroneCost);
    W_D(wireDroneBill);
    W_D(droneBoost);
    W_D(dronePowerRate);
    W_D(availableMatter);
    W_D(acquiredMatter);
    W_D(processedMatter);
    W_D(totalMatter);
    W_D(foundMatter);
    W_D(maxFactoryLevel);
    W_D(maxDroneLevel);

    // -- Power System --
    W_D(farmRate);
    W_D(farmLevel);
    W_D(farmCost);
    W_D(farmBill);
    W_D(batterySize);
    W_D(batteryLevel);
    W_D(batteryCost);
    W_D(batteryBill);
    W_D(storedPower);
    W_D(powMod);
    W_I(momentum);

    // -- Swarm Computing --
    W_I(swarmStatus);
    W_D(swarmGifts);
    W_D(nextGift);
    W_D(giftPeriod);
    W_D(giftCountdown);
    W_D(giftBits);
    W_D(giftBitGenerationRate);
    W_D(elapsedTime);
    W_I(sliderPos);
    W_D(boredomLevel);
    W_I(boredomFlag);
    W_I(boredomMsg);
    W_D(entertainCost);
    W_D(disorgCounter);
    W_I(disorgFlag);
    W_I(disorgMsg);
    W_D(synchCost);

    // -- Space / Probes --
    W_D(probeCount);
    W_D(probeLaunchLevel);
    W_D(probeDescendents);
    W_D(probeCost);
    W_I(probeTrust);
    W_I(probeUsedTrust);
    W_D(probeTrustCost);
    W_I(maxTrust);
    W_D(maxTrustCost);
    W_I(probeSpeed);
    W_I(probeNav);
    W_I(probeRep);
    W_I(probeHaz);
    W_I(probeFac);
    W_I(probeHarv);
    W_I(probeWire);
    W_I(probeCombat);
    W_D(partialProbeSpawn);
    W_D(partialProbeHaz);
    W_D(probesLostHaz);
    W_D(probesLostDrift);
    W_D(probesLostCombat);
    W_D(drifterCount);
    W_D(probeXBaseRate);
    W_D(probeRepBaseRate);
    W_D(probeHazBaseRate);
    W_D(probeDriftBaseRate);
    W_D(probeFacBaseRate);
    W_D(probeHarvBaseRate);
    W_D(probeWireBaseRate);

    // -- Combat --
    for (int i = 0; i < MAX_BATTLES; i++) {
        char key[12];
        snprintf(key, sizeof(key), "battle%d", i);
        nbt_write_compound(w, key);
        nbt_write_int32(w,  "id",            gs->battles[i].id);
        nbt_write_double(w, "clipProbes",    gs->battles[i].clipProbes);
        nbt_write_double(w, "drifterProbes", gs->battles[i].drifterProbes);
        nbt_write_int32(w,  "victory",       gs->battles[i].victory);
        nbt_write_int32(w,  "loss",          gs->battles[i].loss);
        nbt_write_int32(w,  "whiteFlag",     gs->battles[i].whiteFlag);
        nbt_write_double(w, "territory",     gs->battles[i].territory);
        nbt_write_int32(w,  "reportCount",   gs->battles[i].reportCount);
        nbt_write_int32(w,  "garbageFlag",   gs->battles[i].garbageFlag);
        nbt_write_end(w);
    }

    W_I(battlesLen);

    nbt_write_compound(w, "battleNumbers");
    for (int i = 0; i < (int)NUM_BATTLE_NAMES; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        nbt_write_int32(w, key, gs->battleNumbers[i]);
    }
    nbt_write_end(w);

    W_I(battleID);
    W_S(battleName);
    W_I(battleNameFlag);
    W_I(maxBattles);
    W_I(battleClock);
    W_I(battleAlarm);
    W_I(outcomeTimer);
    W_D(drifterCombat);
    W_D(warTrigger);
    W_D(attackSpeed);
    W_D(attackSpeedMod);
    W_I(attackSpeedFlag);
    W_D(battleSpeed);
    W_D(unitSize);
    W_D(driftersKilled);
    W_I(battleEndDelay);
    W_I(battleEndTimer);
    W_I(masterBattleClock);
    W_D(honor);
    W_I(honorCount);
    W_D(bonusHonor);
    W_D(honorReward);
    W_S(threnodyTitle);
    W_D(threnodyCost);
    W_D(probeCombatBaseRate);

    // -- Investment Engine --
    for (int i = 0; i < MAX_STOCKS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "stock%d", i);
        nbt_write_compound(w, key);
        nbt_write_int32(w,  "id",     gs->stocks[i].id);
        nbt_write_string(w, "symbol", gs->stocks[i].symbol);
        nbt_write_double(w, "price",  gs->stocks[i].price);
        nbt_write_double(w, "amount", gs->stocks[i].amount);
        nbt_write_double(w, "total",  gs->stocks[i].total);
        nbt_write_double(w, "profit", gs->stocks[i].profit);
        nbt_write_int32(w,  "age",    gs->stocks[i].age);
        nbt_write_end(w);
    }

    W_I(portfolioSize);
    W_I(stockID);
    W_D(secTotal);
    W_D(portTotal);
    W_I(sellDelay);
    W_I(riskiness);
    W_I(maxPort);
    W_D(m);
    W_I(investLevel);
    W_D(investUpgradeCost);
    W_D(stockGainThreshold);
    W_D(ledger);
    W_I(stockReportCounter);

    // -- Strategy / Tournament --
    W_D(yomi);
    W_D(yomiBoost);
    W_D(tourneyCost);
    W_I(tourneyLvl);
    W_I(pick);
    W_I(tourneyInProg);
    W_I(resultsFlag);
    W_I(resultsTimer);
    W_I(stratCounter);
    W_I(roundNum);
    W_I(currentRound);
    W_I(rCounter);
    W_I(rounds);
    W_I(hMove);
    W_I(vMove);
    W_I(hMovePrev);
    W_I(vMovePrev);
    W_I(aa); W_I(ab); W_I(ba); W_I(bb);
    W_I(winnerPtr);
    W_I(placeScore);
    W_I(showScore);
    W_I(high);
    W_I(pickScore);

    nbt_write_compound(w, "stratActive");
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        nbt_write_int32(w, key, gs->stratActive[i]);
    }
    nbt_write_end(w);

    // -- Flags --
    // (humanFlag, spaceFlag in peek compound)
    W_I(compFlag);
    W_I(battleFlag);
    W_I(qFlag);
    W_I(swarmFlag);
    W_I(tothFlag);
    W_I(egoFlag);
    W_I(milestoneFlag);
    W_I(autoClipperFlag);
    W_I(megaClipperFlag);
    W_I(revPerSecFlag);
    W_I(projectsFlag);
    W_I(factoryFlag);
    W_I(harvesterFlag);
    W_I(wireDroneFlag);
    W_I(wireProductionFlag);
    W_I(creationFlag);
    W_I(investmentEngineFlag);
    W_I(strategyEngineFlag);
    W_I(wireBuyerFlag);
    W_I(wireBuyerStatus);
    W_I(autoTourneyFlag);
    W_I(autoTourneyStatus);
    W_I(safetyProjectOn);
    W_I(trustFlag);

    // -- Endgame / Dismantling --
    // (dismantle is in peek compound)
    W_I(endTimer1);
    W_I(endTimer2);
    W_I(endTimer3);
    W_I(endTimer4);
    W_I(endTimer5);
    W_I(endTimer6);
    W_I(driftKingMessageCost);
    W_D(bribe);

    // -- Prestige --
    W_I(prestigeU);
    W_I(prestigeS);

    // -- Timing / Misc --
    // (ticks is in peek compound)
    W_I(blinkCounter);
    W_D(x);
    W_I(testFlag);
    W_I(resetFlag);

    // -- Tanmatsu-Specific --
    nbt_write_compound(w, "prng");
    nbt_write_int64(w, "s0", (int64_t)gs->prng.s[0]);
    nbt_write_int64(w, "s1", (int64_t)gs->prng.s[1]);
    nbt_write_int64(w, "s2", (int64_t)gs->prng.s[2]);
    nbt_write_int64(w, "s3", (int64_t)gs->prng.s[3]);
    nbt_write_end(w);

    // (lastSaveTimestamp is in peek compound)
    W_I(offlineProgressLevel);
    W_I(turboClickFlag);
    W_I(turboClickRate);
    W_I(turboCounter);

    // -- Projects (by name) --
    nbt_write_compound(w, "projects");
    for (int i = 0; i < (int)NUM_PROJ_SLOT_NAMES; i++) {
        int slot = proj_slot_names[i].slot;
        if (slot < 0 || slot >= NUM_PROJECTS) continue;
        // Only save projects that differ from defaults (flag!=0 or uses!=1)
        if (gs->projectFlags[slot] == 0 && gs->projectUses[slot] == 1) continue;
        nbt_write_compound(w, proj_slot_names[i].name);
        nbt_write_int32(w, "flag", gs->projectFlags[slot]);
        nbt_write_int32(w, "uses", gs->projectUses[slot]);
        nbt_write_end(w);
    }
    nbt_write_end(w);

    // -- Console Messages --
    nbt_write_compound(w, "messages");
    for (int i = 0; i < MAX_MESSAGES; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        nbt_write_string(w, key, gs->messages[i]);
    }
    nbt_write_end(w);
    W_I(messageCount);
}

// ---------------------------------------------------------------------------
// Read helpers
// ---------------------------------------------------------------------------

// Match macros for the load dispatch table
#define R_D(name_str, field)   if (strcmp(name, name_str) == 0) { gs->field = nbt_read_double(r); return; }
#define R_I(name_str, field)   if (strcmp(name, name_str) == 0) { gs->field = nbt_read_int32(r); return; }
#define R_I64(name_str, field) if (strcmp(name, name_str) == 0) { gs->field = (typeof(gs->field))nbt_read_int64(r); return; }
#define R_S(name_str, field)   if (strcmp(name, name_str) == 0) { nbt_read_string(r, gs->field, sizeof(gs->field)); return; }

// Read a compound containing indexed numeric entries into an array
static void read_int_array(NbtReader* r, int* arr, int max) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_INT32) {
            int idx = atoi(key);
            int32_t val = nbt_read_int32(r);
            if (idx >= 0 && idx < max) arr[idx] = val;
        } else {
            nbt_skip_payload(r, type);
        }
    }
}

static void read_double_array(NbtReader* r, double* arr, int max) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_DOUBLE) {
            int idx = atoi(key);
            double val = nbt_read_double(r);
            if (idx >= 0 && idx < max) arr[idx] = val;
        } else {
            nbt_skip_payload(r, type);
        }
    }
}

static void read_string_array(NbtReader* r, char arr[][MAX_MSG_LEN], int max) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_STRING) {
            int idx = atoi(key);
            if (idx >= 0 && idx < max) {
                nbt_read_string(r, arr[idx], MAX_MSG_LEN);
            } else {
                nbt_skip_payload(r, type);
            }
        } else {
            nbt_skip_payload(r, type);
        }
    }
}

static void read_qchip(NbtReader* r, QChip* chip) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_DOUBLE && strcmp(key, "waveSeed") == 0) chip->waveSeed = nbt_read_double(r);
        else if (type == NBT_DOUBLE && strcmp(key, "value") == 0) chip->value = nbt_read_double(r);
        else if (type == NBT_INT32 && strcmp(key, "active") == 0) chip->active = nbt_read_int32(r);
        else nbt_skip_payload(r, type);
    }
}

static void read_stock(NbtReader* r, Stock* s) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_INT32 && strcmp(key, "id") == 0) s->id = nbt_read_int32(r);
        else if (type == NBT_STRING && strcmp(key, "symbol") == 0) nbt_read_string(r, s->symbol, sizeof(s->symbol));
        else if (type == NBT_DOUBLE && strcmp(key, "price") == 0) s->price = nbt_read_double(r);
        else if (type == NBT_DOUBLE && strcmp(key, "amount") == 0) s->amount = nbt_read_double(r);
        else if (type == NBT_DOUBLE && strcmp(key, "total") == 0) s->total = nbt_read_double(r);
        else if (type == NBT_DOUBLE && strcmp(key, "profit") == 0) s->profit = nbt_read_double(r);
        else if (type == NBT_INT32 && strcmp(key, "age") == 0) s->age = nbt_read_int32(r);
        else nbt_skip_payload(r, type);
    }
}

static void read_battle(NbtReader* r, Battle* b) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_INT32 && strcmp(key, "id") == 0) b->id = nbt_read_int32(r);
        else if (type == NBT_DOUBLE && strcmp(key, "clipProbes") == 0) b->clipProbes = nbt_read_double(r);
        else if (type == NBT_DOUBLE && strcmp(key, "drifterProbes") == 0) b->drifterProbes = nbt_read_double(r);
        else if (type == NBT_INT32 && strcmp(key, "victory") == 0) b->victory = nbt_read_int32(r);
        else if (type == NBT_INT32 && strcmp(key, "loss") == 0) b->loss = nbt_read_int32(r);
        else if (type == NBT_INT32 && strcmp(key, "whiteFlag") == 0) b->whiteFlag = nbt_read_int32(r);
        else if (type == NBT_DOUBLE && strcmp(key, "territory") == 0) b->territory = nbt_read_double(r);
        else if (type == NBT_INT32 && strcmp(key, "reportCount") == 0) b->reportCount = nbt_read_int32(r);
        else if (type == NBT_INT32 && strcmp(key, "garbageFlag") == 0) b->garbageFlag = nbt_read_int32(r);
        else nbt_skip_payload(r, type);
    }
}

static void read_prng(NbtReader* r, PRNGState* prng) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_INT64 && strcmp(key, "s0") == 0) prng->s[0] = (uint64_t)nbt_read_int64(r);
        else if (type == NBT_INT64 && strcmp(key, "s1") == 0) prng->s[1] = (uint64_t)nbt_read_int64(r);
        else if (type == NBT_INT64 && strcmp(key, "s2") == 0) prng->s[2] = (uint64_t)nbt_read_int64(r);
        else if (type == NBT_INT64 && strcmp(key, "s3") == 0) prng->s[3] = (uint64_t)nbt_read_int64(r);
        else nbt_skip_payload(r, type);
    }
}

static void read_projects(NbtReader* r, GameState* gs) {
    char key[64];
    int type;
    while ((type = nbt_read_tag(r, key, sizeof(key))) != NBT_END) {
        if (type < 0) break;
        if (type == NBT_COMPOUND) {
            int slot = proj_name_to_slot(key);
            if (slot >= 0 && slot < NUM_PROJECTS) {
                // Read flag and uses from the compound
                char sub_key[64];
                int sub_type;
                while ((sub_type = nbt_read_tag(r, sub_key, sizeof(sub_key))) != NBT_END) {
                    if (sub_type < 0) break;
                    if (sub_type == NBT_INT32 && strcmp(sub_key, "flag") == 0) {
                        gs->projectFlags[slot] = nbt_read_int32(r);
                    } else if (sub_type == NBT_INT32 && strcmp(sub_key, "uses") == 0) {
                        gs->projectUses[slot] = nbt_read_int32(r);
                    } else {
                        nbt_skip_payload(r, sub_type);
                    }
                }
            } else {
                // Unknown project — skip
                nbt_skip_payload(r, type);
            }
        } else {
            nbt_skip_payload(r, type);
        }
    }
}

// Dispatch a single scalar tag read into the correct GameState field.
static void read_scalar(NbtReader* r, GameState* gs, const char* name, int type) {
    if (type == NBT_DOUBLE) {
        R_D("clips", clips);
        R_D("unusedClips", unusedClips);
        R_D("wire", wire);
        R_D("nanoWire", nanoWire);
        R_D("clipRate", clipRate);
        R_D("clipRateTemp", clipRateTemp);
        R_D("prevClips", prevClips);
        R_D("clipRateTracker", clipRateTracker);
        R_D("clipmakerRate", clipmakerRate);
        R_D("clipmakerLevel", clipmakerLevel);
        R_D("clipperCost", clipperCost);
        R_D("clippperCost", clippperCost);
        R_D("clipperBoost", clipperBoost);
        R_D("unsoldClips", unsoldClips);
        R_D("megaClipperLevel", megaClipperLevel);
        R_D("megaClipperCost", megaClipperCost);
        R_D("megaClipperBoost", megaClipperBoost);
        R_D("funds", funds);
        R_D("margin", margin);
        R_D("wireCost", wireCost);
        R_D("wireBasePrice", wireBasePrice);
        R_D("wirePriceCounter", wirePriceCounter);
        R_D("wirePriceTimer", wirePriceTimer);
        R_D("wireSupply", wireSupply);
        R_D("wirePurchase", wirePurchase);
        R_D("adCost", adCost);
        R_D("demand", demand);
        R_D("demandBoost", demandBoost);
        R_D("marketing", marketing);
        R_D("marketingLvl", marketingLvl);
        R_D("marketingEffectiveness", marketingEffectiveness);
        R_D("clipsSold", clipsSold);
        R_D("avgRev", avgRev);
        R_D("income", income);
        R_D("transaction", transaction);
        R_D("bankroll", bankroll);
        R_D("standardOps", standardOps);
        R_D("tempOps", tempOps);
        R_D("operations", operations);
        R_D("opFade", opFade);
        R_D("opFadeTimer", opFadeTimer);
        R_D("opFadeDelay", opFadeDelay);
        R_D("trust", trust);
        R_D("nextTrust", nextTrust);
        R_D("fib1", fib1);
        R_D("fib2", fib2);
        R_D("creativity", creativity);
        R_D("creativitySpeed", creativitySpeed);
        R_D("creativityCounter", creativityCounter);
        R_D("qClock", qClock);
        R_D("qChipCost", qChipCost);
        R_D("qFade", qFade);
        R_D("factoryLevel", factoryLevel);
        R_D("factoryBoost", factoryBoost);
        R_D("factoryRate", factoryRate);
        R_D("factoryCost", factoryCost);
        R_D("factoryBill", factoryBill);
        R_D("factoryPowerRate", factoryPowerRate);
        R_D("harvesterLevel", harvesterLevel);
        R_D("harvesterRate", harvesterRate);
        R_D("harvesterCost", harvesterCost);
        R_D("harvesterBill", harvesterBill);
        R_D("wireDroneLevel", wireDroneLevel);
        R_D("wireDroneRate", wireDroneRate);
        R_D("wireDroneCost", wireDroneCost);
        R_D("wireDroneBill", wireDroneBill);
        R_D("droneBoost", droneBoost);
        R_D("dronePowerRate", dronePowerRate);
        R_D("availableMatter", availableMatter);
        R_D("acquiredMatter", acquiredMatter);
        R_D("processedMatter", processedMatter);
        R_D("totalMatter", totalMatter);
        R_D("foundMatter", foundMatter);
        R_D("maxFactoryLevel", maxFactoryLevel);
        R_D("maxDroneLevel", maxDroneLevel);
        R_D("farmRate", farmRate);
        R_D("farmLevel", farmLevel);
        R_D("farmCost", farmCost);
        R_D("farmBill", farmBill);
        R_D("batterySize", batterySize);
        R_D("batteryLevel", batteryLevel);
        R_D("batteryCost", batteryCost);
        R_D("batteryBill", batteryBill);
        R_D("storedPower", storedPower);
        R_D("powMod", powMod);
        R_D("swarmGifts", swarmGifts);
        R_D("nextGift", nextGift);
        R_D("giftPeriod", giftPeriod);
        R_D("giftCountdown", giftCountdown);
        R_D("giftBits", giftBits);
        R_D("giftBitGenerationRate", giftBitGenerationRate);
        R_D("elapsedTime", elapsedTime);
        R_D("boredomLevel", boredomLevel);
        R_D("entertainCost", entertainCost);
        R_D("disorgCounter", disorgCounter);
        R_D("synchCost", synchCost);
        R_D("probeCount", probeCount);
        R_D("probeLaunchLevel", probeLaunchLevel);
        R_D("probeDescendents", probeDescendents);
        R_D("probeCost", probeCost);
        R_D("probeTrustCost", probeTrustCost);
        R_D("maxTrustCost", maxTrustCost);
        R_D("partialProbeSpawn", partialProbeSpawn);
        R_D("partialProbeHaz", partialProbeHaz);
        R_D("probesLostHaz", probesLostHaz);
        R_D("probesLostDrift", probesLostDrift);
        R_D("probesLostCombat", probesLostCombat);
        R_D("drifterCount", drifterCount);
        R_D("probeXBaseRate", probeXBaseRate);
        R_D("probeRepBaseRate", probeRepBaseRate);
        R_D("probeHazBaseRate", probeHazBaseRate);
        R_D("probeDriftBaseRate", probeDriftBaseRate);
        R_D("probeFacBaseRate", probeFacBaseRate);
        R_D("probeHarvBaseRate", probeHarvBaseRate);
        R_D("probeWireBaseRate", probeWireBaseRate);
        R_D("drifterCombat", drifterCombat);
        R_D("warTrigger", warTrigger);
        R_D("attackSpeed", attackSpeed);
        R_D("attackSpeedMod", attackSpeedMod);
        R_D("battleSpeed", battleSpeed);
        R_D("unitSize", unitSize);
        R_D("driftersKilled", driftersKilled);
        R_D("honor", honor);
        R_D("bonusHonor", bonusHonor);
        R_D("honorReward", honorReward);
        R_D("threnodyCost", threnodyCost);
        R_D("probeCombatBaseRate", probeCombatBaseRate);
        R_D("secTotal", secTotal);
        R_D("portTotal", portTotal);
        R_D("m", m);
        R_D("investUpgradeCost", investUpgradeCost);
        R_D("stockGainThreshold", stockGainThreshold);
        R_D("ledger", ledger);
        R_D("yomi", yomi);
        R_D("yomiBoost", yomiBoost);
        R_D("tourneyCost", tourneyCost);
        R_D("bribe", bribe);
        R_D("ticks", ticks);
        R_D("x", x);
    } else if (type == NBT_INT32) {
        R_I("finalClips", finalClips);
        R_I("incomeTrackerLen", incomeTrackerLen);
        R_I("processors", processors);
        R_I("memory", memory);
        R_I("creativityOn", creativityOn);
        R_I("boostLvl", boostLvl);
        R_I("nextQchip", nextQchip);
        R_I("momentum", momentum);
        R_I("swarmStatus", swarmStatus);
        R_I("sliderPos", sliderPos);
        R_I("boredomFlag", boredomFlag);
        R_I("boredomMsg", boredomMsg);
        R_I("disorgFlag", disorgFlag);
        R_I("disorgMsg", disorgMsg);
        R_I("probeTrust", probeTrust);
        R_I("probeUsedTrust", probeUsedTrust);
        R_I("maxTrust", maxTrust);
        R_I("probeSpeed", probeSpeed);
        R_I("probeNav", probeNav);
        R_I("probeRep", probeRep);
        R_I("probeHaz", probeHaz);
        R_I("probeFac", probeFac);
        R_I("probeHarv", probeHarv);
        R_I("probeWire", probeWire);
        R_I("probeCombat", probeCombat);
        R_I("battlesLen", battlesLen);
        R_I("battleID", battleID);
        R_I("battleNameFlag", battleNameFlag);
        R_I("maxBattles", maxBattles);
        R_I("battleClock", battleClock);
        R_I("battleAlarm", battleAlarm);
        R_I("outcomeTimer", outcomeTimer);
        R_I("attackSpeedFlag", attackSpeedFlag);
        R_I("battleEndDelay", battleEndDelay);
        R_I("battleEndTimer", battleEndTimer);
        R_I("masterBattleClock", masterBattleClock);
        R_I("honorCount", honorCount);
        R_I("portfolioSize", portfolioSize);
        R_I("stockID", stockID);
        R_I("sellDelay", sellDelay);
        R_I("riskiness", riskiness);
        R_I("maxPort", maxPort);
        R_I("investLevel", investLevel);
        R_I("stockReportCounter", stockReportCounter);
        R_I("tourneyLvl", tourneyLvl);
        R_I("pick", pick);
        R_I("tourneyInProg", tourneyInProg);
        R_I("resultsFlag", resultsFlag);
        R_I("resultsTimer", resultsTimer);
        R_I("stratCounter", stratCounter);
        R_I("roundNum", roundNum);
        R_I("currentRound", currentRound);
        R_I("rCounter", rCounter);
        R_I("rounds", rounds);
        R_I("hMove", hMove);
        R_I("vMove", vMove);
        R_I("hMovePrev", hMovePrev);
        R_I("vMovePrev", vMovePrev);
        R_I("aa", aa);
        R_I("ab", ab);
        R_I("ba", ba);
        R_I("bb", bb);
        R_I("winnerPtr", winnerPtr);
        R_I("placeScore", placeScore);
        R_I("showScore", showScore);
        R_I("high", high);
        R_I("pickScore", pickScore);
        R_I("humanFlag", humanFlag);
        R_I("compFlag", compFlag);
        R_I("spaceFlag", spaceFlag);
        R_I("battleFlag", battleFlag);
        R_I("qFlag", qFlag);
        R_I("swarmFlag", swarmFlag);
        R_I("tothFlag", tothFlag);
        R_I("egoFlag", egoFlag);
        R_I("milestoneFlag", milestoneFlag);
        R_I("autoClipperFlag", autoClipperFlag);
        R_I("megaClipperFlag", megaClipperFlag);
        R_I("revPerSecFlag", revPerSecFlag);
        R_I("projectsFlag", projectsFlag);
        R_I("factoryFlag", factoryFlag);
        R_I("harvesterFlag", harvesterFlag);
        R_I("wireDroneFlag", wireDroneFlag);
        R_I("wireProductionFlag", wireProductionFlag);
        R_I("creationFlag", creationFlag);
        R_I("investmentEngineFlag", investmentEngineFlag);
        R_I("strategyEngineFlag", strategyEngineFlag);
        R_I("wireBuyerFlag", wireBuyerFlag);
        R_I("wireBuyerStatus", wireBuyerStatus);
        R_I("autoTourneyFlag", autoTourneyFlag);
        R_I("autoTourneyStatus", autoTourneyStatus);
        R_I("safetyProjectOn", safetyProjectOn);
        R_I("trustFlag", trustFlag);
        R_I("dismantle", dismantle);
        R_I("endTimer1", endTimer1);
        R_I("endTimer2", endTimer2);
        R_I("endTimer3", endTimer3);
        R_I("endTimer4", endTimer4);
        R_I("endTimer5", endTimer5);
        R_I("endTimer6", endTimer6);
        R_I("driftKingMessageCost", driftKingMessageCost);
        R_I("prestigeU", prestigeU);
        R_I("prestigeS", prestigeS);
        R_I("blinkCounter", blinkCounter);
        R_I("testFlag", testFlag);
        R_I("resetFlag", resetFlag);
        R_I("offlineProgressLevel", offlineProgressLevel);
        R_I("turboClickFlag", turboClickFlag);
        R_I("turboClickRate", turboClickRate);
        R_I("turboCounter", turboCounter);
        R_I("messageCount", messageCount);
    } else if (type == NBT_INT64) {
        R_I64("lastSaveTimestamp", lastSaveTimestamp);
    } else if (type == NBT_STRING) {
        R_S("battleName", battleName);
        R_S("threnodyTitle", threnodyTitle);
    }

    // Unknown tag — skip it
    nbt_skip_payload(r, type);
}

void game_load_read_state(NbtReader* r, GameState* gs) {
    char name[64];
    int type;

    while ((type = nbt_read_tag(r, name, sizeof(name))) != NBT_END) {
        if (type < 0) break;

        // Compounds with special handling
        if (type == NBT_COMPOUND) {
            // "peek" contains fields that are also needed for quick slot preview
            if (strcmp(name, "peek") == 0) {
                char pname[64];
                int ptype;
                while ((ptype = nbt_read_tag(r, pname, sizeof(pname))) != NBT_END) {
                    if (ptype < 0) break;
                    read_scalar(r, gs, pname, ptype);
                }
                continue;
            }
            if (strcmp(name, "prng") == 0) { read_prng(r, &gs->prng); continue; }
            if (strcmp(name, "projects") == 0) { read_projects(r, gs); continue; }
            if (strcmp(name, "incomeTracker") == 0) { read_double_array(r, gs->incomeTracker, MAX_INCOME_TRACK); continue; }
            if (strcmp(name, "battleNumbers") == 0) { read_int_array(r, gs->battleNumbers, NUM_BATTLE_NAMES); continue; }
            if (strcmp(name, "stratActive") == 0) { read_int_array(r, gs->stratActive, NUM_STRATEGIES); continue; }
            if (strcmp(name, "messages") == 0) { read_string_array(r, gs->messages, MAX_MESSAGES); continue; }

            // qChipN
            if (strncmp(name, "qChip", 5) == 0) {
                int idx = atoi(name + 5);
                if (idx >= 0 && idx < NUM_QCHIPS) { read_qchip(r, &gs->qChips[idx]); continue; }
            }
            // stockN
            if (strncmp(name, "stock", 5) == 0) {
                int idx = atoi(name + 5);
                if (idx >= 0 && idx < MAX_STOCKS) { read_stock(r, &gs->stocks[idx]); continue; }
            }
            // battleN
            if (strncmp(name, "battle", 6) == 0 && name[6] >= '0' && name[6] <= '9') {
                int idx = atoi(name + 6);
                if (idx >= 0 && idx < MAX_BATTLES) { read_battle(r, &gs->battles[idx]); continue; }
            }

            // Unknown compound — skip
            nbt_skip_payload(r, type);
            continue;
        }

        // Scalar tags
        read_scalar(r, gs, name, type);
    }
}
