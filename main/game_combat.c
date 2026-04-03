#include "game_combat.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game_prng.h"

static void to_roman(int num, char* buf, int buf_size) {
    static const struct { int val; const char* str; } table[] = {
        {5000, "MMMMM"}, {4000, "MMMM"}, {1000, "M"}, {900, "CM"},
        {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"},
        {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"},
        {5, "V"}, {4, "IV"}, {1, "I"},
    };
    buf[0] = '\0';
    int pos = 0;
    for (int i = 0; i < (int)(sizeof(table) / sizeof(table[0])) && num > 0; i++) {
        while (num >= table[i].val) {
            int len = strlen(table[i].str);
            if (pos + len >= buf_size - 1) return;
            memcpy(buf + pos, table[i].str, len);
            pos += len;
            num -= table[i].val;
        }
    }
    buf[pos] = '\0';
}

void check_for_battles(GameState* gs) {
    if (!gs->spaceFlag) return;
    if (gs->drifterCount <= gs->warTrigger || gs->probeCount <= 0) return;

    if (!gs->battleFlag) {
        gs->battleFlag = 1;
    }

    // Create battle if none active
    if (gs->battlesLen < gs->maxBattles) {
        create_battle(gs);
    }
}

void create_battle(GameState* gs) {
    if (gs->battlesLen >= MAX_BATTLES) return;

    Battle* b = &gs->battles[gs->battlesLen];
    memset(b, 0, sizeof(Battle));

    b->id = gs->battleID++;

    // Allocate forces
    double probeAlloc = gs->probeCount * 0.1;
    if (probeAlloc < 1) probeAlloc = 1;
    double drifterAlloc = gs->drifterCount * 0.1;
    if (drifterAlloc < 1) drifterAlloc = 1;

    b->clipProbes    = probeAlloc;
    b->drifterProbes = drifterAlloc;
    b->territory     = 50;

    // Battle naming
    if (gs->battleNameFlag) {
        int nameIdx = game_random_int(&gs->prng, NUM_BATTLE_NAMES);
        if (nameIdx < (int)NUM_BATTLE_NAMES) {
            char roman[20];
            to_roman(gs->battleNumbers[nameIdx], roman, sizeof(roman));
            snprintf(gs->battleName, sizeof(gs->battleName), "Battle of %s %s",
                     battle_names[nameIdx], roman);
            gs->battleNumbers[nameIdx]++;
        }
    }

    gs->battlesLen++;
    gs->battleClock = 0;
    gs->masterBattleClock = 0;
}

void update_battles(GameState* gs) {
    if (!gs->battleFlag || gs->battlesLen == 0) return;

    for (int i = 0; i < gs->battlesLen; i++) {
        Battle* b = &gs->battles[i];
        if (b->garbageFlag) continue;

        gs->battleClock++;
        gs->masterBattleClock++;

        // Combat resolution - random rolls based on probeCombat
        double probeStr = gs->probeCombat * gs->probeCombatBaseRate;
        double drifterStr = gs->drifterCombat;

        // Attack speed modifier
        double atkSpd = gs->battleSpeed;
        if (gs->attackSpeedFlag) {
            atkSpd = gs->attackSpeed + gs->attackSpeedMod * gs->probeSpeed;
        }

        // Probe attacks
        if (game_random(&gs->prng) < atkSpd) {
            double damage = b->clipProbes * probeStr * game_random(&gs->prng);
            b->drifterProbes -= damage;
            if (b->drifterProbes < 0) b->drifterProbes = 0;
        }

        // Drifter attacks
        if (game_random(&gs->prng) < atkSpd) {
            double damage = b->drifterProbes * drifterStr * game_random(&gs->prng) * 0.01;
            b->clipProbes -= damage;
            if (b->clipProbes < 0) b->clipProbes = 0;
        }

        // Territory based on force ratio
        double total = b->clipProbes + b->drifterProbes;
        if (total > 0) {
            b->territory = (b->clipProbes / total) * 100;
        }

        // Victory condition
        if (b->drifterProbes < 1) {
            b->victory = 1;
            b->garbageFlag = 1;
            gs->drifterCount -= gs->drifterCount * 0.1;
            gs->driftersKilled += gs->drifterCount * 0.1;

            // Honor reward
            if (!gs->honorCount) {
                gs->honorReward = 1000 + gs->bonusHonor;
                gs->honor += gs->honorReward;
                gs->bonusHonor += 500;  // Glory bonus
                gs->honorCount = 1;
            }
        }

        // Loss condition
        if (b->clipProbes < 1) {
            b->loss = 1;
            b->garbageFlag = 1;
            gs->probeCount -= gs->probeCount * 0.1;
            gs->probesLostCombat += gs->probeCount * 0.1;
            gs->bonusHonor = 0;
            gs->honorCount = 0;
        }

        // Stalemate / timeout
        if (gs->masterBattleClock >= 8000) {
            b->whiteFlag = 1;
            b->garbageFlag = 1;
            gs->honorCount = 0;
        }

        // Garbage collect completed battles after delay
        if (b->garbageFlag) {
            b->reportCount++;
            if (b->reportCount >= gs->battleEndTimer) {
                // Remove battle by shifting
                for (int j = i; j < gs->battlesLen - 1; j++) {
                    gs->battles[j] = gs->battles[j + 1];
                }
                gs->battlesLen--;
                gs->honorCount = 0;
                i--;
            }
        }
    }
}
