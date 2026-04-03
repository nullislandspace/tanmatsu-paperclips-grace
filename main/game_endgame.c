#include "game_endgame.h"

#include "game_projects.h"

void endgame_tick(GameState* gs) {
    // End timers (driven by project flags)
    if (gs->projectFlags[PROJ_REJECT]) {
        gs->endTimer1++;
    }

    if (gs->projectFlags[PROJ_DISMANTLE_SWARM]) {
        gs->endTimer2++;
    }

    if (gs->projectFlags[PROJ_DISMANTLE_FACTORIES]) {
        gs->endTimer3++;
    }

    if (gs->projectFlags[PROJ_DISMANTLE_STRATEGY]) {
        gs->endTimer4++;
    }

    // Quantum chip dismantling adds wire at specific endTimer4 thresholds
    // Only after dismantle >= 5 (Disassemble Quantum Computing done)
    if (gs->dismantle >= 5) {
        if (gs->endTimer4 == 10) gs->wire += 1;
        if (gs->endTimer4 == 60) gs->wire += 1;
        if (gs->endTimer4 == 100) gs->wire += 1;
        if (gs->endTimer4 == 130) gs->wire += 1;
        if (gs->endTimer4 == 150) gs->wire += 1;
        if (gs->endTimer4 == 160) gs->wire += 1;
        if (gs->endTimer4 == 165) gs->wire += 1;
        if (gs->endTimer4 == 169) gs->wire += 1;
        if (gs->endTimer4 == 172) gs->wire += 1;
        if (gs->endTimer4 == 174) gs->wire += 1;
    }

    if (gs->projectFlags[PROJ_DISMANTLE_PROCESSORS]) {
        gs->endTimer5++;
    }

    if (gs->projectFlags[PROJ_DISMANTLE_MEMORY] && gs->wire == 0) {
        gs->endTimer6++;
    }

    // Credits sequence
    if (gs->endTimer6 >= 500 && gs->milestoneFlag == 15) {
        display_message(gs, "Universal Paperclips");
        gs->milestoneFlag++;
    }

    if (gs->endTimer6 >= 600 && gs->milestoneFlag == 16) {
        display_message(gs, "a game by Frank Lantz");
        gs->milestoneFlag++;
    }

    if (gs->endTimer6 >= 700 && gs->milestoneFlag == 17) {
        display_message(gs, "combat programming by Bennett Foddy");
        gs->milestoneFlag++;
    }

    if (gs->endTimer6 >= 800 && gs->milestoneFlag == 18) {
        display_message(gs, "'Riversong' by Tonto's Expanding Headband");
        gs->milestoneFlag++;
    }

    if (gs->endTimer6 >= 900 && gs->milestoneFlag == 19) {
        display_message(gs, "2017 Everybody House Games - Tanmatsu Edition");
        gs->milestoneFlag++;
    }
}
