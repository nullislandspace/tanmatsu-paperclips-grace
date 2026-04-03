#include "game_space.h"

#include <math.h>

#include "game_prng.h"

void launch_probe(GameState* gs) {
    if (gs->unusedClips < gs->probeCost) return;
    gs->unusedClips -= gs->probeCost;
    gs->probeCount++;
    gs->probeLaunchLevel++;
}

void raise_probe_stat(GameState* gs, int* stat) {
    gs->probeUsedTrust = gs->probeSpeed + gs->probeNav + gs->probeRep + gs->probeHaz +
                         gs->probeFac + gs->probeHarv + gs->probeWire + gs->probeCombat;
    if (gs->probeTrust - gs->probeUsedTrust < 1) return;
    (*stat)++;
}

void lower_probe_stat(GameState* gs, int* stat) {
    (void)gs;
    if (*stat < 1) return;
    (*stat)--;
}

void increase_probe_trust(GameState* gs) {
    if (gs->probeTrust >= gs->maxTrust) return;
    double cost = floor(pow(gs->probeTrust + 1, 1.47) * 200);
    if (gs->yomi < cost) return;
    gs->yomi -= cost;
    gs->probeTrust++;
    gs->probeTrustCost = floor(pow(gs->probeTrust + 1, 1.47) * 200);
}

void increase_max_trust(GameState* gs) {
    if (gs->honor < gs->maxTrustCost) return;
    gs->honor -= gs->maxTrustCost;
    gs->maxTrust += 10;
}

void explore_universe(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0) return;

    double xRate = floor(gs->probeCount) * gs->probeXBaseRate * gs->probeSpeed * gs->probeNav;
    if (xRate > gs->totalMatter - gs->foundMatter) {
        xRate = gs->totalMatter - gs->foundMatter;
    }
    gs->foundMatter += xRate;
    gs->availableMatter += xRate;
}

void spawn_probes(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0) return;
    if (gs->probeRep <= 0) return;

    double nextGen = gs->probeCount * gs->probeRepBaseRate * gs->probeRep;

    // Cap at ~10^48
    if (gs->probeCount >= 1e48) nextGen = 0;

    // Partial spawn for slow growth
    if (nextGen > 0 && nextGen < 1) {
        gs->partialProbeSpawn += nextGen;
        if (gs->partialProbeSpawn >= 1) {
            gs->probeCount += floor(gs->partialProbeSpawn);
            gs->probeDescendents += floor(gs->partialProbeSpawn);
            gs->partialProbeSpawn -= floor(gs->partialProbeSpawn);
        }
    } else if (nextGen >= 1) {
        gs->probeCount += nextGen;
        gs->probeDescendents += nextGen;
        gs->partialProbeSpawn = 0;
    }
}

void encounter_hazards(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0) return;
    if (gs->probeHaz <= 0 && gs->probeHazBaseRate <= 0) return;

    double lossRate = gs->probeHazBaseRate;
    if (gs->probeHaz > 0) {
        lossRate = gs->probeHazBaseRate / gs->probeHaz;
    }

    double losses = gs->probeCount * lossRate;

    if (losses > 0 && losses < 1) {
        gs->partialProbeHaz += losses;
        if (gs->partialProbeHaz >= 1) {
            double actualLoss = floor(gs->partialProbeHaz);
            gs->probeCount -= actualLoss;
            gs->probesLostHaz += actualLoss;
            gs->partialProbeHaz -= actualLoss;
        }
    } else if (losses >= 1) {
        gs->probeCount -= losses;
        gs->probesLostHaz += losses;
    }

    if (gs->probeCount < 0) gs->probeCount = 0;
}

void drift(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0) return;
    // Drift is stopped by project 148 (Reject)
    if (gs->projectFlags[48]) return;

    double driftLoss = gs->probeCount * gs->probeDriftBaseRate;
    if (driftLoss > 0) {
        gs->probeCount -= driftLoss;
        gs->drifterCount += driftLoss;
        gs->probesLostDrift += driftLoss;
    }

    if (gs->probeCount < 0) gs->probeCount = 0;
}

void spawn_factories(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0 || gs->probeFac <= 0) return;

    double rate = gs->probeCount * gs->probeFacBaseRate * gs->probeFac;
    if (rate >= 1 && gs->unusedClips >= 100000000.0 * rate) {
        gs->unusedClips -= 100000000.0 * floor(rate);
        gs->factoryLevel += floor(rate);
        gs->factoryBill += 100000000.0 * floor(rate);
    }
}

void spawn_harvesters(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0 || gs->probeHarv <= 0) return;

    double rate = gs->probeCount * gs->probeHarvBaseRate * gs->probeHarv;
    if (rate >= 1 && gs->unusedClips >= 2000000.0 * rate) {
        gs->unusedClips -= 2000000.0 * floor(rate);
        gs->harvesterLevel += floor(rate);
        gs->harvesterBill += 2000000.0 * floor(rate);
    }
}

void spawn_wire_drones(GameState* gs) {
    if (!gs->spaceFlag || gs->probeCount <= 0 || gs->probeWire <= 0) return;

    double rate = gs->probeCount * gs->probeWireBaseRate * gs->probeWire;
    if (rate >= 1 && gs->unusedClips >= 2000000.0 * rate) {
        gs->unusedClips -= 2000000.0 * floor(rate);
        gs->wireDroneLevel += floor(rate);
        gs->wireDroneBill += 2000000.0 * floor(rate);
    }
}
