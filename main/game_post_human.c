#include "game_post_human.h"

#include <math.h>

#include "game_prng.h"

// Factory cost multiplier (piecewise)
static double fcmod(double level) {
    if (level <= 7) return 11 - level;
    if (level <= 12) return 2;
    if (level <= 19) return 1.5;
    if (level <= 38) return 1.25;
    if (level <= 78) return 1.15;
    return 1.10;
}

void make_factory(GameState* gs) {
    if (gs->unusedClips < gs->factoryCost) return;
    gs->unusedClips -= gs->factoryCost;
    gs->factoryBill += gs->factoryCost;
    gs->factoryLevel++;
    if (gs->factoryLevel > gs->maxFactoryLevel) gs->maxFactoryLevel = gs->factoryLevel;
    gs->factoryCost *= fcmod(gs->factoryLevel);
}

void make_harvester(GameState* gs) {
    if (gs->unusedClips < gs->harvesterCost) return;
    gs->unusedClips -= gs->harvesterCost;
    gs->harvesterBill += gs->harvesterCost;
    gs->harvesterLevel++;
    double total = gs->harvesterLevel + gs->wireDroneLevel;
    if (total > gs->maxDroneLevel) gs->maxDroneLevel = total;
    gs->harvesterCost = pow(gs->harvesterLevel + 1, 2.25) * 1000000;
}

void make_wire_drone(GameState* gs) {
    if (gs->unusedClips < gs->wireDroneCost) return;
    gs->unusedClips -= gs->wireDroneCost;
    gs->wireDroneBill += gs->wireDroneCost;
    gs->wireDroneLevel++;
    double total = gs->harvesterLevel + gs->wireDroneLevel;
    if (total > gs->maxDroneLevel) gs->maxDroneLevel = total;
    gs->wireDroneCost = pow(gs->wireDroneLevel + 1, 2.25) * 1000000;
}

void make_farm(GameState* gs) {
    if (gs->unusedClips < gs->farmCost) return;
    gs->unusedClips -= gs->farmCost;
    gs->farmBill += gs->farmCost;
    gs->farmLevel++;
    gs->farmCost = pow(gs->farmLevel + 1, 2.78) * 100000000;
}

void make_battery(GameState* gs) {
    if (gs->unusedClips < gs->batteryCost) return;
    gs->unusedClips -= gs->batteryCost;
    gs->batteryBill += gs->batteryCost;
    gs->batteryLevel++;
    gs->batteryCost = pow(gs->batteryLevel + 1, 2.54) * 10000000;
}

void reboot_factory(GameState* gs) {
    if (gs->factoryLevel == 0) return;
    gs->unusedClips += gs->factoryBill;
    gs->factoryLevel = 0;
    gs->factoryBill = 0;
    gs->factoryCost = 100000000;
}

void reboot_harvester(GameState* gs) {
    if (gs->harvesterLevel == 0) return;
    gs->unusedClips += gs->harvesterBill;
    gs->harvesterLevel = 0;
    gs->harvesterBill = 0;
    gs->harvesterCost = 2000000;
}

void reboot_wire_drone(GameState* gs) {
    if (gs->wireDroneLevel == 0) return;
    gs->unusedClips += gs->wireDroneBill;
    gs->wireDroneLevel = 0;
    gs->wireDroneBill = 0;
    gs->wireDroneCost = 2000000;
}

void reboot_farm(GameState* gs) {
    if (gs->farmLevel == 0) return;
    gs->unusedClips += gs->farmBill;
    gs->farmLevel = 0;
    gs->farmBill = 0;
    gs->farmCost = 10000000;
}

void reboot_battery(GameState* gs) {
    if (gs->batteryLevel == 0) return;
    gs->unusedClips += gs->batteryBill;
    gs->batteryLevel = 0;
    gs->batteryBill = 0;
    gs->batteryCost = 1000000;
}

void acquire_matter(GameState* gs) {
    if (gs->harvesterLevel <= 0 || gs->humanFlag) return;

    double dbsth = 1;
    if (gs->droneBoost > 1) {
        dbsth = gs->droneBoost * floor(gs->harvesterLevel);
    }

    double mtr = gs->powMod * dbsth * floor(gs->harvesterLevel) * gs->harvesterRate;
    mtr = mtr * ((200 - gs->sliderPos) / 100.0);

    if (mtr > gs->availableMatter) {
        mtr = gs->availableMatter;
    }

    gs->availableMatter -= mtr;
    gs->acquiredMatter += mtr;
}

void process_matter(GameState* gs) {
    if (gs->wireDroneLevel <= 0 || gs->humanFlag) return;

    double dbstw = 1;
    if (gs->droneBoost > 1) {
        dbstw = gs->droneBoost * floor(gs->wireDroneLevel);
    }

    double a = gs->powMod * dbstw * floor(gs->wireDroneLevel) * gs->wireDroneRate;
    a = a * ((200 - gs->sliderPos) / 100.0);

    if (a > gs->acquiredMatter) {
        a = gs->acquiredMatter;
    }

    gs->acquiredMatter -= a;
    gs->wire += a;
}

void update_power(GameState* gs) {
    if (gs->humanFlag || gs->spaceFlag) return;
    if (gs->farmLevel <= 0) return;

    double supply  = gs->farmLevel * gs->farmRate / 100.0;
    double dDemand = (gs->harvesterLevel * gs->dronePowerRate / 100.0) +
                     (gs->wireDroneLevel * gs->dronePowerRate / 100.0);
    double fDemand = gs->factoryLevel * gs->factoryPowerRate / 100.0;
    double demand  = dDemand + fDemand;
    double cap     = gs->batteryLevel * gs->batterySize;

    if (supply >= demand) {
        double xsSupply = supply - demand;
        if (gs->storedPower < cap) {
            if (xsSupply > cap - gs->storedPower) {
                xsSupply = cap - gs->storedPower;
            }
            gs->storedPower += xsSupply;
        }
        if (gs->powMod < 1) gs->powMod = 1;
        if (gs->momentum) gs->powMod += 0.0001;
    } else {
        double xsDemand = demand - supply;
        if (gs->storedPower > 0) {
            if (gs->storedPower >= xsDemand) {
                gs->storedPower -= xsDemand;
                if (gs->momentum) gs->powMod += 0.0001;
            } else {
                xsDemand -= gs->storedPower;
                gs->storedPower = 0;
                double nuSupply = supply - xsDemand;
                gs->powMod = (demand > 0) ? nuSupply / demand : 0;
            }
        } else {
            gs->powMod = (demand > 0) ? supply / demand : 0;
        }
    }
}

void update_swarm(GameState* gs) {
    if (!gs->swarmFlag || gs->humanFlag) return;

    double droneCount = gs->harvesterLevel + gs->wireDroneLevel;
    if (droneCount <= 0) {
        gs->swarmStatus = SWARM_NO_RESPONSE;
        return;
    }

    // Gift generation
    gs->giftBitGenerationRate = log(droneCount > 1 ? droneCount : 2) * (gs->sliderPos / 100.0);
    gs->giftBits += gs->giftBitGenerationRate;

    gs->nextGift = round(log10(droneCount > 1 ? droneCount : 2) * gs->sliderPos / 100.0);

    if (gs->giftBits >= gs->giftPeriod && gs->nextGift > 0) {
        gs->swarmGifts += gs->nextGift;
        gs->giftBits = 0;
        gs->giftCountdown = gs->giftPeriod;
    }

    gs->giftCountdown = gs->giftPeriod - gs->giftBits;
    if (gs->giftCountdown < 0) gs->giftCountdown = 0;

    // Boredom (when slider at 0 / all work)
    if (gs->sliderPos < 25 && droneCount > 100) {
        gs->boredomLevel += 0.001;
        if (gs->boredomLevel >= 50 && !gs->boredomFlag) {
            gs->boredomFlag = 1;
            gs->swarmStatus = SWARM_BORED;
        }
    } else if (gs->boredomFlag) {
        gs->boredomLevel -= 0.01;
        if (gs->boredomLevel <= 0) {
            gs->boredomFlag = 0;
            gs->boredomLevel = 0;
        }
    }

    // Disorganization (over time)
    if (droneCount > 500) {
        gs->disorgCounter += 0.0001;
        if (gs->disorgCounter >= 50 && !gs->disorgFlag) {
            gs->disorgFlag = 1;
            gs->swarmStatus = SWARM_DISORGANIZED;
        }
    }

    // Status
    if (!gs->boredomFlag && !gs->disorgFlag && droneCount > 0) {
        gs->swarmStatus = SWARM_ACTIVE;
    }
}

void entertain_swarm(GameState* gs) {
    if (gs->creativity < gs->entertainCost) return;
    gs->creativity -= gs->entertainCost;
    gs->boredomLevel = 0;
    gs->boredomFlag = 0;
    gs->entertainCost *= 2;
    if (gs->swarmStatus == SWARM_BORED) gs->swarmStatus = SWARM_ACTIVE;
}

void synch_swarm(GameState* gs) {
    if (gs->yomi < gs->synchCost) return;
    gs->yomi -= gs->synchCost;
    gs->disorgCounter = 0;
    gs->disorgFlag = 0;
    gs->synchCost *= 2;
    if (gs->swarmStatus == SWARM_DISORGANIZED) gs->swarmStatus = SWARM_ACTIVE;
}
