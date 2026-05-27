#include <stdio.h>

#include "esp_timer.h"

#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "bsp/led.h"
#include "bsp/power.h"
#include "gl_input.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "hal/lcd_types.h"
#include "nvs_flash.h"
#include "pax_fonts.h"
#include "pax_gfx.h"
#include "pax_text.h"
#include "portmacro.h"

#include "fastopen.h"
#include "graceloader.h"
#include "pax_codecs.h"
#include "esp_heap_caps.h"

#include "game_cheats.h"
#include "game_format.h"
#include "game_projects.h"
#include "game_save.h"
#include "game_state.h"
#include "game_tick.h"
#include "icons.h"
#include "ui_core.h"
#include "ui_render.h"
#include "rendertext.h"

#define DEBUG_FPS

static char const TAG[] = "main";

// Display globals
static size_t                       display_h_res        = 0;
static size_t                       display_v_res        = 0;
static bsp_display_color_format_t   display_color_format = BSP_DISPLAY_COLOR_FORMAT_16_565RGB;
static bsp_display_endianness_t     display_data_endian  = BSP_DISPLAY_ENDIAN_LITTLE;
static pax_buf_t                    fb                   = {0};
static QueueHandle_t                input_event_queue    = NULL;

// Double-buffered game state for dual-core architecture
static GameState     g_state[2];
static volatile int  active_index = 0;
static portMUX_TYPE  state_mux    = portMUX_INITIALIZER_UNLOCKED;

// When true, Core 1 skips ticking and waits. Set by Core 0 before replacing
// game state (load/reset), cleared after the new state is written to both buffers.
static volatile bool g_game_paused = false;

// Cross-core tournament request flag (not in GameState to preserve save format).
// Core 0 sets to 1, Core 1 reads and clears.
volatile int g_tourney_requested = 0;

// Project manager shared with game task
static ProjectManager g_pm;
static portMUX_TYPE   pm_mux = portMUX_INITIALIZER_UNLOCKED;

#define GAME_TICK_MS 10

static void blit(void) {
    bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(&fb));
}


// Full in-game load: pause Core 1, load slot, replay offline, repair projects, unpause.
// Called from Core 0 (UI thread). Returns 0 on success.
int load_game_into_engine(int slot) {
    g_game_paused = true;
    vTaskDelay(pdMS_TO_TICKS(GAME_TICK_MS * 2));

    // Load + offline replay + project repair, all while Core 1 is paused
    int rc = game_load_full(&g_state[0], slot, &fb, blit);
    if (rc == 0) {
        memcpy(&g_state[1], &g_state[0], sizeof(GameState));
    }

    g_game_paused = false;
    return rc;
}

// Full in-game reset: pause Core 1, init fresh state, unpause.
void reset_game_in_engine(void) {
    g_game_paused = true;
    vTaskDelay(pdMS_TO_TICKS(GAME_TICK_MS * 2));

    game_state_init(&g_state[0]);
    game_load_prestige(&g_state[0]);
    memcpy(&g_state[1], &g_state[0], sizeof(GameState));

    g_game_paused = false;
}

// Game logic task running on Core 1 at 100Hz
static void game_task(void* arg) {
    ProjectManager pm;
    project_manager_init(&pm);

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // If paused by Core 0 (state being replaced), spin-wait
        if (g_game_paused) {
            project_manager_init(&pm);  // Will rebuild from new state on resume
            while (g_game_paused) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            last_wake = xTaskGetTickCount();  // Reset timing after pause
        }

        GameState* active = &g_state[active_index];
        game_tick(active, &pm);

        int inactive = 1 - active_index;
        memcpy(&g_state[inactive], active, sizeof(GameState));

        portENTER_CRITICAL(&state_mux);
        active_index = inactive;
        portEXIT_CRITICAL(&state_mux);

        portENTER_CRITICAL(&pm_mux);
        memcpy(&g_pm, &pm, sizeof(ProjectManager));
        portEXIT_CRITICAL(&pm_mux);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(GAME_TICK_MS));
    }
}

static const GameState* get_render_state(void) {
    int idx;
    portENTER_CRITICAL(&state_mux);
    idx = 1 - active_index;
    portEXIT_CRITICAL(&state_mux);
    return &g_state[idx];
}

static void get_render_pm(ProjectManager* out) {
    portENTER_CRITICAL(&pm_mux);
    memcpy(out, &g_pm, sizeof(ProjectManager));
    portEXIT_CRITICAL(&pm_mux);
}

// ============================================================
// Startup menu: slot selection before game launches
// ============================================================

// Menu items: slot 0 (auto), slots 1-4, "New Game", "Quit to Launcher"
#define STARTUP_SLOTS      SAVE_SLOT_COUNT
#define STARTUP_NEW_GAME   SAVE_SLOT_COUNT
#define STARTUP_QUIT       (SAVE_SLOT_COUNT + 1)
#define STARTUP_ITEM_COUNT (SAVE_SLOT_COUNT + 2)

// Cached slot info for startup menu (peeked once, not per frame)
static SaveSlotInfo s_startup_slots[SAVE_SLOT_COUNT];
static bool         s_startup_slot_exists[SAVE_SLOT_COUNT];

static void startup_peek_slots(void) {
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        s_startup_slot_exists[i] = (game_save_peek(i, &s_startup_slots[i]) == 0);
    }
}

static void render_startup_menu(int cursor) {
    int w = pax_buf_get_width(&fb);
    int h = pax_buf_get_height(&fb);
    const pax_font_t* font = pax_font_sky_mono;

    pax_background(&fb, 0xFF1A1A2E);

    // Title
    rendertext_draw(&fb, 0xFFFFC107, font, 28, (w - 400) / 2, 40, "UNIVERSAL PAPERCLIPS");
    rendertext_draw(&fb, 0xFF808090, font, 14, (w - 200) / 2, 76, "Tanmatsu Edition");

    // Slot list
    int startY = 120;
    char buf[192];
    char numbuf[64];

    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        int y = startY + i * 40;
        bool focused = (cursor == i);

        if (focused) {
            pax_simple_rect(&fb, 0xFF2A2A4E, 60, y, w - 120, 36);
        }

        if (s_startup_slot_exists[i]) {
            const SaveSlotInfo* info = &s_startup_slots[i];
            const char* phase = "Human Era";
            if (info->dismantle > 0)      phase = "Dismantling";
            else if (info->spaceFlag)     phase = "Space Era";
            else if (!info->humanFlag)    phase = "Post-Human";

            number_cruncher(info->clips, 0, numbuf, sizeof(numbuf));

            // Format time played
            char timebuf[32];
            int secs = (int)(info->ticks / 100.0);
            int hrs  = secs / 3600;
            int mins = (secs % 3600) / 60;
            if (hrs > 0)
                snprintf(timebuf, sizeof(timebuf), "%dh %dm", hrs, mins);
            else
                snprintf(timebuf, sizeof(timebuf), "%dm", mins);

            if (i == 0) {
                snprintf(buf, sizeof(buf), "  Autosave  -  %s clips  %s  %s", numbuf, phase, timebuf);
            } else {
                snprintf(buf, sizeof(buf), "  Slot %d     -  %s clips  %s  %s", i, numbuf, phase, timebuf);
            }

            pax_col_t col = focused ? 0xFFFFC107 : 0xFFE0E0E0;
            rendertext_draw(&fb, col, font, 14, 70, y + 10, buf);
        } else {
            // Empty slot
            if (i == 0) {
                snprintf(buf, sizeof(buf), "  Autosave  -  (empty)");
            } else {
                snprintf(buf, sizeof(buf), "  Slot %d     -  (empty)", i);
            }
            pax_col_t col = focused ? 0xFF806020 : 0xFF505060;
            rendertext_draw(&fb, col, font, 14, 70, y + 10, buf);
        }
    }

    // New Game
    {
        int y = startY + SAVE_SLOT_COUNT * 40 + 10;
        bool focused = (cursor == STARTUP_NEW_GAME);
        if (focused) pax_simple_rect(&fb, 0xFF2A2A4E, 60, y, w - 120, 36);
        pax_col_t col = focused ? 0xFFFFC107 : 0xFFE0E0E0;
        rendertext_draw(&fb, col, font, 14, 70, y + 10, "  New Game");
    }

    // Quit to Launcher
    {
        int y = startY + SAVE_SLOT_COUNT * 40 + 50;
        bool focused = (cursor == STARTUP_QUIT);
        if (focused) pax_simple_rect(&fb, 0xFF2A2A4E, 60, y, w - 120, 36);
        pax_col_t col = focused ? 0xFFFFC107 : 0xFFE0E0E0;
        rendertext_draw(&fb, col, font, 14, 70, y + 10, "  Quit to Launcher");
    }

    // Hints
    rendertext_draw(&fb, 0xFF505060, font, 11, (w - 300) / 2, h - 30, "Up/Down = select   Enter = confirm   ESC = quit");

    blit();
}

// Returns the chosen slot (0-4), or -1 for new game, or -2 for quit
static int run_startup_menu(void) {
    // Peek all slots once before rendering
    startup_peek_slots();

    int cursor = 0;

    // Start on first occupied slot, or New Game if all empty
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        if (s_startup_slot_exists[i]) { cursor = i; break; }
        if (i == SAVE_SLOT_COUNT - 1) cursor = STARTUP_NEW_GAME;
    }

    while (1) {
        render_startup_menu(cursor);

        bsp_input_event_t event;
        if (xQueueReceive(input_event_queue, &event, portMAX_DELAY) != pdTRUE) continue;

        if (event.type == INPUT_EVENT_TYPE_NAVIGATION && event.args_navigation.state) {
            int key = event.args_navigation.key;

            if (key == BSP_INPUT_NAVIGATION_KEY_UP) {
                if (cursor > 0) cursor--;
            } else if (key == BSP_INPUT_NAVIGATION_KEY_DOWN) {
                if (cursor < STARTUP_ITEM_COUNT - 1) cursor++;
            } else if (key == BSP_INPUT_NAVIGATION_KEY_RETURN || key == BSP_INPUT_NAVIGATION_KEY_GAMEPAD_A) {
                if (cursor < STARTUP_SLOTS) {
                    if (s_startup_slot_exists[cursor]) return cursor;
                    // Empty slot — treat as new game
                    return -1;
                } else if (cursor == STARTUP_NEW_GAME) {
                    return -1;
                } else if (cursor == STARTUP_QUIT) {
                    return -2;
                }
            } else if (key == BSP_INPUT_NAVIGATION_KEY_ESC) {
                return -2;
            }
        }
    }
}

// ============================================================
// Title screen: 2s fade in, 3s hold, 1.5s binary wipe left-to-right to black
// ============================================================

static void run_title_screen(pax_buf_t* fb_main) {
    int w = pax_buf_get_width(fb_main);
    int h = pax_buf_get_height(fb_main);

    // Build path to title image
    const char* basepath = graceloader_get_install_basepath();
    char path[256];
    snprintf(path, sizeof(path), "%s/up_title.png", basepath);

    // Load title PNG into a separate ARGB buffer
    size_t img_size = w * h * 4;
    void* img_buf = heap_caps_calloc(1, img_size, MALLOC_CAP_SPIRAM);
    if (!img_buf) {
        ESP_LOGW(TAG, "Failed to allocate title image buffer");
        return;
    }

    pax_buf_t title;
    pax_buf_init(&title, img_buf, w, h, PAX_BUF_32_8888ARGB);

    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "Title image not found: %s", path);
        pax_buf_destroy(&title);
        free(img_buf);
        return;
    }

    if (!pax_insert_png_fd(&title, f, 0, 0, 0)) {
        ESP_LOGW(TAG, "Failed to decode title image");
        fastclose(f);
        pax_buf_destroy(&title);
        free(img_buf);
        return;
    }
    fastclose(f);

    ESP_LOGI(TAG, "Title screen loaded from %s", path);

    // Phase 1: Fade in (2s)
    const int64_t fade_in_us = 2000000;
    int64_t start_us = esp_timer_get_time();

    while (1) {
        int64_t elapsed = esp_timer_get_time() - start_us;
        if (elapsed >= fade_in_us) break;

        float alpha = (float)elapsed / (float)fade_in_us;
        if (alpha > 1.0f) alpha = 1.0f;

        pax_draw_image_op(fb_main, &title, 0, 0);
        uint8_t darkness = (uint8_t)((1.0f - alpha) * 255.0f);
        pax_simple_rect(fb_main, (pax_col_t)((uint32_t)darkness << 24), 0, 0, w, h);

        bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(fb_main));
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Phase 2: Hold (3s)
    pax_draw_image_op(fb_main, &title, 0, 0);
    bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(fb_main));
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Phase 3: Binary wipe left-to-right (2s)
    // Overwrite columns with random 0/1 characters on black background
    const pax_font_t* wipe_font = pax_font_sky_mono;
    int char_w = 10;
    int char_h = 14;
    int cols = w / char_w;
    int rows = h / char_h;
    const int64_t wipe_us = 1500000;

    // Simple xorshift PRNG for visual noise
    uint32_t rng = (uint32_t)(esp_timer_get_time() & 0xFFFFFFFF);

    // Title image is already in the framebuffer from the hold phase.
    // The wipe front is 10 columns wide with binary digits.
    // 10 columns behind the front, black out the trail.
    // Total travel: cols + trail width (so everything goes black).
    int trail = 10;
    int total_steps = cols + trail;
    int prev_front = -1;
    start_us = esp_timer_get_time();

    while (1) {
        int64_t elapsed = esp_timer_get_time() - start_us;
        if (elapsed >= wipe_us) break;

        float progress = (float)elapsed / (float)wipe_us;
        int front = (int)(progress * total_steps);
        if (front >= total_steps) front = total_steps - 1;

        for (int col = prev_front + 1; col <= front; col++) {
            // Black out the column that falls off the back of the trail
            int black_col = col - trail;
            if (black_col >= 0 && black_col < cols) {
                pax_simple_rect(fb_main, 0xFF000000, black_col * char_w, 0, char_w, h);
            }

            // Draw binary digits at the front column
            if (col < cols) {
                int cx = col * char_w;
                pax_simple_rect(fb_main, 0xFF000000, cx, 0, char_w, h);

                for (int row = 0; row < rows; row++) {
                    rng ^= rng << 13;
                    rng ^= rng >> 17;
                    rng ^= rng << 5;

                    char digit[2] = { '0' + (rng & 1), '\0' };
                    uint8_t bright = 40 + (rng >> 2) % 180;
                    pax_col_t color = (0xFF000000) | ((uint32_t)bright << 8);

                    rendertext_draw(fb_main, color, wipe_font, 11,
                                    cx, row * char_h, digit);
                }
            }
        }
        prev_front = front;

        bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(fb_main));
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Ensure everything is black
    pax_background(fb_main, 0xFF000000);
    bsp_display_blit(0, 0, display_h_res, display_v_res, pax_buf_get_pixels(fb_main));

    pax_buf_destroy(&title);
    free(img_buf);
}

// ============================================================
// app_main
// ============================================================

void app_main(void) {
    gpio_install_isr_service(0);

    // Initialize NVS
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        res = nvs_flash_erase();
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS flash: %d", res);
            return;
        }
        res = nvs_flash_init();
    }
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS flash: %d", res);
        return;
    }

    // Initialize BSP
    const bsp_configuration_t bsp_configuration = {
        .display = {
            .requested_color_format = BSP_DISPLAY_COLOR_FORMAT_24_888RGB,
            .num_fbs                = 1,
        },
    };
    res = bsp_device_initialize(&bsp_configuration);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BSP: %d", res);
        return;
    }

    res = bsp_display_get_parameters(&display_h_res, &display_v_res, &display_color_format, &display_data_endian);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get display parameters: %d", res);
        return;
    }

    pax_buf_type_t format = PAX_BUF_24_888RGB;
    switch (display_color_format) {
        case BSP_DISPLAY_COLOR_FORMAT_16_565RGB: format = PAX_BUF_16_565RGB; break;
        case BSP_DISPLAY_COLOR_FORMAT_24_888RGB: format = PAX_BUF_24_888RGB; break;
        default: break;
    }

    bsp_display_rotation_t display_rotation = bsp_display_get_default_rotation();
    pax_orientation_t      orientation      = PAX_O_UPRIGHT;
    switch (display_rotation) {
        case BSP_DISPLAY_ROTATION_90:  orientation = PAX_O_ROT_CCW;  break;
        case BSP_DISPLAY_ROTATION_180: orientation = PAX_O_ROT_HALF; break;
        case BSP_DISPLAY_ROTATION_270: orientation = PAX_O_ROT_CW;   break;
        default: break;
    }

    pax_buf_init(&fb, NULL, display_h_res, display_v_res, format);
    pax_buf_reversed(&fb, display_data_endian == BSP_DISPLAY_ENDIAN_BIG);
    pax_buf_set_orientation(&fb, orientation);

    // Input queue — graceloader merges native + USB keyboard
    ESP_ERROR_CHECK(gl_input_get_queue(&input_event_queue));

    // LEDs off
    for (int i = 0; i < 6; i++) bsp_led_set_pixel(i, 0x000000);
    bsp_led_send();
    bsp_led_set_mode(false);

    // Filesystems are already mounted by the graceloader (/int and /sd)

    // Title screen
    run_title_screen(&fb);

    // Load F-key icons from /int filesystem
    icons_load();

    // Initialize save system
    game_save_init();

    // --- Startup menu ---
    int choice = run_startup_menu();

    if (choice == -2) {
        // Quit to launcher
        bsp_device_restart_to_launcher();
        return;
    }

    // --- Load or create game state ---
    game_state_init(&g_state[0]);

    if (choice >= 0) {
        // Load from chosen slot (includes offline replay and project repair)
        if (game_load_full(&g_state[0], choice, &fb, blit) != 0) {
            ESP_LOGW(TAG, "Failed to load slot %d, starting new game", choice);
        }
    } else {
        // New game — load prestige if available
        game_load_prestige(&g_state[0]);
    }

    memcpy(&g_state[1], &g_state[0], sizeof(GameState));
    project_manager_init(&g_pm);

    ESP_LOGI(TAG, "GameState: %zu bytes, starting game on Core 1", sizeof(GameState));

    // Start auto-save background task
    game_save_task_start();

    xTaskCreatePinnedToCore(game_task, "game_task", 8192, NULL, 5, NULL, 1);

    // UI state
    UIState ui;
    ui_init(&ui);

    // Track human->post-human transition
    bool was_human = true;

#ifdef DEBUG_FPS
    // FPS tracking
    int64_t fps_last_time = esp_timer_get_time();
    int     fps_frame_count = 0;
    // Timing accumulators (microseconds)
    int64_t t_input_total = 0, t_render_total = 0, t_blit_total = 0, t_leds_total = 0;
#endif

    // --- Main game loop: input + render on Core 0 ---
    while (1) {
#ifdef DEBUG_FPS
        int64_t t0, t1;
        t0 = esp_timer_get_time();
#endif

        // Poll input events (discard during takeover animation)
        // Only call ui_handle_input ONCE per event — it mutates UIState (focus,
        // overlays) which must not be applied twice. Game state changes propagate
        // to the other buffer via Core 1's copy on the next tick (within 10ms).
        bsp_input_event_t event;
        while (xQueueReceive(input_event_queue, &event, 0) == pdTRUE) {
            if (ui.takeoverTimer <= 0) {
                ui_handle_input(&ui, &g_state[active_index], &g_pm, &event);
            }
        }

        // Poll physical spacebar state to drive turbo-click.
        // Keyboard events only fire once on press — no repeats — so we
        // poll the key directly to know when it's released.
        bool space_held = false;
        bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SPACE_L, &space_held);
        if (!space_held) bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SPACE_M, &space_held);
        if (!space_held) bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_SPACE_R, &space_held);
        if (!space_held) {
            g_state[0].turboAction = 0;
            g_state[1].turboAction = 0;
        }

        // Handle pending fast-forward cheat (needs fb + blit, so runs here)
#if CHEATS_ENABLED
        {
            int ff_sec = cheat_ff_pending();
            if (ff_sec > 0) {
                cheat_ff_clear();
                g_game_paused = true;
                vTaskDelay(pdMS_TO_TICKS(GAME_TICK_MS * 2));

                ProjectManager ff_pm;
                project_manager_init(&ff_pm);
                int ticks = ff_sec * 100;
                game_replay_with_progress(&g_state[0], &ff_pm, &fb, blit,
                                          "Fast Forward...", (double)ff_sec, ticks);
                memcpy(&g_state[1], &g_state[0], sizeof(GameState));

                g_game_paused = false;
            }
        }
#endif

        // Get latest game state snapshot for rendering
        const GameState* rs = get_render_state();

        // Detect human -> post-human transition
        if (was_human && !rs->humanFlag) {
            ui.takeoverTimer = 70;  // ~7 seconds of animation
        }
        was_human = rs->humanFlag;

        // Update tab visibility based on game flags
        ui_update_tabs(&ui, rs);

        // Get project manager snapshot
        ProjectManager pm_snap;
        get_render_pm(&pm_snap);

#ifdef DEBUG_FPS
        t1 = esp_timer_get_time();
        t_input_total += t1 - t0;
        t0 = esp_timer_get_time();
#endif

        // Render: takeover animation or normal UI
        if (!ui_render_takeover(&fb, &ui)) {
            ui_render(&fb, rs, &ui, &pm_snap);
        }

#ifdef DEBUG_FPS
        t1 = esp_timer_get_time();
        t_render_total += t1 - t0;
        t0 = esp_timer_get_time();
#endif

        blit();

#ifdef DEBUG_FPS
        t1 = esp_timer_get_time();
        t_blit_total += t1 - t0;

        // FPS measurement: log average every 5 seconds
        fps_frame_count++;
        int64_t now = esp_timer_get_time();
        int64_t elapsed = now - fps_last_time;
        if (elapsed >= 5000000) {
            float avg_fps = (float)fps_frame_count / ((float)elapsed / 1000000.0f);
            float ms_input  = (float)t_input_total  / (fps_frame_count * 1000.0f);
            float ms_render = (float)t_render_total / (fps_frame_count * 1000.0f);
            float ms_blit   = (float)t_blit_total   / (fps_frame_count * 1000.0f);
            float ms_leds   = (float)t_leds_total   / (fps_frame_count * 1000.0f);
            ESP_LOGI(TAG, "FPS: %.1f | input: %.1fms  render: %.1fms  blit: %.1fms  leds: %.1fms",
                     avg_fps, ms_input, ms_render, ms_blit, ms_leds);
            fps_frame_count = 0;
            fps_last_time = now;
            t_input_total = t_render_total = t_blit_total = t_leds_total = 0;
        }

        t0 = esp_timer_get_time();
#endif

        // Update LEDs (takeover animation handles its own)
        if (ui.takeoverTimer <= 0) {
            ui_update_leds(rs, &pm_snap);
        }

#ifdef DEBUG_FPS
        t1 = esp_timer_get_time();
        t_leds_total += t1 - t0;
#endif

        // Yield to avoid starving other tasks
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
