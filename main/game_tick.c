#include "game_tick.h"

#include <math.h>

#include "esp_log.h"
#include "game_combat.h"
#include "game_compute.h"
#include "game_economics.h"
#include "game_endgame.h"
#include "game_investment.h"
#include "game_post_human.h"
#include "game_projects.h"
#include "game_save.h"
#include "game_space.h"
#include "game_tournament.h"

static const char* TAG = "game_tick";

// Slow loop counter (replaces JS secTimer)
static int s_secTimer  = 0;
static int s_saveTimer = 0;

void game_tick(GameState* gs, ProjectManager* pm) {
    gs->ticks++;

    // === Main loop (every tick = 10ms) ===

    // 1-2. Milestone checks (called twice per tick per original JS)
    milestone_check(gs);

    // 4. Operations generation
    if (gs->compFlag) {
        calculate_operations(gs);
    }

    // 5. Trust from clip milestones
    if (gs->humanFlag) {
        calculate_trust(gs);
    }

    // 6. Quantum chip value updates
    if (gs->qFlag) {
        quantum_compute(gs);
    }

    // 8. Manage projects
    manage_projects(gs, pm);

    // 9. Second milestone check
    milestone_check(gs);

    // 10. Clip rate tracking (every 100 ticks)
    update_clip_rate(gs);

    // 11. Stock report counter (every 10000 ticks)
    // (handled in stock_display_update)

    // 12. Auto wire buyer
    if (gs->wireBuyerFlag && gs->wireBuyerStatus && gs->wire <= 1) {
        buy_wire(gs);
    }

    // 13. Space exploration
    explore_universe(gs);

    // 15. Power update
    update_power(gs);

    // 16. Swarm computing
    update_swarm(gs);

    // 17-18. Matter acquisition and wire processing
    acquire_matter(gs);
    process_matter(gs);

    // 19. Factory production (skipped if dismantle >= 4)
    if (gs->dismantle < 4 && gs->factoryLevel > 0 && !gs->humanFlag) {
        double fbst = 1;
        if (gs->factoryBoost > 1) {
            fbst = gs->factoryBoost * gs->factoryLevel;
        }
        double factoryClips = gs->powMod * fbst * floor(gs->factoryLevel) * gs->factoryRate;
        if (factoryClips > 0) {
            clip_click(gs, factoryClips);
        }
    }

    // 20. Space-era per-tick updates
    if (gs->spaceFlag == 1) {
        encounter_hazards(gs);
        spawn_factories(gs);
        spawn_harvesters(gs);
        spawn_wire_drones(gs);
        spawn_probes(gs);
        drift(gs);
        check_for_battles(gs);
        update_battles(gs);
    }

    // 21. AutoClipper production (skipped if dismantle >= 4)
    if (gs->dismantle < 4) {
        double autoAmount = gs->clipperBoost * gs->clipmakerLevel / 100.0;
        if (autoAmount > 0) {
            clip_click(gs, autoAmount);
        }

        double megaAmount = gs->megaClipperBoost * gs->megaClipperLevel * 5.0;
        if (megaAmount > 0) {
            clip_click(gs, megaAmount);
        }
    }

    // Turbo-click repeat (driven by Core 0 spacebar input)
    // turboAction: 0=off, 1=clip, 2=quantum. Runs at 100Hz / turboClickRate.
    if (gs->turboClickFlag && gs->turboAction > 0) {
        gs->turboCounter++;
        if (gs->turboCounter >= gs->turboClickRate) {
            gs->turboCounter = 0;
            if (gs->turboAction == 1) {
                clip_click(gs, 1);
            } else if (gs->turboAction == 2) {
                quantum_compute_action(gs);
            }
        }
    } else {
        gs->turboCounter = 0;
    }

    // 22. Demand curve (humanFlag==1)
    update_demand(gs);

    // 23. Creativity generation
    calculate_creativity(gs);

    // Tournament: start if requested by UI (Core 0 sets global, Core 1 runs it)
    extern volatile int g_tourney_requested;
    if (g_tourney_requested) {
        g_tourney_requested = 0;
        new_tourney(gs);
    }

    // Tournament: run one round per tick while in progress
    if (gs->tourneyInProg) {
        run_tourney(gs);
    }

    // AutoTourney: start new tournament after results shown for 3 seconds
    auto_tourney_tick(gs);

    // qFade decay (visual only but tracked in state)
    gs->qFade -= 0.001;

    // 24. Endgame sequence
    endgame_tick(gs);

    // === Slow loop (every 10th tick = 100ms) ===
    if ((int)gs->ticks % 10 == 0) {
        // Wire price fluctuation
        adjust_wire_price(gs);

        // Sales (humanFlag==1)
        if (gs->humanFlag) {
            sales_tick(gs);

            // Revenue calculation every 1 second (every 10 slow ticks)
            s_secTimer++;
            if (s_secTimer >= 10) {
                calculate_rev(gs);
                s_secTimer = 0;
            }
        }

        // Stock display update
        stock_display_update(gs);
    }

    // === Stock timers ===
    if ((int)gs->ticks % 100 == 0) {
        stock_shop(gs);
    }
    if ((int)gs->ticks % 250 == 0) {
        stock_sell_update(gs);
    }

    // === Auto-save (every 2500 ticks = 25 seconds) ===
    // Snapshots the state; actual SD write happens async on save task
    s_saveTimer++;
    if (s_saveTimer >= 2500) {
        game_save_request(gs);
        s_saveTimer = 0;
    }

    // Log state periodically
    if ((int)gs->ticks % 1000 == 0) {
        ESP_LOGI(TAG, "t=%d clips=%.0f wire=%.0f $=%.2f unsold=%.0f auto=%d mega=%d",
                 (int)gs->ticks, gs->clips, gs->wire, gs->funds, gs->unsoldClips,
                 (int)gs->clipmakerLevel, (int)gs->megaClipperLevel);
    }
}
