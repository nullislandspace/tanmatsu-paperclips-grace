#include "game_economics.h"

#include <math.h>
#include <stdio.h>

#include "game_format.h"
#include "game_prng.h"

void clip_click(GameState* gs, double amount) {
    if (gs->dismantle >= 4) {
        gs->finalClips++;
        return;
    }

    if (gs->wire >= 1) {
        if (amount > gs->wire) {
            amount = gs->wire;
        }

        gs->clips += amount;
        gs->unsoldClips += amount;
        gs->wire -= amount;
        gs->unusedClips += amount;
    }
}

void buy_wire(GameState* gs) {
    if (gs->funds >= gs->wireCost) {
        gs->wirePriceTimer = 0;
        gs->wire += gs->wireSupply;
        gs->funds -= gs->wireCost;
        gs->wirePurchase += 1;
        gs->wireBasePrice += 0.05;
    }
}

void adjust_wire_price(GameState* gs) {
    gs->wirePriceTimer++;

    if (gs->wirePriceTimer > 250 && gs->wireBasePrice > 15) {
        gs->wireBasePrice -= gs->wireBasePrice / 1000.0;
        gs->wirePriceTimer = 0;
    }

    if (game_random(&gs->prng) < 0.015) {
        gs->wirePriceCounter++;
        double wireAdjust = 6.0 * sin(gs->wirePriceCounter);
        gs->wireCost = ceil(gs->wireBasePrice + wireAdjust);
        if (gs->wireCost < 1) gs->wireCost = 1;
    }
}

void raise_price(GameState* gs) {
    gs->margin = round((gs->margin + 0.01) * 100.0) / 100.0;
}

void lower_price(GameState* gs) {
    gs->margin = round((gs->margin - 0.01) * 100.0) / 100.0;
    if (gs->margin < 0.01) {
        gs->margin = 0.01;
    }
}

void update_demand(GameState* gs) {
    if (gs->humanFlag != 1) return;

    gs->marketing = pow(1.1, gs->marketingLvl - 1);
    double margin = gs->margin;
    if (margin < 0.01) margin = 0.01;
    gs->demand    = (0.8 / margin) * gs->marketing * gs->marketingEffectiveness * gs->demandBoost;
    gs->demand    = gs->demand + (gs->demand / 10.0) * gs->prestigeU;
}

void sales_tick(GameState* gs) {
    if (gs->humanFlag != 1) return;

    if (game_random(&gs->prng) < (gs->demand / 100.0)) {
        double amount = floor(0.7 * pow(gs->demand, 1.15));
        sell_clips(gs, amount);
    }
}

void sell_clips(GameState* gs, double number) {
    if (gs->unsoldClips > 0) {
        if (number > gs->unsoldClips) {
            gs->transaction = floor(gs->unsoldClips * gs->margin * 1000.0) / 1000.0;
            gs->funds       = floor((gs->funds + gs->transaction) * 100.0) / 100.0;
            gs->income += gs->transaction;
            gs->clipsSold += gs->unsoldClips;
            gs->unsoldClips = 0;
        } else {
            gs->transaction = floor(number * gs->margin * 1000.0) / 1000.0;
            gs->funds       = floor((gs->funds + gs->transaction) * 100.0) / 100.0;
            gs->income += gs->transaction;
            gs->clipsSold += number;
            gs->unsoldClips -= number;
        }
    }
}

void make_clipper(GameState* gs) {
    if (gs->funds >= gs->clippperCost) {
        gs->clipmakerLevel += 1;
        gs->funds -= gs->clipperCost;
    }
    gs->clipperCost  = pow(1.1, gs->clipmakerLevel) + 5;
    gs->clippperCost = gs->clipperCost;
}

void make_mega_clipper(GameState* gs) {
    if (gs->funds >= gs->megaClipperCost) {
        gs->megaClipperLevel += 1;
        gs->funds -= gs->megaClipperCost;
    }
    gs->megaClipperCost = pow(1.07, gs->megaClipperLevel) * 1000;
}

void buy_marketing(GameState* gs) {
    if (gs->funds >= gs->adCost) {
        gs->funds -= gs->adCost;
        gs->marketingLvl += 1;
        gs->adCost *= 2;
    }
}

// Revenue tracking state (not saved, recalculated)
static double s_incomeNow  = 0;
static double s_incomeThen = 0;

void calculate_rev(GameState* gs) {
    s_incomeThen = s_incomeNow;
    s_incomeNow  = gs->income;
    double incomeLastSecond = round((s_incomeNow - s_incomeThen) * 100.0) / 100.0;

    // Push to tracker (circular)
    if (gs->incomeTrackerLen < MAX_INCOME_TRACK) {
        gs->incomeTracker[gs->incomeTrackerLen] = incomeLastSecond;
        gs->incomeTrackerLen++;
    } else {
        // Shift left
        for (int i = 0; i < MAX_INCOME_TRACK - 1; i++) {
            gs->incomeTracker[i] = gs->incomeTracker[i + 1];
        }
        gs->incomeTracker[MAX_INCOME_TRACK - 1] = incomeLastSecond;
    }

    double sum = 0;
    for (int i = 0; i < gs->incomeTrackerLen; i++) {
        sum = round((sum + gs->incomeTracker[i]) * 100.0) / 100.0;
    }

    double trueAvgRev = sum / gs->incomeTrackerLen;

    double chanceOfPurchase = gs->demand / 100.0;
    if (chanceOfPurchase > 1.0) chanceOfPurchase = 1.0;
    if (gs->unsoldClips < 1.0) chanceOfPurchase = 0;

    gs->avgRev = chanceOfPurchase * 0.7 * pow(gs->demand, 1.15) * gs->margin * 10.0;

    if (gs->demand > gs->unsoldClips) {
        gs->avgRev = trueAvgRev;
    }
}

void update_clip_rate(GameState* gs) {
    gs->clipRateTracker++;
    if (gs->clipRateTracker >= 100) {
        gs->clipRate        = gs->clips - gs->prevClips;
        gs->prevClips       = gs->clips;
        gs->clipRateTracker = 0;
    }
}

void milestone_check(GameState* gs) {
    char buf[MAX_MSG_LEN];
    char timebuf[64];

    if (gs->milestoneFlag == 0 && gs->funds >= 5) {
        gs->milestoneFlag++;
        display_message(gs, "AutoClippers available for purchase");
        gs->autoClipperFlag = 1;
    }

    if (gs->milestoneFlag == 1 && ceil(gs->clips) >= 500) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "500 clips created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 2 && ceil(gs->clips) >= 1000) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "1,000 clips created in %s", timebuf);
        display_message(gs, buf);
    }

    // compFlag unlock: stuck or 2000 clips
    if (gs->compFlag == 0 && gs->unsoldClips < 1 && gs->funds < gs->wireCost && gs->wire < 1) {
        gs->compFlag     = 1;
        gs->projectsFlag = 1;
        display_message(gs, "Trust-Constrained Self-Modification enabled");
    }

    if (gs->projectsFlag == 0 && ceil(gs->clips) >= 50) {
        gs->projectsFlag = 1;
    }

    if (gs->compFlag == 0 && ceil(gs->clips) >= 2000) {
        gs->compFlag     = 1;
        gs->projectsFlag = 1;
        display_message(gs, "Trust-Constrained Self-Modification enabled");
    }

    if (gs->milestoneFlag == 3 && ceil(gs->clips) >= 10000) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "10,000 clips created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 4 && ceil(gs->clips) >= 100000) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "100,000 clips created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 5 && ceil(gs->clips) >= 1000000) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "1,000,000 clips created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 6 && gs->projectFlags[35]) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "Full autonomy attained in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 7 && ceil(gs->clips) >= 1e12) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Trillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 8 && ceil(gs->clips) >= 1e15) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Quadrillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 9 && ceil(gs->clips) >= 1e18) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Quintillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 10 && ceil(gs->clips) >= 1e21) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Sextillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 11 && ceil(gs->clips) >= 1e24) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Septillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 12 && ceil(gs->clips) >= 1e27) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "One Octillion Clips Created in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 13 && gs->spaceFlag == 1) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "Terrestrial resources fully utilized in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 14 && gs->clips >= gs->totalMatter) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "Universal Paperclips achieved in %s", timebuf);
        display_message(gs, buf);
    }

    if (gs->milestoneFlag == 14 && gs->foundMatter >= gs->totalMatter && gs->availableMatter < 1 && gs->wire < 1) {
        gs->milestoneFlag++;
        time_cruncher(gs->ticks, timebuf, sizeof(timebuf));
        snprintf(buf, sizeof(buf), "Universal Paperclips achieved in %s", timebuf);
        display_message(gs, buf);
    }

    // autoClipperFlag from buttonUpdate
    if (gs->funds >= 5) {
        gs->autoClipperFlag = 1;
    }
}
