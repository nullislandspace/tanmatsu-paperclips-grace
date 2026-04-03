#pragma once

#include <stdbool.h>

#include "bsp/input.h"
#include "game_projects.h"
#include "game_save.h"
#include "game_state.h"

// Logical tab IDs
typedef enum {
    TAB_CLIPS,
    TAB_BUSINESS,
    TAB_COMPUTE,
    TAB_PROJECTS,
    TAB_STRATEGY,
    TAB_INVEST,
    TAB_DRONES,
    TAB_POWER,
    TAB_COUNT,
} TabID;

// Maximum tabs displayed at once (F1-F6)
#define MAX_VISIBLE_TABS 6

// Overlay states
typedef enum {
    OVERLAY_NONE,
    OVERLAY_SAVE_LOAD,
    OVERLAY_CHEAT_MENU,
} OverlayState;

// Save/load mode
typedef enum {
    SAVE_MODE_SAVE,
    SAVE_MODE_LOAD,
} SaveLoadMode;

// UI state (lives alongside GameState, not saved)
typedef struct {
    // Tab state: fixed F-key assignments, tabVisible marks which are active
    TabID   visibleTabs[MAX_VISIBLE_TABS];  // F1-F6 -> TabID mapping
    bool    tabVisible[MAX_VISIBLE_TABS];   // which slots are currently active
    int     visibleTabCount;
    int     activeTabIdx;  // index into visibleTabs (0-5 = F1-F6)

    // Per-tab focus (scroll position for projects, selected item for others)
    int     focusIdx[TAB_COUNT];
    int     scrollOffset[TAB_COUNT];

    // Overlay
    OverlayState overlay;

    // Save/Load menu
    SaveLoadMode saveLoadMode;
    int          saveSlotIdx;
    SaveSlotInfo slotCache[SAVE_SLOT_COUNT];
    bool         slotExists[SAVE_SLOT_COUNT];
    bool         slotCacheDirty;  // true = needs re-peek

    // Cheat menu
    int cheatMenuIdx;
    int cheatMenuSection;  // 0=cheats, 1=jumps

    // Quantum compute display
    double  qCompResult;  // Last result from quantum compute (fades with qFade)

    // Modifier keys (updated each frame from events)
    bool    shiftHeld;
    bool    ctrlHeld;
    bool    altHeld;

    // Takeover animation (human -> post-human transition)
    int     takeoverTimer;  // frames remaining, 0 = inactive
} UIState;

// Initialize UI state
void ui_init(UIState* ui);

// Determine which tabs are visible based on game flags
void ui_update_tabs(UIState* ui, const GameState* gs);

// Get the TabID of the currently active tab
TabID ui_active_tab(const UIState* ui);

// Process an input event. Returns true if the event was consumed.
bool ui_handle_input(UIState* ui, GameState* gs, ProjectManager* pm, const bsp_input_event_t* event);

// Get bulk-buy multiplier based on modifier keys
int ui_get_multiplier(const UIState* ui);
