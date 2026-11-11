/*
 * LeakGuard — Minimalist Monochrome Dashboard
 * Built with C11 & SDL2.
 * Pure Black & White typography and graphs with nuanced grey tones.
 * High-end glassmorphism, specular highlights, and monochrome telemetry.
 */

#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define W 1180
#define H 760

typedef struct {
    char name[80];
    int bytes;
    char source[160];
    int line;
    char status[20];
} Row;

typedef struct {
    const char *name;
    int alloc, freed, leaked, invalid, repeated;
    int allocBytes, freedBytes, leakedBytes;
} Scenario;

static const Scenario scenarios[] = {
    {"CLEAN RUN",        12, 12, 0, 0, 0, 1024, 1024,   0},
    {"LEAK SCAN",        15, 11, 4, 0, 0, 2048, 1400, 648},
    {"INTEGRITY ERRORS", 10,  8, 2, 1, 2,  890,  650, 240}
};

static int selected = 0;
static Row liveRows[256];
static int liveCount = 0;
static int liveAlloc = 0, liveFreed = 0, liveLeaks = 0;
static int liveAllocBytes = 0, liveFreedBytes = 0, liveLeakedBytes = 0;
static bool hasLiveReport = false, showingLive = true;
static int mouseX = 0, mouseY = 0;

/* Clean custom RGBA color helper */
static inline SDL_Color RGBA(int r, int g, int b, int a) {
    SDL_Color c = {(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a};
    return c;
}

/* Grayscale palette */
static inline SDL_Color Gray(int l) {
    return RGBA(l, l, l, 255);
}

static inline SDL_Color GrayA(int l, int a) {
    return RGBA(l, l, l, a);
}

/* Load CSV report */
static void loadReport(void) {
    char reportPath[1024] = "leakguard_report.csv";
    char* base = SDL_GetBasePath();
    if (base) {
        snprintf(reportPath, sizeof(reportPath), "%sleakguard_report.csv", base);
        SDL_free(base);
    }
    FILE* f = fopen(reportPath, "r");
    if (!f) f = fopen("leakguard_report.csv", "r");
    if (!f) return;

    char linebuf[512];
    if (!fgets(linebuf, sizeof(linebuf), f)) {
        fclose(f);
        return;
    }

    liveCount = liveAlloc = liveFreed = liveLeaks = 0;
    liveAllocBytes = liveFreedBytes = liveLeakedBytes = 0;

    while (fgets(linebuf, sizeof(linebuf), f) && liveCount < 256) {
        char addr[80], src[160], status[20];
        int bytes = 0, lineno = 0;
        if (sscanf(linebuf, "%79[^,],%d,%159[^,],%d,%19s", addr, &bytes, src, &lineno, status) == 5) {
            snprintf(liveRows[liveCount].name, sizeof(liveRows[liveCount].name), "%s", addr);
            liveRows[liveCount].bytes = bytes;
            snprintf(liveRows[liveCount].source, sizeof(liveRows[liveCount].source), "%s", src);
            liveRows[liveCount].line = lineno;
            snprintf(liveRows[liveCount].status, sizeof(liveRows[liveCount].status), "%s", status);

            liveCount++;
            liveAlloc++;
            liveAllocBytes += bytes;
            if (!strcmp(status, "FREED")) {
                liveFreed++;
                liveFreedBytes += bytes;
            } else {
                liveLeaks++;
                liveLeakedBytes += bytes;
            }
        }
    }
    fclose(f);
    hasLiveReport = (liveCount > 0);
    showingLive = hasLiveReport;
}

/* ========================================================================= */
/*  MINIMALIST 5x7 FONT ENGINE                                               */
/* ========================================================================= */
static const unsigned char* glyph(char c) {
    static const unsigned char letters[26][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14, 4, 4, 4, 4, 4,14},
        { 7, 2, 2, 2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14, 1, 1,30}, {31, 4, 4, 4, 4, 4, 4}, {17,17,17,17,17,17,14},
        {17,17,17,17,17,10, 4}, {17,17,17,21,21,21,10}, {17,17,10, 4,10,17,17},
        {17,17,10, 4, 4, 4, 4}, {31, 1, 2, 4, 8,16,31}
    };
    static const unsigned char digits[10][7] = {
        {14,17,19,21,25,17,14}, { 4,12, 4, 4, 4, 4,14}, {14,17, 1, 2, 4, 8,31},
        {30, 1, 1,14, 1, 1,30}, { 2, 6,10,18,31, 2, 2}, {31,16,16,30, 1, 1,30},
        {14,16,16,30,17,17,14}, {31, 1, 2, 4, 8, 8, 8}, {14,17,17,14,17,17,14},
        {14,17,17,15, 1, 1,14}
    };
    static const unsigned char p_dot[7]    = { 0, 0, 0, 0, 0,12,12};
    static const unsigned char p_colon[7]  = { 0,12,12, 0,12,12, 0};
    static const unsigned char p_slash[7]  = { 1, 2, 4, 8,16,16, 0};
    static const unsigned char p_minus[7]  = { 0, 0, 0,31, 0, 0, 0};
    static const unsigned char p_plus[7]   = { 0, 4, 4,31, 4, 4, 0};
    static const unsigned char p_pct[7]    = {19,19, 2, 4, 8,25,25};
    static const unsigned char p_brack_l[7]= {14, 8, 8, 8, 8, 8,14};
    static const unsigned char p_brack_r[7]= {14, 2, 2, 2, 2, 2,14};
    static const unsigned char p_paren_l[7]= { 6, 8,16,16,16, 8, 6};
    static const unsigned char p_paren_r[7]= {12, 2, 1, 1, 1, 2,12};
    static const unsigned char p_pipe[7]   = { 4, 4, 4, 4, 4, 4, 4};
    static const unsigned char p_under[7]  = { 0, 0, 0, 0, 0, 0,31};
    static const unsigned char p_bullet[7] = { 0, 0,14,14,14, 0, 0};
    static const unsigned char p_amp[7]    = {12,18,20, 8,21,18,13};

    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];

    switch (c) {
        case '.': return p_dot;
        case ':': return p_colon;
        case '/': return p_slash;
        case '-': return p_minus;
        case '+': return p_plus;
        case '%': return p_pct;
        case '[': return p_brack_l;
        case ']': return p_brack_r;
        case '(': return p_paren_l;
        case ')': return p_paren_r;
        case '|': return p_pipe;
        case '_': return p_under;
        case '&': return p_amp;
        case '*':
        case '^': return p_bullet;
        default:  return NULL;
    }
}

static void draw_text(SDL_Renderer* r, int x, int y, const char* s, int scale, SDL_Color col) {
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    for (; *s; s++, x += 6 * scale) {
        const unsigned char* g = glyph(*s);
        if (!g) continue;
        for (int yy = 0; yy < 7; yy++) {
            for (int xx = 0; xx < 5; xx++) {
                if (g[yy] & (1 << (4 - xx))) {
                    SDL_Rect q = {x + xx * scale, y + yy * scale, scale, scale};
                    SDL_RenderFillRect(r, &q);
                }
            }
        }
    }
}

/* ========================================================================= */
/*  MONOCHROME GLOSSY PRIMITIVES                                             */
/* ========================================================================= */

/* Fill rounded rectangle */
static void fill_rounded_rect(SDL_Renderer* r, int x, int y, int w, int h, int rad, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect mid = {x, y + rad, w, h - 2 * rad};
    SDL_RenderFillRect(r, &mid);

    for (int dy = -rad; dy <= rad; dy++) {
        int dx = (int)sqrt((double)(rad * rad - dy * dy));
        if (dy < 0) {
            SDL_Rect top = {x + rad - dx, y + rad + dy, w - 2 * rad + 2 * dx, 1};
            SDL_RenderFillRect(r, &top);
        } else if (dy > 0) {
            SDL_Rect bot = {x + rad - dx, y + h - rad - 1 + dy, w - 2 * rad + 2 * dx, 1};
            SDL_RenderFillRect(r, &bot);
        }
    }
}

/* Outline rounded rectangle */
static void stroke_rounded_rect(SDL_Renderer* r, int x, int y, int w, int h, int rad, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(r, x + rad, y, x + w - rad, y);
    SDL_RenderDrawLine(r, x + rad, y + h - 1, x + w - rad, y + h - 1);
    SDL_RenderDrawLine(r, x, y + rad, x, y + h - rad);
    SDL_RenderDrawLine(r, x + w - 1, y + rad, x + w - 1, y + h - rad);

    for (int dy = -rad; dy <= 0; dy++) {
        int dx = (int)sqrt((double)(rad * rad - dy * dy));
        SDL_RenderDrawPoint(r, x + rad - dx, y + rad + dy);
        SDL_RenderDrawPoint(r, x + w - rad + dx - 1, y + rad + dy);
        SDL_RenderDrawPoint(r, x + rad - dx, y + h - rad - dy - 1);
        SDL_RenderDrawPoint(r, x + w - rad + dx - 1, y + h - rad - dy - 1);
    }
}

/* Draw luxury frosted monochrome glass card with specular highlights */
static void draw_glass_card(SDL_Renderer* r, int x, int y, int w, int h, int rad, bool hover) {
    // 1. Deep charcoal/black translucent fill
    SDL_Color bg = hover ? GrayA(24, 240) : GrayA(15, 230);
    fill_rounded_rect(r, x, y, w, h, rad, bg);

    // 2. Glossy specular gradient on the top 40%
    int sheenH = h * 40 / 100;
    for (int i = 0; i < sheenH; i++) {
        int alpha = (int)(hover ? 24 * (1.0 - (double)i / sheenH) : 15 * (1.0 - (double)i / sheenH));
        SDL_SetRenderDrawColor(r, 255, 255, 255, alpha);
        SDL_RenderDrawLine(r, x + rad, y + 1 + i, x + w - rad, y + 1 + i);
    }

    // 3. Top rim specular highlight line
    SDL_SetRenderDrawColor(r, 255, 255, 255, hover ? 80 : 45);
    SDL_RenderDrawLine(r, x + rad + 2, y + 1, x + w - rad - 2, y + 1);

    // 4. Subtle frosted border
    SDL_Color border = hover ? GrayA(255, 90) : GrayA(255, 25);
    stroke_rounded_rect(r, x, y, w, h, rad, border);
}

/* Draw monochrome pill badge */
static void draw_pill(SDL_Renderer* r, int x, int y, int w, int h, bool bright, const char* label) {
    if (bright) {
        fill_rounded_rect(r, x, y, w, h, h / 2, GrayA(255, 25));
        stroke_rounded_rect(r, x, y, w, h, h / 2, GrayA(255, 120));
        // glowing white dot
        int dotR = 3;
        fill_rounded_rect(r, x + 8, y + h / 2 - dotR, dotR * 2, dotR * 2, dotR, Gray(255));
        draw_text(r, x + 18, y + h / 2 - 3, label, 1, Gray(255));
    } else {
        fill_rounded_rect(r, x, y, w, h, h / 2, GrayA(255, 12));
        stroke_rounded_rect(r, x, y, w, h, h / 2, GrayA(255, 40));
        // muted grey dot
        int dotR = 3;
        fill_rounded_rect(r, x + 8, y + h / 2 - dotR, dotR * 2, dotR * 2, dotR, Gray(140));
        draw_text(r, x + 18, y + h / 2 - 3, label, 1, Gray(180));
    }
}

/* ========================================================================= */
/*  MAIN RENDER PIPELINE                                                     */
/* ========================================================================= */
static void draw(SDL_Renderer* r) {
    const SDL_Color c_bg       = Gray(4);        // Pitch Black
    const SDL_Color c_white    = Gray(255);      // Pure 100% White
    const SDL_Color c_silver   = Gray(215);      // Silver / Light Gray
    const SDL_Color c_muted    = Gray(130);      // Mid Gray

    // 1. Pure Obsidian Black Canvas
    SDL_SetRenderDrawColor(r, c_bg.r, c_bg.g, c_bg.b, 255);
    SDL_RenderClear(r);

    // -------------------------------------------------------------------------
    // 2. Top Header & Engine Status
    // -------------------------------------------------------------------------
    draw_glass_card(r, 24, 18, W - 48, 62, 12, false);

    // Monogram Brand Icon
    fill_rounded_rect(r, 42, 33, 30, 30, 6, GrayA(255, 20));
    stroke_rounded_rect(r, 42, 33, 30, 30, 6, GrayA(255, 160));
    draw_text(r, 49, 41, "LG", 2, c_white);

    draw_text(r, 86, 35, "LEAKGUARD", 2, c_white);
    draw_text(r, 87, 54, "MEMORY INTELLIGENCE & AUDIT TELEMETRY", 1, c_muted);

    // Status pill
    draw_pill(r, W - 200, 34, 160, 28, true, "ENGINE ONLINE");

    // -------------------------------------------------------------------------
    // 3. Resolve Telemetry Data
    // -------------------------------------------------------------------------
    const Scenario* scn = &scenarios[selected];
    int allocCount   = showingLive ? liveAlloc : scn->alloc;
    int freedCount   = showingLive ? liveFreed : scn->freed;
    int leakCount    = showingLive ? liveLeaks : scn->leaked;
    int invalidCount = showingLive ? 0 : (scn->invalid + scn->repeated);

    int allocBytes   = showingLive ? liveAllocBytes : scn->allocBytes;
    int freedBytes   = showingLive ? liveFreedBytes : scn->freedBytes;
    int leakBytes    = showingLive ? liveLeakedBytes : scn->leakedBytes;

    // -------------------------------------------------------------------------
    // 4. Four Glossy Monochrome KPI Cards
    // -------------------------------------------------------------------------
    int cardY = 96;
    int cardW = 265;
    int cardH = 112;
    int cardGap = (W - 48 - (cardW * 4)) / 3;

    struct {
        const char* title;
        int count;
        int bytes;
        bool highlight;
        const char* sub;
    } kpis[4] = {
        {"ALLOCATED BLOCKS", allocCount, allocBytes, false, "TRACKED ALLOCATIONS"},
        {"RELEASED BLOCKS",  freedCount, freedBytes, false, "SAFELY DEALLOCATED"},
        {"ACTIVE LEAKS",     leakCount,  leakBytes,  leakCount > 0,
                             leakCount > 0 ? "REQUIRES RESOLUTION" : "CLEAN - ZERO LEAKS"},
        {"SAFETY VIOLATIONS",invalidCount, 0,        invalidCount > 0,
                             invalidCount > 0 ? "CORRUPTIONS REJECTED" : "NO CORRUPTION"}
    };

    for (int i = 0; i < 4; i++) {
        int cx = 24 + i * (cardW + cardGap);
        bool hover = (mouseX >= cx && mouseX <= cx + cardW && mouseY >= cardY && mouseY <= cardY + cardH);
        draw_glass_card(r, cx, cardY, cardW, cardH, 12, hover);

        // Header label
        draw_text(r, cx + 18, cardY + 16, kpis[i].title, 1, c_muted);

        // Main stat value: Pure White
        char numStr[32];
        snprintf(numStr, sizeof(numStr), "%d", kpis[i].count);
        draw_text(r, cx + 18, cardY + 38, numStr, 3, c_white);

        // Byte count in grey
        char byteStr[64];
        if (i < 3) {
            if (kpis[i].bytes >= 1024)
                snprintf(byteStr, sizeof(byteStr), "%.1f KB", kpis[i].bytes / 1024.0);
            else
                snprintf(byteStr, sizeof(byteStr), "%d BYTES", kpis[i].bytes);
            draw_text(r, cx + 18 + (int)strlen(numStr) * 18 + 10, cardY + 48, byteStr, 1, c_silver);
        }

        // Subtitle indicator
        SDL_Color subCol = kpis[i].highlight ? c_white : c_muted;
        draw_text(r, cx + 18, cardY + 86, kpis[i].sub, 1, subCol);
    }

    // -------------------------------------------------------------------------
    // 5. Middle Section: Two Glass Analytical Panels
    // -------------------------------------------------------------------------
    int midY = 224;
    int midH = 264;
    int leftW = 716;
    int rightW = W - 48 - leftW - 16;
    int rightX = 24 + leftW + 16;

    // --- Panel A: Allocation Dynamics (Black & White Bar Chart) ---
    draw_glass_card(r, 24, midY, leftW, midH, 12, false);
    draw_text(r, 44, midY + 18, "ALLOCATION DYNAMICS", 2, c_white);
    draw_text(r, 44, midY + 38, "CHRONOLOGICAL MEMORY FLOW: ALLOCATED (WHITE) VS RELEASED (GREY)", 1, c_muted);

    // Subtle horizontal grid lines
    int chartBaseY = midY + 215;
    int chartMaxH  = 135;
    for (int g = 0; g <= 4; g++) {
        int gy = chartBaseY - g * (chartMaxH / 4);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 12);
        SDL_RenderDrawLine(r, 60, gy, 24 + leftW - 40, gy);
    }

    // Bars: 5 Intervals (T1 to T5)
    int intervals = 5;
    int groupWidth = (leftW - 120) / intervals;
    int maxBlock = allocCount > 0 ? allocCount : 1;

    for (int i = 0; i < intervals; i++) {
        int gx = 70 + i * groupWidth;

        int aVal = (allocCount * (i + 2)) / (intervals + 1);
        int fVal = (freedCount * (i + 1)) / (intervals + 1);
        if (aVal > allocCount) aVal = allocCount;
        if (fVal > freedCount) fVal = freedCount;

        int hA = (int)((double)aVal / maxBlock * chartMaxH);
        int hF = (int)((double)fVal / maxBlock * chartMaxH);
        if (hA < 4 && aVal > 0) hA = 4;
        if (hF < 4 && fVal > 0) hF = 4;

        int barW = 28;

        // Bar 1: Allocated (Glossy Pure White with gradient)
        if (hA > 0) {
            for (int dy = 0; dy < hA; dy++) {
                double t = (double)dy / hA;
                int lum = (int)(110 + (255 - 110) * t);
                SDL_SetRenderDrawColor(r, lum, lum, lum, 240);
                SDL_RenderDrawLine(r, gx, chartBaseY - dy, gx + barW - 1, chartBaseY - dy);
            }
            // Specular top highlight cap
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            SDL_RenderDrawLine(r, gx, chartBaseY - hA, gx + barW - 1, chartBaseY - hA);
        }

        // Bar 2: Released (Glossy Subtle Grey with gradient)
        int fx = gx + barW + 6;
        if (hF > 0) {
            for (int dy = 0; dy < hF; dy++) {
                double t = (double)dy / hF;
                int lum = (int)(40 + (100 - 40) * t);
                SDL_SetRenderDrawColor(r, lum, lum, lum, 220);
                SDL_RenderDrawLine(r, fx, chartBaseY - dy, fx + barW - 1, chartBaseY - dy);
            }
            // Top specular cap in light grey
            SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
            SDL_RenderDrawLine(r, fx, chartBaseY - hF, fx + barW - 1, chartBaseY - hF);
        }

        // Stage label
        char tLbl[8];
        snprintf(tLbl, sizeof(tLbl), "T%d", i + 1);
        draw_text(r, gx + barW / 2 + 2, chartBaseY + 12, tLbl, 1, c_muted);
    }

    // Chart Legend: Black, White & Grey
    fill_rounded_rect(r, leftW - 230, midY + 22, 10, 10, 2, c_white);
    draw_text(r, leftW - 214, midY + 23, "ALLOCATED", 1, c_silver);
    fill_rounded_rect(r, leftW - 130, midY + 22, 10, 10, 2, Gray(90));
    draw_text(r, leftW - 114, midY + 23, "RELEASED", 1, c_silver);

    // --- Panel B: Session Health & Radial Donut Gauge ---
    draw_glass_card(r, rightX, midY, rightW, midH, 12, false);
    draw_text(r, rightX + 20, midY + 18, "SESSION HEALTH", 2, c_white);
    draw_text(r, rightX + 20, midY + 38, "RETENTION EFFICIENCY", 1, c_muted);

    // High-tech Radial Segment Donut (Monochrome)
    int donutCX = rightX + rightW / 2;
    int donutCY = midY + 130;
    int numPips = 48;
    double releaseRatio = allocCount > 0 ? ((double)freedCount / allocCount) : 1.0;
    int activePips = (int)(numPips * releaseRatio);

    // Circular guide track
    for (int deg = 0; deg < 360; deg += 4) {
        double a = deg * M_PI / 180.0;
        int px = donutCX + (int)(64.0 * cos(a));
        int py = donutCY + (int)(64.0 * sin(a));
        SDL_SetRenderDrawColor(r, 255, 255, 255, 12);
        SDL_RenderDrawPoint(r, px, py);
    }

    for (int p = 0; p < numPips; p++) {
        double angle = (p * 2.0 * M_PI / numPips) - (M_PI / 2.0);
        double rInner = 56.0;
        double rOuter = 72.0;

        int x1 = donutCX + (int)(rInner * cos(angle));
        int y1 = donutCY + (int)(rInner * sin(angle));
        int x2 = donutCX + (int)(rOuter * cos(angle));
        int y2 = donutCY + (int)(rOuter * sin(angle));

        if (p < activePips) {
            // Pure White for recovered blocks
            SDL_SetRenderDrawColor(r, 255, 255, 255, 240);
        } else {
            // Dark Grey for unreleased blocks
            SDL_SetRenderDrawColor(r, 60, 60, 60, 180);
        }
        SDL_RenderDrawLine(r, x1, y1, x2, y2);
    }

    // Centered percentage: Pure White
    char pctStr[16];
    int pctVal = (int)(releaseRatio * 100.0);
    snprintf(pctStr, sizeof(pctStr), "%d%%", pctVal);
    int pW = (int)strlen(pctStr) * 12;
    draw_text(r, donutCX - pW / 2, donutCY - 14, pctStr, 2, c_white);
    draw_text(r, donutCX - 24, donutCY + 8, "RECOVERY", 1, c_muted);

    // Health status pill in monochrome
    if (leakCount == 0) {
        draw_pill(r, donutCX - 65, midY + 218, 130, 26, true, "OPTIMAL");
    } else {
        char leakWarn[32];
        snprintf(leakWarn, sizeof(leakWarn), "%d LEAKS DETECTED", leakCount);
        draw_pill(r, donutCX - 75, midY + 218, 150, 26, true, leakWarn);
    }

    // -------------------------------------------------------------------------
    // 6. Bottom Section: Active Allocation Ledger (Table)
    // -------------------------------------------------------------------------
    int tableY = 502;
    int tableH = 175;
    draw_glass_card(r, 24, tableY, W - 48, tableH, 12, false);

    // Table Header
    draw_text(r, 44, tableY + 16, "ACTIVE ALLOCATION LEDGER", 1, c_white);
    char countBuf[64];
    snprintf(countBuf, sizeof(countBuf), "TELEMETRY: %s (%d TOTAL RECORDS)",
             showingLive ? "LIVE EXECUTION" : "SCENARIO PREVIEW",
             showingLive ? liveCount : 4);
    draw_text(r, W - 44 - (int)strlen(countBuf) * 6, tableY + 16, countBuf, 1, c_muted);

    // Column Labels Header Strip
    int colY = tableY + 36;
    fill_rounded_rect(r, 38, colY - 3, W - 76, 20, 4, GrayA(255, 10));
    draw_text(r, 48,  colY + 3, "ADDRESS", 1, c_muted);
    draw_text(r, 240, colY + 3, "SIZE", 1, c_muted);
    draw_text(r, 380, colY + 3, "SOURCE LOCATION", 1, c_muted);
    draw_text(r, 860, colY + 3, "STATUS", 1, c_muted);

    // Sample data for scenario preview
    Row sampleRows[4] = {
        {"0x1028fa00", 128, "engine_core.cpp", 42, "FREED"},
        {"0x1028fa80",  64, "render_pipeline.cpp", 108, "FREED"},
        {"0x1028fb00", 512, "mesh_loader.cpp", 85, "LEAKED"},
        {"0x1028fd20", 256, "audio_stream.cpp", 210, "LEAKED"}
    };
    if (leakCount == 0) {
        snprintf(sampleRows[2].status, sizeof(sampleRows[2].status), "FREED");
        snprintf(sampleRows[3].status, sizeof(sampleRows[3].status), "FREED");
    }

    Row* displayRows = showingLive ? liveRows : sampleRows;
    int totalRows = showingLive ? liveCount : 4;
    int rowsToRender = totalRows > 5 ? 5 : totalRows;

    for (int rIdx = 0; rIdx < rowsToRender; rIdx++) {
        int ry = colY + 24 + rIdx * 21;
        bool rowHover = (mouseX >= 38 && mouseX <= W - 38 && mouseY >= ry - 2 && mouseY <= ry + 18);

        // Hover highlight
        if (rowHover) {
            fill_rounded_rect(r, 38, ry - 2, W - 76, 19, 4, GrayA(255, 14));
        } else {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 8);
            SDL_RenderDrawLine(r, 44, ry - 2, W - 44, ry - 2);
        }

        // Address: Pure White
        draw_text(r, 48, ry + 3, displayRows[rIdx].name, 1, c_white);

        // Size: Silver
        char bBuf[32];
        if (displayRows[rIdx].bytes >= 1024)
            snprintf(bBuf, sizeof(bBuf), "%.1f KB", displayRows[rIdx].bytes / 1024.0);
        else
            snprintf(bBuf, sizeof(bBuf), "%d B", displayRows[rIdx].bytes);
        draw_text(r, 240, ry + 3, bBuf, 1, c_silver);

        // Source & Line: Mid Grey
        char srcBuf[160];
        snprintf(srcBuf, sizeof(srcBuf), "%.45s:%d", displayRows[rIdx].source, displayRows[rIdx].line);
        draw_text(r, 380, ry + 3, srcBuf, 1, c_muted);

        // Status Badge (Monochrome Contrast)
        bool isFreed = !strcmp(displayRows[rIdx].status, "FREED");
        if (isFreed) {
            // Muted dark badge for FREED
            fill_rounded_rect(r, 860, ry, 74, 16, 4, GrayA(255, 14));
            stroke_rounded_rect(r, 860, ry, 74, 16, 4, GrayA(255, 45));
            draw_text(r, 874, ry + 4, "FREED", 1, c_silver);
        } else {
            // High-contrast solid white badge for LEAKED so it immediately jumps out
            fill_rounded_rect(r, 860, ry, 74, 16, 4, Gray(255));
            draw_text(r, 874, ry + 4, "LEAKED", 1, Gray(0)); // Black text on white pill!
        }
    }

    // -------------------------------------------------------------------------
    // 7. Footer: Floating Minimalist Monochrome Dock
    // -------------------------------------------------------------------------
    int dockY = 690;
    int dockH = 50;
    draw_glass_card(r, 24, dockY, W - 48, dockH, 12, false);

    draw_text(r, 44, dockY + 18, "MODE:", 1, c_muted);

    const char* tabs[4] = {"LIVE REPORT", "CLEAN RUN", "LEAK SCAN", "INTEGRITY ERRORS"};
    int tabW = 200;
    int tabH = 32;

    for (int t = 0; t < 4; t++) {
        int tx = 110 + t * (tabW + 16);
        int ty = dockY + 9;
        bool active = (t == 0) ? showingLive : (!showingLive && (selected == t - 1));
        bool hover  = (mouseX >= tx && mouseX <= tx + tabW && mouseY >= ty && mouseY <= ty + tabH);

        if (active) {
            // Solid Pure White Pill with Pitch Black Text
            fill_rounded_rect(r, tx, ty, tabW, tabH, 8, Gray(255));
            int textLen = (int)strlen(tabs[t]) * 6;
            draw_text(r, tx + (tabW - textLen) / 2, ty + 12, tabs[t], 1, Gray(0));
        } else {
            // Translucent dark pill with grey text
            SDL_Color bgCol  = hover ? GrayA(255, 24) : GrayA(255, 8);
            SDL_Color tabCol = hover ? c_white : c_muted;
            fill_rounded_rect(r, tx, ty, tabW, tabH, 8, bgCol);
            stroke_rounded_rect(r, tx, ty, tabW, tabH, 8, GrayA(255, hover ? 60 : 20));
            int textLen = (int)strlen(tabs[t]) * 6;
            draw_text(r, tx + (tabW - textLen) / 2, ty + 12, tabs[t], 1, tabCol);
        }
    }
}

/* ========================================================================= */
/*  ENTRY POINT & EVENT LOOP                                                 */
/* ========================================================================= */
int main(int argc, char** argv) {
    loadReport();
    bool screenshotMode = (argc > 1 && strcmp(argv[1], "--screenshot") == 0);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    Uint32 winFlags = screenshotMode
        ? (SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE)
        : (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    SDL_Window* window = SDL_CreateWindow(
        "LeakGuard | Minimalist Monochrome Memory Dashboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H,
        winFlags
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }

    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Enable high-end Alpha Blending for glossy glass transparency
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool running = true;
    int winW = W, winH = H;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                winW = event.window.data1;
                winH = event.window.data2;
            } else if (event.type == SDL_MOUSEMOTION) {
                mouseX = (int)(event.motion.x * ((float)W / winW));
                mouseY = (int)(event.motion.y * ((float)H / winH));
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mx = (int)(event.button.x * ((float)W / winW));
                int my = (int)(event.button.y * ((float)H / winH));

                // Bottom dock click selection
                int dockY = 690;
                int tabW  = 200;
                int tabH  = 32;

                if (my >= dockY + 9 && my <= dockY + 9 + tabH) {
                    for (int t = 0; t < 4; t++) {
                        int tx = 110 + t * (tabW + 16);
                        if (mx >= tx && mx <= tx + tabW) {
                            if (t == 0) {
                                loadReport(); // Fresh reload from CSV
                                showingLive = true;
                            } else {
                                showingLive = false;
                                selected = t - 1;
                            }
                        }
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_r) {
                    loadReport();
                    showingLive = true;
                } else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_3) {
                    selected = event.key.keysym.sym - SDLK_1;
                    showingLive = false;
                }
            }
        }

        // Render at logical scale
        int rw, rh;
        SDL_GetRendererOutputSize(renderer, &rw, &rh);
        SDL_RenderSetScale(renderer, (float)rw / W, (float)rh / H);

        draw(renderer);

        if (screenshotMode) {
            SDL_Surface* sshot = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
            SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
            SDL_SaveBMP(sshot, "gui_preview.bmp");
            SDL_FreeSurface(sshot);
            SDL_RenderPresent(renderer);
            break;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
