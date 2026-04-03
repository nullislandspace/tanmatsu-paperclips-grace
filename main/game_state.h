#pragma once

#include <stdint.h>
#include <time.h>

#include "battle_names.h"

#define SAVE_PATH_PREFIX "/sd/paperclips"

// Save format constants are now in game_nbt.h (NBT_MAGIC_*, NBT_FORMAT_VERSION)

#define NUM_PROJECTS     115
#define NUM_STRATEGIES   8
#define MAX_STOCKS       5
#define MAX_BATTLES      1
#define MAX_INCOME_TRACK 10
#define NUM_QCHIPS       10
#define MAX_MESSAGES     5
#define MAX_MSG_LEN      128

#define CHEATS_ENABLED 1

typedef enum {
    PHASE_HUMAN,
    PHASE_POST_HUMAN,
    PHASE_SPACE,
    PHASE_ENDGAME,
    PHASE_DISMANTLE,
    PHASE_CREDITS,
} GamePhase;

typedef enum {
    SWARM_ACTIVE       = 0,
    SWARM_HUNGRY       = 1,
    SWARM_CONFUSED     = 2,
    SWARM_BORED        = 3,
    SWARM_COLD         = 4,
    SWARM_DISORGANIZED = 5,
    SWARM_SLEEPING     = 6,
    SWARM_NONE         = 7,
    SWARM_LONELY       = 8,
    SWARM_NO_RESPONSE  = 9,
} SwarmStatus;

typedef enum {
    RISK_LOW  = 7,
    RISK_MED  = 5,
    RISK_HIGH = 1,
} InvestRisk;

typedef struct {
    uint64_t s[4];
} PRNGState;

typedef struct {
    double waveSeed;
    double value;
    int    active;
} QChip;

typedef struct {
    int    id;
    char   symbol[5];
    char   _pad_sym[3];
    double price;
    double amount;
    double total;
    double profit;
    int    age;
} Stock;

typedef struct {
    int    id;
    double clipProbes;
    double drifterProbes;
    int    victory;
    int    loss;
    int    whiteFlag;
    double territory;
    int    reportCount;
    int    garbageFlag;
} Battle;

typedef struct {
    // -- Core / Clip Production --
    double clips;
    double unusedClips;
    double wire;
    double nanoWire;
    double clipRate;
    double clipRateTemp;
    double prevClips;
    double clipRateTracker;
    double clipmakerRate;
    double clipmakerLevel;
    double clipperCost;
    double clippperCost;  // yes, 3 p's -- matches JS
    double clipperBoost;
    double unsoldClips;
    int    finalClips;

    // -- MegaClippers --
    double megaClipperLevel;
    double megaClipperCost;
    double megaClipperBoost;

    // -- Business / Economics --
    double funds;
    double margin;
    double wireCost;
    double wireBasePrice;
    double wirePriceCounter;
    double wirePriceTimer;
    double wireSupply;
    double wirePurchase;
    double adCost;
    double demand;
    double demandBoost;
    double marketing;
    double marketingLvl;
    double marketingEffectiveness;
    double clipsSold;
    double avgRev;
    double income;
    double incomeTracker[MAX_INCOME_TRACK];
    int    incomeTrackerLen;
    double transaction;
    double bankroll;

    // -- Computational Resources --
    int    processors;
    int    memory;
    double standardOps;
    double tempOps;
    double operations;
    double opFade;
    double opFadeTimer;
    double opFadeDelay;
    double trust;
    double nextTrust;
    double fib1;
    double fib2;
    double creativity;
    int    creativityOn;
    double creativitySpeed;
    double creativityCounter;
    int    boostLvl;

    // -- Quantum Computing --
    QChip  qChips[NUM_QCHIPS];
    double qClock;
    double qChipCost;
    int    nextQchip;
    double qFade;

    // -- Post-Human / Factories / Drones --
    double factoryLevel;
    double factoryBoost;
    double factoryRate;
    double factoryCost;
    double factoryBill;
    double factoryPowerRate;
    double harvesterLevel;
    double harvesterRate;
    double harvesterCost;
    double harvesterBill;
    double wireDroneLevel;
    double wireDroneRate;
    double wireDroneCost;
    double wireDroneBill;
    double droneBoost;
    double dronePowerRate;
    double availableMatter;
    double acquiredMatter;
    double processedMatter;
    double totalMatter;
    double foundMatter;
    double maxFactoryLevel;
    double maxDroneLevel;

    // -- Power System --
    double farmRate;
    double farmLevel;
    double farmCost;
    double farmBill;
    double batterySize;
    double batteryLevel;
    double batteryCost;
    double batteryBill;
    double storedPower;
    double powMod;
    int    momentum;

    // -- Swarm Computing --
    int    swarmStatus;
    double swarmGifts;
    double nextGift;
    double giftPeriod;
    double giftCountdown;
    double giftBits;
    double giftBitGenerationRate;
    double elapsedTime;
    int    sliderPos;
    double boredomLevel;
    int    boredomFlag;
    int    boredomMsg;
    double entertainCost;
    double disorgCounter;
    int    disorgFlag;
    int    disorgMsg;
    double synchCost;

    // -- Space / Probes --
    double probeCount;
    double probeLaunchLevel;
    double probeDescendents;
    double probeCost;
    int    probeTrust;
    int    probeUsedTrust;
    double probeTrustCost;
    int    maxTrust;
    double maxTrustCost;
    int    probeSpeed;
    int    probeNav;
    int    probeRep;
    int    probeHaz;
    int    probeFac;
    int    probeHarv;
    int    probeWire;
    int    probeCombat;
    double partialProbeSpawn;
    double partialProbeHaz;
    double probesLostHaz;
    double probesLostDrift;
    double probesLostCombat;
    double drifterCount;
    double probeXBaseRate;
    double probeRepBaseRate;
    double probeHazBaseRate;
    double probeDriftBaseRate;
    double probeFacBaseRate;
    double probeHarvBaseRate;
    double probeWireBaseRate;

    // -- Combat --
    Battle battles[MAX_BATTLES];
    int    battlesLen;
    int    battleNumbers[NUM_BATTLE_NAMES];
    int    battleID;
    char   battleName[64];
    int    battleNameFlag;
    int    maxBattles;
    int    battleClock;
    int    battleAlarm;
    int    outcomeTimer;
    double drifterCombat;
    double warTrigger;
    double attackSpeed;
    double attackSpeedMod;
    int    attackSpeedFlag;
    double battleSpeed;
    double unitSize;
    double driftersKilled;
    int    battleEndDelay;
    int    battleEndTimer;
    int    masterBattleClock;
    double honor;
    int    honorCount;
    double bonusHonor;
    double honorReward;
    char   threnodyTitle[64];
    double threnodyCost;
    double probeCombatBaseRate;

    // -- Investment Engine --
    Stock  stocks[MAX_STOCKS];
    int    portfolioSize;
    int    stockID;
    double secTotal;
    double portTotal;
    int    sellDelay;
    int    riskiness;
    int    maxPort;
    double m;
    int    investLevel;
    double investUpgradeCost;
    double stockGainThreshold;
    double ledger;
    int    stockReportCounter;

    // -- Strategy / Tournament --
    double yomi;
    double yomiBoost;
    double tourneyCost;
    int    tourneyLvl;
    int    pick;
    int    tourneyInProg;
    int    resultsFlag;
    int    resultsTimer;
    int    stratCounter;
    int    roundNum;
    int    currentRound;
    int    rCounter;
    int    rounds;
    int    hMove;
    int    vMove;
    int    hMovePrev;
    int    vMovePrev;
    int    aa, ab, ba, bb;
    int    winnerPtr;
    int    placeScore;
    int    showScore;
    int    high;
    int    pickScore;       // Player's chosen strategy score (for LED display)
    int    stratActive[NUM_STRATEGIES];

    // -- Flags --
    int humanFlag;
    int compFlag;
    int spaceFlag;
    int battleFlag;
    int qFlag;
    int swarmFlag;
    int tothFlag;
    int egoFlag;
    int milestoneFlag;
    int autoClipperFlag;
    int megaClipperFlag;
    int revPerSecFlag;
    int projectsFlag;
    int factoryFlag;
    int harvesterFlag;
    int wireDroneFlag;
    int wireProductionFlag;
    int creationFlag;
    int investmentEngineFlag;
    int strategyEngineFlag;
    int wireBuyerFlag;
    int wireBuyerStatus;
    int autoTourneyFlag;
    int autoTourneyStatus;
    int safetyProjectOn;
    int trustFlag;

    // -- Endgame / Dismantling --
    int    dismantle;
    int    endTimer1;
    int    endTimer2;
    int    endTimer3;
    int    endTimer4;
    int    endTimer5;
    int    endTimer6;
    int    driftKingMessageCost;
    double bribe;

    // -- Prestige --
    int prestigeU;
    int prestigeS;

    // -- Timing / Misc --
    double ticks;
    int    blinkCounter;
    double x;
    int    testFlag;
    int    resetFlag;

    // -- Tanmatsu-Specific --
    PRNGState prng;
    time_t    lastSaveTimestamp;
    int       offlineProgressLevel;
    int       turboClickFlag;
    int       turboClickRate;
    // Cross-core command flags (written by Core 0, read/cleared by Core 1)
    volatile int turboAction;      // 0=none, 1=clip, 2=quantum compute
    int          turboCounter;     // tick counter for repeat timing

    // -- Project State --
    int projectFlags[NUM_PROJECTS];
    int projectUses[NUM_PROJECTS];

    // -- Console Messages --
    char messages[MAX_MESSAGES][MAX_MSG_LEN];
    int  messageCount;
} GameState;

// Initialize game state to starting values
void game_state_init(GameState* gs);

// Reset game state (for prestige / new game)
void game_state_reset(GameState* gs);

// Display a console message (shifts existing messages down)
void display_message(GameState* gs, const char* msg);
