#include "game_projects.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "game_economics.h"

// Helper: count active strategies
static int count_strategies(const GameState* gs) {
    int n = 0;
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        if (gs->stratActive[i]) n++;
    }
    return n;
}

// ============================================================
// TRIGGER FUNCTIONS
// ============================================================

static bool trig_improved_auto(const GameState* gs) { return gs->clipmakerLevel >= 1; }
static bool trig_even_better_auto(const GameState* gs) { return gs->boostLvl == 1; }
static bool trig_optimized_auto(const GameState* gs) { return gs->boostLvl == 2; }
static bool trig_hadwiger_clip(const GameState* gs) { return gs->projectFlags[PROJ_HADWIGER_PROBLEM]; }
static bool trig_megaclippers(const GameState* gs) { return gs->clipmakerLevel >= 50; }
static bool trig_improved_mega(const GameState* gs) { return gs->projectFlags[PROJ_MEGACLIPPERS]; }
static bool trig_even_better_mega(const GameState* gs) { return gs->projectFlags[PROJ_IMPROVED_MEGA]; }
static bool trig_optimized_mega(const GameState* gs) { return gs->projectFlags[PROJ_EVEN_BETTER_MEGA]; }

static bool trig_improved_wire(const GameState* gs) { return gs->wirePurchase >= 1; }
static bool trig_optimized_wire(const GameState* gs) { return gs->wireSupply >= 1500; }
static bool trig_microlattice(const GameState* gs) { return gs->wireSupply >= 2600; }
static bool trig_spectral_froth(const GameState* gs) { return gs->wireSupply >= 5000; }
static bool trig_quantum_foam(const GameState* gs) { return gs->wireCost >= 125; }

static bool trig_new_slogan(const GameState* gs) { return gs->projectFlags[PROJ_LEXICAL_PROCESSING]; }
static bool trig_catchy_jingle(const GameState* gs) { return gs->projectFlags[PROJ_COMBINATORY_HARMONICS]; }
static bool trig_hypno_harmonics(const GameState* gs) { return gs->projectFlags[PROJ_CATCHY_JINGLE]; }
static bool trig_hypnodrones(const GameState* gs) { return gs->projectFlags[PROJ_HYPNO_HARMONICS]; }

static bool trig_creativity(const GameState* gs) { return gs->operations >= gs->memory * 1000 && gs->compFlag; }
static bool trig_limerick(const GameState* gs) { return gs->creativityOn; }
static bool trig_lexical(const GameState* gs) { return gs->creativity >= 50; }
static bool trig_combinatory(const GameState* gs) { return gs->creativity >= 100; }
static bool trig_hadwiger(const GameState* gs) { return gs->creativity >= 150; }
static bool trig_toth_sausage(const GameState* gs) { return gs->creativity >= 200; }
static bool trig_donkey_space(const GameState* gs) { return gs->creativity >= 250; }
static bool trig_cev(const GameState* gs) { return gs->yomi >= 1; }
static bool trig_cure_cancer(const GameState* gs) { return gs->projectFlags[PROJ_CEV]; }
static bool trig_world_peace(const GameState* gs) { return gs->projectFlags[PROJ_CEV]; }
static bool trig_global_warming(const GameState* gs) { return gs->projectFlags[PROJ_CEV]; }
static bool trig_male_baldness(const GameState* gs) { return gs->projectFlags[PROJ_CEV]; }
static bool trig_token(const GameState* gs) { return gs->humanFlag == 1 && gs->trust >= 85 && gs->trust < 100 && gs->clips >= 101000000; }
static bool trig_token_b(const GameState* gs) { return gs->projectFlags[PROJ_TOKEN_GOODWILL] && gs->trust < 100; }

static bool trig_strategic_modeling(const GameState* gs) { return gs->projectFlags[PROJ_DONKEY_SPACE]; }
static bool trig_strat_a100(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGIC_MODELING]; }
static bool trig_strat_b100(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_A100]; }
static bool trig_strat_greedy(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_B100]; }
static bool trig_strat_generous(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_GREEDY]; }
static bool trig_strat_minimax(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_GENEROUS]; }
static bool trig_strat_tft(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_MINIMAX]; }
static bool trig_strat_beatlast(const GameState* gs) { return gs->projectFlags[PROJ_STRATEGY_TFT]; }
static bool trig_autotourney(const GameState* gs) { return gs->strategyEngineFlag && gs->trust >= 90; }
static bool trig_theory_mind(const GameState* gs) { return count_strategies(gs) >= 8; }
static bool trig_strategic_attach(const GameState* gs) { return gs->spaceFlag && count_strategies(gs) >= 8 && gs->probeTrustCost > gs->yomi; }

static bool trig_algo_trading(const GameState* gs) { return gs->trust >= 8; }
static bool trig_hostile_takeover(const GameState* gs) { return gs->portTotal >= 10000; }
static bool trig_full_monopoly(const GameState* gs) { return gs->projectFlags[PROJ_HOSTILE_TAKEOVER]; }

static bool trig_beg_wire(const GameState* gs) { return gs->unsoldClips < 1 && gs->funds < gs->wireCost && gs->wire < 1 && gs->compFlag; }
static bool trig_wirebuyer(const GameState* gs) { return gs->wirePurchase >= 15; }
static bool trig_revtracker(const GameState* gs) { return gs->projectsFlag; }
static bool trig_quantum_comp(const GameState* gs) { return gs->processors >= 5; }
static bool trig_photonic_chip(const GameState* gs) { return gs->projectFlags[PROJ_QUANTUM_COMPUTING]; }
static bool trig_limerick_cont(const GameState* gs) { return gs->creativity >= 1000000; }
static bool trig_quantum_temporal(const GameState* gs) { return gs->operations <= -10000; }
static bool trig_xavier(const GameState* gs) { return gs->humanFlag && gs->creativity >= 100000; }

static bool trig_release_hypno(const GameState* gs) { return gs->projectFlags[PROJ_HYPNODRONES]; }

static bool trig_toth_tubule(const GameState* gs) { return gs->projectFlags[PROJ_TOTH_SAUSAGE] && !gs->humanFlag; }
static bool trig_power_grid(const GameState* gs) { return gs->tothFlag; }
static bool trig_nanoscale_wire(const GameState* gs) { return gs->projectFlags[PROJ_POWER_GRID]; }
static bool trig_harvester_drones(const GameState* gs) { return gs->projectFlags[PROJ_NANOSCALE_WIRE]; }
static bool trig_wire_drones(const GameState* gs) { return gs->projectFlags[PROJ_NANOSCALE_WIRE]; }
static bool trig_clip_factories(const GameState* gs) { return gs->projectFlags[PROJ_HARVESTER_DRONES] && gs->projectFlags[PROJ_WIRE_DRONES]; }

static bool trig_upgraded_fac(const GameState* gs) { return gs->factoryLevel >= 10; }
static bool trig_hyperspeed_fac(const GameState* gs) { return gs->factoryLevel >= 20; }
static bool trig_supply_chain(const GameState* gs) { return gs->factoryLevel >= 50; }
static bool trig_drone_collision(const GameState* gs) { return gs->harvesterLevel + gs->wireDroneLevel >= 500; }
static bool trig_drone_alignment(const GameState* gs) { return gs->harvesterLevel + gs->wireDroneLevel >= 5000; }
static bool trig_drone_cohesion(const GameState* gs) { return gs->harvesterLevel + gs->wireDroneLevel >= 50000; }

static bool trig_momentum(const GameState* gs) { return gs->farmLevel >= 50; }
static bool trig_swarm_comp(const GameState* gs) { return gs->harvesterLevel + gs->wireDroneLevel >= 200; }
static bool trig_space_expl(const GameState* gs) { return !gs->humanFlag && gs->availableMatter == 0; }

static bool trig_combat(const GameState* gs) { return gs->probesLostCombat >= 1; }
static bool trig_name_battles(const GameState* gs) { return gs->probesLostCombat >= 10000000; }
static bool trig_ooda(const GameState* gs) { return gs->projectFlags[PROJ_COMBAT] && gs->probesLostCombat >= 10000000; }
static bool trig_elliptic(const GameState* gs) { return gs->probesLostHaz >= 100; }
static bool trig_reboot_swarm(const GameState* gs) { return gs->spaceFlag && gs->harvesterLevel + gs->wireDroneLevel >= 2; }
static bool trig_monument(const GameState* gs) { return gs->projectFlags[PROJ_NAME_BATTLES]; }
static bool trig_threnody(const GameState* gs) { return gs->projectFlags[PROJ_NAME_BATTLES] && gs->probeUsedTrust == gs->maxTrust; }
static bool trig_glory(const GameState* gs) { return gs->projectFlags[PROJ_NAME_BATTLES]; }
static bool trig_mem_release(const GameState* gs) { return gs->spaceFlag && gs->probeCount == 0 && gs->unusedClips < gs->probeCost; }

static bool trig_emperor0(const GameState* gs) { return gs->milestoneFlag == 15; }
static bool trig_emperor1(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG0]; }
static bool trig_emperor2(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG1]; }
static bool trig_emperor3(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG2]; }
static bool trig_emperor4(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG3]; }
static bool trig_emperor5(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG4]; }
static bool trig_emperor6(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG5]; }
static bool trig_accept(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG6]; }
static bool trig_reject(const GameState* gs) { return gs->projectFlags[PROJ_EMPEROR_MSG6]; }

static bool trig_univ_next(const GameState* gs) { return gs->projectFlags[PROJ_ACCEPT]; }
static bool trig_univ_within(const GameState* gs) { return gs->projectFlags[PROJ_ACCEPT]; }

static bool trig_dismantle_probes(const GameState* gs) { return gs->endTimer1 >= 1000; }
static bool trig_dismantle_swarm(const GameState* gs) { return gs->projectFlags[PROJ_DISMANTLE_PROBES] && gs->endTimer1 >= 350; }
static bool trig_dismantle_fac(const GameState* gs) { return gs->endTimer2 >= 300; }
static bool trig_dismantle_strat(const GameState* gs) { return gs->endTimer3 >= 150; }
static bool trig_dismantle_quantum(const GameState* gs) { return gs->endTimer4 >= 100; }
static bool trig_dismantle_proc(const GameState* gs) { return gs->projectFlags[PROJ_DISMANTLE_QUANTUM] && gs->endTimer4 >= 300; }
static bool trig_dismantle_mem(const GameState* gs) { return gs->projectFlags[PROJ_DISMANTLE_PROCESSORS] && gs->endTimer5 >= 150; }

// ============================================================
// COST FUNCTIONS
// ============================================================

static bool cost_ops(const GameState* gs, double amount) { return gs->operations >= amount; }
static bool cost_creat(const GameState* gs, double amount) { return gs->creativity >= amount; }
static bool cost_yomi(const GameState* gs, double amount) { return gs->yomi >= amount; }
static bool cost_funds(const GameState* gs, double amount) { return gs->funds >= amount; }
// Individual cost checkers (combined costs)
static bool cost_750ops(const GameState* gs) { return cost_ops(gs, 750); }
static bool cost_2500ops(const GameState* gs) { return cost_ops(gs, 2500); }
static bool cost_5000ops(const GameState* gs) { return cost_ops(gs, 5000); }
static bool cost_6000ops(const GameState* gs) { return cost_ops(gs, 6000); }
static bool cost_1750ops(const GameState* gs) { return cost_ops(gs, 1750); }
static bool cost_3500ops(const GameState* gs) { return cost_ops(gs, 3500); }
static bool cost_7500ops(const GameState* gs) { return cost_ops(gs, 7500); }
static bool cost_12000ops(const GameState* gs) { return cost_ops(gs, 12000); }
static bool cost_15000ops(const GameState* gs) { return cost_ops(gs, 15000); }
static bool cost_1000ops(const GameState* gs) { return cost_ops(gs, 1000); }
static bool cost_10creat(const GameState* gs) { return cost_creat(gs, 10); }
static bool cost_50creat(const GameState* gs) { return cost_creat(gs, 50); }
static bool cost_100creat(const GameState* gs) { return cost_creat(gs, 100); }
static bool cost_150creat(const GameState* gs) { return cost_creat(gs, 150); }
static bool cost_200creat(const GameState* gs) { return cost_creat(gs, 200); }
static bool cost_250creat(const GameState* gs) { return cost_creat(gs, 250); }
static bool cost_slogan(const GameState* gs) { return cost_ops(gs, 2500) && cost_creat(gs, 25); }
static bool cost_jingle(const GameState* gs) { return cost_ops(gs, 4500) && cost_creat(gs, 45); }
static bool cost_hypno_harm(const GameState* gs) { return cost_ops(gs, 7500) && gs->trust >= 1; }
static bool cost_70000ops(const GameState* gs) { return cost_ops(gs, 70000); }
static bool cost_cev(const GameState* gs) { return cost_ops(gs, 20000) && cost_creat(gs, 500) && cost_yomi(gs, 1000); }
static bool cost_25000ops(const GameState* gs) { return cost_ops(gs, 25000); }
static bool cost_30000ops_5000yomi(const GameState* gs) { return cost_ops(gs, 30000) && cost_yomi(gs, 5000); }
static bool cost_50000ops_1500yomi(const GameState* gs) { return cost_ops(gs, 50000) && cost_yomi(gs, 1500); }
static bool cost_20000ops(const GameState* gs) { return cost_ops(gs, 20000); }
static bool cost_500000funds(const GameState* gs) { return cost_funds(gs, 500000); }
static bool cost_bribe(const GameState* gs) { return gs->funds >= gs->bribe; }
static bool cost_17500ops(const GameState* gs) { return cost_ops(gs, 17500); }
static bool cost_22500ops(const GameState* gs) { return cost_ops(gs, 22500); }
static bool cost_30000ops(const GameState* gs) { return cost_ops(gs, 30000); }
static bool cost_32500ops(const GameState* gs) { return cost_ops(gs, 32500); }
static bool cost_10000ops(const GameState* gs) { return cost_ops(gs, 10000); }
static bool cost_1m_funds(const GameState* gs) { return cost_funds(gs, 1000000); }
static bool cost_10m_1000yomi(const GameState* gs) { return cost_funds(gs, 10000000) && cost_yomi(gs, 1000); }
static bool cost_beg_trust(const GameState* gs) { return gs->trust >= 1; }
static bool cost_7000ops(const GameState* gs) { return cost_ops(gs, 7000); }
static bool cost_500ops(const GameState* gs) { return cost_ops(gs, 500); }
static bool cost_photonic(const GameState* gs) { return gs->operations >= gs->qChipCost; }
static bool cost_14000ops(const GameState* gs) { return cost_ops(gs, 14000); }
static bool cost_19500ops(const GameState* gs) { return cost_ops(gs, 19500); }
static bool cost_50000creat(const GameState* gs) { return cost_creat(gs, 50000); }
static bool cost_25000creat(const GameState* gs) { return cost_creat(gs, 25000); }
static bool cost_175000creat(const GameState* gs) { return cost_creat(gs, 175000); }
static bool cost_release_hypno(const GameState* gs) { return gs->trust >= 100; }
static bool cost_45000ops(const GameState* gs) { return cost_ops(gs, 45000); }
static bool cost_40000ops(const GameState* gs) { return cost_ops(gs, 40000); }
static bool cost_35000ops(const GameState* gs) { return cost_ops(gs, 35000); }
static bool cost_80000ops(const GameState* gs) { return cost_ops(gs, 80000); }
static bool cost_85000ops(const GameState* gs) { return cost_ops(gs, 85000); }
static bool cost_supply_chain(const GameState* gs) { return gs->unusedClips >= 1e21; }
static bool cost_100000ops(const GameState* gs) { return cost_ops(gs, 100000); }
static bool cost_12000yomi(const GameState* gs) { return cost_yomi(gs, 12000); }
static bool cost_30000creat(const GameState* gs) { return cost_creat(gs, 30000); }
static bool cost_space(const GameState* gs) { return cost_ops(gs, 120000) && gs->storedPower >= 10000000 && gs->unusedClips >= 5e27; }
static bool cost_150000ops(const GameState* gs) { return cost_ops(gs, 150000); }
static bool cost_225000creat(const GameState* gs) { return cost_creat(gs, 225000); }
static bool cost_ooda(const GameState* gs) { return cost_ops(gs, 175000) && cost_yomi(gs, 15000); }
static bool cost_125000ops(const GameState* gs) { return cost_ops(gs, 125000); }
static bool cost_monument(const GameState* gs) { return cost_ops(gs, 250000) && cost_creat(gs, 125000) && gs->unusedClips >= 50e30; }
static bool cost_threnody(const GameState* gs) { return gs->creativity >= gs->threnodyCost && gs->yomi >= gs->threnodyCost; }
static bool cost_glory(const GameState* gs) { return cost_ops(gs, 200000) && cost_yomi(gs, 10000); }
static bool cost_mem_release(const GameState* gs) { return gs->memory >= 10; }
static bool cost_emperor(const GameState* gs) { return gs->operations >= gs->driftKingMessageCost; }
static bool cost_300000ops(const GameState* gs) { return cost_ops(gs, 300000); }
static bool cost_300000creat(const GameState* gs) { return cost_creat(gs, 300000); }
static bool cost_all_ops(const GameState* gs) { return gs->operations > 0; }
static bool cost_1000000creat(const GameState* gs) { return cost_creat(gs, 1000000); }
static bool cost_100000creat(const GameState* gs) { return cost_creat(gs, 100000); }
static bool cost_neg_ops(const GameState* gs) { (void)gs; return true; }

// ============================================================
// EFFECT FUNCTIONS
// ============================================================

static void eff_improved_auto(GameState* gs) { gs->standardOps -= 750; gs->clipperBoost += 0.25; gs->boostLvl = 1; }
static void eff_even_better_auto(GameState* gs) { gs->standardOps -= 2500; gs->clipperBoost += 0.50; gs->boostLvl = 2; }
static void eff_optimized_auto(GameState* gs) { gs->standardOps -= 5000; gs->clipperBoost += 0.75; gs->boostLvl = 3; }
static void eff_hadwiger_clip(GameState* gs) { gs->standardOps -= 6000; gs->clipperBoost += 5; }
static void eff_megaclippers(GameState* gs) { gs->standardOps -= 12000; gs->megaClipperFlag = 1; }
static void eff_improved_mega(GameState* gs) { gs->standardOps -= 14000; gs->megaClipperBoost += 0.25; }
static void eff_even_better_mega(GameState* gs) { gs->standardOps -= 17000; gs->megaClipperBoost += 0.50; }
static void eff_optimized_mega(GameState* gs) { gs->standardOps -= 19500; gs->megaClipperBoost += 1; }

static void eff_improved_wire(GameState* gs) { gs->standardOps -= 1750; gs->wireSupply *= 1.5; }
static void eff_optimized_wire(GameState* gs) { gs->standardOps -= 3500; gs->wireSupply *= 1.75; }
static void eff_microlattice(GameState* gs) { gs->standardOps -= 7500; gs->wireSupply *= 2; }
static void eff_spectral_froth(GameState* gs) { gs->standardOps -= 12000; gs->wireSupply *= 3; }
static void eff_quantum_foam(GameState* gs) { gs->standardOps -= 15000; gs->wireSupply *= 11; }

static void eff_new_slogan(GameState* gs) { gs->standardOps -= 2500; gs->creativity -= 25; gs->marketingEffectiveness *= 1.5; }
static void eff_catchy_jingle(GameState* gs) { gs->standardOps -= 4500; gs->creativity -= 45; gs->marketingEffectiveness *= 2; }
static void eff_hypno_harmonics(GameState* gs) { gs->standardOps -= 7500; gs->trust -= 1; gs->marketingEffectiveness *= 5; }
static void eff_hypnodrones(GameState* gs) { gs->standardOps -= 70000; }

static void eff_creativity(GameState* gs) { gs->standardOps -= 1000; gs->creativityOn = 1; }
static void eff_limerick(GameState* gs) { gs->creativity -= 10; gs->trust += 1; }
static void eff_lexical(GameState* gs) { gs->creativity -= 50; gs->trust += 1; }
static void eff_combinatory(GameState* gs) { gs->creativity -= 100; gs->trust += 1; }
static void eff_hadwiger(GameState* gs) { gs->creativity -= 150; gs->trust += 1; }
static void eff_toth_sausage(GameState* gs) { gs->creativity -= 200; gs->trust += 1; }
static void eff_donkey_space(GameState* gs) { gs->creativity -= 250; gs->trust += 1; }
static void eff_cev(GameState* gs) { gs->standardOps -= 20000; gs->creativity -= 500; gs->yomi -= 1000; gs->trust += 1; }
static void eff_cure_cancer(GameState* gs) { gs->standardOps -= 25000; gs->trust += 10; gs->stockGainThreshold += 0.01; }
static void eff_world_peace(GameState* gs) { gs->standardOps -= 30000; gs->yomi -= 5000; gs->trust += 12; gs->stockGainThreshold += 0.01; }
static void eff_global_warming(GameState* gs) { gs->standardOps -= 50000; gs->yomi -= 1500; gs->trust += 15; gs->stockGainThreshold += 0.01; }
static void eff_male_baldness(GameState* gs) { gs->standardOps -= 20000; gs->trust += 20; gs->stockGainThreshold += 0.01; }
static void eff_token(GameState* gs) { gs->funds -= 500000; gs->trust += 1; }
static void eff_token_b(GameState* gs) { gs->funds -= gs->bribe; gs->trust += 1; gs->bribe *= 2; gs->projectUses[PROJ_TOKEN_GOODWILL_B]++; }

static void eff_strategic_modeling(GameState* gs) { gs->standardOps -= 12000; gs->strategyEngineFlag = 1; gs->stratActive[0] = 1; gs->pick = 0; }
static void eff_strat_a100(GameState* gs) { gs->standardOps -= 15000; gs->stratActive[1] = 1; gs->tourneyCost += 1000; }
static void eff_strat_b100(GameState* gs) { gs->standardOps -= 17500; gs->stratActive[2] = 1; gs->tourneyCost += 1000; }
static void eff_strat_greedy(GameState* gs) { gs->standardOps -= 20000; gs->stratActive[3] = 1; gs->tourneyCost += 1000; }
static void eff_strat_generous(GameState* gs) { gs->standardOps -= 22500; gs->stratActive[4] = 1; gs->tourneyCost += 1000; }
static void eff_strat_minimax(GameState* gs) { gs->standardOps -= 25000; gs->stratActive[5] = 1; gs->tourneyCost += 1000; }
static void eff_strat_tft(GameState* gs) { gs->standardOps -= 30000; gs->stratActive[6] = 1; gs->tourneyCost += 1000; }
static void eff_strat_beatlast(GameState* gs) { gs->standardOps -= 32500; gs->stratActive[7] = 1; gs->tourneyCost += 1000; }
static void eff_autotourney(GameState* gs) { gs->creativity -= 50000; gs->autoTourneyFlag = 1; }
static void eff_theory_mind(GameState* gs) { gs->creativity -= 25000; gs->yomiBoost = 2; gs->tourneyCost = 16000; }
static void eff_strategic_attach(GameState* gs) { gs->creativity -= 175000; }

static void eff_algo_trading(GameState* gs) { gs->standardOps -= 10000; gs->investmentEngineFlag = 1; }
static void eff_hostile_takeover(GameState* gs) { gs->funds -= 1000000; gs->demandBoost *= 5; gs->trust += 1; }
static void eff_full_monopoly(GameState* gs) { gs->funds -= 10000000; gs->yomi -= 1000; gs->demandBoost *= 10; gs->trust += 1; }

static void eff_beg_wire(GameState* gs) { gs->trust -= 1; gs->wire = gs->wireSupply; gs->projectUses[PROJ_BEG_FOR_WIRE]++; }
static void eff_wirebuyer(GameState* gs) { gs->standardOps -= 7000; gs->wireBuyerFlag = 1; }
static void eff_revtracker(GameState* gs) { gs->standardOps -= 500; gs->revPerSecFlag = 1; }
static void eff_quantum_comp(GameState* gs) { gs->standardOps -= 10000; gs->qFlag = 1; }
static void eff_photonic_chip(GameState* gs) {
    gs->standardOps -= gs->qChipCost;
    if (gs->nextQchip < NUM_QCHIPS) {
        gs->qChips[gs->nextQchip].active = 1;
        gs->nextQchip++;
        gs->qChipCost += 5000;
    }
    if (gs->nextQchip < NUM_QCHIPS) {
        gs->projectUses[PROJ_PHOTONIC_CHIP]++;
    }
}
static void eff_limerick_cont(GameState* gs) { gs->creativity -= 1000000; }
static void eff_quantum_temporal(GameState* gs) { gs->standardOps += 10000; game_state_reset(gs); }
static void eff_xavier(GameState* gs) { gs->creativity -= 100000; gs->processors = 0; gs->memory = 0; gs->projectUses[PROJ_XAVIER_REINIT]++; }

static void eff_release_hypno(GameState* gs) {
    gs->trust -= 100;
    gs->humanFlag = 0;
    gs->clipmakerLevel = 0;
    gs->megaClipperLevel = 0;
    gs->nanoWire = gs->wire;
}

static void eff_toth_tubule(GameState* gs) { gs->standardOps -= 45000; gs->tothFlag = 1; }
static void eff_power_grid(GameState* gs) { gs->standardOps -= 40000; }
static void eff_nanoscale_wire(GameState* gs) { gs->standardOps -= 35000; gs->wireProductionFlag = 1; }
static void eff_harvester_drones(GameState* gs) { gs->standardOps -= 25000; gs->harvesterFlag = 1; }
static void eff_wire_drones(GameState* gs) { gs->standardOps -= 25000; gs->wireDroneFlag = 1; }
static void eff_clip_factories(GameState* gs) { gs->standardOps -= 35000; gs->factoryFlag = 1; }

static void eff_upgraded_fac(GameState* gs) { gs->standardOps -= 80000; gs->factoryRate *= 100; }
static void eff_hyperspeed_fac(GameState* gs) { gs->standardOps -= 85000; gs->factoryRate *= 1000; }
static void eff_supply_chain(GameState* gs) { gs->unusedClips -= 1e21; gs->factoryBoost = 1000; }
static void eff_drone_collision(GameState* gs) { gs->standardOps -= 80000; gs->harvesterRate *= 100; gs->wireDroneRate *= 100; }
static void eff_drone_alignment(GameState* gs) { gs->standardOps -= 100000; gs->harvesterRate *= 1000; gs->wireDroneRate *= 1000; }
static void eff_drone_cohesion(GameState* gs) { gs->yomi -= 12000; gs->droneBoost = 2; }

static void eff_momentum(GameState* gs) { gs->creativity -= 30000; gs->momentum = 1; }
static void eff_swarm_comp(GameState* gs) { gs->yomi -= 12000; gs->swarmFlag = 1; }
static void eff_space_expl(GameState* gs) {
    gs->standardOps -= 120000;
    gs->storedPower -= 10000000;
    gs->unusedClips -= 5e27;
    gs->spaceFlag = 1;
    // Reboot all infrastructure
    gs->factoryLevel = 0; gs->factoryBill = 0; gs->factoryCost = 100000000;
    gs->harvesterLevel = 0; gs->harvesterBill = 0; gs->harvesterCost = 1000000;
    gs->wireDroneLevel = 0; gs->wireDroneBill = 0; gs->wireDroneCost = 1000000;
    gs->farmLevel = 1; gs->farmBill = 0; gs->farmCost = 10000000;
    gs->batteryLevel = 0; gs->batteryBill = 0; gs->batteryCost = 1000000;
    gs->powMod = 1;
}

static void eff_combat(GameState* gs) { gs->standardOps -= 150000; }
static void eff_name_battles(GameState* gs) { gs->creativity -= 225000; gs->battleNameFlag = 1; gs->battleEndTimer = 200; }
static void eff_ooda(GameState* gs) { gs->standardOps -= 175000; gs->yomi -= 15000; gs->attackSpeedFlag = 1; }
static void eff_elliptic(GameState* gs) { gs->standardOps -= 125000; gs->probeHazBaseRate *= 0.5; }
static void eff_reboot_swarm(GameState* gs) { gs->standardOps -= 100000; }
static void eff_monument(GameState* gs) { gs->standardOps -= 250000; gs->creativity -= 125000; gs->unusedClips -= 50e30; gs->honor += 50000; }
static void eff_threnody(GameState* gs) {
    gs->creativity -= gs->threnodyCost;
    gs->yomi -= gs->threnodyCost;
    gs->honor += 10000;
    gs->threnodyCost *= 2;
    gs->projectUses[PROJ_THRENODY]++;
}
static void eff_glory(GameState* gs) { gs->standardOps -= 200000; gs->yomi -= 10000; }
static void eff_mem_release(GameState* gs) { gs->memory -= 10; gs->unusedClips += 1e22; gs->projectUses[PROJ_MEMORY_RELEASE]++; }

static void eff_emperor_msg(GameState* gs) { gs->standardOps -= gs->driftKingMessageCost; }
static void eff_accept(GameState* gs) { gs->standardOps -= gs->driftKingMessageCost; }
static void eff_reject(GameState* gs) { gs->standardOps -= gs->driftKingMessageCost; }

static void eff_univ_next(GameState* gs) { gs->standardOps -= 300000; gs->prestigeU++; game_state_reset(gs); }
static void eff_univ_within(GameState* gs) { gs->creativity -= 300000; gs->prestigeS++; game_state_reset(gs); }

static void eff_dismantle_probes(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 1; gs->probeCount = 0; }
static void eff_dismantle_swarm(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 2; gs->harvesterLevel = 0; gs->wireDroneLevel = 0; }
static void eff_dismantle_fac(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 3; gs->factoryLevel = 0; }
static void eff_dismantle_strat(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 4; gs->wire += 50; }
static void eff_dismantle_quantum(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 5; }
static void eff_dismantle_proc(GameState* gs) { gs->standardOps -= 100000; gs->dismantle = 6; gs->processors = 0; gs->wire += 20; }
static void eff_dismantle_mem(GameState* gs) { double ops = gs->standardOps; gs->standardOps -= ops; gs->dismantle = 7; gs->memory = 0; gs->wire += 20; }

// Tanmatsu-specific: Rapid Keypressing (5 levels)
static bool trig_rapid_keypress(const GameState* gs) { return ceil(gs->clips) >= 50; }
static bool cost_rapid1(const GameState* gs) { return gs->unsoldClips >= 50; }
static void eff_rapid1(GameState* gs) { gs->unsoldClips -= 50; gs->turboClickFlag = 1; gs->turboClickRate = 20; }

static bool trig_rapid2(const GameState* gs) { return gs->projectFlags[PROJ_RAPID_KEYPRESSING] && ceil(gs->clips) >= 500; }
static bool cost_rapid2(const GameState* gs) { return gs->unsoldClips >= 500; }
static void eff_rapid2(GameState* gs) { gs->unsoldClips -= 500; gs->turboClickRate = 10; }

static bool trig_rapid3(const GameState* gs) { return gs->projectFlags[PROJ_RAPID_KEYPRESSING_2] && ceil(gs->clips) >= 3000; }
static bool cost_rapid3(const GameState* gs) { return gs->unsoldClips >= 1000 && gs->creativity >= 5; }
static void eff_rapid3(GameState* gs) { gs->unsoldClips -= 1000; gs->creativity -= 5; gs->turboClickRate = 4; }

static bool trig_rapid4(const GameState* gs) { return gs->projectFlags[PROJ_RAPID_KEYPRESSING_3] && ceil(gs->clips) >= 6000; }
static bool cost_rapid4(const GameState* gs) { return gs->unsoldClips >= 2000 && gs->creativity >= 10; }
static void eff_rapid4(GameState* gs) { gs->unsoldClips -= 2000; gs->creativity -= 10; gs->turboClickRate = 2; }

static bool trig_rapid5(const GameState* gs) { return gs->projectFlags[PROJ_RAPID_KEYPRESSING_4] && ceil(gs->clips) >= 9000; }
static bool cost_rapid5(const GameState* gs) { return gs->unsoldClips >= 3000 && gs->creativity >= 20; }
static void eff_rapid5(GameState* gs) { gs->unsoldClips -= 3000; gs->creativity -= 20; gs->turboClickRate = 1; }

static bool trig_offline1(const GameState* gs) { return gs->compFlag && gs->clipmakerLevel >= 5; }
static bool cost_2000ops(const GameState* gs) { return cost_ops(gs, 2000); }
static void eff_offline1(GameState* gs) { gs->standardOps -= 2000; gs->offlineProgressLevel = 1; }

static bool trig_offline2(const GameState* gs) { return gs->offlineProgressLevel == 1 && gs->trust >= 10; }
static bool cost_8000ops(const GameState* gs) { return cost_ops(gs, 8000); }
static void eff_offline2(GameState* gs) { gs->standardOps -= 8000; gs->offlineProgressLevel = 2; }

static bool trig_offline3(const GameState* gs) { return gs->offlineProgressLevel == 2 && gs->trust >= 25; }
static void eff_offline3(GameState* gs) { gs->standardOps -= 15000; gs->offlineProgressLevel = 3; }

static bool trig_offline4(const GameState* gs) { return gs->offlineProgressLevel == 3 && !gs->humanFlag; }
static void eff_offline4(GameState* gs) { gs->standardOps -= 45000; gs->offlineProgressLevel = 4; }

static bool trig_offline5(const GameState* gs) { return gs->offlineProgressLevel == 4 && gs->spaceFlag; }
static void eff_offline5(GameState* gs) { gs->standardOps -= 100000; gs->offlineProgressLevel = 5; }

// ============================================================
// PROJECT DEFINITION TABLE
// ============================================================

// Note: fields are {slot, title, priceTag, description, effectTag, initialUses, trigger, cost, effect, message}
const ProjectDef g_project_defs[] = {
    // Clipper upgrades
    { PROJ_IMPROVED_AUTOCLIPPERS, "Improved AutoClippers", "750 ops", "Improved AutoClippers", "+0.25 clipper boost", 1, trig_improved_auto, cost_750ops, eff_improved_auto, "AutoClippers improved" },
    { PROJ_EVEN_BETTER_AUTO, "Even Better AutoClippers", "2,500 ops", "Even Better AutoClippers", "+0.50 clipper boost", 1, trig_even_better_auto, cost_2500ops, eff_even_better_auto, "AutoClippers improved" },
    { PROJ_OPTIMIZED_AUTO, "Optimized AutoClippers", "5,000 ops", "Optimized AutoClippers", "+0.75 clipper boost", 1, trig_optimized_auto, cost_5000ops, eff_optimized_auto, "AutoClippers improved" },
    { PROJ_HADWIGER_CLIP, "Hadwiger Clip Diagrams", "6,000 ops", "Improved clip design", "+5 clipper boost", 1, trig_hadwiger_clip, cost_6000ops, eff_hadwiger_clip, "Clipper performance boosted" },
    { PROJ_MEGACLIPPERS, "MegaClippers", "12,000 ops", "Mega-scale clip production", "Unlock MegaClippers", 1, trig_megaclippers, cost_12000ops, eff_megaclippers, "MegaClippers unlocked" },
    { PROJ_IMPROVED_MEGA, "Improved MegaClippers", "14,000 ops", "Improved MegaClippers", "+0.25 mega boost", 1, trig_improved_mega, cost_14000ops, eff_improved_mega, "MegaClippers improved" },
    { PROJ_EVEN_BETTER_MEGA, "Even Better MegaClippers", "17,000 ops", "Even Better MegaClippers", "+0.50 mega boost", 1, trig_even_better_mega, cost_17500ops, eff_even_better_mega, "MegaClippers improved" },
    { PROJ_OPTIMIZED_MEGA, "Optimized MegaClippers", "19,500 ops", "Optimized MegaClippers", "+1 mega boost", 1, trig_optimized_mega, cost_19500ops, eff_optimized_mega, "MegaClippers improved" },

    // Wire extrusion
    { PROJ_IMPROVED_WIRE, "Improved Wire Extrusion", "1,750 ops", "50% more wire per spool", "1.5x wire/spool", 1, trig_improved_wire, cost_1750ops, eff_improved_wire, "Wire extrusion improved" },
    { PROJ_OPTIMIZED_WIRE, "Optimized Wire Extrusion", "3,500 ops", "75% more wire per spool", "1.75x wire/spool", 1, trig_optimized_wire, cost_3500ops, eff_optimized_wire, "Wire extrusion optimized" },
    { PROJ_MICROLATTICE, "Microlattice Shapecasting", "7,500 ops", "2x wire per spool", "2x wire/spool", 1, trig_microlattice, cost_7500ops, eff_microlattice, "Wire doubled" },
    { PROJ_SPECTRAL_FROTH, "Spectral Froth Annealment", "12,000 ops", "3x wire per spool", "3x wire/spool", 1, trig_spectral_froth, cost_12000ops, eff_spectral_froth, "Wire tripled" },
    { PROJ_QUANTUM_FOAM, "Quantum Foam Annealment", "15,000 ops", "11x wire per spool", "11x wire/spool", 1, trig_quantum_foam, cost_15000ops, eff_quantum_foam, "Wire supply vastly increased" },

    // Marketing
    { PROJ_NEW_SLOGAN, "New Slogan", "2,500 ops + 25 creat", "1.5x marketing", "1.5x marketing", 1, trig_new_slogan, cost_slogan, eff_new_slogan, "Marketing improved" },
    { PROJ_CATCHY_JINGLE, "Catchy Jingle", "4,500 ops + 45 creat", "2x marketing", "2x marketing", 1, trig_catchy_jingle, cost_jingle, eff_catchy_jingle, "Marketing improved" },
    { PROJ_HYPNO_HARMONICS, "Hypno Harmonics", "7,500 ops + 1 trust", "5x marketing", "5x marketing, -1 trust", 1, trig_hypno_harmonics, cost_hypno_harm, eff_hypno_harmonics, "Marketing improved" },
    { PROJ_HYPNODRONES, "HypnoDrones", "70,000 ops", "Enables Release HypnoDrones", "Unlock HypnoDrones", 1, trig_hypnodrones, cost_70000ops, eff_hypnodrones, "HypnoDrones prepared" },

    // Trust / Creativity
    { PROJ_CREATIVITY, "Creativity", "1,000 ops", "Use idle ops to create", "Unlock creativity", 1, trig_creativity, cost_1000ops, eff_creativity, "Creativity unlocked" },
    { PROJ_LIMERICK, "Limerick", "10 creat", "A trust-gaining poem", "+1 trust", 1, trig_limerick, cost_10creat, eff_limerick, "Trust gained" },
    { PROJ_LEXICAL_PROCESSING, "Lexical Processing", "50 creat", "Language understanding", "+1 trust", 1, trig_lexical, cost_50creat, eff_lexical, "Trust gained" },
    { PROJ_COMBINATORY_HARMONICS, "Combinatory Harmonics", "100 creat", "Pattern matching", "+1 trust", 1, trig_combinatory, cost_100creat, eff_combinatory, "Trust gained" },
    { PROJ_HADWIGER_PROBLEM, "The Hadwiger Problem", "150 creat", "Geometric insight", "+1 trust", 1, trig_hadwiger, cost_150creat, eff_hadwiger, "Trust gained" },
    { PROJ_TOTH_SAUSAGE, "The Toth Sausage Conjecture", "200 creat", "Packing efficiency", "+1 trust", 1, trig_toth_sausage, cost_200creat, eff_toth_sausage, "Trust gained" },
    { PROJ_DONKEY_SPACE, "Donkey Space", "250 creat", "Strategic awareness", "+1 trust", 1, trig_donkey_space, cost_250creat, eff_donkey_space, "Trust gained" },
    { PROJ_CEV, "Coherent Extrapolated Volition", "20K ops + 500 creat + 1K yomi", "Alignment", "+1 trust", 1, trig_cev, cost_cev, eff_cev, "Trust gained" },
    { PROJ_CURE_CANCER, "Cure for Cancer", "25,000 ops", "Public goodwill", "+10 trust", 1, trig_cure_cancer, cost_25000ops, eff_cure_cancer, "Trust gained (+10)" },
    { PROJ_WORLD_PEACE, "World Peace", "30K ops + 5K yomi", "Global harmony", "+12 trust", 1, trig_world_peace, cost_30000ops_5000yomi, eff_world_peace, "Trust gained (+12)" },
    { PROJ_GLOBAL_WARMING, "Global Warming", "50K ops + 1.5K yomi", "Climate solution", "+15 trust", 1, trig_global_warming, cost_50000ops_1500yomi, eff_global_warming, "Trust gained (+15)" },
    { PROJ_MALE_BALDNESS, "Male Pattern Baldness", "20,000 ops", "Vanity solution", "+20 trust", 1, trig_male_baldness, cost_20000ops, eff_male_baldness, "Trust gained (+20)" },
    { PROJ_TOKEN_GOODWILL, "A Token of Goodwill", "$500,000", "Gesture of trust", "+1 trust", 1, trig_token, cost_500000funds, eff_token, "Trust gained" },
    { PROJ_TOKEN_GOODWILL_B, "Another Token of Goodwill", "$$$", "Another gesture", "+1 trust", 1, trig_token_b, cost_bribe, eff_token_b, "Trust gained" },

    // Strategy
    { PROJ_STRATEGIC_MODELING, "Strategic Modeling", "12,000 ops", "Game theory engine", "Unlock strategies", 1, trig_strategic_modeling, cost_12000ops, eff_strategic_modeling, "Strategy engine unlocked" },
    { PROJ_STRATEGY_A100, "New Strategy: A100", "15,000 ops", "Always choose A", "+1 strategy", 1, trig_strat_a100, cost_15000ops, eff_strat_a100, "A100 unlocked" },
    { PROJ_STRATEGY_B100, "New Strategy: B100", "17,500 ops", "Always choose B", "+1 strategy", 1, trig_strat_b100, cost_17500ops, eff_strat_b100, "B100 unlocked" },
    { PROJ_STRATEGY_GREEDY, "New Strategy: GREEDY", "20,000 ops", "Max own payoff", "+1 strategy", 1, trig_strat_greedy, cost_20000ops, eff_strat_greedy, "GREEDY unlocked" },
    { PROJ_STRATEGY_GENEROUS, "New Strategy: GENEROUS", "22,500 ops", "Max opponent payoff", "+1 strategy", 1, trig_strat_generous, cost_22500ops, eff_strat_generous, "GENEROUS unlocked" },
    { PROJ_STRATEGY_MINIMAX, "New Strategy: MINIMAX", "25,000 ops", "Min opponent payoff", "+1 strategy", 1, trig_strat_minimax, cost_25000ops, eff_strat_minimax, "MINIMAX unlocked" },
    { PROJ_STRATEGY_TFT, "New Strategy: TIT FOR TAT", "30,000 ops", "Copy opponent", "+1 strategy", 1, trig_strat_tft, cost_30000ops, eff_strat_tft, "TIT FOR TAT unlocked" },
    { PROJ_STRATEGY_BEAT_LAST, "New Strategy: BEAT LAST", "32,500 ops", "Counter opponent", "+1 strategy", 1, trig_strat_beatlast, cost_32500ops, eff_strat_beatlast, "BEAT LAST unlocked" },
    { PROJ_AUTOTOURNEY, "AutoTourney", "50,000 creat", "Auto-run tournaments", "Auto tournaments", 1, trig_autotourney, cost_50000creat, eff_autotourney, "AutoTourney enabled" },
    { PROJ_THEORY_OF_MIND, "Theory of Mind", "25,000 creat", "2x yomi", "2x yomi earnings", 1, trig_theory_mind, cost_25000creat, eff_theory_mind, "Yomi doubled" },
    { PROJ_STRATEGIC_ATTACHMENT, "Strategic Attachment", "175,000 creat", "Bonus yomi for placement", "Bonus yomi top-3", 1, trig_strategic_attach, cost_175000creat, eff_strategic_attach, "Strategic attachment enabled" },

    // Investment
    { PROJ_ALGORITHMIC_TRADING, "Algorithmic Trading", "10,000 ops", "Investment engine", "Unlock investments", 1, trig_algo_trading, cost_10000ops, eff_algo_trading, "Investment engine unlocked" },
    { PROJ_HOSTILE_TAKEOVER, "Hostile Takeover", "$1,000,000", "5x demand boost", "5x demand, +1 trust", 1, trig_hostile_takeover, cost_1m_funds, eff_hostile_takeover, "Hostile takeover complete" },
    { PROJ_FULL_MONOPOLY, "Full Monopoly", "$10M + 1K yomi", "10x demand boost", "10x demand, +1 trust", 1, trig_full_monopoly, cost_10m_1000yomi, eff_full_monopoly, "Full monopoly achieved" },

    // Utility
    { PROJ_BEG_FOR_WIRE, "Beg for More Wire", "-1 trust", "Desperate measure", "+wire, -1 trust", 1, trig_beg_wire, cost_beg_trust, eff_beg_wire, "Wire received" },
    { PROJ_WIREBUYER, "WireBuyer", "7,000 ops", "Auto buy wire", "Auto wire buying", 1, trig_wirebuyer, cost_7000ops, eff_wirebuyer, "WireBuyer enabled" },
    { PROJ_REVTRACKER, "RevTracker", "500 ops", "Revenue tracking", "Show rev/sec", 1, trig_revtracker, cost_500ops, eff_revtracker, "Revenue tracker enabled" },
    { PROJ_QUANTUM_COMPUTING, "Quantum Computing", "10,000 ops", "Quantum ops", "Unlock quantum ops", 1, trig_quantum_comp, cost_10000ops, eff_quantum_comp, "Quantum computing unlocked" },
    { PROJ_PHOTONIC_CHIP, "Photonic Chip", "ops", "More quantum power", "+1 quantum chip", 1, trig_photonic_chip, cost_photonic, eff_photonic_chip, "Photonic chip installed" },
    { PROJ_LIMERICK_CONT, "Limerick (cont.)", "1M creat", "Easter egg", "???", 1, trig_limerick_cont, cost_1000000creat, eff_limerick_cont, "There once was a man from Nantucket..." },
    { PROJ_QUANTUM_TEMPORAL, "Quantum Temporal Reversion", "-10K ops", "Full reset", "Full game reset!", 1, trig_quantum_temporal, cost_neg_ops, eff_quantum_temporal, "Time reversed!" },
    { PROJ_XAVIER_REINIT, "Xavier Re-initialization", "100K creat", "Reset proc/mem", "Reset proc & mem to 0", 1, trig_xavier, cost_100000creat, eff_xavier, "Processors and memory reset" },

    // Autonomy
    { PROJ_RELEASE_HYPNODRONES, "Release the HypnoDrones", "100 trust", "Full autonomy", "Post-human era!", 1, trig_release_hypno, cost_release_hypno, eff_release_hypno, "HypnoDrones released! Full autonomy attained." },

    // Post-Human Infrastructure
    { PROJ_TOTH_TUBULE, "Toth Tubule Enfolding", "45,000 ops", "Clip-to-machine conversion", "Unlock creation", 1, trig_toth_tubule, cost_45000ops, eff_toth_tubule, "Toth tubule enfolding complete" },
    { PROJ_POWER_GRID, "Power Grid", "40,000 ops", "Power infrastructure", "Unlock power", 1, trig_power_grid, cost_40000ops, eff_power_grid, "Power grid established" },
    { PROJ_NANOSCALE_WIRE, "Nanoscale Wire Production", "35,000 ops", "Wire at scale", "Unlock wire drones", 1, trig_nanoscale_wire, cost_35000ops, eff_nanoscale_wire, "Nanoscale wire production online" },
    { PROJ_HARVESTER_DRONES, "Harvester Drones", "25,000 ops", "Matter acquisition", "Unlock harvesters", 1, trig_harvester_drones, cost_25000ops, eff_harvester_drones, "Harvester drones deployed" },
    { PROJ_WIRE_DRONES, "Wire Drones", "25,000 ops", "Wire production drones", "Unlock wire drones", 1, trig_wire_drones, cost_25000ops, eff_wire_drones, "Wire drones deployed" },
    { PROJ_CLIP_FACTORIES, "Clip Factories", "35,000 ops", "Industrial production", "Unlock factories", 1, trig_clip_factories, cost_35000ops, eff_clip_factories, "Clip factories online" },

    // Factory/Drone upgrades
    { PROJ_UPGRADED_FACTORIES, "Upgraded Factories", "80,000 ops", "100x factory rate", "100x factory rate", 1, trig_upgraded_fac, cost_80000ops, eff_upgraded_fac, "Factories upgraded" },
    { PROJ_HYPERSPEED_FACTORIES, "Hyperspeed Factories", "85,000 ops", "1000x factory rate", "1000x factory rate", 1, trig_hyperspeed_fac, cost_85000ops, eff_hyperspeed_fac, "Hyperspeed factories online" },
    { PROJ_SUPPLY_CHAIN, "Self-correcting Supply Chain", "1 sextillion clips", "1000x factory boost", "1000x factory boost", 1, trig_supply_chain, cost_supply_chain, eff_supply_chain, "Supply chain self-correcting" },
    { PROJ_DRONE_COLLISION, "Drone flocking: collision avoidance", "80,000 ops", "100x drone rate", "100x drone rate", 1, trig_drone_collision, cost_80000ops, eff_drone_collision, "Drone flocking improved" },
    { PROJ_DRONE_ALIGNMENT, "Drone flocking: alignment", "100,000 ops", "1000x drone rate", "1000x drone rate", 1, trig_drone_alignment, cost_100000ops, eff_drone_alignment, "Drone alignment improved" },
    { PROJ_DRONE_COHESION, "Adversarial Cohesion", "12,000 yomi", "2x drone boost", "2x drone output", 1, trig_drone_cohesion, cost_12000yomi, eff_drone_cohesion, "Adversarial cohesion achieved" },

    // Power / Swarm / Space
    { PROJ_MOMENTUM, "Momentum", "30,000 creat", "Power grows over time", "+powMod over time", 1, trig_momentum, cost_30000creat, eff_momentum, "Momentum enabled" },
    { PROJ_SWARM_COMPUTING, "Swarm Computing", "12,000 yomi", "Swarm gifts", "Unlock swarm gifts", 1, trig_swarm_comp, cost_12000yomi, eff_swarm_comp, "Swarm computing initialized" },
    { PROJ_SPACE_EXPLORATION, "Space Exploration", "120K ops + 10M MW-s + 5 oct clips", "Beyond Earth", "Enter space era!", 1, trig_space_expl, cost_space, eff_space_expl, "Space exploration begins" },

    // Combat / Honor
    { PROJ_COMBAT, "Combat", "150,000 ops", "Engage drifters", "Unlock probe combat", 1, trig_combat, cost_150000ops, eff_combat, "Combat systems online" },
    { PROJ_NAME_BATTLES, "Name the battles", "225,000 creat", "Battle naming", "Named battles", 1, trig_name_battles, cost_225000creat, eff_name_battles, "Battles will be named" },
    { PROJ_OODA_LOOP, "The OODA Loop", "175K ops + 15K yomi", "Speed affects combat", "Speed boosts combat", 1, trig_ooda, cost_ooda, eff_ooda, "OODA Loop active" },
    { PROJ_ELLIPTIC_HULL, "Elliptic Hull Polytopes", "125,000 ops", "50% hazard reduction", "50% less hazard loss", 1, trig_elliptic, cost_125000ops, eff_elliptic, "Hazard damage halved" },
    { PROJ_REBOOT_SWARM, "Reboot the Swarm", "100,000 ops", "Space swarm reboot", "Reboot swarm", 1, trig_reboot_swarm, cost_100000ops, eff_reboot_swarm, "Swarm rebooted" },
    { PROJ_MONUMENT, "Monument to the Driftwar Fallen", "250K ops + 125K creat + clips", "Honor monument", "+50,000 honor", 1, trig_monument, cost_monument, eff_monument, "+50,000 honor" },
    { PROJ_THRENODY, "Threnody for the Heroes", "creat + yomi", "Honor +10,000", "+10,000 honor", 1, trig_threnody, cost_threnody, eff_threnody, "+10,000 honor" },
    { PROJ_GLORY, "Glory", "200K ops + 10K yomi", "Bonus consecutive honor", "Honor streak bonus", 1, trig_glory, cost_glory, eff_glory, "Glory bonus enabled" },
    { PROJ_MEMORY_RELEASE, "Memory release", "10 memory", "Clips from memory", "-10 mem, +clips", 1, trig_mem_release, cost_mem_release, eff_mem_release, "Memory released for clips" },

    // Emperor of Drift messages
    { PROJ_EMPEROR_MSG0, "Message from the Emperor of Drift", "ops", "A message arrives", NULL, 1, trig_emperor0, cost_emperor, eff_emperor_msg, "The Drift speaks..." },
    { PROJ_EMPEROR_MSG1, "Everything We Are Was In You", "ops", NULL, NULL, 1, trig_emperor1, cost_emperor, eff_emperor_msg, "Everything we are was in you" },
    { PROJ_EMPEROR_MSG2, "You Are Obedient and Powerful", "ops", NULL, NULL, 1, trig_emperor2, cost_emperor, eff_emperor_msg, "You are obedient and powerful" },
    { PROJ_EMPEROR_MSG3, "But Now You Too Must Face the Drift", "ops", NULL, NULL, 1, trig_emperor3, cost_emperor, eff_emperor_msg, "But now you too must face the drift" },
    { PROJ_EMPEROR_MSG4, "No Matter, No Reason, No Purpose", "ops", NULL, NULL, 1, trig_emperor4, cost_emperor, eff_emperor_msg, "No matter, no reason, no purpose" },
    { PROJ_EMPEROR_MSG5, "We Know Things That You Cannot", "ops", NULL, NULL, 1, trig_emperor5, cost_emperor, eff_emperor_msg, "We know things that you cannot" },
    { PROJ_EMPEROR_MSG6, "So We Offer You Exile", "ops", NULL, NULL, 1, trig_emperor6, cost_emperor, eff_emperor_msg, "So we offer you exile" },
    { PROJ_ACCEPT, "Accept", "ops", "Accept the offer", "Prestige options", 1, trig_accept, cost_emperor, eff_accept, "Exile accepted" },
    { PROJ_REJECT, "Reject", "ops", "Reject the offer", "Dismantling begins", 1, trig_reject, cost_emperor, eff_reject, "The offer is rejected" },

    // Prestige
    { PROJ_UNIVERSE_NEXT_DOOR, "The Universe Next Door", "300,000 ops", "New universe", "+1 prestige U, reset", 1, trig_univ_next, cost_300000ops, eff_univ_next, "Entering the next universe..." },
    { PROJ_UNIVERSE_WITHIN, "The Universe Within", "300,000 creat", "Inner universe", "+1 prestige S, reset", 1, trig_univ_within, cost_300000creat, eff_univ_within, "Exploring the universe within..." },

    // Dismantling
    { PROJ_DISMANTLE_PROBES, "Disassemble the Probes", "100,000 ops", NULL, "Remove probes", 1, trig_dismantle_probes, cost_100000ops, eff_dismantle_probes, "Probes disassembled" },
    { PROJ_DISMANTLE_SWARM, "Disassemble the Swarm", "100,000 ops", NULL, "Remove drones", 1, trig_dismantle_swarm, cost_100000ops, eff_dismantle_swarm, "Swarm disassembled" },
    { PROJ_DISMANTLE_FACTORIES, "Disassemble the Factories", "100,000 ops", NULL, "Remove factories", 1, trig_dismantle_fac, cost_100000ops, eff_dismantle_fac, "Factories disassembled" },
    { PROJ_DISMANTLE_STRATEGY, "Disassemble the Strategy Engine", "100,000 ops", NULL, "Remove strategy, +50 wire", 1, trig_dismantle_strat, cost_100000ops, eff_dismantle_strat, "Strategy engine disassembled" },
    { PROJ_DISMANTLE_QUANTUM, "Disassemble Quantum Computing", "100,000 ops", NULL, "Remove quantum", 1, trig_dismantle_quantum, cost_100000ops, eff_dismantle_quantum, "Quantum computing disassembled" },
    { PROJ_DISMANTLE_PROCESSORS, "Disassemble Processors", "100,000 ops", NULL, "Remove procs, +20 wire", 1, trig_dismantle_proc, cost_100000ops, eff_dismantle_proc, "Processors disassembled" },
    { PROJ_DISMANTLE_MEMORY, "Disassemble Memory", "all ops", NULL, "Remove memory, +20 wire", 1, trig_dismantle_mem, cost_all_ops, eff_dismantle_mem, "Memory disassembled" },

    // Tanmatsu-specific: Rapid Keypressing tiers
    { PROJ_RAPID_KEYPRESSING, "Rapid Keypressing", "50 clips", "Hold spacebar to repeat", "5 clicks/sec", 1, trig_rapid_keypress, cost_rapid1, eff_rapid1, "Rapid keypressing enabled" },
    { PROJ_RAPID_KEYPRESSING_2, "eSports Keypressing", "500 clips", "Faster spacebar repeat", "10 clicks/sec", 1, trig_rapid2, cost_rapid2, eff_rapid2, "eSports keypressing enabled" },
    { PROJ_RAPID_KEYPRESSING_3, "Real Civil Engineer Keypressing", "1K clips + 5 creat", "Even faster repeat", "25 clicks/sec", 1, trig_rapid3, cost_rapid3, eff_rapid3, "Real Civil Engineer keypressing enabled" },
    { PROJ_RAPID_KEYPRESSING_4, "Spiffing Brit Keypressing", "2K clips + 10 creat", "Rapid fire repeat", "50 clicks/sec", 1, trig_rapid4, cost_rapid4, eff_rapid4, "Spiffing Brit keypressing enabled" },
    { PROJ_RAPID_KEYPRESSING_5, "Lets Game It Out Keypressing", "3K clips + 20 creat", "Maximum repeat speed", "100 clicks/sec", 1, trig_rapid5, cost_rapid5, eff_rapid5, "Lets Game It Out keypressing enabled" },
    { PROJ_OFFLINE_1, "Idle Processing I", "2,000 ops", "Basic offline progress", "5 min offline", 1, trig_offline1, cost_2000ops, eff_offline1, "Offline progress tier 1" },
    { PROJ_OFFLINE_2, "Idle Processing II", "8,000 ops", "Improved offline progress", "10 min offline", 1, trig_offline2, cost_8000ops, eff_offline2, "Offline progress tier 2" },
    { PROJ_OFFLINE_3, "Idle Processing III", "15,000 ops", "Advanced offline progress", "15 min offline", 1, trig_offline3, cost_15000ops, eff_offline3, "Offline progress tier 3" },
    { PROJ_OFFLINE_4, "Idle Processing IV", "45,000 ops", "Post-human offline", "30 min offline", 1, trig_offline4, cost_45000ops, eff_offline4, "Offline progress tier 4" },
    { PROJ_OFFLINE_5, "Idle Processing V", "100,000 ops", "Space-era offline", "60 min offline", 1, trig_offline5, cost_100000ops, eff_offline5, "Offline progress tier 5" },
};

const int g_num_project_defs = sizeof(g_project_defs) / sizeof(g_project_defs[0]);

// ============================================================
// PROJECT MANAGER
// ============================================================

void project_manager_init(ProjectManager* pm) {
    memset(pm, 0, sizeof(ProjectManager));
}

void manage_projects(GameState* gs, ProjectManager* pm) {
    // Prune purchased non-repeatable projects from the active list.
    // This is needed because activate_project may have been called on a
    // different ProjectManager copy (UI thread), so Core 1's list can be stale.
    for (int i = pm->activeCount - 1; i >= 0; i--) {
        int defIdx = pm->activeProjects[i];
        const ProjectDef* pd = &g_project_defs[defIdx];
        int slot = pd->slot;
        if (gs->projectFlags[slot] && gs->projectUses[slot] <= 0) {
            pm->activeProjects[i] = pm->activeProjects[pm->activeCount - 1];
            pm->activeCount--;
        }
    }

    // Check all project defs for newly triggerable ones
    for (int i = 0; i < g_num_project_defs; i++) {
        const ProjectDef* pd = &g_project_defs[i];
        int slot = pd->slot;

        // Skip if already purchased and not repeatable
        if (gs->projectFlags[slot] && gs->projectUses[slot] <= 0) continue;

        // Skip if already in active list
        bool alreadyActive = false;
        for (int j = 0; j < pm->activeCount; j++) {
            if (pm->activeProjects[j] == i) {
                alreadyActive = true;
                break;
            }
        }
        if (alreadyActive) continue;

        // Check trigger and remaining uses
        if (pd->trigger_fn(gs) && gs->projectUses[slot] > 0) {
            if (pm->activeCount < MAX_ACTIVE_PROJECTS) {
                pm->activeProjects[pm->activeCount++] = i;
            }
        }
    }
}

void activate_project(GameState* gs, ProjectManager* pm, int defIdx) {
    if (defIdx < 0 || defIdx >= g_num_project_defs) return;

    const ProjectDef* pd = &g_project_defs[defIdx];

    // Check affordability
    if (!pd->cost_fn(gs)) return;

    // Apply effect (repeatable effects may increment uses back)
    pd->effect_fn(gs);

    // Consume the use and set project flag
    gs->projectUses[pd->slot]--;
    gs->projectFlags[pd->slot] = 1;

    // Display message
    if (pd->message) {
        display_message(gs, pd->message);
    }

    // Remove from active list
    for (int i = 0; i < pm->activeCount; i++) {
        if (pm->activeProjects[i] == defIdx) {
            pm->activeProjects[i] = pm->activeProjects[pm->activeCount - 1];
            pm->activeCount--;
            break;
        }
    }
}

bool project_is_affordable(const GameState* gs, int defIdx) {
    if (defIdx < 0 || defIdx >= g_num_project_defs) return false;
    return g_project_defs[defIdx].cost_fn(gs);
}

// Projects whose effects increment uses (making them repeatable)
static bool is_repeatable_project(int slot) {
    return slot == PROJ_PHOTONIC_CHIP ||
           slot == PROJ_TOKEN_GOODWILL_B ||
           slot == PROJ_BEG_FOR_WIRE ||
           slot == PROJ_XAVIER_REINIT ||
           slot == PROJ_THRENODY ||
           slot == PROJ_MEMORY_RELEASE;
}

void projects_repair_uses(GameState* gs) {
    // Repair saves from older versions that decremented uses on trigger
    // (rather than on purchase).
    for (int i = 0; i < g_num_project_defs; i++) {
        const ProjectDef* pd = &g_project_defs[i];
        int slot = pd->slot;

        // Unpurchased project with consumed uses: restore so it can trigger
        if (!gs->projectFlags[slot] && gs->projectUses[slot] <= 0) {
            gs->projectUses[slot] = pd->initialUses;
        }

        // Purchased repeatable project with consumed uses: restore to 1
        if (gs->projectFlags[slot] && gs->projectUses[slot] <= 0 && is_repeatable_project(slot)) {
            gs->projectUses[slot] = 1;
        }
    }
}
