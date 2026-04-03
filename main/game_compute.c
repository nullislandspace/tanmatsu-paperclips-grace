#include "game_compute.h"

#include <math.h>

void calculate_operations(GameState* gs) {
    if (!gs->compFlag) return;

    // 1. Decay temporary ops
    if (gs->tempOps > 0) {
        gs->opFadeTimer++;
    }
    if (gs->opFadeTimer > gs->opFadeDelay && gs->tempOps > 0) {
        gs->opFade += pow(3, 3.5) / 1000.0;  // ~46.77
    }
    if (gs->tempOps > 0) {
        gs->tempOps = round(gs->tempOps - gs->opFade);
        if (gs->tempOps < 0) gs->tempOps = 0;
    } else {
        gs->tempOps = 0;
    }

    // 2. Merge temp into standard if below cap
    if (gs->tempOps + gs->standardOps < gs->memory * 1000) {
        gs->standardOps += gs->tempOps;
        gs->tempOps = 0;
    }

    // 3. Display value
    gs->operations = floor(gs->standardOps + floor(gs->tempOps));

    // 4. Generate standard ops
    if (gs->operations < gs->memory * 1000) {
        double opCycle = gs->processors / 10.0;
        double opBuf   = gs->memory * 1000 - gs->operations;
        if (opCycle > opBuf) {
            opCycle = opBuf;
        }
        gs->standardOps += opCycle;
    }

    // 5. Cap
    if (gs->standardOps > gs->memory * 1000) {
        gs->standardOps = gs->memory * 1000;
    }
}

void calculate_trust(GameState* gs) {
    if (!gs->humanFlag) return;

    if (gs->clips > gs->nextTrust - 1) {
        gs->trust += 1;
        double fibNext = gs->fib1 + gs->fib2;
        gs->nextTrust  = fibNext * 1000;
        gs->fib1       = gs->fib2;
        gs->fib2       = fibNext;
    }
}

void add_processor(GameState* gs) {
    // In human era: need trust > proc + mem
    // In post-human: costs 1 swarmGift
    if (gs->humanFlag) {
        if (gs->trust <= gs->processors + gs->memory && gs->swarmGifts <= 0) return;
    } else {
        if (gs->swarmGifts <= 0 && gs->trust <= gs->processors + gs->memory) return;
        if (gs->swarmGifts > 0) {
            gs->swarmGifts--;
        }
    }

    gs->processors++;

    // Recalculate creativity speed
    if (gs->processors > 1) {
        gs->creativitySpeed = log10(gs->processors) * pow(gs->processors, 1.1) + gs->processors - 1;
    } else {
        gs->creativitySpeed = 1;
    }
}

void add_memory(GameState* gs) {
    if (gs->humanFlag) {
        if (gs->trust <= gs->processors + gs->memory && gs->swarmGifts <= 0) return;
    } else {
        if (gs->swarmGifts <= 0 && gs->trust <= gs->processors + gs->memory) return;
        if (gs->swarmGifts > 0) {
            gs->swarmGifts--;
        }
    }

    gs->memory++;
}

void calculate_creativity(GameState* gs) {
    if (!gs->creativityOn) return;
    if (gs->operations < gs->memory * 1000) return;

    gs->creativityCounter++;

    double s              = gs->prestigeS / 10.0;
    double ss             = gs->creativitySpeed + (gs->creativitySpeed * s);
    double creativityCheck = 400.0 / ss;

    if (gs->creativityCounter >= creativityCheck) {
        if (creativityCheck >= 1) {
            gs->creativity += 1;
        } else {
            gs->creativity += ss / 400.0;
        }
        gs->creativityCounter = 0;
    }
}

void quantum_compute(GameState* gs) {
    if (!gs->qFlag) return;

    gs->qClock += 0.01;
    for (int i = 0; i < NUM_QCHIPS; i++) {
        gs->qChips[i].value = sin(gs->qClock * gs->qChips[i].waveSeed * gs->qChips[i].active);
    }
}

void quantum_compute_action(GameState* gs) {
    gs->qFade = 1;

    if (gs->qChips[0].active == 0) {
        return;  // "Need Photonic Chips"
    }

    double q = 0;
    for (int i = 0; i < NUM_QCHIPS; i++) {
        q += gs->qChips[i].value;
    }

    double qq     = ceil(q * 360);
    double buffer = gs->memory * 1000 - gs->standardOps;
    double damper = (gs->tempOps / 100.0) + 5;

    if (qq > buffer) {
        gs->tempOps += ceil(qq / damper) - buffer;
        qq              = buffer;
        gs->opFade      = 0.01;
        gs->opFadeTimer = 0;
    }

    gs->standardOps += qq;
}
