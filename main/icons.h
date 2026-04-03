#pragma once

#include "pax_gfx.h"
#include <stdbool.h>

typedef enum {
    ICON_ESC,
    ICON_F1,
    ICON_F2,
    ICON_F3,
    ICON_F4,
    ICON_F5,
    ICON_F6,
    ICON_KEY_COUNT,
} icon_key_t;

// Load F1-F6 + ESC icons from /int/icons/*.png
void icons_load(void);

// Get a loaded icon buffer (NULL if not loaded)
pax_buf_t* icons_get(icon_key_t key);

// Whether any icons failed to load
bool icons_any_missing(void);
