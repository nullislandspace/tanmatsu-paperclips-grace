#pragma once

#include "game_state.h"

// Deposit all funds into bankroll
void invest_deposit(GameState* gs);

// Withdraw bankroll to funds
void invest_withdraw(GameState* gs);

// Upgrade investment engine (costs yomi)
void invest_upgrade(GameState* gs);

// Stock shop - attempt to buy stock (every 1s / 100 ticks)
void stock_shop(GameState* gs);

// Update stock prices and sell (every 2.5s / 250 ticks)
void stock_sell_update(GameState* gs);

// Update stock display values (every 100ms / 10 ticks)
void stock_display_update(GameState* gs);
