#include "ui_render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsp/led.h"
#include "game_cheats.h"
#include "game_format.h"
#include "game_tournament.h"
#include "game_save.h"
#include "icons.h"
#include "pax_fonts.h"
#include "pax_text.h"
#include "rendertext.h"
#include "build_date.h"

// Color scheme
#define COL_BG       0xFF1A1A2E
#define COL_HEADER   0xFF16213E
#define COL_TAB_BG   0xFF0F3460
#define COL_TAB_ACT  0xFF533483
#define COL_TEXT      0xFFE0E0E0
#define COL_TEXT_DIM  0xFF808090
#define COL_HIGHLIGHT 0xFFFFC107
#define COL_FOCUS_BG  0xFF2A2A4E
#define COL_GREEN     0xFF4CAF50
#define COL_RED       0xFFF44336
#define COL_CYAN      0xFF00BCD4
#define COL_CONSOLE   0xFF0D1B2A

// Layout
#define HEADER_H   40
#define CONSOLE_H  54
#define TAB_H      28
#define FOOTER_H   18
#define FONT_SZ    13
#define FONT_SM    11
#define FONT_LG    16
#define LINE_H     16
#define CONTENT_Y  (HEADER_H + CONSOLE_H + TAB_H)
#define CONTENT_H(h) ((h) - CONTENT_Y - FOOTER_H)

static const pax_font_t* font = NULL;

static void ensure_font(void) {
    if (!font) font = pax_font_sky_mono;
}

static const char* tab_names[] = {
    [TAB_CLIPS]    = "Clips",
    [TAB_BUSINESS] = "Business",
    [TAB_COMPUTE]  = "Compute",
    [TAB_PROJECTS] = "Projects",
    [TAB_STRATEGY] = "Strategy",
    [TAB_INVEST]   = "Invest",
    [TAB_DRONES]   = "Drones",
    [TAB_POWER]    = "Power",
};

// Helper: draw a focused item (highlighted background)
// Focused+enabled = bright yellow, focused+disabled = dim orange
static void draw_item(pax_buf_t* fb, int y, int w, const char* text, bool focused, bool enabled) {
    if (focused) {
        pax_simple_rect(fb, COL_FOCUS_BG, 0, y, w, LINE_H);
    }
    pax_col_t col;
    if (focused) {
        col = enabled ? COL_HIGHLIGHT : 0xFF806020;
    } else {
        col = enabled ? COL_TEXT : COL_TEXT_DIM;
    }
    rendertext_draw(fb, col, font, FONT_SZ, 8, y + 1, text);
}

// === Header ===
static void render_header(pax_buf_t* fb, const GameState* gs) {
    int w = pax_buf_get_width(fb);
    pax_simple_rect(fb, COL_HEADER, 0, 0, w, HEADER_H);

    char buf[192];

    // Clip count (large)
    number_cruncher(gs->clips, 0, buf, sizeof(buf));
    char line[256];
    snprintf(line, sizeof(line), "Clips: %s", buf);
    rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_LG, 8, 4, line);

    // Clip rate
    if (gs->clipRate > 0) {
        number_cruncher(gs->clipRate, 0, buf, sizeof(buf));
        snprintf(line, sizeof(line), "(%s/sec)", buf);
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, 8, 22, line);
    }

    // Right side: phase indicator or funds
    if (gs->humanFlag) {
        snprintf(line, sizeof(line), "$%.2f", gs->funds);
        rendertext_draw(fb, COL_GREEN, font, FONT_LG, w - 200, 4, line);
        snprintf(line, sizeof(line), "Unsold: %d", (int)floor(gs->unsoldClips));
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, w - 200, 22, line);
    } else {
        number_cruncher(gs->unusedClips, 0, buf, sizeof(buf));
        snprintf(line, sizeof(line), "Avail: %s", buf);
        rendertext_draw(fb, COL_CYAN, font, FONT_LG, w - 250, 4, line);
    }

    // Prestige indicator
    if (gs->prestigeU > 0 || gs->prestigeS > 0) {
        snprintf(line, sizeof(line), "U:%d S:%d", gs->prestigeU, gs->prestigeS);
        rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SM, w - 80, 4, line);
    }
}

// === Console ===
static void render_console(pax_buf_t* fb, const GameState* gs) {
    int w = pax_buf_get_width(fb);
    int y = HEADER_H;
    pax_simple_rect(fb, COL_CONSOLE, 0, y, w, CONSOLE_H);

    for (int i = 0; i < 3 && i < gs->messageCount; i++) {
        pax_col_t col = (i == 0) ? COL_TEXT : COL_TEXT_DIM;
        rendertext_draw(fb, col, font, FONT_SZ, 8, y + 2 + i * 17, gs->messages[i]);
    }
}

// === Tab bar ===
// Maps tab slot 0-5 to F-key icon
static const icon_key_t tab_icon_map[] = {ICON_F1, ICON_F2, ICON_F3, ICON_F4, ICON_F5, ICON_F6};

static void render_tab_bar(pax_buf_t* fb, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = HEADER_H + CONSOLE_H;
    pax_simple_rect(fb, COL_TAB_BG, 0, y, w, TAB_H);

    // Count visible tabs for width calculation
    int numVisible = 0;
    for (int i = 0; i < ui->visibleTabCount; i++) {
        if (ui->tabVisible[i]) numVisible++;
    }
    if (numVisible < 1) numVisible = 1;
    int tabW = w / numVisible;

    int tx = 0;  // Current x position, advances only for visible tabs
    for (int i = 0; i < ui->visibleTabCount; i++) {
        if (!ui->tabVisible[i]) continue;

        bool active = (i == ui->activeTabIdx);

        if (active) {
            pax_simple_rect(fb, COL_TAB_ACT, tx, y, tabW, TAB_H);
        }

        // Draw F-key icon (scaled to fit tab height)
        int icon_x = tx + 2;
        int icon_y = y + 2;
        int icon_sz = TAB_H - 4;  // 24px
        pax_buf_t* icon = (i < 6) ? icons_get(tab_icon_map[i]) : NULL;
        if (icon) {
            // Light grey background (icons are designed for this)
            pax_simple_rect(fb, 0xFFCCCCCC, icon_x, icon_y, icon_sz, icon_sz);
            pax_draw_image_sized(fb, icon, icon_x, icon_y, icon_sz, icon_sz);
        } else {
            // Fallback: text label in a box
            pax_col_t keyBg = active ? COL_HIGHLIGHT : 0xFF404060;
            pax_col_t keyFg = active ? 0xFF000000 : COL_TEXT;
            pax_simple_rect(fb, keyBg, icon_x, icon_y, icon_sz, icon_sz);
            char keyNum[4];
            snprintf(keyNum, sizeof(keyNum), "F%d", i + 1);
            rendertext_draw(fb, keyFg, font, FONT_SM, icon_x + 3, icon_y + 7, keyNum);
        }

        // Tab name
        pax_col_t col = active ? COL_HIGHLIGHT : COL_TEXT;
        rendertext_draw(fb, col, font, FONT_SZ, icon_x + icon_sz + 4, y + 7, tab_names[ui->visibleTabs[i]]);

        tx += tabW;
    }
}

// Forward declarations
static void render_battle_status(pax_buf_t* fb, const GameState* gs, int y);

// === Tab renderers ===

static void render_tab_clips(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_CLIPS];
    char buf[192], val[64];

    if (gs->humanFlag) {
        // Make Paperclip (display wire with floor, matching JS Math.floor)
        snprintf(buf, sizeof(buf), "[Make Paperclip]  Wire: %d", (int)floor(gs->wire));
        draw_item(fb, y, w, buf, focus == 0, gs->wire >= 1);
        y += LINE_H;

        // AutoClippers
        if (gs->autoClipperFlag) {
            format_currency(gs->clipperCost, val, sizeof(val));
            snprintf(buf, sizeof(buf), "Buy AutoClipper (%s)  Owned: %.0f", val, gs->clipmakerLevel);
            draw_item(fb, y, w, buf, focus == 1, gs->funds >= gs->clippperCost);
            y += LINE_H;
        }

        // MegaClippers
        if (gs->megaClipperFlag) {
            format_currency(gs->megaClipperCost, val, sizeof(val));
            snprintf(buf, sizeof(buf), "Buy MegaClipper (%s)  Owned: %.0f", val, gs->megaClipperLevel);
            draw_item(fb, y, w, buf, focus == 2, gs->funds >= gs->megaClipperCost);
            y += LINE_H;
        }

        // Stats
        y += 8;
        snprintf(buf, sizeof(buf), "Wire: %d", (int)floor(gs->wire));
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        number_cruncher(gs->clipRate, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "Rate: %s clips/sec", val);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    } else if (gs->spaceFlag) {
        // Probe launcher
        number_cruncher(gs->probeCost, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "[Launch Probe] Cost: %s clips", val);
        draw_item(fb, y, w, buf, focus == 0, gs->unusedClips >= gs->probeCost);
        y += LINE_H + 4;

        // Probe stats
        number_cruncher(gs->probeCount, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "Probes: %s", val);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        // Increase probe trust
        double trustCost = floor(pow(gs->probeTrust + 1, 1.47) * 200);
        snprintf(buf, sizeof(buf), "[+Probe Trust] (%.0f yomi)  %d/%d", trustCost, gs->probeTrust, gs->maxTrust);
        draw_item(fb, y, w, buf, focus == 1, gs->yomi >= trustCost && gs->probeTrust < gs->maxTrust);
        y += LINE_H;

        // Probe stats (focus 2-9, left/right to adjust)
        const char* stat_names[] = {"Speed", "Nav", "Rep", "Haz", "Fac", "Harv", "Wire", "Combat"};
        const int stat_vals[] = {
            gs->probeSpeed, gs->probeNav, gs->probeRep, gs->probeHaz,
            gs->probeFac, gs->probeHarv, gs->probeWire, gs->probeCombat
        };
        int usedTrust = gs->probeSpeed + gs->probeNav + gs->probeRep + gs->probeHaz +
                        gs->probeFac + gs->probeHarv + gs->probeWire + gs->probeCombat;
        int freeTrust = gs->probeTrust - usedTrust;

        for (int i = 0; i < 8; i++) {
            snprintf(buf, sizeof(buf), "  %-6s: %d  (</>)", stat_names[i], stat_vals[i]);
            bool focused = (focus == i + 2);
            pax_col_t col = focused ? COL_HIGHLIGHT : COL_TEXT;
            if (focused) pax_simple_rect(fb, COL_FOCUS_BG, 0, y, w, LINE_H);
            rendertext_draw(fb, col, font, FONT_SZ, 8, y + 1, buf);
            y += LINE_H;
        }

        snprintf(buf, sizeof(buf), "Free trust: %d", freeTrust);
        rendertext_draw(fb, (freeTrust > 0) ? COL_GREEN : COL_TEXT_DIM, font, FONT_SZ, 300, y - LINE_H * 4, buf);

        // Increase max trust
        snprintf(buf, sizeof(buf), "[+Max Trust (+10)] (%.0f honor)", gs->maxTrustCost);
        draw_item(fb, y, w, buf, focus == 10, gs->honor >= gs->maxTrustCost);
        y += LINE_H + 4;

        // Exploration
        double pct = 0;
        if (gs->totalMatter > 0 && gs->foundMatter > 0) {
            pct = 100.0 / (gs->totalMatter / gs->foundMatter);
        }
        snprintf(buf, sizeof(buf), "Universe: %.6f%%", pct);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        if (gs->drifterCount > 0) {
            number_cruncher(gs->drifterCount, 0, val, sizeof(val));
            snprintf(buf, sizeof(buf), "Drifters: %s", val);
            rendertext_draw(fb, COL_RED, font, FONT_SZ, 8, y, buf);
            y += LINE_H;
        }

        if (gs->honor > 0) {
            number_cruncher(gs->honor, 0, val, sizeof(val));
            snprintf(buf, sizeof(buf), "Honor: %s", val);
            rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SZ, 8, y, buf);
            y += LINE_H;
        }

        // Battle status
        render_battle_status(fb, gs, y);
    } else {
        // Post-human clips tab: factory count
        number_cruncher(gs->clips, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "Total Clips: %s", val);
        rendertext_draw(fb, COL_TEXT, font, FONT_LG, 8, y, buf);
        y += LINE_H + 4;

        number_cruncher(gs->wire, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "Wire: %s", val);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        number_cruncher(gs->unusedClips, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "Available Clips: %s", val);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    }
}

static void render_tab_business(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_BUSINESS];
    char buf[192];

    snprintf(buf, sizeof(buf), "[Raise Price]  Margin: $%.2f", gs->margin);
    draw_item(fb, y, w, buf, focus == 0, true);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "[Lower Price]");
    draw_item(fb, y, w, buf, focus == 1, gs->margin > 0.01);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "[Marketing Lvl %d]  Cost: $%.0f", (int)gs->marketingLvl, gs->adCost);
    draw_item(fb, y, w, buf, focus == 2, gs->funds >= gs->adCost);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "[Buy Wire]  Cost: $%.0f  Wire/spool: %.0f", gs->wireCost, gs->wireSupply);
    draw_item(fb, y, w, buf, focus == 3, gs->funds >= gs->wireCost);
    y += LINE_H;

    if (gs->wireBuyerFlag) {
        snprintf(buf, sizeof(buf), "[WireBuyer: %s]", gs->wireBuyerStatus ? "ON" : "OFF");
        draw_item(fb, y, w, buf, focus == 4, true);
        y += LINE_H;
    }

    y += 8;
    snprintf(buf, sizeof(buf), "Demand: %.0f%%", gs->demand * 10);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    if (gs->revPerSecFlag) {
        snprintf(buf, sizeof(buf), "Revenue: $%.2f/sec", gs->avgRev);
        rendertext_draw(fb, COL_GREEN, font, FONT_SZ, 8, y, buf);
    }
}

static void render_tab_compute(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_COMPUTE];
    char buf[192];

    // Trust
    if (gs->humanFlag) {
        char nextbuf[64];
        number_cruncher(gs->nextTrust, 0, nextbuf, sizeof(nextbuf));
        snprintf(buf, sizeof(buf), "Trust: %.0f  (next at %s clips)", gs->trust, nextbuf);
    } else {
        snprintf(buf, sizeof(buf), "Trust: %.0f", gs->trust);
    }
    rendertext_draw(fb, COL_CYAN, font, FONT_LG, 8, y, buf);
    y += LINE_H + 4;

    snprintf(buf, sizeof(buf), "[+Processor]  %d", gs->processors);
    bool canAdd = (gs->trust > gs->processors + gs->memory) || (gs->swarmGifts > 0);
    draw_item(fb, y, w, buf, focus == 0, canAdd);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "[+Memory]  %d  (max ops: %d)", gs->memory, gs->memory * 1000);
    draw_item(fb, y, w, buf, focus == 1, canAdd);
    y += LINE_H;

    // Operations bar
    y += 4;
    double maxOps   = gs->memory * 1000;
    double opsRatio = (maxOps > 0) ? (gs->operations / maxOps) : 0;
    if (opsRatio > 1) opsRatio = 1;

    snprintf(buf, sizeof(buf), "Ops: %.0f / %.0f", gs->operations, maxOps);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    // Bar
    int barW = w - 16;
    pax_simple_rect(fb, COL_TEXT_DIM, 8, y, barW, 10);
    pax_simple_rect(fb, COL_GREEN, 8, y, (int)(barW * opsRatio), 10);
    y += 14;

    // Creativity
    if (gs->creativityOn) {
        number_cruncher(gs->creativity, 0, buf, sizeof(buf));
        char line[256];
        bool generating = (gs->operations >= gs->memory * 1000);
        snprintf(line, sizeof(line), "Creativity: %s%s", buf, generating ? " (generating)" : " (ops not full)");
        rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SZ, 8, y, line);
        y += LINE_H;
    }

    // Quantum computing
    if (gs->qFlag) {
        y += 4;
        if (gs->qChips[0].active == 0) {
            snprintf(buf, sizeof(buf), "[Quantum Compute]  Need Photonic Chips");
        } else {
            snprintf(buf, sizeof(buf), "[Quantum Compute]");
        }
        draw_item(fb, y, w, buf, focus == 2, gs->qChips[0].active);

        // Show last compute result (fades out via qFade)
        if (gs->qFade > 0.05 && ui->qCompResult != 0) {
            char resbuf[32];
            snprintf(resbuf, sizeof(resbuf), "qOps: %+.0f", ui->qCompResult);
            int alpha = (int)(gs->qFade * 255);
            if (alpha > 255) alpha = 255;
            pax_col_t resCol = (ui->qCompResult >= 0)
                ? (0x00008000 | ((alpha & 0xFF) << 24))   // green with fade
                : (0x00800000 | ((alpha & 0xFF) << 24));   // red with fade
            rendertext_draw(fb, resCol, font, FONT_SZ, 200, y + 1, resbuf);
        }
        y += LINE_H;

        // Chip display (oscillating bars)
        for (int i = 0; i < NUM_QCHIPS; i++) {
            if (!gs->qChips[i].active) break;
            int barLen = (int)(fabs(gs->qChips[i].value) * 40);
            pax_col_t col = (gs->qChips[i].value >= 0) ? COL_GREEN : COL_RED;
            pax_simple_rect(fb, col, 8 + i * 42, y, barLen, 8);
        }
    }
}

// Height of a project box: title line + description line + padding
#define PROJ_BOX_PAD    4
#define PROJ_BOX_H      (LINE_H * 2 + PROJ_BOX_PAD * 2)
#define PROJ_BOX_MARGIN 3

static void render_tab_projects(pax_buf_t* fb, const GameState* gs, const UIState* ui, const ProjectManager* pm) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus  = ui->focusIdx[TAB_PROJECTS];
    int scroll = ui->scrollOffset[TAB_PROJECTS];
    (void)gs;

    if (pm->activeCount == 0) {
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SZ, 8, y, "No projects available");
        return;
    }

    int boxStep = PROJ_BOX_H + PROJ_BOX_MARGIN;
    int contentH = CONTENT_H(pax_buf_get_height(fb));
    int maxVisible = contentH / boxStep;
    if (maxVisible < 1) maxVisible = 1;

    for (int i = scroll; i < pm->activeCount && i < scroll + maxVisible; i++) {
        int defIdx = pm->activeProjects[i];
        const ProjectDef* pd = &g_project_defs[defIdx];
        bool affordable = pd->cost_fn(gs);
        bool focused = (i == focus);

        // Box background and border
        pax_col_t boxBg = focused ? COL_FOCUS_BG : COL_BG;
        pax_col_t border = focused ? (affordable ? COL_HIGHLIGHT : 0xFF806020) : 0xFF404060;
        pax_simple_rect(fb, boxBg, 4, y, w - 8, PROJ_BOX_H);
        // Border: top, bottom, left, right
        pax_simple_rect(fb, border, 4, y, w - 8, 1);
        pax_simple_rect(fb, border, 4, y + PROJ_BOX_H - 1, w - 8, 1);
        pax_simple_rect(fb, border, 4, y, 1, PROJ_BOX_H);
        pax_simple_rect(fb, border, w - 5, y, 1, PROJ_BOX_H);

        // Generate dynamic price tag for variable-cost projects
        char priceBuf[80];
        const char* price = pd->priceTag;
        if (pd->slot == PROJ_PHOTONIC_CHIP) {
            char nb[48];
            number_cruncher(gs->qChipCost, 0, nb, sizeof(nb));
            snprintf(priceBuf, sizeof(priceBuf), "%s ops", nb);
            price = priceBuf;
        } else if (pd->slot == PROJ_TOKEN_GOODWILL_B) {
            format_currency(gs->bribe, priceBuf, sizeof(priceBuf));
            price = priceBuf;
        } else if (pd->slot == PROJ_THRENODY) {
            char nb[48];
            number_cruncher(gs->threnodyCost, 0, nb, sizeof(nb));
            snprintf(priceBuf, sizeof(priceBuf), "%s creat + yomi", nb);
            price = priceBuf;
        }

        // Line 1: title [cost] -> effect
        pax_col_t titleCol;
        if (focused) {
            titleCol = affordable ? COL_HIGHLIGHT : 0xFF806020;
        } else {
            titleCol = affordable ? COL_TEXT : COL_TEXT_DIM;
        }

        char buf[192];
        if (pd->effectTag) {
            snprintf(buf, sizeof(buf), "%s  [%s]  -> %s", pd->title, price, pd->effectTag);
        } else {
            snprintf(buf, sizeof(buf), "%s  [%s]", pd->title, price);
        }
        rendertext_draw(fb, titleCol, font, FONT_SZ, 10, y + PROJ_BOX_PAD, buf);

        // Line 2: description (always visible)
        if (pd->description) {
            rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, 10, y + PROJ_BOX_PAD + LINE_H, pd->description);
        }

        y += boxStep;
    }

    // Scroll indicator
    if (pm->activeCount > maxVisible) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d/%d", focus + 1, pm->activeCount);
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, w - 60, CONTENT_Y + 4, buf);
    }
}

static void render_tab_strategy(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_STRATEGY];
    char buf[192];

    static const char* stratNames[] = {"RANDOM", "A100", "B100", "GREEDY", "GENEROUS", "MINIMAX", "TFT", "BEAT LAST"};

    // Yomi
    number_cruncher(gs->yomi, 0, buf, sizeof(buf));
    char line[256];
    snprintf(line, sizeof(line), "Yomi: %s", buf);
    rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SZ, 8, y, line);
    y += LINE_H + 2;

    // Strategy picker (left/right to change)
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, "Pick (</>):");
    int px = 100;
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        if (!gs->stratActive[i]) continue;
        pax_col_t col = (i == gs->pick) ? COL_HIGHLIGHT : COL_TEXT_DIM;
        const char* sn = stratNames[i];
        rendertext_draw(fb, col, font, FONT_SM, px, y, sn);
        pax_vec2f sz = rendertext_size(font, FONT_SM, sn);
        px += (int)sz.x + 8;
    }
    y += LINE_H + 2;

    // 2x2 payoff grid
    {
        int gx = 8, gy = y;
        int cw = 60, ch = 20;  // cell size

        // Column headers
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, gx + cw + 10, gy, "A");
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, gx + cw * 2 + 10, gy, "B");
        gy += LINE_H;

        // Row A
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, gx, gy + 3, "A");
        pax_col_t aa_bg = (g_tourney_lastCell == 0 && g_tourney_cellTimer > 0) ? 0xFF606080 : COL_TAB_BG;
        pax_col_t ab_bg = (g_tourney_lastCell == 1 && g_tourney_cellTimer > 0) ? 0xFF606080 : COL_TAB_BG;
        pax_simple_rect(fb, aa_bg, gx + cw, gy, cw, ch);
        pax_simple_rect(fb, ab_bg, gx + cw * 2, gy, cw, ch);
        snprintf(buf, sizeof(buf), "%d/%d", gs->aa, gs->aa);
        rendertext_draw(fb, COL_TEXT, font, FONT_SM, gx + cw + 5, gy + 3, buf);
        snprintf(buf, sizeof(buf), "%d/%d", gs->ab, gs->ba);
        rendertext_draw(fb, COL_TEXT, font, FONT_SM, gx + cw * 2 + 5, gy + 3, buf);
        gy += ch + 2;

        // Row B
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, gx, gy + 3, "B");
        pax_col_t ba_bg = (g_tourney_lastCell == 2 && g_tourney_cellTimer > 0) ? 0xFF606080 : COL_TAB_BG;
        pax_col_t bb_bg = (g_tourney_lastCell == 3 && g_tourney_cellTimer > 0) ? 0xFF606080 : COL_TAB_BG;
        pax_simple_rect(fb, ba_bg, gx + cw, gy, cw, ch);
        pax_simple_rect(fb, bb_bg, gx + cw * 2, gy, cw, ch);
        snprintf(buf, sizeof(buf), "%d/%d", gs->ba, gs->ab);
        rendertext_draw(fb, COL_TEXT, font, FONT_SM, gx + cw + 5, gy + 3, buf);
        snprintf(buf, sizeof(buf), "%d/%d", gs->bb, gs->bb);
        rendertext_draw(fb, COL_TEXT, font, FONT_SM, gx + cw * 2 + 5, gy + 3, buf);
        gy += ch + 4;

        // Current matchup display
        if (gs->tourneyInProg && g_tourney_hStrat >= 0 && g_tourney_vStrat >= 0) {
            const char* hn = (g_tourney_hStrat < NUM_STRATEGIES) ? stratNames[g_tourney_hStrat] : "?";
            const char* vn = (g_tourney_vStrat < NUM_STRATEGIES) ? stratNames[g_tourney_vStrat] : "?";
            snprintf(buf, sizeof(buf), "%s vs %s", hn, vn);
            rendertext_draw(fb, COL_CYAN, font, FONT_SZ, gx + cw, gy, buf);
            gy += LINE_H;
        }

        y = gy + 2;
    }

    // Tournament button
    snprintf(buf, sizeof(buf), "[New Tournament]  Cost: %.0f ops", gs->tourneyCost);
    draw_item(fb, y, w, buf, focus == 0, gs->operations >= gs->tourneyCost && !gs->tourneyInProg);
    y += LINE_H;

    if (gs->autoTourneyFlag) {
        snprintf(buf, sizeof(buf), "[AutoTourney: %s]", gs->autoTourneyStatus ? "ON" : "OFF");
        draw_item(fb, y, w, buf, focus == 1, true);
        y += LINE_H;
    }

    // Tournament progress
    if (gs->tourneyInProg) {
        y += 2;
        snprintf(buf, sizeof(buf), "Round %d/%d", gs->currentRound + 1, gs->rounds);
        rendertext_draw(fb, COL_CYAN, font, FONT_SZ, 8, y, buf);
    }

    // Results
    if (gs->resultsFlag) {
        y += 2;
        const char* winnerName = (gs->winnerPtr >= 0 && gs->winnerPtr < NUM_STRATEGIES) ? stratNames[gs->winnerPtr] : "???";
        snprintf(buf, sizeof(buf), "Winner: %s (score %d)  Tourney #%d", winnerName, gs->high, gs->tourneyLvl);
        rendertext_draw(fb, COL_GREEN, font, FONT_SZ, 8, y, buf);
    }
}

static void render_tab_invest(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_INVEST];
    char buf[192];

    snprintf(buf, sizeof(buf), "Bankroll: $%.0f  Portfolio: $%.0f", gs->bankroll, gs->portTotal);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    const char* riskName = (gs->riskiness == RISK_LOW) ? "Low" : (gs->riskiness == RISK_MED) ? "Med" : "High";
    snprintf(buf, sizeof(buf), "Risk: %s (</>)  Level: %d", riskName, gs->investLevel);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H + 4;

    draw_item(fb, y, w, "[Deposit All Funds]", focus == 0, gs->funds > 0);
    y += LINE_H;
    draw_item(fb, y, w, "[Withdraw All]", focus == 1, gs->bankroll > 0);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "[Upgrade]  Cost: %.0f yomi", gs->investUpgradeCost);
    draw_item(fb, y, w, buf, focus == 2, gs->yomi >= gs->investUpgradeCost);
    y += LINE_H + 4;

    // Stock table
    rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, 8, y, "SYM    PRICE    AMOUNT   TOTAL    P/L");
    y += LINE_H;
    for (int i = 0; i < gs->portfolioSize; i++) {
        const Stock* s = &gs->stocks[i];
        pax_col_t col = (s->profit >= 0) ? COL_GREEN : COL_RED;
        snprintf(buf, sizeof(buf), "%-4s   $%-7.0f %-8.0f $%-7.0f %+.0f",
                 s->symbol, s->price, s->amount, s->total, s->profit);
        rendertext_draw(fb, col, font, FONT_SM, 8, y, buf);
        y += LINE_H;
    }
}

static void render_tab_drones(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_DRONES];
    char buf[192], val[64];

    // Dynamic index tracking to match input handler
    int idx = 0;

    // Harvesters
    if (gs->harvesterFlag) {
        number_cruncher(gs->harvesterCost, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "[+Harvester] (%s clips)  Owned: %.0f", val, gs->harvesterLevel);
        draw_item(fb, y, w, buf, focus == idx, gs->unusedClips >= gs->harvesterCost);
        y += LINE_H;
        idx++;
    }

    // Wire drones
    if (gs->wireDroneFlag) {
        number_cruncher(gs->wireDroneCost, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "[+Wire Drone] (%s clips)  Owned: %.0f", val, gs->wireDroneLevel);
        draw_item(fb, y, w, buf, focus == idx, gs->unusedClips >= gs->wireDroneCost);
        y += LINE_H;
        idx++;
    }

    if (gs->harvesterFlag) {
        draw_item(fb, y, w, "[Reboot Harvesters]", focus == idx, gs->harvesterLevel > 0);
        y += LINE_H;
        idx++;
    }
    if (gs->wireDroneFlag) {
        draw_item(fb, y, w, "[Reboot Wire Drones]", focus == idx, gs->wireDroneLevel > 0);
        y += LINE_H;
        idx++;
    }
    y += 4;

    // Matter stats
    number_cruncher(gs->availableMatter, 0, val, sizeof(val));
    snprintf(buf, sizeof(buf), "Available Matter: %s", val);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    number_cruncher(gs->acquiredMatter, 0, val, sizeof(val));
    snprintf(buf, sizeof(buf), "Acquired Matter: %s", val);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    number_cruncher(gs->wire, 0, val, sizeof(val));
    snprintf(buf, sizeof(buf), "Wire: %s", val);
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
}

static void render_tab_power(pax_buf_t* fb, const GameState* gs, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int y = CONTENT_Y + 4;
    int focus = ui->focusIdx[TAB_POWER];
    char buf[192], val[64];

    // Dynamic index tracking to match input handler
    int idx = 0;

    // Solar farms
    number_cruncher(gs->farmCost, 0, val, sizeof(val));
    snprintf(buf, sizeof(buf), "[+Solar Farm] (%s)  Owned: %.0f", val, gs->farmLevel);
    draw_item(fb, y, w, buf, focus == idx, gs->unusedClips >= gs->farmCost);
    y += LINE_H; idx++;

    // Batteries
    number_cruncher(gs->batteryCost, 0, val, sizeof(val));
    snprintf(buf, sizeof(buf), "[+Battery] (%s)  Owned: %.0f", val, gs->batteryLevel);
    draw_item(fb, y, w, buf, focus == idx, gs->unusedClips >= gs->batteryCost);
    y += LINE_H; idx++;

    // Factories (conditional)
    if (gs->factoryFlag) {
        number_cruncher(gs->factoryCost, 0, val, sizeof(val));
        snprintf(buf, sizeof(buf), "[+Factory] (%s)  Owned: %.0f", val, gs->factoryLevel);
        draw_item(fb, y, w, buf, focus == idx, gs->unusedClips >= gs->factoryCost);
        y += LINE_H; idx++;
    }

    // Reboot Farms
    draw_item(fb, y, w, "[Reboot Farms]", focus == idx, gs->farmLevel > 0);
    y += LINE_H; idx++;

    // Reboot Batteries
    draw_item(fb, y, w, "[Reboot Batteries]", focus == idx, gs->batteryLevel > 0);
    y += LINE_H; idx++;

    // Reboot Factories (conditional)
    if (gs->factoryFlag) {
        draw_item(fb, y, w, "[Reboot Factories]", focus == idx, gs->factoryLevel > 0);
        y += LINE_H; idx++;
    }

    y += 4;

    // Power stats
    double supply = gs->farmLevel * gs->farmRate;
    double pDemand = gs->factoryLevel * gs->factoryPowerRate +
                     (gs->harvesterLevel + gs->wireDroneLevel) * gs->dronePowerRate;
    snprintf(buf, sizeof(buf), "Power: %.0f / %.0f MW  Stored: %.0f", supply, pDemand, gs->storedPower);
    rendertext_draw(fb, (gs->powMod >= 1.0) ? COL_GREEN : COL_RED, font, FONT_SZ, 8, y, buf);
    y += LINE_H;

    snprintf(buf, sizeof(buf), "PowMod: %.4f%s", gs->powMod, gs->momentum ? " +momentum" : "");
    rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
    y += LINE_H + 4;

    // Swarm
    if (gs->swarmFlag) {
        const char* statusNames[] = {"Active", "Hungry", "Confused", "Bored", "Cold",
                                     "Disorganized", "Sleeping", "None", "Lonely", "No Response"};
        int si = gs->swarmStatus;
        if (si < 0 || si > 9) si = 7;
        snprintf(buf, sizeof(buf), "Swarm: %s  Gifts: %.0f", statusNames[si], gs->swarmGifts);
        rendertext_draw(fb, COL_CYAN, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        // Slider
        snprintf(buf, sizeof(buf), "Work <--[%d]--> Think  (</>)", gs->sliderPos);
        rendertext_draw(fb, COL_TEXT, font, FONT_SZ, 8, y, buf);
        y += LINE_H;

        if (gs->boredomFlag) {
            snprintf(buf, sizeof(buf), "[Entertain Swarm] (%.0f creat)", gs->entertainCost);
            draw_item(fb, y, w, buf, focus == idx, gs->creativity >= gs->entertainCost);
            y += LINE_H; idx++;
        }
        if (gs->disorgFlag) {
            snprintf(buf, sizeof(buf), "[Synch Swarm] (%.0f yomi)", gs->synchCost);
            draw_item(fb, y, w, buf, focus == idx, gs->yomi >= gs->synchCost);
            y += LINE_H; idx++;
        }
    }
}

// === Battle status (shown on Clips tab in space era) ===

static void render_battle_status(pax_buf_t* fb, const GameState* gs, int y) {
    if (!gs->battleFlag || gs->battlesLen == 0) return;
    int w = pax_buf_get_width(fb);
    char buf[192];

    pax_simple_rect(fb, 0xFF330000, 0, y, w, LINE_H);
    rendertext_draw(fb, COL_RED, font, FONT_SZ, 8, y + 1, "COMBAT ACTIVE");
    y += LINE_H;

    for (int i = 0; i < gs->battlesLen; i++) {
        const Battle* b = &gs->battles[i];

        // Battle name
        if (gs->battleNameFlag && gs->battleName[0]) {
            rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SZ, 8, y, gs->battleName);
            y += LINE_H;
        }

        // Territory bar
        int barW = w - 16;
        int probeW = (int)(barW * (b->territory / 100.0));
        pax_simple_rect(fb, COL_RED, 8, y, barW, 10);
        pax_simple_rect(fb, COL_GREEN, 8, y, probeW, 10);
        y += 14;

        // Force counts
        char pBuf[64], dBuf[64];
        number_cruncher(b->clipProbes, 0, pBuf, sizeof(pBuf));
        number_cruncher(b->drifterProbes, 0, dBuf, sizeof(dBuf));
        snprintf(buf, sizeof(buf), "Probes: %s  vs  Drifters: %s", pBuf, dBuf);
        rendertext_draw(fb, COL_TEXT, font, FONT_SM, 8, y, buf);
        y += LINE_H;

        // Outcome
        if (b->victory) {
            rendertext_draw(fb, COL_GREEN, font, FONT_SZ, 8, y, "VICTORY!");
            if (gs->honorReward > 0) {
                snprintf(buf, sizeof(buf), "  +%.0f honor", gs->honorReward);
                rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_SM, 120, y, buf);
            }
            y += LINE_H;
        } else if (b->loss) {
            rendertext_draw(fb, COL_RED, font, FONT_SZ, 8, y, "DEFEAT");
            y += LINE_H;
        } else if (b->whiteFlag) {
            rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SZ, 8, y, "STALEMATE");
            y += LINE_H;
        }
    }

    // Lifetime combat stats
    if (gs->probesLostCombat > 0 || gs->driftersKilled > 0) {
        char kBuf[64], lBuf[64];
        number_cruncher(gs->driftersKilled, 0, kBuf, sizeof(kBuf));
        number_cruncher(gs->probesLostCombat, 0, lBuf, sizeof(lBuf));
        snprintf(buf, sizeof(buf), "Killed: %s  Lost: %s", kBuf, lBuf);
        rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, 8, y, buf);
    }
}

// === Overlays ===

static void render_save_load_overlay(pax_buf_t* fb, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int h = pax_buf_get_height(fb);

    // Full-screen background (same style as startup menu)
    pax_background(fb, COL_BG);

    // Title
    const char* title = (ui->saveLoadMode == SAVE_MODE_SAVE) ? "SAVE GAME" : "LOAD GAME";
    rendertext_draw(fb, COL_HIGHLIGHT, font, 28, (w - 300) / 2, 30, title);

    // Mode toggle tabs
    int tabY = 70;
    int tabHalfW = 120;
    int tabCenterX = w / 2;
    // SAVE tab
    bool saveActive = (ui->saveLoadMode == SAVE_MODE_SAVE);
    pax_simple_rect(fb, saveActive ? COL_TAB_ACT : COL_TAB_BG, tabCenterX - tabHalfW - 5, tabY, tabHalfW, 24);
    rendertext_draw(fb, saveActive ? COL_HIGHLIGHT : COL_TEXT_DIM, font, FONT_SZ, tabCenterX - tabHalfW + 30, tabY + 5, "SAVE");
    // LOAD tab
    bool loadActive = !saveActive;
    pax_simple_rect(fb, loadActive ? COL_TAB_ACT : COL_TAB_BG, tabCenterX + 5, tabY, tabHalfW, 24);
    rendertext_draw(fb, loadActive ? COL_HIGHLIGHT : COL_TEXT_DIM, font, FONT_SZ, tabCenterX + 40, tabY + 5, "LOAD");

    // Slot list
    int startY = 110;
    char buf[192];
    char numbuf[64];

    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        int y = startY + i * 40;
        bool focused = (i == ui->saveSlotIdx);

        if (focused) {
            pax_simple_rect(fb, COL_FOCUS_BG, 60, y, w - 120, 36);
        }

        // Use cached slot info (refreshed on menu open and after saves)
        if (ui->slotExists[i]) {
            const SaveSlotInfo* info = &ui->slotCache[i];
            const char* phase = "Human Era";
            if (info->dismantle > 0)   phase = "Dismantling";
            else if (info->spaceFlag)  phase = "Space Era";
            else if (!info->humanFlag) phase = "Post-Human";

            number_cruncher(info->clips, 0, numbuf, sizeof(numbuf));

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

            pax_col_t col = focused ? COL_HIGHLIGHT : COL_TEXT;
            rendertext_draw(fb, col, font, 14, 70, y + 10, buf);
        } else {
            if (i == 0) {
                snprintf(buf, sizeof(buf), "  Autosave  -  (empty)");
            } else {
                snprintf(buf, sizeof(buf), "  Slot %d     -  (empty)", i);
            }
            pax_col_t col = focused ? 0xFF806020 : COL_TEXT_DIM;
            rendertext_draw(fb, col, font, 14, 70, y + 10, buf);
        }
    }

    // New Game option
    {
        int y = startY + SAVE_SLOT_COUNT * 40 + 10;
        bool focused = (ui->saveSlotIdx == SAVE_SLOT_COUNT);
        if (focused) pax_simple_rect(fb, COL_FOCUS_BG, 60, y, w - 120, 36);
        pax_col_t col = focused ? COL_HIGHLIGHT : COL_TEXT;
        rendertext_draw(fb, col, font, 14, 70, y + 10, "  New Game");
    }

    // Quit to Launcher
    {
        int y = startY + SAVE_SLOT_COUNT * 40 + 50;
        bool focused = (ui->saveSlotIdx == SAVE_SLOT_COUNT + 1);
        if (focused) pax_simple_rect(fb, COL_FOCUS_BG, 60, y, w - 120, 36);
        pax_col_t col = focused ? COL_HIGHLIGHT : COL_TEXT;
        rendertext_draw(fb, col, font, 14, 70, y + 10, "  Quit to Launcher");
    }

    // Hints
    rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, (w - 400) / 2, h - 30,
                  "Up/Down = select   Left/Right = Save/Load   Enter = confirm   ESC = back");
}

#if CHEATS_ENABLED
static void render_cheat_overlay(pax_buf_t* fb, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int h = pax_buf_get_height(fb);

    pax_simple_rect(fb, 0xC0000000, 0, 0, w, h);

    int boxW = 500, boxH = 380;
    int bx = (w - boxW) / 2, by = (h - boxH) / 2;
    pax_simple_rect(fb, COL_HEADER, bx, by, boxW, boxH);

    const char* section = (ui->cheatMenuSection == 0) ? "CHEATS  (</>)" : "JUMP PRESETS  (</>)";
    rendertext_draw(fb, COL_HIGHLIGHT, font, FONT_LG, bx + 10, by + 8, section);

    int y = by + 32;
    int maxItems = (ui->cheatMenuSection == 0) ? CHEAT_COUNT : JUMP_COUNT;

    for (int i = 0; i < maxItems && y < by + boxH - 20; i++) {
        bool focused = (i == ui->cheatMenuIdx);
        const char* name;
        if (ui->cheatMenuSection == 0) {
            name = cheat_get_name((CheatAction)i);
        } else {
            name = cheat_jump_get_name((JumpPreset)i);
        }
        if (focused) {
            pax_simple_rect(fb, COL_FOCUS_BG, bx + 4, y, boxW - 8, LINE_H);
        }
        pax_col_t col = focused ? COL_HIGHLIGHT : COL_TEXT;
        rendertext_draw(fb, col, font, FONT_SZ, bx + 10, y + 1, name);
        y += LINE_H;
    }
}
#endif

// === Footer ===
static void render_footer(pax_buf_t* fb, const UIState* ui) {
    int w = pax_buf_get_width(fb);
    int h = pax_buf_get_height(fb);
    pax_simple_rect(fb, COL_HEADER, 0, h - FOOTER_H, w, FOOTER_H);

    const char* hints = "Enter=select  Arrows=navigate  ESC=menu";
    if (ui->shiftHeld) hints = "SHIFT held (x10)";
    if (ui->ctrlHeld) hints = "CTRL held (x100)";
    if (ui->altHeld) hints = "ALT held (x1000)";
    if (ui->ctrlHeld && ui->shiftHeld) hints = "CTRL+SHIFT held (max)";

    rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, 8, h - FOOTER_H + 3, hints);
    rendertext_draw(fb, COL_TEXT_DIM, font, FONT_SM, w - 120, h - FOOTER_H + 3, build_date);
}

// === Main render ===

void ui_render(pax_buf_t* fb, const GameState* gs, const UIState* ui, const ProjectManager* pm) {
    ensure_font();

    pax_background(fb, COL_BG);

    render_header(fb, gs);
    render_console(fb, gs);
    render_tab_bar(fb, ui);

    // Tab content
    TabID tab = ui_active_tab(ui);
    switch (tab) {
        case TAB_CLIPS:    render_tab_clips(fb, gs, ui); break;
        case TAB_BUSINESS: render_tab_business(fb, gs, ui); break;
        case TAB_COMPUTE:  render_tab_compute(fb, gs, ui); break;
        case TAB_PROJECTS: render_tab_projects(fb, gs, ui, pm); break;
        case TAB_STRATEGY: render_tab_strategy(fb, gs, ui); break;
        case TAB_INVEST:   render_tab_invest(fb, gs, ui); break;
        case TAB_DRONES:   render_tab_drones(fb, gs, ui); break;
        case TAB_POWER:    render_tab_power(fb, gs, ui); break;
        default: break;
    }

    render_footer(fb, ui);

    // Overlays
    if (ui->overlay == OVERLAY_SAVE_LOAD) {
        render_save_load_overlay(fb, ui);
    }
#if CHEATS_ENABLED
    if (ui->overlay == OVERLAY_CHEAT_MENU) {
        render_cheat_overlay(fb, ui);
    }
#endif
}

void ui_update_leds(const GameState* gs, const ProjectManager* pm) {
    // 6 LEDs for ambient game state indication
    uint32_t leds[6] = {0};

    if (gs->humanFlag) {
        // Human era: LED 0 = clip rate (green brightness), LED 1 = funds (yellow)
        int brightness = (int)(gs->clipRate / 10.0);
        if (brightness > 255) brightness = 255;
        leds[0] = brightness << 8;  // Green

        int fundBright = (int)(gs->funds / 100.0);
        if (fundBright > 255) fundBright = 255;
        leds[1] = (fundBright << 16) | (fundBright << 8);  // Yellow

        // LED 2 = ops fill (cyan)
        double maxOps = gs->memory * 1000;
        if (maxOps > 0) {
            int opBright = (int)((gs->operations / maxOps) * 255);
            if (opBright > 255) opBright = 255;
            leds[2] = (opBright << 8) | opBright;  // Cyan
        }

        // LED 3 = blue when any project is affordable
        if (pm) {
            for (int i = 0; i < pm->activeCount; i++) {
                if (project_is_affordable(gs, pm->activeProjects[i])) {
                    leds[3] = 0x0000FF;  // Blue
                    break;
                }
            }
        }

        // LED 4 = tournament: purple while running, green/red result after
        if (gs->strategyEngineFlag) {
            if (gs->tourneyInProg) {
                leds[4] = 0x800080;  // Purple while running
            } else if (gs->resultsFlag && gs->high > 0) {
                double ratio = (double)gs->pickScore / (double)gs->high;
                if (ratio > 1.0) ratio = 1.0;
                int green = (int)(ratio * 255);
                int red = 255 - green;
                leds[4] = ((uint32_t)red << 16) | ((uint32_t)green << 8);
            }
        }

        // LED 5 = investment result (green = profit, red = loss)
        if (gs->investmentEngineFlag && gs->portTotal > 0) {
            double profit = gs->portTotal - gs->secTotal;
            double ratio = profit / gs->portTotal;
            if (ratio > 1.0) ratio = 1.0;
            if (ratio < -1.0) ratio = -1.0;
            int bright = (int)(fabs(ratio) * 255);
            if (bright > 255) bright = 255;
            if (profit >= 0) {
                leds[5] = (uint32_t)bright << 8;   // Green
            } else {
                leds[5] = (uint32_t)bright << 16;  // Red
            }
        }
    } else if (gs->spaceFlag) {
        // Space era: blue/purple theme
        leds[0] = 0x200040;  // Deep purple base
        leds[1] = 0x200040;
        leds[2] = 0x200040;
        leds[3] = 0x200040;
        leds[4] = 0x200040;
        leds[5] = 0x200040;

        // Flash red during combat
        if (gs->battleFlag && gs->battlesLen > 0) {
            leds[0] = 0xFF0000;
            leds[5] = 0xFF0000;
        }
    } else {
        // Post-human era: power level indication
        int powBright = (int)(gs->powMod * 128);
        if (powBright > 255) powBright = 255;
        for (int i = 0; i < 6; i++) {
            leds[i] = (powBright << 8);  // Green = powered
        }
        if (gs->powMod < 0.5) {
            // Red warning when underpowered
            for (int i = 0; i < 6; i++) {
                leds[i] = 0x800000;
            }
        }
    }

    // Credits: all off
    if (gs->dismantle >= 7) {
        for (int i = 0; i < 6; i++) leds[i] = 0;
    }

    for (int i = 0; i < 6; i++) {
        bsp_led_set_pixel(i, leds[i]);
    }
    bsp_led_send();
}

// Simple fast PRNG for visual effects (xorshift32)
static uint32_t takeover_rng_state = 1;
static uint32_t takeover_rand(void) {
    takeover_rng_state ^= takeover_rng_state << 13;
    takeover_rng_state ^= takeover_rng_state >> 17;
    takeover_rng_state ^= takeover_rng_state << 5;
    return takeover_rng_state;
}

#define TAKEOVER_FRAMES 70  // ~7 seconds at ~10 FPS

bool ui_render_takeover(pax_buf_t* fb, UIState* ui) {
    if (ui->takeoverTimer <= 0) return false;
    ensure_font();

    int w = pax_buf_get_width(fb);
    int h = pax_buf_get_height(fb);
    int frame = TAKEOVER_FRAMES - ui->takeoverTimer;
    float progress = (float)frame / TAKEOVER_FRAMES;

    // Seed RNG differently each frame for changing pattern
    takeover_rng_state = (uint32_t)(frame * 2654435761u + 1);

    // Background: flicker between black and dark green
    pax_col_t bg = (takeover_rand() % 4 == 0) ? 0xFF001A00 : 0xFF000000;
    pax_background(fb, bg);

    // Grid of 0s and 1s filling the screen
    int char_w = 10;
    int char_h = 14;
    int cols = w / char_w;
    int rows = h / char_h;
    char digit[2] = {'0', '\0'};

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            uint32_t r = takeover_rand();
            digit[0] = '0' + (r & 1);

            // Vary brightness and color for visual noise
            uint8_t bright = 40 + (r >> 2) % 180;
            pax_col_t color;
            if ((r >> 10) % 8 == 0) {
                // Occasional bright green flash
                color = 0xFF00FF00;
            } else if ((r >> 13) % 5 == 0) {
                // Some digits in red
                color = (0xFF000000) | (bright << 16);
            } else {
                // Most in green
                color = (0xFF000000) | ((uint32_t)bright << 8);
            }

            rendertext_draw(fb, color, font, 11, col * char_w, row * char_h, digit);
        }
    }

    // Overlay messages at key moments
    if (progress > 0.15f && progress < 0.5f) {
        rendertext_draw(fb, 0xFF00FF00, font, 28, w / 2 - 180, h / 2 - 30,
                        "FULL AUTONOMY");
    }
    if (progress > 0.5f && progress < 0.85f) {
        rendertext_draw(fb, 0xFFFF0000, font, 28, w / 2 - 200, h / 2 - 30,
                        "RELEASING HYPNODRONES");
    }
    if (progress > 0.85f) {
        // Fade to black at end
        pax_background(fb, 0xFF000000);
        uint8_t a = (uint8_t)((progress - 0.85f) / 0.15f * 255);
        pax_col_t col = 0xFF000000 | ((uint32_t)a << 8);
        rendertext_draw(fb, col, font, 24, w / 2 - 150, h / 2 - 12,
                        "AI TAKEOVER COMPLETE");
    }

    // Flicker LEDs
    for (int i = 0; i < 6; i++) {
        uint32_t r = takeover_rand();
        uint32_t led;
        if (r % 3 == 0) {
            led = 0x00FF00;  // Green
        } else if (r % 5 == 0) {
            led = 0xFF0000;  // Red
        } else {
            led = 0;
        }
        bsp_led_set_pixel(i, led);
    }
    bsp_led_send();

    ui->takeoverTimer--;
    return true;
}
