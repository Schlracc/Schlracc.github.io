// ============================================================================
//  scenes.h  -  the graphics. Each scene draws one animation frame onto the
//               80x40 character canvas. Add a scene: write a function with the
//               signature  void name(Canvas& cv, uint32_t f)  and list it in
//               SCENES[] at the bottom. Preview it with dev mode first.
//  `f` is the frame number; animations repeat every art::LOOP frames.
// ============================================================================
#pragma once
#include "art.h"

namespace scenes {
using namespace art;

// ------------------------------------------------------------- palette ----
struct Petal { uint16_t col, dark, hi; };
static const Petal RED    = {RGB(230, 55, 60),   RGB(140, 20, 40),  RGB(255, 150, 140)};
static const Petal PINK   = {RGB(245, 120, 170), RGB(170, 60, 110), RGB(255, 205, 225)};
static const Petal YELLOW = {RGB(250, 205, 60),  RGB(190, 125, 20), RGB(255, 245, 170)};
static const Petal PURPLE = {RGB(165, 100, 230), RGB(95, 45, 160),  RGB(215, 180, 255)};
static const Petal WHITE  = {RGB(240, 236, 232), RGB(165, 160, 175), RGB(255, 255, 255)};
static const Petal ORANGE = {RGB(250, 140, 50),  RGB(180, 70, 20),  RGB(255, 200, 130)};
static const Petal BLUE   = {RGB(85, 120, 245),  RGB(40, 60, 160),  RGB(170, 195, 255)};

static const uint16_t GREEN1 = RGB(90, 190, 90), GREEN2 = RGB(60, 150, 70),
                      GREEN3 = RGB(140, 215, 110), LEAF = RGB(50, 130, 60);

// ------------------------------------------------------- shared helpers ---
// twinkling stars / sparkles that only appear on empty cells
static void sparkles(Canvas& cv, uint32_t f, uint32_t seed, int count, uint16_t col,
                     int r0 = 0, int r1 = ROWS, int c0 = 0, int c1 = COLS, bool dimDots = true) {
  Rng rng(seed);
  for (int i = 0; i < count; i++) {
    int c = rng.irange(c0, c1), r = rng.irange(r0, r1), ph = rng.irange(0, LOOP);
    if (cv.at(c, r) != ' ') continue;
    int p = (int)((f + ph) % 32);
    uint8_t ch = p < 2 ? '.' : p < 4 ? '+' : p < 6 ? '*' : p < 8 ? '+' : p < 10 ? '.' : 0;
    if (ch) cv.put(c, r, ch, col);
    else if (dimDots && i % 3 == 0) cv.put(c, r, '.', mix(col, cv.cell[r][c].bg, 0.55f));
  }
}
// hearts drifting upward
static void floatingHearts(Canvas& cv, uint32_t f, uint32_t seed, int count, uint16_t col,
                           int r0 = 0, int r1 = ROWS) {
  Rng rng(seed);
  for (int i = 0; i < count; i++) {
    float x = rng.range(3, COLS - 4), ph = rng.f();
    float y = r1 - progress(f, 1, ph) * (r1 - r0 + 2);
    x += wave(f, 2, ph) * 1.5f;
    if (i % 2) { cv.putf(x, y, '<', col); cv.putf(x + 1, y, '3', col); }
    else cv.putf(x, y, HEART, col);
  }
}
// petals / leaves falling and drifting
static void fallingPetals(Canvas& cv, uint32_t f, uint32_t seed, int count, const uint16_t* cols,
                          int ncols, const char* chars, float drift = 6) {
  Rng rng(seed);
  int nch = (int)strlen(chars);
  for (int i = 0; i < count; i++) {
    float x0 = rng.range(-5, COLS), ph = rng.f();
    int k = rng.irange(1, 3);
    float p = progress(f, k, ph);
    float y = -2 + p * (ROWS + 4);
    float x = x0 + p * drift + wave(f, 3, ph) * 1.5f;
    cv.putf(x, y, (uint8_t)chars[(i + (int)(p * 8)) % nch], cols[i % ncols]);
  }
}
// video-style blob of 'o' characters
static void cluster(Canvas& cv, float x, float y, float rx, float ry, const Petal& p, uint32_t seed) {
  for (int r = (int)floorf(y - ry); r <= (int)ceilf(y + ry); r++)
    for (int c = (int)floorf(x - rx); c <= (int)ceilf(x + rx); c++) {
      float u = (c - x) / rx, v = (r - y) / ry;
      if (u * u + v * v > 1) continue;
      uint32_t h = hash(seed * 977 + c * 31 + r);
      uint8_t ch = (h % 7 == 0) ? '0' : (h % 11 == 0) ? '@' : 'o';
      cv.put(c, r, ch, mix(p.hi, p.col, (v + 1) * 0.6f));
    }
}
static void daisy(Canvas& cv, float x, float y, float r, const Petal& p, uint16_t center, float rot = 0) {
  int n = r < 2.5f ? 8 : 12;
  cv.spokes(x, y, 1, r, n, rot, p.col);
  cv.spokes(x, y, r * 0.6f, r, n, rot + TAU / (2 * n), p.hi);
  if (r >= 3.5f) cv.ellipse(x, y, 1.3f, 0.8f, "@o", center);
  else cv.putf(x, y, '@', center);
}
// tulip head. (x,y) = top centre, hw = half width in cells, h = rows tall.
// returns the row just below the head (where the stem starts)
static float tulip(Canvas& cv, float x, float y, int hw, int h, const Petal& p) {
  int cx = Canvas::rnd(x), cy = Canvas::rnd(y);
  if (hw <= 0) { cv.put(cx, cy, 'U', p.col); return cy + 1; }
  if (hw == 1) {
    cv.put(cx - 1, cy, '(', p.dark); cv.put(cx, cy, '^', p.hi); cv.put(cx + 1, cy, ')', p.dark);
    cv.put(cx - 1, cy + 1, '\\', p.dark); cv.put(cx, cy + 1, '@', p.col); cv.put(cx + 1, cy + 1, '/', p.dark);
    return cy + 2;
  }
  // petal tips
  for (int o = -hw + 1; o <= hw - 1; o++)
    if (((o + hw) & 1) == 1) cv.put(cx + o, cy, '^', p.hi);
  // body
  for (int i = 1; i < h - 1; i++) {
    cv.put(cx - hw, cy + i, '(', p.hi);
    cv.put(cx + hw, cy + i, ')', p.dark);
    for (int o = -hw + 1; o <= hw - 1; o++) {
      bool seam = (o == -(hw + 1) / 2 || o == (hw + 1) / 2);
      cv.put(cx + o, cy + i, seam ? '|' : '@', seam ? p.dark : (o < 0 ? mix(p.col, p.hi, 0.35f) : p.col));
    }
  }
  // base
  int b = cy + h - 1;
  cv.put(cx - hw + 1, b, '\\', p.dark);
  cv.put(cx + hw - 1, b, '/', p.dark);
  for (int o = -hw + 2; o <= hw - 2; o++) cv.put(cx + o, b, '@', p.dark);
  return (float)(b + 1);
}
static void rose(Canvas& cv, float x, float y, float rx, const Petal& p) {
  float ry = rx * 0.68f;
  cv.ring(x, y, rx + 1.2f, ry + 0.9f, p.dark);  // outer petals
  for (int r = (int)floorf(y - ry); r <= (int)ceilf(y + ry); r++)
    for (int c = (int)floorf(x - rx); c <= (int)ceilf(x + rx); c++) {
      float u = (c - x) / rx, v = (r - y) / ry, d = sqrtf(u * u + v * v);
      if (d > 1) continue;
      float s = d * 2.6f + atan2f(v, u) / TAU;
      s -= floorf(s);
      if (s < 0.28f) cv.put(c, r, slopeChar(-v, u), p.dark);  // spiral fold
      else cv.put(c, r, d > 0.82f ? (u < 0 ? '(' : ')') : (s < 0.6f ? '@' : 'o'), mix(p.hi, p.col, d * 1.3f));
    }
}
static void sunflower(Canvas& cv, float x, float y, float r, float rot) {
  cv.spokes(x, y, r * 0.5f, r, 16, rot, YELLOW.col);
  cv.spokes(x, y, r * 0.5f, r * 0.85f, 16, rot + TAU / 32, ORANGE.col);
  cv.ellipse(x, y, r * 0.5f, r * 0.5f * 0.67f, "#@o#", RGB(110, 60, 25));
  cv.putf(x, y, '@', RGB(60, 30, 10));
}
static void leaf(Canvas& cv, float x, float y, float len, int dir, uint16_t col) {
  // a leaf growing left (dir=-1) or right (dir=+1) from a stem at (x,y)
  int n = (int)len;
  for (int i = 1; i <= n; i++) {
    float yy = y - i * 0.3f, h = sinf(3.14159f * i / (n + 1)) * 0.9f;
    int c = Canvas::rnd(x + dir * i);
    for (int r = Canvas::rnd(yy - h); r <= Canvas::rnd(yy + h); r++) cv.setBg(c, r, col);
    cv.putf(x + dir * i, yy, i == n ? (dir > 0 ? '>' : '<') : '-', GREEN3);
  }
}
// stems from a gathering point up to each flower, plus the ribbon
static void bouquetBase(Canvas& cv, const float* hx, const float* hy, int n, float gx, float gy,
                        uint16_t ribbon, uint16_t ribbonDark) {
  for (int i = 0; i < n; i++) {
    float cx = gx + (hx[i] - gx) * 0.25f, cy = hy[i] + (gy - hy[i]) * 0.45f;
    cv.curve(gx, gy, cx, cy, hx[i], hy[i], (i % 3 == 0) ? GREEN2 : GREEN1, 0, 8);
  }
  // stem bundle below the ribbon, narrowing to a point
  for (int i = 1; i <= 5; i++) {
    int w = 3 - i / 2;
    for (int o = -w; o <= w; o++) cv.put((int)gx + o, (int)gy + i, o < 0 ? '/' : o > 0 ? '\\' : '|', GREEN2);
  }
  // ribbon + bow
  int x = (int)gx, y = (int)gy;
  cv.text(x - 4, y, "<<=", ribbon);
  cv.put(x - 1, y, '(', ribbonDark); cv.put(x, y, '@', ribbon); cv.put(x + 1, y, ')', ribbonDark);
  cv.text(x + 2, y, "=>>", ribbon);
  cv.put(x - 2, y + 1, '/', ribbon); cv.put(x - 3, y + 2, '/', ribbonDark);
  cv.put(x + 2, y + 1, '\\', ribbon); cv.put(x + 3, y + 2, '\\', ribbonDark);
}

// =================================================================== 1 ===
// WILDFLOWERS - the bouquet from the video
static void sceneWildflowers(Canvas& cv, uint32_t f) {
  cv.clear(RGB(50, 56, 80));
  const float gx = 40, gy = 33;
  const int N = 24;
  float hx[N], hy[N];
  Rng rng(7);
  for (int i = 0; i < N; i++) {
    float a = -1.35f + 2.7f * (i + rng.range(-0.3f, 0.3f)) / (N - 1);
    float d = rng.range(14, 27);
    hx[i] = gx + sinf(a) * d * 1.45f + wave(f, 1, rng.f()) * 0.7f;
    hy[i] = gy - cosf(a) * d * 0.95f - 2;
  }
  bouquetBase(cv, hx, hy, N, gx, gy, PINK.col, PINK.dark);
  // little green leaves on the stems
  Rng lr(99);
  for (int i = 0; i < 40; i++) {
    int k = lr.irange(0, N);
    float t = lr.range(0.3f, 0.8f);
    cv.putf(gx + (hx[k] - gx) * t, gy + (hy[k] - gy) * t, lr.irange(0, 2) ? '+' : '*', GREEN3);
  }
  const Petal* pals[] = {&RED, &PINK, &YELLOW, &PURPLE, &WHITE, &ORANGE, &BLUE};
  Rng hr(3);
  for (int i = 0; i < N; i++) {
    const Petal& p = *pals[hr.irange(0, 7)];
    int kind = hr.irange(0, 10);
    if (kind < 6) {
      float rx = hr.range(2, 4.2f), ry = hr.range(1, 1.8f);  // fixed order on every compiler
      cluster(cv, hx[i], hy[i], rx, ry, p, i);
    }
    else if (kind < 8) daisy(cv, hx[i], hy[i], hr.range(2, 3.2f), p, YELLOW.col, progress(f, 1) * TAU / 8);
    else {  // spike of buds
      for (int k = 0; k < 4; k++) cv.putf(hx[i], hy[i] - k, k % 2 ? 'o' : '0', k % 2 ? p.col : p.hi);
    }
  }
  sparkles(cv, f, 11, 70, RGB(235, 235, 255));
  floatingHearts(cv, f, 5, 5, PINK.col, 0, 30);
  cv.textCentered(39, "picked by hand, just for you", PINK.hi);
}

// =================================================================== 2 ===
// TULIP BOUQUET
static void sceneTulipBouquet(Canvas& cv, uint32_t f) {
  cv.clear(RGB(36, 34, 62));
  const float gx = 40, gy = 31;
  struct T { float x, y; int hw, h; const Petal* p; };
  const T tl[] = {
    {40, 3, 3, 5, &RED},     {29, 6, 3, 5, &PINK},    {51, 6, 3, 5, &YELLOW},
    {19, 11, 2, 4, &PURPLE}, {61, 11, 2, 4, &ORANGE}, {34, 12, 3, 5, &WHITE},
    {46, 12, 3, 5, &PINK},   {12, 17, 2, 4, &RED},    {68, 17, 2, 4, &PURPLE},
    {24, 17, 2, 4, &YELLOW}, {56, 17, 2, 4, &RED},
  };
  const int N = sizeof(tl) / sizeof(tl[0]);
  float sx[N], sy[N], tx[N];
  for (int i = 0; i < N; i++) {
    tx[i] = tl[i].x + wave(f, 1, i * 0.13f) * 0.8f;
    sx[i] = tx[i];
    sy[i] = tl[i].y + tl[i].h;
  }
  // long tulip leaves
  for (int k = -1; k <= 1; k += 2) {  // long tulip leaves, both sides
    cv.curveBg(gx, gy, gx + k * 10, gy - 2, gx + k * 22, gy - 12, 1.2f, 0.3f, LEAF);
    cv.curve(gx, gy, gx + k * 10, gy - 2, gx + k * 22, gy - 12, GREEN3);
    cv.curveBg(gx, gy, gx + k * 6, gy - 3, gx + k * 12, gy - 16, 1.0f, 0.2f, RGB(40, 110, 55));
  }
  bouquetBase(cv, sx, sy, N, gx, gy, RGB(240, 200, 220), RGB(180, 120, 160));
  for (int i = 0; i < N; i++) tulip(cv, tx[i], tl[i].y, tl[i].hw, tl[i].h, *tl[i].p);
  sparkles(cv, f, 21, 60, RGB(255, 230, 240));
  floatingHearts(cv, f, 8, 4, RED.hi, 0, 28);
  cv.textCentered(39, "two lips, er.. tulips, for you", PINK.hi);
}

// =================================================================== 3 ===
// TULIP FIELD with a windmill at sunset
static void sceneTulipField(Canvas& cv, uint32_t f) {
  const int H = 17;  // horizon row
  cv.clear(0);
  cv.gradient(RGB(40, 45, 110), RGB(250, 165, 120), 0, H + 1);
  cv.gradient(RGB(40, 90, 50), RGB(20, 60, 30), H + 1, ROWS);
  // sun
  cv.ellipseBg(58, H, 7, 4.5f, RGB(255, 195, 130));
  cv.ellipse(58, H, 4.5f, 3, "o@", RGB(255, 235, 150));
  // sun reflection streaks on the clouds
  cv.text(44, 9, "~~-- ---~~", RGB(255, 190, 170));
  cv.text(8, 6, "--~~~--", RGB(200, 150, 190));
  // birds
  for (int i = 0; i < 3; i++) {
    float x = progress(f, 1, i * 0.08f) * 95 - 10 + i * 3, y = 4 + i + wave(f, 4, i * 0.3f) * 0.6f;
    cv.putf(x, y, (f / 3 + i) % 2 ? 'v' : '-', RGB(40, 30, 60));
  }
  // tulip stripes radiating from the vanishing point
  const Petal* stripes[] = {&RED, &YELLOW, &PINK, &PURPLE, &ORANGE, &WHITE};
  for (int r = H + 1; r < ROWS; r++) {
    float depth = (float)(r - H) / (ROWS - H);  // 0 far .. 1 near
    for (int c = 0; c < COLS; c++) {
      float s = (c - 40.0f) / (r - H + 1.5f) * 0.9f + 0.0137f;  // offset avoids exact ties
      int band = (int)floorf(s + 100);
      float frac = s + 100 - band;
      if (frac > 0.8f) { cv.put(c, r, depth > 0.4f ? '|' : '.', GREEN1); continue; }  // green rows
      const Petal& p = *stripes[band % 6];
      int sway = Canvas::rnd(wave(f, 2, c * 0.015f + r * 0.03f) * depth * 0.8f);
      if (depth < 0.25f)      cv.put(c, r, '.', p.col);
      else if (depth < 0.55f) cv.put(c, r, (c + r) % 2 ? 'v' : 'u', p.col);
      else if (r % 2 == 0)    cv.put(c + sway, r, (c % 2) ? 'U' : 'u', (c % 3) ? p.col : p.hi);
      else                    cv.put(c, r, (c % 2) ? '|' : '/', GREEN2);
    }
  }
  // windmill silhouette
  const uint16_t W = RGB(45, 30, 45), WL = RGB(90, 60, 70);
  for (int r = 8; r <= H + 2; r++) {
    int w = 1 + (r - 8) / 3;
    for (int o = -w; o <= w; o++) {
      cv.setBg(16 + o, r, W);
      cv.put(16 + o, r, o == -w ? '/' : o == w ? '\\' : ' ', WL);
    }
  }
  cv.put(16, H + 1, '#', RGB(255, 200, 120));   // lit door
  cv.put(16, 12, 'o', RGB(255, 200, 120));      // window
  cv.text(14, 7, "/^\\", WL);
  float rot = progress(f, 1) * TAU / 4;          // 4 blades -> quarter turn loops perfectly
  for (int k = 0; k < 4; k++) {
    float a = rot + k * TAU / 4;
    // direction in pixels -> cells, plus a perpendicular for the sail
    float dx = cosf(a), dy = sinf(a);
    float px = -dy, py = dx;
    auto P = [&](float along, float side, float& ox, float& oy) {
      ox = 16 + (dx * along + px * side) / CELL_W;
      oy = 8 + (dy * along + py * side) / CELL_H;
    };
    float x0, y0, x1, y1;
    P(0, 0, x0, y0); P(38, 0, x1, y1);
    cv.line(x0, y0, x1, y1, RGB(60, 40, 50));
    for (float side = 3; side <= 9; side += 3) {
      P(12, side, x0, y0); P(38, side, x1, y1);
      cv.line(x0, y0, x1, y1, RGB(150, 120, 120), side == 9 ? 0 : '#');
    }
  }
  cv.put(16, 8, '@', RGB(200, 170, 150));
  cv.text(52, 1, "~ tulip fields forever ~", RGB(255, 220, 230));
}

// =================================================================== 4 ===
// SINGLE ROSE
static void sceneRose(Canvas& cv, uint32_t f) {
  cv.clear(RGB(40, 18, 34));
  float sw = wave(f, 1) * 0.6f;
  // stem with thorns
  cv.curve(40 + sw, 20, 38, 30, 40, 39, GREEN2);
  for (int r = 23; r < 39; r += 4) cv.put(r % 8 ? 41 : 37, r, r % 8 ? '>' : '<', GREEN3);
  // leaves
  leaf(cv, 39, 28, 8, -1, LEAF);
  leaf(cv, 40, 33, 8, +1, LEAF);
  // sepals
  cv.text(36, 20, "\\v^v/", GREEN1);
  rose(cv, 40 + sw, 12, 8, RED);
  const uint16_t pc[] = {RED.col, RED.hi, RED.dark};
  fallingPetals(cv, f, 4, 9, pc, 3, "oc0", 4);
  sparkles(cv, f, 31, 45, RGB(255, 200, 210));
  cv.text(54, 37, "a rose,", PINK.hi);
  cv.text(54, 38, "for my favorite", PINK.hi);
  cv.text(54, 39, "person", PINK.hi);
}

// =================================================================== 5 ===
// SUNFLOWERS on a sunny day
static void sceneSunflowers(Canvas& cv, uint32_t f) {
  cv.clear(0);
  cv.gradient(RGB(60, 130, 215), RGB(175, 215, 245), 0, 32);
  cv.gradient(RGB(70, 150, 60), RGB(35, 95, 35), 32, ROWS);
  // drifting clouds
  for (int i = 0; i < 3; i++) {
    float x = fmodf(progress(f, 1, i * 0.33f) * 110, 110) - 15;
    float y = 3 + i * 4;
    cv.ellipseBg(x, y, 6, 1.6f, RGB(245, 248, 255));
    cv.ellipseBg(x + 4, y - 0.8f, 4, 1.3f, RGB(245, 248, 255));
  }
  // grass blowing in the wind
  for (int r = 32; r < ROWS; r++)
    for (int c = 0; c < COLS; c++) {
      if (hash(c * 13 + r * 71) % 3) continue;
      float w = wave(f, 2, c * 0.02f - r * 0.015f);
      cv.put(c, r, w > 0.4f ? '/' : w < -0.4f ? '\\' : '|', r % 2 ? GREEN3 : GREEN1);
    }
  struct S { float x, y, r; };
  const S fl[] = {{18, 13, 5}, {41, 9, 6.5f}, {63, 15, 4.5f}};
  for (int i = 0; i < 3; i++) {
    float x = fl[i].x + wave(f, 1, i * 0.2f) * 0.8f;
    cv.curve(x, fl[i].y + 2, x - 1, 28, fl[i].x, ROWS, GREEN2);
    leaf(cv, x - 0.5f, 26 - i, 4, -1, LEAF);
    leaf(cv, x + 0.2f, 30 - i, 4, +1, LEAF);
    sunflower(cv, x, fl[i].y, fl[i].r, progress(f, 1) * TAU / 16 * (i % 2 ? -1 : 1));
  }
  // a bee buzzing in a figure eight
  float t = progress(f, 1) * TAU;
  float bx = 41 + sinf(t) * 26, by = 20 + sinf(2 * t) * 5;
  cv.putf(bx, by, '8', RGB(250, 210, 30));
  cv.putf(bx + (cosf(t) > 0 ? -1 : 1), by, 'o', RGB(30, 30, 30));
  cv.putf(bx, by - 1, (f % 4) < 2 ? '^' : '-', RGB(255, 255, 255));
  cv.text(2, 1, "you are my sunshine", RGB(255, 250, 200));
}

// =================================================================== 6 ===
// CHERRY BLOSSOM branch at night
static void sceneCherryBlossom(Canvas& cv, uint32_t f) {
  cv.clear(0);
  cv.gradient(RGB(25, 25, 60), RGB(80, 55, 100));
  // moon with glow
  cv.ellipseBg(66, 9, 7, 4.6f, RGB(70, 65, 110));
  cv.ellipseBg(66, 9, 5, 3.3f, RGB(230, 225, 200));
  cv.ellipse(66, 9, 3.5f, 2.2f, " .  o", RGB(200, 195, 175));
  sparkles(cv, f, 61, 35, RGB(230, 230, 255), 0, 25);
  // branches
  const uint16_t B = RGB(85, 55, 50), BL = RGB(130, 90, 75);
  cv.curveBg(-2, 6, 20, 8, 46, 17, 2.2f, 0.8f, B);
  cv.curveBg(18, 9, 25, 2, 34, 1, 1.0f, 0.5f, B);
  cv.curveBg(30, 13, 38, 7, 50, 6, 1.0f, 0.5f, B);
  cv.curveBg(42, 16, 52, 21, 60, 27, 0.9f, 0.5f, B);
  cv.curveBg(8, 8, 10, 14, 16, 18, 0.9f, 0.5f, B);
  cv.curve(-2, 6, 20, 8, 46, 17, BL, '=');   // bark texture
  // blossoms along the branches
  struct P { float x, y; };
  const P bl[] = {{6, 5}, {12, 7}, {17, 5}, {22, 3}, {27, 2}, {33, 1}, {24, 10}, {30, 9}, {36, 7},
                  {42, 6}, {48, 5}, {39, 13}, {45, 15}, {50, 19}, {55, 22}, {59, 26}, {11, 13},
                  {15, 17}, {35, 12}, {44, 9}, {20, 8}, {53, 24}, {9, 10}};
  for (unsigned i = 0; i < sizeof(bl) / sizeof(bl[0]); i++) {
    float x = bl[i].x + wave(f, 1, i * 0.05f) * 0.5f, y = bl[i].y;
    const uint16_t petal = (i % 3) ? PINK.hi : PINK.col;
    cv.putf(x - 1, y, 'o', petal); cv.putf(x + 1, y, 'o', petal);
    cv.putf(x, y - 1, 'o', petal); cv.putf(x, y + 1, 'o', petal);
    cv.putf(x, y, '*', RGB(255, 240, 120));
  }
  const uint16_t pc[] = {PINK.hi, PINK.col, RGB(255, 230, 240)};
  fallingPetals(cv, f, 6, 22, pc, 3, "o.'", 10);
  cv.text(4, 37, "softly, like spring", PINK.hi);
}

// =================================================================== 7 ===
// LAVENDER FIELD at dusk
static void sceneLavender(Canvas& cv, uint32_t f) {
  const int H = 15;
  cv.clear(0);
  cv.gradient(RGB(60, 40, 110), RGB(245, 155, 115), 0, H);
  cv.ellipseBg(40, H, 9, 5.5f, RGB(255, 185, 120));     // sun glow
  cv.ellipseBg(40, H, 6, 3.8f, RGB(255, 225, 150));     // sun
  cv.gradient(RGB(85, 60, 120), RGB(25, 35, 35), H, ROWS);
  for (int c = 0; c < COLS; c++) {                        // far hills
    int hill = H - 1 + Canvas::rnd(sinf(c * 0.13f) * 0.8f + sinf(c * 0.05f + 1) * 0.8f);
    for (int r = hill; r <= H; r++) cv.setBg(c, r, RGB(95, 65, 125));
  }
  sparkles(cv, f, 71, 18, RGB(255, 240, 255), 0, 7, 0, COLS, false);
  // rows of lavender, far -> near
  const float rowsY[] = {17, 18.5f, 20.5f, 23, 26, 29.5f, 33.5f, 38.5f};
  for (int row = 0; row < 8; row++) {
    float y = rowsY[row];
    int len = 1 + row;
    float spacing = 1.5f + row * 0.55f;
    uint16_t mound = mix(RGB(90, 60, 130), RGB(55, 35, 85), row / 7.0f);
    float start = fmodf(row * 0.9f, spacing);
    for (int j = 0; start + j * spacing < COLS + 1; j++) {
      float x = start + j * spacing + (hash(j + row * 1000) % 100) / 100.0f * spacing * 0.4f;
      float lean = wave(f, 1, x * 0.012f + row * 0.05f) * (0.3f + row * 0.22f);
      if (row >= 3) cv.line(x, y + 1, x, y + 1 + row * 0.5f, GREEN2, '|');
      for (int k = 0; k < len; k++) {
        float bx = x + lean * (float)k / len;
        bool alt = (k + row) % 2;
        int c = Canvas::rnd(bx), r = Canvas::rnd(y - k);
        if (row >= 2) cv.setBg(c, r, mound);
        cv.put(c, r, alt ? 'o' : '0', alt ? PURPLE.col : PURPLE.hi);
        if (row >= 5 && alt) { cv.put(c - 1, r, '*', PURPLE.col); cv.put(c + 1, r, '*', PURPLE.col); }
      }
    }
  }
  // butterfly
  float t = progress(f, 1) * TAU;
  float bx = 40 + cosf(t) * 24, by = 10 + sinf(t * 2) * 3;
  bool open = (f % 4) < 2;
  cv.putf(bx - 1, by, open ? '}' : '>', YELLOW.hi);
  cv.putf(bx + 1, by, open ? '{' : '<', YELLOW.hi);
  cv.putf(bx, by, ':', RGB(60, 40, 40));
  cv.text(2, 1, "calm like lavender", RGB(255, 225, 245));
}

// =================================================================== 8 ===
// DAISIES IN A VASE
static void sceneDaisyVase(Canvas& cv, uint32_t f) {
  cv.clear(RGB(58, 52, 78));
  // wallpaper dots
  for (int r = 1; r < 33; r += 3)
    for (int c = (r / 3) % 2 * 3; c < COLS; c += 6) cv.put(c, r, '.', RGB(85, 78, 108));
  // table
  cv.fillBg(0, 34, COLS - 1, ROWS - 1, RGB(105, 70, 45));
  for (int c = 0; c < COLS; c++) {
    cv.put(c, 34, '=', RGB(140, 100, 65));
    if (hash(c) % 4 == 0) cv.put(c, 36 + hash(c) % 3, '-', RGB(130, 90, 60));
  }
  // vase
  const uint16_t V = RGB(70, 120, 200), VD = RGB(40, 75, 150), VL = RGB(150, 190, 245);
  for (int r = 23; r <= 34; r++) {
    float t = (r - 23) / 11.0f;
    float w = r == 23 ? 5 : r == 24 ? 3.5f : 3.5f + sinf(t * 3.1f) * 4.5f;
    int iw = Canvas::rnd(w);
    for (int o = -iw; o <= iw; o++) {
      cv.setBg(40 + o, r, V);
      cv.put(40 + o, r, o == -iw ? '(' : o == iw ? ')' : ((o + r) % 4 == 0 ? '~' : ' '),
             o == -iw ? VL : o == iw ? VD : VL);
    }
  }
  cv.put(37, 28, '\'', RGB(255, 255, 255)); cv.put(37, 29, '\'', RGB(255, 255, 255));
  // stems + daisies
  struct D { float x, y, r; bool white; };
  const D ds[] = {{40, 6, 4.5f, true}, {27, 9, 4, true}, {53, 9, 4, true}, {18, 16, 3.2f, true},
                  {62, 16, 3.2f, true}, {33, 15, 3, false}, {47, 15, 3, false}, {40, 14, 2.4f, true}};
  for (unsigned i = 0; i < sizeof(ds) / sizeof(ds[0]); i++) {
    float x = ds[i].x + wave(f, 1, i * 0.11f) * 0.7f;
    cv.curve(40 + ((int)i % 3 - 1), 23, 40 + (x - 40) * 0.3f, ds[i].y + 8, x, ds[i].y + 1, GREEN1, 0, 8);
  }
  leaf(cv, 38, 21, 5, -1, LEAF);
  leaf(cv, 42, 20, 5, +1, LEAF);
  for (unsigned i = 0; i < sizeof(ds) / sizeof(ds[0]); i++) {
    float x = ds[i].x + wave(f, 1, i * 0.11f) * 0.7f;
    daisy(cv, x, ds[i].y, ds[i].r, ds[i].white ? WHITE : YELLOW, ds[i].white ? YELLOW.col : ORANGE.col,
          i % 2 ? 0.0f : TAU / 24);
  }
  sparkles(cv, f, 81, 25, RGB(255, 250, 220), 0, 33);
  cv.text(60, 38, "fresh daisies", RGB(255, 230, 190));
}

// =================================================================== 9 ===
// MOON GARDEN with fireflies
static void sceneMoonGarden(Canvas& cv, uint32_t f) {
  cv.clear(0);
  cv.gradient(RGB(10, 14, 36), RGB(28, 40, 80));
  // crescent moon
  cv.ellipseBg(62, 9, 8, 5.3f, RGB(24, 30, 62));
  cv.ellipse(62, 9, 5.5f, 3.7f, "@0", RGB(255, 245, 190));
  for (int r = 4; r <= 14; r++)
    for (int c = 58; c <= 70; c++) {
      float u = (c - 65.5f) / 4.6f, v = (r - 7.6f) / 3.2f;
      if (u * u + v * v <= 1) cv.put(c, r, ' ', 0);
    }
  sparkles(cv, f, 91, 90, RGB(220, 230, 255), 0, 28);
  // ground silhouette
  for (int c = 0; c < COLS; c++) {
    int top = 34 + Canvas::rnd(sinf(c * 0.15f) * 1.2f);
    for (int r = top; r < ROWS; r++) {
      cv.setBg(c, r, RGB(10, 25, 20));
      if (r == top) cv.put(c, r, (c % 3) ? '^' : 'v', RGB(30, 70, 45));
    }
  }
  // glowing moonflowers
  struct M { float x, y, r; };
  const M ms[] = {{12, 22, 3}, {24, 18, 3.6f}, {37, 24, 2.6f}, {50, 20, 3.4f}, {66, 25, 3}, {6, 29, 2}, {74, 30, 2}};
  for (unsigned i = 0; i < sizeof(ms) / sizeof(ms[0]); i++) {
    float x = ms[i].x + wave(f, 1, i * 0.17f) * 0.6f;
    cv.curve(x, ms[i].y + 1, x + 2, 30, ms[i].x, 35, RGB(40, 110, 80));
    float glow = 0.5f + 0.5f * wave(f, 2, i * 0.13f);
    cv.ellipseBg(x, ms[i].y, ms[i].r + 2, (ms[i].r + 2) * 0.67f, mix(RGB(20, 30, 65), RGB(55, 70, 120), glow));
    daisy(cv, x, ms[i].y, ms[i].r, {RGB(220, 235, 255), RGB(120, 140, 200), RGB(255, 255, 255)},
          RGB(200, 255, 230), 0);
  }
  // fireflies
  Rng rng(17);
  for (int i = 0; i < 14; i++) {
    float cx = rng.range(4, 76), cy = rng.range(14, 33), ph = rng.f();
    float x = cx + wave(f, 1, ph) * 4, y = cy + wave(f, 2, ph + 0.25f) * 1.5f;
    int p = (int)((f + i * 7) % 24);
    if (p < 16) cv.putf(x, y, p < 4 || p > 12 ? '.' : '*', p < 4 || p > 12 ? RGB(150, 200, 80) : RGB(230, 255, 120));
  }
  cv.text(3, 2, "goodnight, sweet dreams", RGB(200, 210, 255));
}

// ================================================================== 10 ===
// FORGET-ME-NOT HEART (beating)
static void sceneForgetMeNot(Canvas& cv, uint32_t f) {
  cv.clear(RGB(255, 236, 240));
  float beat = 1 + 0.07f * fmaxf(0.0f, wave(f, 4)) + 0.03f * fmaxf(0.0f, wave(f, 4, 0.15f));
  const uint16_t blues[] = {RGB(70, 120, 230), RGB(110, 160, 245), RGB(60, 95, 200), RGB(140, 180, 250)};
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++) {
      float u = (c - 40) / (17.0f * beat), v = -(r - 18) / (14.5f * beat);
      float a = u * u + v * v - 1;
      if (a * a * a - u * u * v * v * v > 0) continue;
      uint32_t h = hash(c * 131 + r * 7);
      if ((c + r) % 3 == 0 && h % 3 == 0) cv.put(c, r, 'o', RGB(250, 205, 60));
      else cv.put(c, r, h % 5 == 0 ? '@' : h % 5 == 1 ? '*' : 'o', blues[h % 4]);
    }
  floatingHearts(cv, f, 12, 6, RGB(240, 120, 150));
  sparkles(cv, f, 101, 30, RGB(240, 150, 180), 0, 34, 0, COLS, false);
  cv.textCentered(37, "forget me not", RGB(60, 95, 200));
}

// ================================================================== 11 ===
// WATER LILY POND with koi
static void sceneWaterLilies(Canvas& cv, uint32_t f) {
  cv.clear(0);
  cv.gradient(RGB(12, 50, 62), RGB(22, 85, 92));
  // koi swimming
  for (int i = 0; i < 3; i++) {
    float p = progress(f, 1, i * 0.37f);
    bool right = i % 2 == 0;
    float x = right ? p * 100 - 10 : 90 - p * 100, y = 8 + i * 11 + wave(f, 2, i * 0.3f) * 1.5f;
    uint16_t col = i == 1 ? WHITE.col : ORANGE.col;
    if (right) { cv.putf(x - 2, y, '>', col); cv.putf(x - 1, y, '<', col); cv.putf(x, y, '>', col); }
    else       { cv.putf(x, y, '<', col); cv.putf(x + 1, y, '>', col); cv.putf(x + 2, y, '<', col); }
  }
  // ripples
  const float rp[][2] = {{20, 12}, {58, 26}, {44, 6}};
  for (int i = 0; i < 3; i++) {
    float p = progress(f, 1, i * 0.33f);
    cv.ring(rp[i][0], rp[i][1], 1 + p * 12, (1 + p * 12) * 0.6f, mix(RGB(150, 210, 215), RGB(22, 80, 90), p));
  }
  // water shimmer
  for (int i = 0; i < 40; i++) {
    int c = hash(i * 3) % COLS, r = hash(i * 5 + 1) % ROWS;
    if (cv.at(c, r) == ' ' && (f / 6 + i) % 4 == 0) cv.put(c, r, '~', RGB(70, 140, 150));
  }
  // lily pads + flowers
  struct L { float x, y, r; int flower; };
  const L ls[] = {{14, 26, 6, 1}, {32, 31, 5, 0}, {50, 14, 6.5f, 2}, {68, 30, 5.5f, 1},
                  {66, 8, 4, 0}, {28, 9, 4.5f, 2}, {6, 13, 3.5f, 0}};
  for (unsigned i = 0; i < sizeof(ls) / sizeof(ls[0]); i++) {
    float bob = wave(f, 1, i * 0.21f) * 0.4f;
    float x = ls[i].x, y = ls[i].y + bob, rx = ls[i].r, ry = ls[i].r * 0.55f;
    for (int r = (int)floorf(y - ry); r <= (int)ceilf(y + ry); r++)
      for (int c = (int)floorf(x - rx); c <= (int)ceilf(x + rx); c++) {
        float u = (c - x) / rx, v = (r - y) / ry;
        if (u * u + v * v > 1) continue;
        if (u > 0.1f && fabsf(v) < u * 0.45f) continue;  // the notch
        cv.setBg(c, r, RGB(40, 120, 55));
        cv.put(c, r, (hash(c * 7 + r * 13) % 4 == 0) ? '-' : ' ', RGB(75, 160, 80));
      }
    if (ls[i].flower) {
      const Petal& p = ls[i].flower == 1 ? PINK : WHITE;
      float fx = x - rx * 0.3f, fy = y - 0.5f;
      cv.putf(fx - 1, fy - 1, '\\', p.col); cv.putf(fx, fy - 1, '|', p.hi); cv.putf(fx + 1, fy - 1, '/', p.col);
      cv.putf(fx - 2, fy, '<', p.col); cv.putf(fx - 1, fy, '(', p.hi);
      cv.putf(fx, fy, '@', YELLOW.col);
      cv.putf(fx + 1, fy, ')', p.hi); cv.putf(fx + 2, fy, '>', p.col);
    }
  }
  cv.text(3, 38, "still waters", RGB(170, 220, 220));
}

// ================================================================== 12 ===
// POPPY MEADOW with butterflies
static void scenePoppies(Canvas& cv, uint32_t f) {
  const int H = 14;
  cv.clear(0);
  cv.gradient(RGB(110, 175, 240), RGB(215, 232, 250), 0, H);
  cv.gradient(RGB(95, 160, 70), RGB(40, 100, 40), H, ROWS);
  for (int c = 0; c < COLS; c++) {  // distant hills
    int hill = H - 2 + Canvas::rnd(sinf(c * 0.09f + 2) * 1.6f);
    for (int r = hill; r < H; r++) cv.setBg(c, r, RGB(120, 170, 110));
  }
  // grass waves
  for (int r = H; r < ROWS; r++)
    for (int c = 0; c < COLS; c++) {
      if (hash(c * 17 + r * 53) % 3) continue;
      float w = wave(f, 2, c * 0.025f - r * 0.02f);
      cv.put(c, r, w > 0.45f ? '/' : w < -0.45f ? '\\' : '|', r % 2 ? GREEN3 : GREEN1);
    }
  // poppies: far = tiny, near = big
  Rng rng(42);
  for (int i = 0; i < 90; i++) {
    float y = rng.range(H + 1, ROWS - 2), x = rng.range(0, COLS);
    float depth = (y - H) / (ROWS - H);
    x += wave(f, 2, x * 0.025f - y * 0.02f) * depth * 0.8f;
    const Petal& p = (i % 9 == 0) ? ORANGE : RED;
    if (depth < 0.25f) cv.putf(x, y, 'o', p.col);
    else if (depth < 0.55f) { cv.putf(x - 0.5f, y, '@', p.col); cv.putf(x + 0.5f, y, '@', p.col); }
    else {
      float rx = 1.2f + depth * 1.8f;
      cv.line(x, y + 1, x, y + 4, GREEN2, '|');
      cv.ellipse(x, y, rx, rx * 0.6f, "@O", p.col);
      cv.putf(x - rx, y, '(', p.dark); cv.putf(x + rx, y, ')', p.dark);
      cv.putf(x, y, '#', RGB(30, 20, 30));
    }
  }
  // butterflies
  for (int i = 0; i < 2; i++) {
    float t = progress(f, 1, i * 0.5f) * TAU;
    float bx = 40 + sinf(t) * 30, by = 10 + i * 6 + sinf(t * 3) * 2;
    bool open = ((f + i * 2) % 4) < 2;
    uint16_t col = i ? BLUE.hi : ORANGE.col;
    cv.putf(bx - 1, by, open ? '}' : '>', col);
    cv.putf(bx + 1, by, open ? '{' : '<', col);
    cv.putf(bx, by, ':', RGB(40, 30, 30));
  }
  cv.text(2, 1, "wild & free", RGB(40, 60, 110));
}

// ============================================================ scene list ==
struct Scene {
  const char* name;
  void (*draw)(Canvas&, uint32_t);
};
static const Scene SCENES[] = {
  {"Wildflower Bouquet", sceneWildflowers},
  {"Tulip Bouquet",      sceneTulipBouquet},
  {"Tulip Field",        sceneTulipField},
  {"Single Rose",        sceneRose},
  {"Sunflowers",         sceneSunflowers},
  {"Cherry Blossom",     sceneCherryBlossom},
  {"Lavender Field",     sceneLavender},
  {"Daisies in a Vase",  sceneDaisyVase},
  {"Moon Garden",        sceneMoonGarden},
  {"Forget-Me-Not",      sceneForgetMeNot},
  {"Water Lilies",       sceneWaterLilies},
  {"Poppy Meadow",       scenePoppies},
};
static const int SCENE_COUNT = sizeof(SCENES) / sizeof(SCENES[0]);

}  // namespace scenes
