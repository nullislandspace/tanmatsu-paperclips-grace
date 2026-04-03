#pragma once

#include <stdbool.h>

#include "game_state.h"

// Project slot indices - maps original JS project IDs to array positions
// We use the original project ID numbers directly as indices into projectFlags/projectUses
// Since NUM_PROJECTS=100, IDs > 99 need mapping to available slots

// Project IDs matching the JS original (0-99 used directly)
// IDs >= 100 are mapped to slots 70-99
#define PROJ_IMPROVED_AUTOCLIPPERS     1
#define PROJ_BEG_FOR_WIRE              2
#define PROJ_CREATIVITY                3
#define PROJ_EVEN_BETTER_AUTO          4
#define PROJ_OPTIMIZED_AUTO            5
#define PROJ_LIMERICK                  6
#define PROJ_IMPROVED_WIRE             7
#define PROJ_OPTIMIZED_WIRE            8
#define PROJ_MICROLATTICE              9
#define PROJ_SPECTRAL_FROTH            10
#define PROJ_NEW_SLOGAN                11
#define PROJ_CATCHY_JINGLE             12
#define PROJ_LEXICAL_PROCESSING        13
#define PROJ_COMBINATORY_HARMONICS     14
#define PROJ_HADWIGER_PROBLEM          15
#define PROJ_HADWIGER_CLIP             16
#define PROJ_TOTH_SAUSAGE              17
#define PROJ_TOTH_TUBULE               18
#define PROJ_DONKEY_SPACE              19
#define PROJ_STRATEGIC_MODELING        20
#define PROJ_ALGORITHMIC_TRADING       21
#define PROJ_MEGACLIPPERS              22
#define PROJ_IMPROVED_MEGA             23
#define PROJ_EVEN_BETTER_MEGA          24
#define PROJ_OPTIMIZED_MEGA            25
#define PROJ_WIREBUYER                 26
#define PROJ_CEV                       27
#define PROJ_CURE_CANCER               28
#define PROJ_WORLD_PEACE               29
#define PROJ_GLOBAL_WARMING            30
#define PROJ_MALE_BALDNESS             31
#define PROJ_HYPNO_HARMONICS           34
#define PROJ_RELEASE_HYPNODRONES       35
#define PROJ_HOSTILE_TAKEOVER          37
#define PROJ_FULL_MONOPOLY             38
#define PROJ_TOKEN_GOODWILL            40
#define PROJ_NANOSCALE_WIRE            41
#define PROJ_REVTRACKER                42
#define PROJ_HARVESTER_DRONES          43
#define PROJ_WIRE_DRONES               44
#define PROJ_CLIP_FACTORIES            45
#define PROJ_SPACE_EXPLORATION         46
#define PROJ_QUANTUM_COMPUTING         50
#define PROJ_PHOTONIC_CHIP             51
#define PROJ_STRATEGY_A100             60
#define PROJ_STRATEGY_B100             61
#define PROJ_STRATEGY_GREEDY           62
#define PROJ_STRATEGY_GENEROUS         63
#define PROJ_STRATEGY_MINIMAX          64
#define PROJ_STRATEGY_TFT              65
#define PROJ_STRATEGY_BEAT_LAST        66

// Mapped slots for IDs >= 100
#define PROJ_QUANTUM_FOAM              69  // 10b in JS
#define PROJ_HYPNODRONES               70  // 70 in JS
#define PROJ_UPGRADED_FACTORIES        71  // 100
#define PROJ_HYPERSPEED_FACTORIES      72  // 101
#define PROJ_SUPPLY_CHAIN              73  // 102
#define PROJ_DRONE_COLLISION           74  // 110
#define PROJ_DRONE_ALIGNMENT           75  // 111
#define PROJ_DRONE_COHESION            76  // 112
#define PROJ_AUTOTOURNEY               77  // 118
#define PROJ_THEORY_OF_MIND            78  // 119
#define PROJ_OODA_LOOP                 79  // 120
#define PROJ_NAME_BATTLES              80  // 121
#define PROJ_MOMENTUM                  81  // 125
#define PROJ_SWARM_COMPUTING           82  // 126
#define PROJ_POWER_GRID                83  // 127
#define PROJ_STRATEGIC_ATTACHMENT      84  // 128
#define PROJ_ELLIPTIC_HULL             85  // 129
#define PROJ_REBOOT_SWARM              86  // 130
#define PROJ_COMBAT                    87  // 131
#define PROJ_MONUMENT                  88  // 132
#define PROJ_THRENODY                  89  // 133
#define PROJ_GLORY                     90  // 134
#define PROJ_MEMORY_RELEASE            91  // 135
#define PROJ_EMPEROR_MSG0              52  // 140
#define PROJ_EMPEROR_MSG1              53  // 141
#define PROJ_EMPEROR_MSG2              54  // 142
#define PROJ_EMPEROR_MSG3              55  // 143
#define PROJ_EMPEROR_MSG4              56  // 144
#define PROJ_EMPEROR_MSG5              57  // 145
#define PROJ_EMPEROR_MSG6              58  // 146
#define PROJ_ACCEPT                    47  // 147
#define PROJ_REJECT                    48  // 148
#define PROJ_UNIVERSE_NEXT_DOOR        92  // 200
#define PROJ_UNIVERSE_WITHIN           93  // 201
#define PROJ_DISMANTLE_PROBES          32  // 210
#define PROJ_DISMANTLE_SWARM           33  // 211
#define PROJ_DISMANTLE_FACTORIES       36  // 212
#define PROJ_DISMANTLE_STRATEGY        39  // 213
#define PROJ_DISMANTLE_QUANTUM         94  // 214
#define PROJ_DISMANTLE_PROCESSORS      95  // 215
#define PROJ_DISMANTLE_MEMORY          96  // 216
#define PROJ_QUANTUM_TEMPORAL          97  // 217
#define PROJ_LIMERICK_CONT             98  // 218
#define PROJ_XAVIER_REINIT             99  // 219
#define PROJ_TOKEN_GOODWILL_B          49  // 40b

// Tanmatsu-specific projects
// Tanmatsu-specific projects (slots 100+ to avoid collisions with original game)
#define PROJ_RAPID_KEYPRESSING        100  // turbo click level 1
#define PROJ_RAPID_KEYPRESSING_2      101  // turbo click level 2
#define PROJ_RAPID_KEYPRESSING_3      102  // turbo click level 3
#define PROJ_RAPID_KEYPRESSING_4      103  // turbo click level 4
#define PROJ_RAPID_KEYPRESSING_5      104  // turbo click level 5
#define PROJ_OFFLINE_1                105  // offline progress tier 1
#define PROJ_OFFLINE_2                106  // offline progress tier 2
#define PROJ_OFFLINE_3                107  // offline progress tier 3
#define PROJ_OFFLINE_4                108  // offline progress tier 4
#define PROJ_OFFLINE_5                109  // offline progress tier 5

// Total number of defined projects
#define NUM_PROJECT_DEFS 106

typedef struct {
    int         slot;         // Index into projectFlags/projectUses
    const char* title;
    const char* priceTag;
    const char* description;
    const char* effectTag;    // Short mechanical effect description (e.g. "+1 trust")
    int         initialUses;  // Usually 1
    bool (*trigger_fn)(const GameState*);
    bool (*cost_fn)(const GameState*);
    void (*effect_fn)(GameState*);
    const char* message;      // Display message on activation (NULL = none)
} ProjectDef;

// Static project definition table
extern const ProjectDef g_project_defs[];
extern const int        g_num_project_defs;

// Active project tracking (not in GameState - rebuilt each tick)
#define MAX_ACTIVE_PROJECTS 32

typedef struct {
    int activeProjects[MAX_ACTIVE_PROJECTS];  // indices into g_project_defs
    int activeCount;
} ProjectManager;

// Initialize project manager
void project_manager_init(ProjectManager* pm);

// Check triggers and manage active projects (per main tick)
void manage_projects(GameState* gs, ProjectManager* pm);

// Activate (purchase) a specific project by its def index
void activate_project(GameState* gs, ProjectManager* pm, int defIdx);

// Check if a project is affordable (for UI enable/disable)
bool project_is_affordable(const GameState* gs, int defIdx);

// Repair projectUses after loading a save.
// Resets uses to 1 for any unpurchased project whose uses were consumed
// by a previous manage_projects pass (uses=0 but flag=0).
void projects_repair_uses(GameState* gs);
