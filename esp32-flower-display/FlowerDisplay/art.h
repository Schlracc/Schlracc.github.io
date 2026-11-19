// ============================================================================
//  art.h  -  tiny ASCII-art canvas engine (shared by the ESP32 sketch and the
//            PC dev preview, so what you see in dev mode is pixel-identical)
//
//  The 320x240 landscape screen is an 80 x 40 grid of character cells.
//  Each cell is 4x6 pixels and holds: a character, a foreground colour and a
//  background colour. Glyphs come from a 3x5 pixel font (font3x5.h).
// ============================================================================
#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "font3x5.h"

namespace art {

constexpr int CELL_W = 4, CELL_H = 6;
constexpr int COLS = 80, ROWS = 40;
constexpr int SCREEN_W = COLS * CELL_W;   // 320
constexpr int SCREEN_H = ROWS * CELL_H;   // 240
constexpr int LOOP = 96;                  // every animation repeats after this many frames
constexpr float TAU = 6.28318530718f;

// Special glyphs (see tools/gen_font.py)
constexpr uint8_t HEART = 0x80, BLOCK = 0x81, SHADE = 0x82, DOT = 0x84;

// ---------------------------------------------------------------- colours --
constexpr uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
inline uint16_t mix(uint16_t a, uint16_t b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31;
  int br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (int)((br - ar) * t + 0.5f);
  int g = ag + (int)((bg - ag) * t + 0.5f);
  int bl = ab + (int)((bb - ab) * t + 0.5f);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

// ------------------------------------------------------------- randomness --
inline uint32_t hash(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}
struct Rng {
  uint32_t s;
  explicit Rng(uint32_t seed) : s(hash(seed) | 1) {}
  uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  float f() { return (next() >> 8) * (1.0f / 16777216.0f); }        // 0..1
  float range(float a, float b) { return a + (b - a) * f(); }
  int irange(int a, int b) { return a + (int)(next() % (uint32_t)(b - a)); } // [a,b)
  template <class T, int N> const T& pick(const T (&arr)[N]) { return arr[next() % N]; }
};

// --------------------------------------------------- looping animation --
// All of these repeat exactly every LOOP frames so the preview GIFs loop.
inline float wave(uint32_t f, int cycles, float phase = 0) {
  return sinf(TAU * ((float)cycles * (float)(f % LOOP) / LOOP + phase));
}
inline float progress(uint32_t f, int cycles, float phase = 0) {   // 0..1 sawtooth
  float p = (float)cycles * (float)(f % LOOP) / LOOP + phase;
  return p - floorf(p);
}

// Character for a line going in direction (dx,dy) measured in PIXELS.
inline char slopeChar(float dx, float dy) {
  float ax = fabsf(dx), ay = fabsf(dy);
  if (ay < ax * 0.45f) return '-';
  if (ax < ay * 0.45f) return '|';
  return ((dx > 0) == (dy > 0)) ? '\\' : '/';
}

struct Cell {
  uint8_t ch;
  uint16_t fg, bg;
  bool operator==(const Cell& o) const { return ch == o.ch && fg == o.fg && bg == o.bg; }
  bool operator!=(const Cell& o) const { return !(*this == o); }
};

// ================================================================= Canvas ==
class Canvas {
 public:
  Cell cell[ROWS][COLS];

  static int rnd(float v) { return (int)floorf(v + 0.5f); }
  static bool in(int c, int r) { return c >= 0 && c < COLS && r >= 0 && r < ROWS; }

  void clear(uint16_t bg) {
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++) cell[r][c] = {' ', bg, bg};
  }
  // vertical background gradient across rows [r0, r1)
  void gradient(uint16_t top, uint16_t bottom, int r0 = 0, int r1 = ROWS) {
    for (int r = r0; r < r1; r++) {
      uint16_t col = mix(top, bottom, (r1 - r0) > 1 ? (float)(r - r0) / (r1 - r0 - 1) : 0);
      for (int c = 0; c < COLS; c++)
        if (in(c, r)) cell[r][c].bg = col;
    }
  }
  void setBg(int c, int r, uint16_t bg) { if (in(c, r)) cell[r][c].bg = bg; }
  void fillBg(int c0, int r0, int c1, int r1, uint16_t bg) {
    for (int r = r0; r <= r1; r++) for (int c = c0; c <= c1; c++) setBg(c, r, bg);
  }
  void put(int c, int r, uint8_t ch, uint16_t fg) {
    if (in(c, r)) { cell[r][c].ch = ch; cell[r][c].fg = fg; }
  }
  void putf(float x, float y, uint8_t ch, uint16_t fg) { put(rnd(x), rnd(y), ch, fg); }
  uint8_t at(int c, int r) const { return in(c, r) ? cell[r][c].ch : 0; }

  void text(int c, int r, const char* s, uint16_t fg) {
    for (; *s; s++, c++) if (*s != ' ') put(c, r, (uint8_t)*s, fg);
  }
  void textCentered(int r, const char* s, uint16_t fg) {
    text((COLS - (int)strlen(s)) / 2, r, s, fg);
  }

  // straight line made of / | \ - characters (or a forced character)
  void line(float x0, float y0, float x1, float y1, uint16_t fg, uint8_t forced = 0) {
    float dx = x1 - x0, dy = y1 - y0;
    int steps = (int)ceilf(fmaxf(fabsf(dx), fabsf(dy)));
    if (steps < 1) steps = 1;
    uint8_t ch = forced ? forced : (uint8_t)slopeChar(dx * CELL_W, dy * CELL_H);
    for (int i = 0; i <= steps; i++) {
      float t = (float)i / steps;
      putf(x0 + dx * t, y0 + dy * t, ch, fg);
    }
  }
  // quadratic bezier curve (x0,y0) -> control (cx,cy) -> (x1,y1)
  void curve(float x0, float y0, float cx, float cy, float x1, float y1, uint16_t fg,
             uint8_t forced = 0, int segs = 10) {
    float px = x0, py = y0;
    for (int i = 1; i <= segs; i++) {
      float t = (float)i / segs, u = 1 - t;
      float x = u * u * x0 + 2 * u * t * cx + t * t * x1;
      float y = u * u * y0 + 2 * u * t * cy + t * t * y1;
      line(px, py, x, y, fg, forced);
      px = x; py = y;
    }
  }
  // thick tapered curve painted as background colour (branches, trunks)
  void curveBg(float x0, float y0, float cx, float cy, float x1, float y1, float w0, float w1,
               uint16_t bg, int steps = 40) {
    for (int i = 0; i <= steps; i++) {
      float t = (float)i / steps, u = 1 - t;
      float x = u * u * x0 + 2 * u * t * cx + t * t * x1;
      float y = u * u * y0 + 2 * u * t * cy + t * t * y1;
      float w = w0 + (w1 - w0) * t;
      ellipseBg(x, y, fmaxf(w, 0.5f), fmaxf(w * 0.67f, 0.5f), bg);
    }
  }
  // filled ellipse; characters are taken from `chars` in a repeating pattern
  void ellipse(float cx, float cy, float rx, float ry, const char* chars, uint16_t fg) {
    int n = (int)strlen(chars);
    for (int r = (int)floorf(cy - ry); r <= (int)ceilf(cy + ry); r++)
      for (int c = (int)floorf(cx - rx); c <= (int)ceilf(cx + rx); c++) {
        float u = (c - cx) / rx, v = (r - cy) / ry;
        if (u * u + v * v <= 1.0f) put(c, r, (uint8_t)chars[(unsigned)(c * 7 + r * 3) % n], fg);
      }
  }
  void ellipseBg(float cx, float cy, float rx, float ry, uint16_t bg) {
    for (int r = (int)floorf(cy - ry); r <= (int)ceilf(cy + ry); r++)
      for (int c = (int)floorf(cx - rx); c <= (int)ceilf(cx + rx); c++) {
        float u = (c - cx) / rx, v = (r - cy) / ry;
        if (u * u + v * v <= 1.0f) setBg(c, r, bg);
      }
  }
  // outline ring of an ellipse using slope characters
  void ring(float cx, float cy, float rx, float ry, uint16_t fg, uint8_t forced = 0) {
    int n = (int)(TAU * fmaxf(rx, ry) * 1.3f) + 8;
    for (int i = 0; i < n; i++) {
      float a = TAU * i / n;
      float x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
      // tangent direction
      uint8_t ch = forced ? forced : (uint8_t)slopeChar(-sinf(a) * rx * CELL_W, cosf(a) * ry * CELL_H);
      putf(x, y, ch, fg);
    }
  }
  // spokes radiating from a centre (flower petals, sun rays...)
  void spokes(float cx, float cy, float r0, float r1, int count, float rot, uint16_t fg,
              uint8_t forced = 0) {
    for (int k = 0; k < count; k++) {
      float a = rot + TAU * k / count;
      float ux = cosf(a), uy = sinf(a) * (float)CELL_W / CELL_H;  // keep it round in pixels
      uint8_t ch = forced ? forced : (uint8_t)slopeChar(ux * CELL_W, uy * CELL_H);
      for (float d = r0; d <= r1 + 0.01f; d += 0.5f) putf(cx + ux * d, cy + uy * d, ch, fg);
    }
  }
};

// =============================================================== renderer ==
// Render one 4x6 cell into `out` (row stride = `stride` pixels).
inline void renderCell(const Cell& k, uint16_t* out, int stride) {
  uint16_t g = glyph3x5(k.ch);
  for (int y = 0; y < CELL_H; y++)
    for (int x = 0; x < CELL_W; x++) {
      bool on = (x < 3 && y < 5) && ((g >> (14 - (y * 3 + x))) & 1);
      out[y * stride + x] = on ? k.fg : k.bg;
    }
}
// Render one full text row (320 x 6 pixels) into `out`.
inline void renderRow(const Canvas& cv, int r, uint16_t* out) {
  for (int c = 0; c < COLS; c++) renderCell(cv.cell[r][c], out + c * CELL_W, SCREEN_W);
}

}  // namespace art
