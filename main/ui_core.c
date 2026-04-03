#include "ui_core.h"

#include <math.h>
#include <string.h>

#include "game_cheats.h"
#include "game_compute.h"
#include "game_economics.h"
#include "game_investment.h"
#include "game_post_human.h"
#include "game_save.h"
#include "game_space.h"

// Must match ui_render.c layout constants for project boxes
#define PROJ_VISIBLE_MAX 7

void ui_init(UIState* ui) {
    memset(ui, 0, sizeof(UIState));
    ui->visibleTabs[0]  = TAB_CLIPS;
    ui->tabVisible[0]   = true;
    ui->visibleTabCount = 1;
    ui->activeTabIdx    = 0;
    ui->saveSlotIdx     = 1;
    ui->slotCacheDirty  = true;
}

// Refresh the cached slot info (reads from SD — only call when needed)
static void refresh_slot_cache(UIState* ui) {
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        ui->slotExists[i] = (game_save_peek(i, &ui->slotCache[i]) == 0);
    }
    ui->slotCacheDirty = false;
}

// Helper: assign a tab to a fixed F-key slot
static void set_tab(UIState* ui, int* n, TabID tab, bool visible) {
    if (*n < MAX_VISIBLE_TABS) {
        ui->visibleTabs[*n] = tab;
        ui->tabVisible[*n]  = visible;
        (*n)++;
    }
}

void ui_update_tabs(UIState* ui, const GameState* gs) {
    int n = 0;

    if (gs->spaceFlag) {
        // Space era: F1=Clips F2=Drones F3=Compute F4=Projects F5=Strategy F6=Power
        set_tab(ui, &n, TAB_CLIPS,    true);
        set_tab(ui, &n, TAB_DRONES,   gs->dismantle < 2);
        set_tab(ui, &n, TAB_COMPUTE,  gs->compFlag && gs->dismantle < 7);
        set_tab(ui, &n, TAB_PROJECTS, gs->projectsFlag && gs->dismantle < 7);
        set_tab(ui, &n, TAB_STRATEGY, gs->strategyEngineFlag && gs->dismantle < 4);
        set_tab(ui, &n, TAB_POWER,    gs->dismantle < 3);
    } else if (!gs->humanFlag) {
        // Post-human: F1=Clips F2=Drones F3=Compute F4=Projects F5=Strategy F6=Power
        set_tab(ui, &n, TAB_CLIPS,    true);
        set_tab(ui, &n, TAB_DRONES,   true);
        set_tab(ui, &n, TAB_COMPUTE,  gs->compFlag);
        set_tab(ui, &n, TAB_PROJECTS, gs->projectsFlag);
        set_tab(ui, &n, TAB_STRATEGY, gs->strategyEngineFlag);
        set_tab(ui, &n, TAB_POWER,    true);
    } else {
        // Human era: F1=Clips F2=Business F3=Compute F4=Projects F5=Strategy F6=Invest
        set_tab(ui, &n, TAB_CLIPS,    true);
        set_tab(ui, &n, TAB_BUSINESS, gs->autoClipperFlag || gs->compFlag);
        set_tab(ui, &n, TAB_COMPUTE,  gs->compFlag);
        set_tab(ui, &n, TAB_PROJECTS, gs->projectsFlag);
        set_tab(ui, &n, TAB_STRATEGY, gs->strategyEngineFlag);
        set_tab(ui, &n, TAB_INVEST,   gs->investmentEngineFlag);
    }

    ui->visibleTabCount = n;

    // If active tab is no longer visible, find the nearest visible one
    if (ui->activeTabIdx >= n || !ui->tabVisible[ui->activeTabIdx]) {
        ui->activeTabIdx = 0;  // Clips is always visible
    }
}

TabID ui_active_tab(const UIState* ui) {
    if (ui->activeTabIdx >= 0 && ui->activeTabIdx < ui->visibleTabCount
        && ui->tabVisible[ui->activeTabIdx]) {
        return ui->visibleTabs[ui->activeTabIdx];
    }
    return TAB_CLIPS;
}

int ui_get_multiplier(const UIState* ui) {
    if (ui->ctrlHeld && ui->shiftHeld) return 999999;
    if (ui->altHeld) return 1000;
    if (ui->ctrlHeld) return 100;
    if (ui->shiftHeld) return 10;
    return 1;
}

// === Item count helpers (must match render layout) ===

static int clips_item_count(const GameState* gs) {
    if (gs->humanFlag) {
        int n = 1;  // Make Paperclip
        if (gs->autoClipperFlag) n++;
        if (gs->megaClipperFlag) n++;
        return n;
    } else if (gs->spaceFlag) {
        // 0=Launch, 1=+Trust, 2-9=probe stats, 10=+MaxTrust
        return 11;
    }
    return 0;
}

static int business_item_count(const GameState* gs) {
    int n = 4;  // Raise, Lower, Marketing, Buy Wire
    if (gs->wireBuyerFlag) n++;
    return n;
}

static int compute_item_count(const GameState* gs) {
    int n = 2;  // +Proc, +Mem
    if (gs->qFlag) n++;  // Quantum Compute
    return n;
}

static int strategy_item_count(const GameState* gs) {
    int n = 1;  // New Tournament
    if (gs->autoTourneyFlag) n++;
    return n;
}

static int invest_item_count(const GameState* gs) {
    (void)gs;
    return 3;  // Deposit, Withdraw, Upgrade
}

static int drones_item_count(const GameState* gs) {
    int n = 0;
    if (gs->harvesterFlag) n += 2;  // +Harvester, Reboot Harv
    if (gs->wireDroneFlag) n += 2;  // +WireDrone, Reboot Wire
    return n;
}

static int power_item_count(const GameState* gs) {
    int n = 2;  // +Farm, +Battery
    if (gs->factoryFlag) n++;  // +Factory
    n += 2;  // Reboot Farm, Reboot Battery
    if (gs->factoryFlag) n++;  // Reboot Factory
    if (gs->swarmFlag && gs->boredomFlag) n++;  // Entertain
    if (gs->swarmFlag && gs->disorgFlag) n++;   // Synch
    return n;
}

static void clamp_focus(UIState* ui, TabID tab, int maxItems) {
    if (maxItems <= 0) maxItems = 1;
    if (ui->focusIdx[tab] >= maxItems) ui->focusIdx[tab] = maxItems - 1;
    if (ui->focusIdx[tab] < 0) ui->focusIdx[tab] = 0;
}

// === Input handlers per tab ===

static void handle_clips_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int mult  = ui_get_multiplier(ui);
    int max   = clips_item_count(gs);
    int focus = ui->focusIdx[TAB_CLIPS];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_CLIPS]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_CLIPS]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        if (gs->humanFlag) {
            // Dynamic mapping: items shift when autoClipper/mega are hidden
            int idx = 0;
            if (focus == idx) { for (int i = 0; i < mult; i++) clip_click(gs, 1); return; }
            idx++;
            if (gs->autoClipperFlag) {
                if (focus == idx) { for (int i = 0; i < mult; i++) make_clipper(gs); return; }
                idx++;
            }
            if (gs->megaClipperFlag) {
                if (focus == idx) { for (int i = 0; i < mult; i++) make_mega_clipper(gs); return; }
            }
        } else if (gs->spaceFlag) {
            switch (focus) {
                case 0:  for (int i = 0; i < mult; i++) launch_probe(gs); break;
                case 1:  increase_probe_trust(gs); break;
                case 10: increase_max_trust(gs); break;
                default: break;
            }
        }
    } else if (gs->spaceFlag && focus >= 2 && focus <= 9) {
        int* stats[] = {
            &gs->probeSpeed, &gs->probeNav, &gs->probeRep, &gs->probeHaz,
            &gs->probeFac, &gs->probeHarv, &gs->probeWire, &gs->probeCombat
        };
        int idx = focus - 2;
        if (key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
            raise_probe_stat(gs, stats[idx]);
        } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT) {
            lower_probe_stat(gs, stats[idx]);
        }
    }
}

// Business tab uses dynamic indices: items shift when wireBuyer is hidden
static void handle_business_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int mult  = ui_get_multiplier(ui);
    int max   = business_item_count(gs);
    int focus = ui->focusIdx[TAB_BUSINESS];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_BUSINESS]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_BUSINESS]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        switch (focus) {
            case 0: raise_price(gs); break;
            case 1: lower_price(gs); break;
            case 2: for (int i = 0; i < mult; i++) buy_marketing(gs); break;
            case 3: for (int i = 0; i < mult; i++) buy_wire(gs); break;
            case 4:
                if (gs->wireBuyerFlag) gs->wireBuyerStatus = !gs->wireBuyerStatus;
                break;
        }
    }
    clamp_focus(ui, TAB_BUSINESS, max);
}

static void handle_compute_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int max   = compute_item_count(gs);
    int focus = ui->focusIdx[TAB_COMPUTE];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_COMPUTE]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_COMPUTE]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        int mult = ui_get_multiplier(ui);
        switch (focus) {
            case 0: for (int i = 0; i < mult; i++) add_processor(gs); break;
            case 1: for (int i = 0; i < mult; i++) add_memory(gs); break;
            case 2:
                if (gs->qFlag) {
                    double q = 0;
                    for (int i = 0; i < NUM_QCHIPS; i++) q += gs->qChips[i].value;
                    ui->qCompResult = ceil(q * 360);
                    quantum_compute_action(gs);
                }
                break;
        }
    }
}

static void handle_projects_input(UIState* ui, GameState* gs, ProjectManager* pm, int key, bool pressed) {
    if (!pressed) return;
    int* focus  = &ui->focusIdx[TAB_PROJECTS];
    int* scroll = &ui->scrollOffset[TAB_PROJECTS];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (*focus > 0) (*focus)--;
        if (*focus < *scroll) *scroll = *focus;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (*focus < pm->activeCount - 1) (*focus)++;
        if (*focus >= *scroll + PROJ_VISIBLE_MAX) *scroll = *focus - (PROJ_VISIBLE_MAX - 1);
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        if (*focus >= 0 && *focus < pm->activeCount) {
            int defIdx = pm->activeProjects[*focus];
            activate_project(gs, pm, defIdx);
            if (*focus >= pm->activeCount && pm->activeCount > 0) {
                *focus = pm->activeCount - 1;
            }
        }
    }
    // Clamp after any action (project count may have changed)
    if (pm->activeCount > 0) {
        if (*focus >= pm->activeCount) *focus = pm->activeCount - 1;
    } else {
        *focus = 0;
    }
}

// Ensure pick points to a valid active strategy
static void clamp_pick(GameState* gs) {
    if (gs->pick >= 0 && gs->pick < NUM_STRATEGIES && gs->stratActive[gs->pick]) return;
    // Find first active strategy
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        if (gs->stratActive[i]) { gs->pick = i; return; }
    }
    gs->pick = 0;
}

static void handle_strategy_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int max   = strategy_item_count(gs);
    int focus = ui->focusIdx[TAB_STRATEGY];

    // Ensure pick is valid (may be stale after load)
    clamp_pick(gs);

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_STRATEGY]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_STRATEGY]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT) {
        // Select previous active strategy
        int p = gs->pick - 1;
        while (p >= 0 && !gs->stratActive[p]) p--;
        if (p >= 0) gs->pick = p;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
        // Select next active strategy
        int p = gs->pick + 1;
        while (p < NUM_STRATEGIES && !gs->stratActive[p]) p++;
        if (p < NUM_STRATEGIES) gs->pick = p;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        switch (focus) {
            case 0: {
                    extern volatile int g_tourney_requested;
                    g_tourney_requested = 1;  // Core 1 will start it
                } break;
            case 1: gs->autoTourneyStatus = !gs->autoTourneyStatus; break;
        }
    }
}

static void handle_invest_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int max   = invest_item_count(gs);
    int focus = ui->focusIdx[TAB_INVEST];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_INVEST]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_INVEST]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT) {
        if (gs->riskiness == RISK_HIGH) gs->riskiness = RISK_MED;
        else if (gs->riskiness == RISK_MED) gs->riskiness = RISK_LOW;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
        if (gs->riskiness == RISK_LOW) gs->riskiness = RISK_MED;
        else if (gs->riskiness == RISK_MED) gs->riskiness = RISK_HIGH;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        switch (focus) {
            case 0: invest_deposit(gs); break;
            case 1: invest_withdraw(gs); break;
            case 2: invest_upgrade(gs); break;
        }
    }
}

static void handle_drones_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int mult  = ui_get_multiplier(ui);
    int max   = drones_item_count(gs);
    int focus = ui->focusIdx[TAB_DRONES];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_DRONES]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_DRONES]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        int idx = 0;
        if (gs->harvesterFlag) {
            if (focus == idx) { for (int i = 0; i < mult; i++) make_harvester(gs); goto done; }
            idx++;
        }
        if (gs->wireDroneFlag) {
            if (focus == idx) { for (int i = 0; i < mult; i++) make_wire_drone(gs); goto done; }
            idx++;
        }
        if (gs->harvesterFlag) {
            if (focus == idx) { reboot_harvester(gs); goto done; }
            idx++;
        }
        if (gs->wireDroneFlag) {
            if (focus == idx) { reboot_wire_drone(gs); goto done; }
            idx++;
        }
        done:;
    }
}

// Power tab: dynamic items since factory and swarm buttons are conditional
static void handle_power_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;
    int mult  = ui_get_multiplier(ui);
    int max   = power_item_count(gs);
    int focus = ui->focusIdx[TAB_POWER];

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (focus > 0) ui->focusIdx[TAB_POWER]--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (focus < max - 1) ui->focusIdx[TAB_POWER]++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT) {
        if (gs->sliderPos > 0) gs->sliderPos -= 10;
        if (gs->sliderPos < 0) gs->sliderPos = 0;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
        if (gs->sliderPos < 200) gs->sliderPos += 10;
        if (gs->sliderPos > 200) gs->sliderPos = 200;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        // Map focus to action using dynamic index
        int idx = 0;
        // +Farm
        if (focus == idx) { for (int i = 0; i < mult; i++) make_farm(gs); goto done; }
        idx++;
        // +Battery
        if (focus == idx) { for (int i = 0; i < mult; i++) make_battery(gs); goto done; }
        idx++;
        // +Factory (conditional)
        if (gs->factoryFlag) {
            if (focus == idx) { for (int i = 0; i < mult; i++) make_factory(gs); goto done; }
            idx++;
        }
        // Reboot Farm
        if (focus == idx) { reboot_farm(gs); goto done; }
        idx++;
        // Reboot Battery
        if (focus == idx) { reboot_battery(gs); goto done; }
        idx++;
        // Reboot Factory (conditional)
        if (gs->factoryFlag) {
            if (focus == idx) { reboot_factory(gs); goto done; }
            idx++;
        }
        // Entertain (conditional)
        if (gs->swarmFlag && gs->boredomFlag) {
            if (focus == idx) { entertain_swarm(gs); goto done; }
            idx++;
        }
        // Synch (conditional)
        if (gs->swarmFlag && gs->disorgFlag) {
            if (focus == idx) { synch_swarm(gs); goto done; }
        }
    }
done:
    clamp_focus(ui, TAB_POWER, max);
}

// === Save/Load overlay ===

#define SAVE_MENU_ITEMS (SAVE_SLOT_COUNT + 2)  // slots + New Game + Quit

static void handle_save_load_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;

    if (key == BSP_INPUT_NAVIGATION_KEY_ESC) {
        ui->overlay = OVERLAY_NONE;
        return;
    }

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (ui->saveSlotIdx > 0) ui->saveSlotIdx--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (ui->saveSlotIdx < SAVE_MENU_ITEMS - 1) ui->saveSlotIdx++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT || key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
        ui->saveLoadMode = (ui->saveLoadMode == SAVE_MODE_SAVE) ? SAVE_MODE_LOAD : SAVE_MODE_SAVE;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        extern int  load_game_into_engine(int slot);
        extern void reset_game_in_engine(void);

        if (ui->saveSlotIdx < SAVE_SLOT_COUNT) {
            if (ui->saveLoadMode == SAVE_MODE_SAVE) {
                game_save(gs, ui->saveSlotIdx);
                display_message(gs, "Game saved");
                refresh_slot_cache(ui);  // Update displayed slot info
            } else {
                if (load_game_into_engine(ui->saveSlotIdx) == 0) {
                    display_message(gs, "Game loaded");
                } else {
                    display_message(gs, "No save in this slot");
                }
            }
            ui->overlay = OVERLAY_NONE;
        } else if (ui->saveSlotIdx == SAVE_SLOT_COUNT) {
            reset_game_in_engine();
            ui->overlay = OVERLAY_NONE;
        } else if (ui->saveSlotIdx == SAVE_SLOT_COUNT + 1) {
            extern void bsp_device_restart_to_launcher(void);
            bsp_device_restart_to_launcher();
        }
    }
}

// === Cheat menu overlay ===

#if CHEATS_ENABLED
static void handle_cheat_input(UIState* ui, GameState* gs, int key, bool pressed) {
    if (!pressed) return;

    if (key == BSP_INPUT_NAVIGATION_KEY_ESC) {
        ui->overlay = OVERLAY_NONE;
        return;
    }

    int maxItems = (ui->cheatMenuSection == 0) ? CHEAT_COUNT : JUMP_COUNT;

    if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
        if (ui->cheatMenuIdx > 0) ui->cheatMenuIdx--;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
        if (ui->cheatMenuIdx < maxItems - 1) ui->cheatMenuIdx++;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_LEFT || key == BSP_INPUT_NAVIGATION_KEY_RIGHT) {
        ui->cheatMenuSection = 1 - ui->cheatMenuSection;
        ui->cheatMenuIdx = 0;
    } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
        if (ui->cheatMenuSection == 0) {
            cheat_execute(gs, (CheatAction)ui->cheatMenuIdx);
        } else {
            cheat_jump(gs, (JumpPreset)ui->cheatMenuIdx);
        }
    }
}
#endif

// === Main input handler ===

bool ui_handle_input(UIState* ui, GameState* gs, ProjectManager* pm, const bsp_input_event_t* event) {
    // Track modifier keys
    if (event->type == INPUT_EVENT_TYPE_KEYBOARD) {
        ui->shiftHeld = (event->args_keyboard.modifiers & BSP_INPUT_MODIFIER_SHIFT) != 0;
        ui->ctrlHeld  = (event->args_keyboard.modifiers & BSP_INPUT_MODIFIER_CTRL) != 0;
        ui->altHeld   = (event->args_keyboard.modifiers & BSP_INPUT_MODIFIER_ALT) != 0;
    }

    if (event->type == INPUT_EVENT_TYPE_NAVIGATION) {
        int  key     = event->args_navigation.key;
        bool pressed = event->args_navigation.state;
        uint32_t mods = event->args_navigation.modifiers;

        ui->shiftHeld = (mods & BSP_INPUT_MODIFIER_SHIFT) != 0;
        ui->ctrlHeld  = (mods & BSP_INPUT_MODIFIER_CTRL) != 0;
        ui->altHeld   = (mods & BSP_INPUT_MODIFIER_ALT) != 0;

        // Handle overlays first
        if (ui->overlay == OVERLAY_SAVE_LOAD) {
            handle_save_load_input(ui, gs, key, pressed);
            return true;
        }
#if CHEATS_ENABLED
        if (ui->overlay == OVERLAY_CHEAT_MENU) {
            handle_cheat_input(ui, gs, key, pressed);
            return true;
        }
#endif

        if (!pressed) return false;

        // ESC -> Save/Load menu
        if (key == BSP_INPUT_NAVIGATION_KEY_ESC) {
            ui->overlay = OVERLAY_SAVE_LOAD;
            ui->saveSlotIdx = 1;
            refresh_slot_cache(ui);
            return true;
        }

#if CHEATS_ENABLED
        if (key == BSP_INPUT_NAVIGATION_KEY_F6 && ui->ctrlHeld) {
            ui->overlay = OVERLAY_CHEAT_MENU;
            ui->cheatMenuIdx = 0;
            ui->cheatMenuSection = 0;
            return true;
        }
#endif

        // F1-F6 -> Switch tab (only if that slot is visible)
        if (key >= BSP_INPUT_NAVIGATION_KEY_F1 && key <= BSP_INPUT_NAVIGATION_KEY_F6) {
            int idx = key - BSP_INPUT_NAVIGATION_KEY_F1;
            if (idx < ui->visibleTabCount && ui->tabVisible[idx]) {
                ui->activeTabIdx = idx;
            }
            return true;
        }

        // Route to active tab handler
        TabID tab = ui_active_tab(ui);
        switch (tab) {
            case TAB_CLIPS:    handle_clips_input(ui, gs, key, pressed); break;
            case TAB_BUSINESS: handle_business_input(ui, gs, key, pressed); break;
            case TAB_COMPUTE:  handle_compute_input(ui, gs, key, pressed); break;
            case TAB_PROJECTS: handle_projects_input(ui, gs, pm, key, pressed); break;
            case TAB_STRATEGY: handle_strategy_input(ui, gs, key, pressed); break;
            case TAB_INVEST:   handle_invest_input(ui, gs, key, pressed); break;
            case TAB_DRONES:   handle_drones_input(ui, gs, key, pressed); break;
            case TAB_POWER:    handle_power_input(ui, gs, key, pressed); break;
            default: break;
        }
        return true;
    }

    // Keyboard events: spacebar for clip/quantum
    // Fires once immediately, sets turboAction for Core 1 to repeat at 100Hz
    if (event->type == INPUT_EVENT_TYPE_KEYBOARD) {
        if (event->args_keyboard.ascii == ' ') {
            TabID tab = ui_active_tab(ui);
            if (tab == TAB_CLIPS && gs->humanFlag) {
                clip_click(gs, 1);
                gs->turboAction = 1;  // Core 1 will repeat
            } else if (tab == TAB_COMPUTE && gs->qFlag) {
                // Capture result for display before the action modifies state
                double q = 0;
                for (int i = 0; i < NUM_QCHIPS; i++) q += gs->qChips[i].value;
                ui->qCompResult = ceil(q * 360);
                quantum_compute_action(gs);
                gs->turboAction = 2;  // Core 1 will repeat
            }
            return true;
        }
    }

    return false;
}
