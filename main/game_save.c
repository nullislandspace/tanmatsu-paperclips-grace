#include "game_save.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_nbt.h"
#include "game_save_nbt.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "fastopen.h"
#include "game_format.h"
#include "game_tick.h"
#include "pax_fonts.h"
#include "pax_gfx.h"
#include "pax_text.h"
#include "rendertext.h"

static const char* TAG = "game_save";

static void get_slot_path(int slot, char* buf, size_t size) {
    snprintf(buf, size, SAVE_PATH_PREFIX "/save%d.bin", slot);
}

void game_save_init(void) {
    struct stat st;
    if (stat(SAVE_PATH_PREFIX, &st) != 0) {
        mkdir(SAVE_PATH_PREFIX, 0755);
        ESP_LOGI(TAG, "Created save directory: " SAVE_PATH_PREFIX);
    }
}

int game_save(GameState* gs, int slot) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return -1;

    gs->lastSaveTimestamp = time(NULL);

    char path[128];
    get_slot_path(slot, path, sizeof(path));

    FILE* f = fastopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return -1;
    }

    NbtWriter w;
    nbt_write_open(&w, f);
    nbt_write_compound(&w, "root");
    game_save_write_state(&w, gs);
    nbt_write_end(&w);
    fastclose(f);

    if (w.error) {
        ESP_LOGE(TAG, "Failed to write save data to %s", path);
        return -1;
    }

    ESP_LOGI(TAG, "Saved to slot %d (NBT)", slot);
    return 0;
}

int game_load(GameState* gs, int slot) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return -1;

    char path[128];
    get_slot_path(slot, path, sizeof(path));

    FILE* f = fastopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "No save file at %s", path);
        return -1;
    }

    NbtReader r;
    if (nbt_read_open(&r, f) < 0) {
        ESP_LOGE(TAG, "Invalid save file header in %s", path);
        fastclose(f);
        return -1;
    }

    ESP_LOGI(TAG, "Loading slot %d (format v%d, swap=%d)", slot, r.version, r.swap);

    // Initialize defaults — missing tags will keep these values
    game_state_init(gs);

    // Read root compound
    char name[64];
    int type = nbt_read_tag(&r, name, sizeof(name));
    if (type != NBT_COMPOUND) {
        ESP_LOGE(TAG, "Expected root compound, got type %d", type);
        fastclose(f);
        return -1;
    }

    game_load_read_state(&r, gs);
    fastclose(f);

    if (r.error) {
        ESP_LOGE(TAG, "Error reading save data from %s", path);
        return -1;
    }

    ESP_LOGI(TAG, "Loaded from slot %d (NBT v%d)", slot, r.version);
    return 0;
}

int game_save_peek(int slot, SaveSlotInfo* info) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT || !info) return -1;

    char path[128];
    get_slot_path(slot, path, sizeof(path));

    FILE* f = fastopen(path, "rb");
    if (!f) return -1;

    NbtReader r;
    if (nbt_read_open(&r, f) < 0) {
        fastclose(f);
        return -1;
    }

    // Read root compound
    char name[64];
    int type = nbt_read_tag(&r, name, sizeof(name));
    if (type != NBT_COMPOUND) {
        fastclose(f);
        return -1;
    }

    // Read the "peek" compound (written first for fast access)
    memset(info, 0, sizeof(*info));
    type = nbt_read_tag(&r, name, sizeof(name));
    if (type == NBT_COMPOUND && strcmp(name, "peek") == 0) {
        while ((type = nbt_read_tag(&r, name, sizeof(name))) != NBT_END) {
            if (type < 0) break;
            if (type == NBT_DOUBLE && strcmp(name, "clips") == 0) info->clips = nbt_read_double(&r);
            else if (type == NBT_DOUBLE && strcmp(name, "ticks") == 0) info->ticks = nbt_read_double(&r);
            else if (type == NBT_INT64 && strcmp(name, "lastSaveTimestamp") == 0) info->timestamp = (time_t)nbt_read_int64(&r);
            else if (type == NBT_INT32 && strcmp(name, "humanFlag") == 0) info->humanFlag = nbt_read_int32(&r);
            else if (type == NBT_INT32 && strcmp(name, "spaceFlag") == 0) info->spaceFlag = nbt_read_int32(&r);
            else if (type == NBT_INT32 && strcmp(name, "dismantle") == 0) info->dismantle = nbt_read_int32(&r);
            else nbt_skip_payload(&r, type);
        }
    }

    fastclose(f);
    return r.error ? -1 : 0;
}

int game_save_exists(int slot) {
    if (slot < 0 || slot >= SAVE_SLOT_COUNT) return 0;

    char path[128];
    get_slot_path(slot, path, sizeof(path));

    struct stat st;
    return stat(path, &st) == 0;
}

int game_save_prestige(const GameState* gs) {
    char path[128];
    snprintf(path, sizeof(path), SAVE_PATH_PREFIX "/prestige.bin");

    FILE* f = fastopen(path, "wb");
    if (!f) return -1;

    int data[2] = {gs->prestigeU, gs->prestigeS};
    size_t written = fwrite(data, sizeof(data), 1, f);
    fastclose(f);

    return (written == 1) ? 0 : -1;
}

int game_load_prestige(GameState* gs) {
    char path[128];
    snprintf(path, sizeof(path), SAVE_PATH_PREFIX "/prestige.bin");

    FILE* f = fastopen(path, "rb");
    if (!f) return -1;

    int data[2] = {0};
    size_t nread = fread(data, sizeof(data), 1, f);
    fastclose(f);

    if (nread == 1) {
        gs->prestigeU = data[0];
        gs->prestigeS = data[1];
        return 0;
    }
    return -1;
}

// ============================================================
// Auto-save background task
// ============================================================

// Snapshot buffer for async save (allocated in PSRAM)
static GameState*       s_save_snapshot = NULL;
static SemaphoreHandle_t s_save_sem     = NULL;
static volatile bool     s_save_pending = false;

void game_save_request(GameState* gs) {
    if (!s_save_snapshot || !s_save_sem) return;
    if (s_save_pending) return;  // Previous save still in progress

    // Take a snapshot under the semaphore
    if (xSemaphoreTake(s_save_sem, 0) == pdTRUE) {
        memcpy(s_save_snapshot, gs, sizeof(GameState));
        s_save_snapshot->lastSaveTimestamp = time(NULL);
        s_save_pending = true;
        xSemaphoreGive(s_save_sem);
    }
}

static void save_task(void* arg) {
    (void)arg;

    while (1) {
        // Poll every 500ms
        vTaskDelay(pdMS_TO_TICKS(500));

        if (!s_save_pending) continue;

        if (xSemaphoreTake(s_save_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (s_save_pending) {
                // Write the snapshot to disk using NBT format
                char path[128];
                get_slot_path(SAVE_SLOT_AUTO, path, sizeof(path));

                FILE* f = fastopen(path, "wb");
                if (f) {
                    NbtWriter w;
                    nbt_write_open(&w, f);
                    nbt_write_compound(&w, "root");
                    game_save_write_state(&w, s_save_snapshot);
                    nbt_write_end(&w);
                    fastclose(f);
                    if (w.error) {
                        ESP_LOGE(TAG, "Auto-save write error");
                    } else {
                        ESP_LOGI(TAG, "Auto-saved (async, NBT)");
                    }
                } else {
                    ESP_LOGE(TAG, "Auto-save failed: can't open %s", path);
                }
                s_save_pending = false;
            }
            xSemaphoreGive(s_save_sem);
        }
    }
}

void game_save_task_start(void) {
    // Allocate snapshot in PSRAM to avoid using precious internal RAM
    s_save_snapshot = heap_caps_calloc(1, sizeof(GameState), MALLOC_CAP_SPIRAM);
    if (!s_save_snapshot) {
        // Fall back to regular malloc
        s_save_snapshot = calloc(1, sizeof(GameState));
    }
    if (!s_save_snapshot) {
        ESP_LOGE(TAG, "Failed to allocate save snapshot buffer");
        return;
    }

    s_save_sem = xSemaphoreCreateMutex();
    if (!s_save_sem) {
        ESP_LOGE(TAG, "Failed to create save semaphore");
        free(s_save_snapshot);
        s_save_snapshot = NULL;
        return;
    }

    // Low priority task on Core 0 (same as UI, won't interfere with game on Core 1)
    xTaskCreatePinnedToCore(save_task, "save_task", 4096, NULL, 1, NULL, 0);
    ESP_LOGI(TAG, "Auto-save task started");
}

// ============================================================
// Offline progress replay
// ============================================================

// Max ticks per offline tier (100 ticks/sec)
// Max offline replay time per tier (in seconds)
static int offline_tier_max_seconds(int tier) {
    switch (tier) {
        case 1: return 5 * 60;    //  5 minutes
        case 2: return 10 * 60;   // 10 minutes
        case 3: return 15 * 60;   // 15 minutes
        case 4: return 30 * 60;   // 30 minutes
        case 5: return 60 * 60;   // 60 minutes
        default: return 0;
    }
}

int game_replay_with_progress(GameState* gs, ProjectManager* pm,
                              pax_buf_t* fb, void (*blit_fn)(void),
                              const char* label, double elapsed_sec, int total_ticks) {
    if (total_ticks <= 0) return 0;

    int w = pax_buf_get_width(fb);
    int h = pax_buf_get_height(fb);

    int batch_size  = 500;
    int total_done  = 0;
    int update_freq = total_ticks / 50;
    if (update_freq < batch_size) update_freq = batch_size;

    int64_t start_us = esp_timer_get_time();
    bool    aborted  = false;

    while (total_done < total_ticks) {
        int remaining = total_ticks - total_done;
        int run       = (remaining < batch_size) ? remaining : batch_size;

        for (int i = 0; i < run; i++) {
            game_tick(gs, pm);
        }
        total_done += run;

        // Abort if dismantling started during replay
        if (gs->dismantle > 0) {
            ESP_LOGW(TAG, "Replay stopped: dismantling started");
            aborted = true;
            break;
        }

        // Update progress bar
        if (total_done % update_freq < batch_size || total_done >= total_ticks || aborted) {
            float pct = (float)total_done / (float)total_ticks;

            pax_background(fb, 0xFF1A1A2E);

            rendertext_draw(fb, 0xFFFFFFFF, pax_font_sky_mono, 24,
                          (w - 300) / 2, h / 2 - 60, label);

            char timebuf[64];
            int mins = (int)(elapsed_sec / 60);
            int secs = (int)elapsed_sec % 60;
            if (mins > 0)
                snprintf(timebuf, sizeof(timebuf), "%d min %d sec", mins, secs);
            else
                snprintf(timebuf, sizeof(timebuf), "%d sec", secs);
            rendertext_draw(fb, 0xFF808090, pax_font_sky_mono, 13,
                          (w - 200) / 2, h / 2 - 30, timebuf);

            int bar_x = 80, bar_y = h / 2, bar_w = w - 160, bar_h = 20;
            pax_simple_rect(fb, 0xFF333355, bar_x, bar_y, bar_w, bar_h);
            pax_simple_rect(fb, 0xFF4CAF50, bar_x, bar_y, (int)(bar_w * pct), bar_h);

            char pctbuf[16];
            snprintf(pctbuf, sizeof(pctbuf), "%d%%", (int)(pct * 100));
            rendertext_draw(fb, 0xFFFFFFFF, pax_font_sky_mono, 13,
                          bar_x + bar_w / 2 - 15, bar_y + 3, pctbuf);

            char tickbuf[64];
            snprintf(tickbuf, sizeof(tickbuf), "%d / %d ticks", total_done, total_ticks);
            rendertext_draw(fb, 0xFF808090, pax_font_sky_mono, 11,
                          (w - 150) / 2, bar_y + bar_h + 8, tickbuf);

            blit_fn();
        }
    }

    ESP_LOGI(TAG, "Replay %s: %d ticks in %lld ms",
             aborted ? "aborted" : "complete", total_done,
             (esp_timer_get_time() - start_us) / 1000);
    return total_done;
}

int game_save_replay_offline(GameState* gs, ProjectManager* pm,
                             pax_buf_t* fb, void (*blit_fn)(void)) {
    if (gs->offlineProgressLevel <= 0) return 0;
    if (gs->dismantle > 0) return 0;

    time_t now = time(NULL);
    if (gs->lastSaveTimestamp <= 0 || now <= gs->lastSaveTimestamp) return 0;

    double elapsed_sec = difftime(now, gs->lastSaveTimestamp);
    if (elapsed_sec < 5.0) return 0;

    int max_sec = offline_tier_max_seconds(gs->offlineProgressLevel);
    if (elapsed_sec > max_sec) {
        elapsed_sec = max_sec;
    }

    int elapsed_ticks = (int)(elapsed_sec * 100.0);
    if (elapsed_ticks <= 0) return 0;

    ESP_LOGI(TAG, "Offline replay: %.0f sec elapsed (max %d sec), %d ticks to replay (tier %d)",
             elapsed_sec, max_sec, elapsed_ticks, gs->offlineProgressLevel);

    return game_replay_with_progress(gs, pm, fb, blit_fn,
                                     "Catching up...", elapsed_sec, elapsed_ticks);
}

int game_load_full(GameState* gs, int slot, pax_buf_t* fb, void (*blit_fn)(void)) {
    if (game_load(gs, slot) != 0) return -1;

    ProjectManager replay_pm;
    project_manager_init(&replay_pm);
    int replayed = game_save_replay_offline(gs, &replay_pm, fb, blit_fn);
    if (replayed > 0) {
        ESP_LOGI(TAG, "Replayed %d offline ticks", replayed);
    }

    return 0;
}
