#include "game_tournament.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game_prng.h"
#include "game_projects.h"

// Strategy move picker
static int pick_move(GameState* gs, int stratIdx, int opponentLastMove) {
    switch (stratIdx) {
        case 0:  // RANDOM
            return (game_random(&gs->prng) < 0.5) ? 1 : 2;
        case 1:  // A100
            return 1;
        case 2:  // B100
            return 2;
        case 3: {  // GREEDY
            int a_payoff = gs->aa + gs->ab;
            int b_payoff = gs->ba + gs->bb;
            return (a_payoff >= b_payoff) ? 1 : 2;
        }
        case 4: {  // GENEROUS
            int a_gives = gs->aa + gs->ba;
            int b_gives = gs->ab + gs->bb;
            return (a_gives >= b_gives) ? 1 : 2;
        }
        case 5: {  // MINIMAX
            int a_gives = (gs->aa > gs->ba) ? gs->aa : gs->ba;
            int b_gives = (gs->ab > gs->bb) ? gs->ab : gs->bb;
            return (a_gives <= b_gives) ? 1 : 2;
        }
        case 6:  // TIT FOR TAT
            return opponentLastMove ? opponentLastMove : 1;
        case 7:  // BEAT LAST
            if (opponentLastMove == 1) {
                return (gs->aa >= gs->ba) ? 1 : 2;
            } else {
                return (gs->ab >= gs->bb) ? 1 : 2;
            }
        default:
            return 1;
    }
}

// Persistent tournament state (not saved)
static int  t_strats[NUM_STRATEGIES];
static int  t_numStrats;
static int  t_scores[NUM_STRATEGIES];
static int  t_hPrev, t_vPrev;
static int  t_pairI, t_pairJ;     // Current pair indices into t_strats[]
static int  t_subRound;           // 0-9 within current pair

// Display state (readable by UI for rendering)
int    g_tourney_hStrat  = -1;    // Strategy index of horizontal player
int    g_tourney_vStrat  = -1;    // Strategy index of vertical player
int    g_tourney_lastCell = -1;   // Last highlighted cell: 0=AA, 1=AB, 2=BA, 3=BB
int    g_tourney_cellTimer = 0;   // Ticks remaining to show cell highlight

// Ticks per sub-round (100ms = 10 ticks, matching JS setTimeout 50+50)
#define TICKS_PER_SUBROUND 10

static int t_tickCounter;         // Counts ticks within a sub-round

void new_tourney(GameState* gs) {
    if (gs->standardOps < gs->tourneyCost) return;
    gs->standardOps -= gs->tourneyCost;

    // Random 2x2 payoff grid
    gs->aa = game_random_int(&gs->prng, 10) + 1;
    gs->ab = game_random_int(&gs->prng, 10) + 1;
    gs->ba = game_random_int(&gs->prng, 10) + 1;
    gs->bb = game_random_int(&gs->prng, 10) + 1;

    // Collect active strategies
    t_numStrats = 0;
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        if (gs->stratActive[i]) {
            t_strats[t_numStrats++] = i;
        }
    }

    memset(t_scores, 0, sizeof(t_scores));
    t_hPrev = 1;
    t_vPrev = 1;

    gs->rounds        = t_numStrats * t_numStrats;
    gs->tourneyInProg = 1;
    gs->resultsFlag   = 0;
    gs->currentRound  = 0;
    gs->tourneyLvl++;

    t_pairI       = 0;
    t_pairJ       = 0;
    t_subRound    = 0;
    t_tickCounter = 0;

    // Display: show first pair
    if (t_numStrats > 0) {
        g_tourney_hStrat = t_strats[0];
        g_tourney_vStrat = t_strats[0];
    }
    g_tourney_lastCell  = -1;
    g_tourney_cellTimer = 0;
}

// Play one sub-round. Returns true when the entire tournament is complete.
static bool tourney_step_subround(GameState* gs) {
    if (t_numStrats == 0) return true;
    if (t_pairI >= t_numStrats) return true;

    int si = t_strats[t_pairI];
    int sj = t_strats[t_pairJ];

    // Update display: which strategies are playing
    g_tourney_hStrat = si;
    g_tourney_vStrat = sj;

    // Pick moves
    int hm = pick_move(gs, si, t_vPrev);
    int vm = pick_move(gs, sj, t_hPrev);

    // Calculate payoffs and highlight cell
    if (hm == 1 && vm == 1) {
        t_scores[t_pairI] += gs->aa;
        t_scores[t_pairJ] += gs->aa;
        g_tourney_lastCell = 0;
    } else if (hm == 1 && vm == 2) {
        t_scores[t_pairI] += gs->ab;
        t_scores[t_pairJ] += gs->ba;
        g_tourney_lastCell = 1;
    } else if (hm == 2 && vm == 1) {
        t_scores[t_pairI] += gs->ba;
        t_scores[t_pairJ] += gs->ab;
        g_tourney_lastCell = 2;
    } else {
        t_scores[t_pairI] += gs->bb;
        t_scores[t_pairJ] += gs->bb;
        g_tourney_lastCell = 3;
    }
    g_tourney_cellTimer = TICKS_PER_SUBROUND / 2;  // Highlight for half the period

    t_hPrev = hm;
    t_vPrev = vm;

    // Advance sub-round
    t_subRound++;
    if (t_subRound >= 10) {
        // Pair done, advance to next
        t_subRound = 0;
        t_hPrev = 1;
        t_vPrev = 1;
        gs->currentRound++;

        t_pairJ++;
        if (t_pairJ >= t_numStrats) {
            t_pairJ = 0;
            t_pairI++;
        }

        if (t_pairI >= t_numStrats) {
            return true;  // All pairs done
        }
    }

    return false;
}

static void tourney_finish(GameState* gs) {
    int best = -1, bestScore = -1;
    int second = -1, secondScore = -1;
    int third = -1, thirdScore = -1;

    for (int i = 0; i < t_numStrats; i++) {
        if (t_scores[i] > bestScore) {
            thirdScore = secondScore; third = second;
            secondScore = bestScore; second = best;
            bestScore = t_scores[i]; best = i;
        } else if (t_scores[i] > secondScore) {
            thirdScore = secondScore; third = second;
            secondScore = t_scores[i]; second = i;
        } else if (t_scores[i] > thirdScore) {
            thirdScore = t_scores[i]; third = i;
        }
    }

    gs->winnerPtr  = (best >= 0) ? t_strats[best] : 0;
    gs->high       = bestScore;
    gs->placeScore = secondScore;
    gs->showScore  = thirdScore;

    // Award yomi
    int pickIdx = -1;
    for (int i = 0; i < t_numStrats; i++) {
        if (t_strats[i] == gs->pick) {
            pickIdx = i;
            break;
        }
    }
    if (pickIdx < 0 && t_numStrats > 0) {
        pickIdx = 0;
        gs->pick = t_strats[0];
    }

    if (pickIdx >= 0) {
        gs->pickScore = t_scores[pickIdx];
        double yomiEarned = t_scores[pickIdx] * gs->yomiBoost;
        gs->yomi += yomiEarned;

        if (gs->projectFlags[PROJ_STRATEGIC_ATTACHMENT]) {
            if (pickIdx == best)        gs->yomi += 20000;
            else if (pickIdx == second) gs->yomi += 15000;
            else if (pickIdx == third)  gs->yomi += 10000;
        }
    }

    gs->tourneyInProg = 0;
    gs->resultsFlag   = 1;
    gs->resultsTimer  = 0;

    g_tourney_hStrat    = -1;
    g_tourney_vStrat    = -1;
    g_tourney_lastCell  = -1;
    g_tourney_cellTimer = 0;
}

void run_tourney(GameState* gs) {
    if (!gs->tourneyInProg) return;

    // Decay cell highlight
    if (g_tourney_cellTimer > 0) g_tourney_cellTimer--;

    // Only advance one sub-round every TICKS_PER_SUBROUND ticks
    t_tickCounter++;
    if (t_tickCounter < TICKS_PER_SUBROUND) return;
    t_tickCounter = 0;

    bool done = tourney_step_subround(gs);
    if (done) {
        tourney_finish(gs);
    }
}

void auto_tourney_tick(GameState* gs) {
    if (!gs->autoTourneyFlag || !gs->autoTourneyStatus) return;
    if (!gs->resultsFlag) return;

    gs->resultsTimer++;
    if (gs->resultsTimer >= 300 && gs->standardOps >= gs->tourneyCost) {
        new_tourney(gs);
        gs->resultsTimer = 0;
    }
}
