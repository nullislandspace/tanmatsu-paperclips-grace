#include "game_investment.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game_prng.h"

void invest_deposit(GameState* gs) {
    gs->ledger -= floor(gs->funds);
    gs->bankroll = floor(gs->bankroll + gs->funds);
    gs->funds = 0;
}

void invest_withdraw(GameState* gs) {
    gs->ledger += gs->bankroll;
    gs->funds += gs->bankroll;
    gs->bankroll = 0;
}

void invest_upgrade(GameState* gs) {
    if (gs->yomi < gs->investUpgradeCost) return;
    gs->yomi -= gs->investUpgradeCost;
    gs->investLevel++;
    gs->stockGainThreshold += 0.01;
    gs->investUpgradeCost = floor(pow(gs->investLevel + 1, M_E) * 100);
}

static void create_stock(GameState* gs, double dollars) {
    if (gs->portfolioSize >= MAX_STOCKS) return;

    // Generate random symbol (1-4 uppercase letters)
    char sym[5] = {0};
    int len = game_random_int(&gs->prng, 4) + 1;
    for (int i = 0; i < len; i++) {
        sym[i] = 'A' + game_random_int(&gs->prng, 26);
    }

    // Generate random price
    double roll = game_random(&gs->prng);
    double price;
    if (roll > 0.99)       price = ceil(game_random(&gs->prng) * 3000);
    else if (roll > 0.85)  price = ceil(game_random(&gs->prng) * 500);
    else if (roll > 0.60)  price = ceil(game_random(&gs->prng) * 150);
    else if (roll > 0.20)  price = ceil(game_random(&gs->prng) * 50);
    else                   price = ceil(game_random(&gs->prng) * 15);

    if (price > dollars) price = ceil(dollars * roll);
    if (price < 1) price = 1;

    double amount = floor(dollars / price);
    if (amount > 1000000) amount = 1000000;
    if (amount < 1) return;

    double cost = price * amount;
    gs->bankroll -= cost;

    Stock* s = &gs->stocks[gs->portfolioSize];
    s->id = gs->stockID++;
    memcpy(s->symbol, sym, 5);
    s->price  = price;
    s->amount = amount;
    s->total  = price * amount;
    s->profit = 0;
    s->age    = 0;

    gs->portfolioSize++;
}

void stock_shop(GameState* gs) {
    if (!gs->investmentEngineFlag || !gs->humanFlag) return;

    double budget = ceil(gs->portTotal / gs->riskiness);
    int r = 11 - gs->riskiness;
    double reserves = ceil(gs->portTotal / r);
    if (gs->riskiness == 1) reserves = 0;

    // Budget adjustments based on bankroll vs reserves
    if ((gs->bankroll - budget) < reserves && gs->riskiness == 1 && gs->bankroll > (gs->portTotal / 10)) {
        budget = gs->bankroll;
    } else if ((gs->bankroll - budget) < reserves && gs->riskiness == 1) {
        budget = 0;
    } else if ((gs->bankroll - budget) < reserves) {
        budget = gs->bankroll - reserves;
    }

    if (gs->portfolioSize < MAX_STOCKS && gs->bankroll >= 5 && budget >= 1 && (gs->bankroll - budget) >= reserves) {
        if (game_random(&gs->prng) < 0.25) {
            create_stock(gs, budget);
        }
    }
}

void stock_sell_update(GameState* gs) {
    if (!gs->investmentEngineFlag) return;

    // Update stock prices
    for (int i = 0; i < gs->portfolioSize; i++) {
        Stock* s = &gs->stocks[i];
        s->age++;

        if (game_random(&gs->prng) < 0.6) {
            int gain = (game_random(&gs->prng) > gs->stockGainThreshold) ? 0 : 1;
            double delta = ceil((game_random(&gs->prng) * s->price) / (4.0 * gs->riskiness));

            if (gain) {
                s->price += delta;
                s->profit += delta * s->amount;
            } else {
                s->price -= delta;
                s->profit -= delta * s->amount;
            }

            if (s->price == 0 && game_random(&gs->prng) > 0.24) {
                s->price = 1;
            }
            s->total = s->price * s->amount;
        }
    }

    // Attempt to sell oldest stock
    gs->sellDelay++;
    if (gs->portfolioSize > 0 && gs->sellDelay >= 5 && game_random(&gs->prng) <= 0.3 && gs->humanFlag) {
        gs->bankroll += gs->stocks[0].total;

        // Remove stock 0 by shifting
        for (int i = 0; i < gs->portfolioSize - 1; i++) {
            gs->stocks[i] = gs->stocks[i + 1];
        }
        gs->portfolioSize--;
        gs->sellDelay = 0;
    }
}

void stock_display_update(GameState* gs) {
    if (!gs->investmentEngineFlag) return;

    gs->secTotal = 0;
    for (int i = 0; i < gs->portfolioSize; i++) {
        gs->secTotal += gs->stocks[i].total;
    }
    gs->portTotal = gs->bankroll + gs->secTotal;

    // Lifetime investment report (every 10000 ticks)
    gs->stockReportCounter++;
    if (gs->stockReportCounter >= 100) {
        gs->stockReportCounter = 0;
    }
}
