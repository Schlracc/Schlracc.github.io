# -*- coding: utf-8 -*-
"""
flower_preview.py  -  see the ESP32 flower graphics in Spyder (or any Python).

This is a line-for-line Python copy of FlowerDisplay/art.h + scenes.h, so the
pictures are what the 320x240 screen will show. It only needs numpy and
matplotlib, which Spyder already has.

HOW TO USE IN SPYDER
  1. Open this file and press F5 (Run).
  2. The pictures appear in the "Plots" pane (top right).
     - one sheet with all 12 scenes
     - then each scene on its own, at the real 320x240 size
  3. Change the settings just below and run again.

ANIMATION (optional): set ANIMATE = True. A separate window opens and plays
the scene; press SPACE or click in the window to go to the next one, like the
real button. (Spyder's Plots pane can't animate, so the script switches
matplotlib to a pop-up window for you.)
"""

# ============================ SETTINGS ======================================
SCENE = "all"      # "all", or a scene number 1-12 (e.g. 2 = Tulip Bouquet)
FRAME = 30         # which animation frame to show (0-95)
SHOW_EACH = True   # also show every scene in its own full-size figure
ANIMATE = False    # True = pop-up window that plays the animation
SAVE_PNG = False   # True = also save scene_XX.png files next to this script
# ============================================================================

import math
import os
import numpy as np
import matplotlib.pyplot as plt

# ------------------------------------------------------------------ engine --
CELL_W, CELL_H = 4, 6
COLS, ROWS = 80, 40
SCREEN_W, SCREEN_H = COLS * CELL_W, ROWS * CELL_H   # 320 x 240
LOOP = 96                                            # animations repeat every 96 frames
TAU = 6.28318530718
HEART, BLOCK, SHADE, DOT = 0x80, 0x81, 0x82, 0x84
M32 = 0xFFFFFFFF

# 3x5 pixel font (same table as FlowerDisplay/font3x5.h)
FONT3X5 = [
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x2482, 0x5A00, 0x5F7D, 0x3C9E, 0x42A1, 0x2AAB, 0x2400,
    0x1491, 0x4494, 0x0AA8, 0x05D0, 0x0014, 0x01C0, 0x0002, 0x12A4,
    0x2B6A, 0x2C97, 0x62A7, 0x628E, 0x5BC9, 0x798E, 0x39EF, 0x7292,
    0x7BEF, 0x7BCE, 0x0410, 0x0414, 0x1511, 0x0E38, 0x4454, 0x6282,
    0x2BE3, 0x2BED, 0x6BAE, 0x3923, 0x6B6E, 0x79A7, 0x79A4, 0x396B,
    0x5BED, 0x7497, 0x126A, 0x5BAD, 0x4927, 0x5FED, 0x6B6D, 0x2B6A,
    0x6BA4, 0x2B7B, 0x6BAD, 0x388E, 0x7492, 0x5B6F, 0x5B6A, 0x5BFD,
    0x5AAD, 0x5A92, 0x72A7, 0x6926, 0x4889, 0x324B, 0x2A00, 0x0007,
    0x4400, 0x0CEB, 0x4D6E, 0x0723, 0x176B, 0x05E3, 0x39E4, 0x075E,
    0x4D6D, 0x2092, 0x106A, 0x4BB5, 0x6493, 0x0FED, 0x0D6D, 0x056A,
    0x0D74, 0x0759, 0x0BA4, 0x078E, 0x2E93, 0x0B6B, 0x0B6A, 0x0B7D,
    0x0A95, 0x0B54, 0x0EF7, 0x3513, 0x2492, 0x6456, 0x09C8, 0x0000,
    0x5FD0, 0x7FFF, 0x5555, 0x05D0, 0x0080,
]
# glyph bitmaps as a (133, 6, 4) boolean array: 3x5 glyph inside a 4x6 cell
GLYPHS = np.zeros((len(FONT3X5), CELL_H, CELL_W), dtype=bool)
for _i, _g in enumerate(FONT3X5):
    for _y in range(5):
        for _x in range(3):
            GLYPHS[_i, _y, _x] = (_g >> (14 - (_y * 3 + _x))) & 1


def RGB(r, g, b):
    """24-bit colour -> RGB565 (what the screen uses)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def mix(a, b, t):
    """Blend two RGB565 colours, t = 0..1."""
    if t <= 0:
        return a
    if t >= 1:
        return b
    ar, ag, ab = a >> 11, (a >> 5) & 63, a & 31
    br, bg, bb = b >> 11, (b >> 5) & 63, b & 31
    r = ar + int((br - ar) * t + 0.5)
    g = ag + int((bg - ag) * t + 0.5)
    bl = ab + int((bb - ab) * t + 0.5)
    return (r << 11) | (g << 5) | bl


def hash32(x):
    x &= M32
    x ^= x >> 16
    x = (x * 0x7feb352d) & M32
    x ^= x >> 15
    x = (x * 0x846ca68b) & M32
    x ^= x >> 16
    return x


class Rng:
    """Same xorshift random generator as the ESP32, so layouts match."""

    def __init__(self, seed):
        self.s = hash32(seed) | 1

    def next(self):
        s = self.s
        s ^= (s << 13) & M32
        s ^= s >> 17
        s ^= (s << 5) & M32
        self.s = s
        return s

    def f(self):                      # 0..1
        return (self.next() >> 8) * (1.0 / 16777216.0)

    def range(self, a, b):
        return a + (b - a) * self.f()

    def irange(self, a, b):           # [a, b)
        return a + self.next() % ((b - a) & M32)


def wave(f, cycles, phase=0.0):
    return math.sin(TAU * (cycles * (f % LOOP) / LOOP + phase))


def progress(f, cycles, phase=0.0):  # 0..1 sawtooth
    p = cycles * (f % LOOP) / LOOP + phase
    return p - math.floor(p)


def slope_char(dx, dy):
    """Character for a line going in direction (dx, dy) in pixels."""
    ax, ay = abs(dx), abs(dy)
    if ay < ax * 0.45:
        return '-'
    if ax < ay * 0.45:
        return '|'
    return '\\' if (dx > 0) == (dy > 0) else '/'


def rnd(v):
    return math.floor(v + 0.5)


def _code(ch):
    return ord(ch) if isinstance(ch, str) else ch


class Canvas:
    """80 x 40 grid of cells; each cell = character + foreground + background."""

    def __init__(self):
        self.ch = [[32] * COLS for _ in range(ROWS)]
        self.fg = [[0] * COLS for _ in range(ROWS)]
        self.bg = [[0] * COLS for _ in range(ROWS)]

    @staticmethod
    def inside(c, r):
        return 0 <= c < COLS and 0 <= r < ROWS

    def clear(self, bg):
        for r in range(ROWS):
            for c in range(COLS):
                self.ch[r][c] = 32
                self.fg[r][c] = bg
                self.bg[r][c] = bg

    def gradient(self, top, bottom, r0=0, r1=ROWS):
        for r in range(r0, r1):
            t = (r - r0) / (r1 - r0 - 1) if (r1 - r0) > 1 else 0
            col = mix(top, bottom, t)
            if 0 <= r < ROWS:
                for c in range(COLS):
                    self.bg[r][c] = col

    def set_bg(self, c, r, bg):
        if self.inside(c, r):
            self.bg[r][c] = bg

    def fill_bg(self, c0, r0, c1, r1, bg):
        for r in range(r0, r1 + 1):
            for c in range(c0, c1 + 1):
                self.set_bg(c, r, bg)

    def put(self, c, r, ch, fg):
        if self.inside(c, r):
            self.ch[r][c] = _code(ch)
            self.fg[r][c] = fg

    def putf(self, x, y, ch, fg):
        self.put(rnd(x), rnd(y), ch, fg)

    def at(self, c, r):
        return self.ch[r][c] if self.inside(c, r) else 0

    def text(self, c, r, s, fg):
        for k, ch in enumerate(s):
            if ch != ' ':
                self.put(c + k, r, ch, fg)

    def text_centered(self, r, s, fg):
        self.text((COLS - len(s)) // 2, r, s, fg)

    def line(self, x0, y0, x1, y1, fg, forced=0):
        dx, dy = x1 - x0, y1 - y0
        steps = max(1, math.ceil(max(abs(dx), abs(dy))))
        ch = forced if forced else slope_char(dx * CELL_W, dy * CELL_H)
        for i in range(steps + 1):
            t = i / steps
            self.putf(x0 + dx * t, y0 + dy * t, ch, fg)

    def curve(self, x0, y0, cx, cy, x1, y1, fg, forced=0, segs=10):
        px, py = x0, y0
        for i in range(1, segs + 1):
            t = i / segs
            u = 1 - t
            x = u * u * x0 + 2 * u * t * cx + t * t * x1
            y = u * u * y0 + 2 * u * t * cy + t * t * y1
            self.line(px, py, x, y, fg, forced)
            px, py = x, y

    def curve_bg(self, x0, y0, cx, cy, x1, y1, w0, w1, bg, steps=40):
        for i in range(steps + 1):
            t = i / steps
            u = 1 - t
            x = u * u * x0 + 2 * u * t * cx + t * t * x1
            y = u * u * y0 + 2 * u * t * cy + t * t * y1
            w = w0 + (w1 - w0) * t
            self.ellipse_bg(x, y, max(w, 0.5), max(w * 0.67, 0.5), bg)

    def ellipse(self, cx, cy, rx, ry, chars, fg):
        n = len(chars)
        for r in range(math.floor(cy - ry), math.ceil(cy + ry) + 1):
            for c in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
                u, v = (c - cx) / rx, (r - cy) / ry
                if u * u + v * v <= 1.0:
                    self.put(c, r, chars[((c * 7 + r * 3) & M32) % n], fg)

    def ellipse_bg(self, cx, cy, rx, ry, bg):
        for r in range(math.floor(cy - ry), math.ceil(cy + ry) + 1):
            for c in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
                u, v = (c - cx) / rx, (r - cy) / ry
                if u * u + v * v <= 1.0:
                    self.set_bg(c, r, bg)

    def ring(self, cx, cy, rx, ry, fg, forced=0):
        n = int(TAU * max(rx, ry) * 1.3) + 8
        for i in range(n):
            a = TAU * i / n
            ch = forced if forced else slope_char(-math.sin(a) * rx * CELL_W, math.cos(a) * ry * CELL_H)
            self.putf(cx + math.cos(a) * rx, cy + math.sin(a) * ry, ch, fg)

    def spokes(self, cx, cy, r0, r1, count, rot, fg, forced=0):
        for k in range(count):
            a = rot + TAU * k / count
            ux, uy = math.cos(a), math.sin(a) * CELL_W / CELL_H
            ch = forced if forced else slope_char(ux * CELL_W, uy * CELL_H)
            d = r0
            while d <= r1 + 0.01:
                self.putf(cx + ux * d, cy + uy * d, ch, fg)
                d += 0.5

    # ---- turn the character grid into a 240x320 RGB image ----
    def to_image(self):
        ch = np.minimum(np.array(self.ch), len(FONT3X5) - 1)
        mask = GLYPHS[ch]                                   # (40, 80, 6, 4)
        mask = mask.transpose(0, 2, 1, 3).reshape(SCREEN_H, SCREEN_W)
        fg = np.repeat(np.repeat(_to_rgb(np.array(self.fg)), CELL_H, 0), CELL_W, 1)
        bg = np.repeat(np.repeat(_to_rgb(np.array(self.bg)), CELL_H, 0), CELL_W, 1)
        return np.where(mask[..., None], fg, bg).astype(np.uint8)


def _to_rgb(c565):
    r = (c565 >> 11) & 31
    g = (c565 >> 5) & 63
    b = c565 & 31
    return np.stack([r * 255 // 31, g * 255 // 63, b * 255 // 31], axis=-1)


# ----------------------------------------------------------------- palette --
class Petal:
    def __init__(self, col, dark, hi):
        self.col, self.dark, self.hi = col, dark, hi


RED = Petal(RGB(230, 55, 60), RGB(140, 20, 40), RGB(255, 150, 140))
PINK = Petal(RGB(245, 120, 170), RGB(170, 60, 110), RGB(255, 205, 225))
YELLOW = Petal(RGB(250, 205, 60), RGB(190, 125, 20), RGB(255, 245, 170))
PURPLE = Petal(RGB(165, 100, 230), RGB(95, 45, 160), RGB(215, 180, 255))
WHITE = Petal(RGB(240, 236, 232), RGB(165, 160, 175), RGB(255, 255, 255))
ORANGE = Petal(RGB(250, 140, 50), RGB(180, 70, 20), RGB(255, 200, 130))
BLUE = Petal(RGB(85, 120, 245), RGB(40, 60, 160), RGB(170, 195, 255))
GREEN1, GREEN2 = RGB(90, 190, 90), RGB(60, 150, 70)
GREEN3, LEAF = RGB(140, 215, 110), RGB(50, 130, 60)


# ---------------------------------------------------------- flower helpers --
def sparkles(cv, f, seed, count, col, r0=0, r1=ROWS, c0=0, c1=COLS, dim_dots=True):
    rng = Rng(seed)
    for i in range(count):
        c = rng.irange(c0, c1)
        r = rng.irange(r0, r1)
        ph = rng.irange(0, LOOP)
        if cv.at(c, r) != 32:
            continue
        p = (f + ph) % 32
        ch = '.' if p < 2 else '+' if p < 4 else '*' if p < 6 else '+' if p < 8 else '.' if p < 10 else None
        if ch:
            cv.put(c, r, ch, col)
        elif dim_dots and i % 3 == 0:
            cv.put(c, r, '.', mix(col, cv.bg[r][c], 0.55))


def floating_hearts(cv, f, seed, count, col, r0=0, r1=ROWS):
    rng = Rng(seed)
    for i in range(count):
        x = rng.range(3, COLS - 4)
        ph = rng.f()
        y = r1 - progress(f, 1, ph) * (r1 - r0 + 2)
        x += wave(f, 2, ph) * 1.5
        if i % 2:
            cv.putf(x, y, '<', col)
            cv.putf(x + 1, y, '3', col)
        else:
            cv.putf(x, y, HEART, col)


def falling_petals(cv, f, seed, count, cols, chars, drift=6):
    rng = Rng(seed)
    for i in range(count):
        x0 = rng.range(-5, COLS)
        ph = rng.f()
        k = rng.irange(1, 3)
        p = progress(f, k, ph)
        y = -2 + p * (ROWS + 4)
        x = x0 + p * drift + wave(f, 3, ph) * 1.5
        cv.putf(x, y, chars[(i + int(p * 8)) % len(chars)], cols[i % len(cols)])


def cluster(cv, x, y, rx, ry, p, seed):
    for r in range(math.floor(y - ry), math.ceil(y + ry) + 1):
        for c in range(math.floor(x - rx), math.ceil(x + rx) + 1):
            u, v = (c - x) / rx, (r - y) / ry
            if u * u + v * v > 1:
                continue
            h = hash32(seed * 977 + c * 31 + r)
            ch = '0' if h % 7 == 0 else '@' if h % 11 == 0 else 'o'
            cv.put(c, r, ch, mix(p.hi, p.col, (v + 1) * 0.6))


def daisy(cv, x, y, r, p, center, rot=0.0):
    n = 8 if r < 2.5 else 12
    cv.spokes(x, y, 1, r, n, rot, p.col)
    cv.spokes(x, y, r * 0.6, r, n, rot + TAU / (2 * n), p.hi)
    if r >= 3.5:
        cv.ellipse(x, y, 1.3, 0.8, "@o", center)
    else:
        cv.putf(x, y, '@', center)


def tulip(cv, x, y, hw, h, p):
    """Tulip head. (x, y) = top centre, hw = half width, h = rows tall."""
    cx, cy = rnd(x), rnd(y)
    if hw <= 0:
        cv.put(cx, cy, 'U', p.col)
        return cy + 1
    if hw == 1:
        cv.put(cx - 1, cy, '(', p.dark); cv.put(cx, cy, '^', p.hi); cv.put(cx + 1, cy, ')', p.dark)
        cv.put(cx - 1, cy + 1, '\\', p.dark); cv.put(cx, cy + 1, '@', p.col); cv.put(cx + 1, cy + 1, '/', p.dark)
        return cy + 2
    for o in range(-hw + 1, hw):                      # petal tips
        if (o + hw) & 1:
            cv.put(cx + o, cy, '^', p.hi)
    seam = (hw + 1) // 2
    for i in range(1, h - 1):                         # body
        cv.put(cx - hw, cy + i, '(', p.hi)
        cv.put(cx + hw, cy + i, ')', p.dark)
        for o in range(-hw + 1, hw):
            is_seam = o in (-seam, seam)
            col = p.dark if is_seam else (mix(p.col, p.hi, 0.35) if o < 0 else p.col)
            cv.put(cx + o, cy + i, '|' if is_seam else '@', col)
    b = cy + h - 1                                    # base
    cv.put(cx - hw + 1, b, '\\', p.dark)
    cv.put(cx + hw - 1, b, '/', p.dark)
    for o in range(-hw + 2, hw - 1):
        cv.put(cx + o, b, '@', p.dark)
    return b + 1


def rose(cv, x, y, rx, p):
    ry = rx * 0.68
    cv.ring(x, y, rx + 1.2, ry + 0.9, p.dark)
    for r in range(math.floor(y - ry), math.ceil(y + ry) + 1):
        for c in range(math.floor(x - rx), math.ceil(x + rx) + 1):
            u, v = (c - x) / rx, (r - y) / ry
            d = math.sqrt(u * u + v * v)
            if d > 1:
                continue
            s = d * 2.6 + math.atan2(v, u) / TAU
            s -= math.floor(s)
            if s < 0.28:
                cv.put(c, r, slope_char(-v, u), p.dark)
            else:
                ch = ('(' if u < 0 else ')') if d > 0.82 else ('@' if s < 0.6 else 'o')
                cv.put(c, r, ch, mix(p.hi, p.col, d * 1.3))


def sunflower(cv, x, y, r, rot):
    cv.spokes(x, y, r * 0.5, r, 16, rot, YELLOW.col)
    cv.spokes(x, y, r * 0.5, r * 0.85, 16, rot + TAU / 32, ORANGE.col)
    cv.ellipse(x, y, r * 0.5, r * 0.5 * 0.67, "#@o#", RGB(110, 60, 25))
    cv.putf(x, y, '@', RGB(60, 30, 10))


def leaf(cv, x, y, length, direction, col):
    n = int(length)
    for i in range(1, n + 1):
        yy = y - i * 0.3
        h = math.sin(3.14159 * i / (n + 1)) * 0.9
        c = rnd(x + direction * i)
        for r in range(rnd(yy - h), rnd(yy + h) + 1):
            cv.set_bg(c, r, col)
        tip = ('>' if direction > 0 else '<') if i == n else '-'
        cv.putf(x + direction * i, yy, tip, GREEN3)


def bouquet_base(cv, hx, hy, gx, gy, ribbon, ribbon_dark):
    for i in range(len(hx)):
        cx = gx + (hx[i] - gx) * 0.25
        cy = hy[i] + (gy - hy[i]) * 0.45
        cv.curve(gx, gy, cx, cy, hx[i], hy[i], GREEN2 if i % 3 == 0 else GREEN1, 0, 8)
    for i in range(1, 6):                              # stem bundle
        w = 3 - i // 2
        for o in range(-w, w + 1):
            cv.put(int(gx) + o, int(gy) + i, '/' if o < 0 else '\\' if o > 0 else '|', GREEN2)
    x, y = int(gx), int(gy)                            # ribbon + bow
    cv.text(x - 4, y, "<<=", ribbon)
    cv.put(x - 1, y, '(', ribbon_dark); cv.put(x, y, '@', ribbon); cv.put(x + 1, y, ')', ribbon_dark)
    cv.text(x + 2, y, "=>>", ribbon)
    cv.put(x - 2, y + 1, '/', ribbon); cv.put(x - 3, y + 2, '/', ribbon_dark)
    cv.put(x + 2, y + 1, '\\', ribbon); cv.put(x + 3, y + 2, '\\', ribbon_dark)


# ================================================================== scenes ==
def scene_wildflowers(cv, f):
    cv.clear(RGB(50, 56, 80))
    gx, gy, N = 40, 33, 24
    rng = Rng(7)
    hx, hy = [], []
    for i in range(N):
        a = -1.35 + 2.7 * (i + rng.range(-0.3, 0.3)) / (N - 1)
        d = rng.range(14, 27)
        hx.append(gx + math.sin(a) * d * 1.45 + wave(f, 1, rng.f()) * 0.7)
        hy.append(gy - math.cos(a) * d * 0.95 - 2)
    bouquet_base(cv, hx, hy, gx, gy, PINK.col, PINK.dark)
    lr = Rng(99)
    for _ in range(40):
        k = lr.irange(0, N)
        t = lr.range(0.3, 0.8)
        cv.putf(gx + (hx[k] - gx) * t, gy + (hy[k] - gy) * t, '+' if lr.irange(0, 2) else '*', GREEN3)
    pals = [RED, PINK, YELLOW, PURPLE, WHITE, ORANGE, BLUE]
    hr = Rng(3)
    for i in range(N):
        p = pals[hr.irange(0, 7)]
        kind = hr.irange(0, 10)
        if kind < 6:
            rx = hr.range(2, 4.2)
            ry = hr.range(1, 1.8)
            cluster(cv, hx[i], hy[i], rx, ry, p, i)
        elif kind < 8:
            daisy(cv, hx[i], hy[i], hr.range(2, 3.2), p, YELLOW.col, progress(f, 1) * TAU / 8)
        else:
            for k in range(4):
                cv.putf(hx[i], hy[i] - k, 'o' if k % 2 else '0', p.col if k % 2 else p.hi)
    sparkles(cv, f, 11, 70, RGB(235, 235, 255))
    floating_hearts(cv, f, 5, 5, PINK.col, 0, 30)
    cv.text_centered(39, "picked by hand, just for you", PINK.hi)


def scene_tulip_bouquet(cv, f):
    cv.clear(RGB(36, 34, 62))
    gx, gy = 40, 31
    tl = [(40, 3, 3, 5, RED), (29, 6, 3, 5, PINK), (51, 6, 3, 5, YELLOW),
          (19, 11, 2, 4, PURPLE), (61, 11, 2, 4, ORANGE), (34, 12, 3, 5, WHITE),
          (46, 12, 3, 5, PINK), (12, 17, 2, 4, RED), (68, 17, 2, 4, PURPLE),
          (24, 17, 2, 4, YELLOW), (56, 17, 2, 4, RED)]
    tx = [t[0] + wave(f, 1, i * 0.13) * 0.8 for i, t in enumerate(tl)]
    sy = [t[1] + t[3] for t in tl]
    for k in (-1, 1):                                  # long tulip leaves
        cv.curve_bg(gx, gy, gx + k * 10, gy - 2, gx + k * 22, gy - 12, 1.2, 0.3, LEAF)
        cv.curve(gx, gy, gx + k * 10, gy - 2, gx + k * 22, gy - 12, GREEN3)
        cv.curve_bg(gx, gy, gx + k * 6, gy - 3, gx + k * 12, gy - 16, 1.0, 0.2, RGB(40, 110, 55))
    bouquet_base(cv, tx, sy, gx, gy, RGB(240, 200, 220), RGB(180, 120, 160))
    for i, (x, y, hw, h, p) in enumerate(tl):
        tulip(cv, tx[i], y, hw, h, p)
    sparkles(cv, f, 21, 60, RGB(255, 230, 240))
    floating_hearts(cv, f, 8, 4, RED.hi, 0, 28)
    cv.text_centered(39, "two lips, er.. tulips, for you", PINK.hi)


def scene_tulip_field(cv, f):
    H = 17
    cv.clear(0)
    cv.gradient(RGB(40, 45, 110), RGB(250, 165, 120), 0, H + 1)
    cv.gradient(RGB(40, 90, 50), RGB(20, 60, 30), H + 1, ROWS)
    cv.ellipse_bg(58, H, 7, 4.5, RGB(255, 195, 130))
    cv.ellipse(58, H, 4.5, 3, "o@", RGB(255, 235, 150))
    cv.text(44, 9, "~~-- ---~~", RGB(255, 190, 170))
    cv.text(8, 6, "--~~~--", RGB(200, 150, 190))
    for i in range(3):                                 # birds
        x = progress(f, 1, i * 0.08) * 95 - 10 + i * 3
        y = 4 + i + wave(f, 4, i * 0.3) * 0.6
        cv.putf(x, y, 'v' if (f // 3 + i) % 2 else '-', RGB(40, 30, 60))
    stripes = [RED, YELLOW, PINK, PURPLE, ORANGE, WHITE]
    for r in range(H + 1, ROWS):
        depth = (r - H) / (ROWS - H)
        for c in range(COLS):
            s = (c - 40.0) / (r - H + 1.5) * 0.9 + 0.0137
            band = math.floor(s + 100)
            if s + 100 - band > 0.8:                   # green rows
                cv.put(c, r, '|' if depth > 0.4 else '.', GREEN1)
                continue
            p = stripes[band % 6]
            sway = rnd(wave(f, 2, c * 0.015 + r * 0.03) * depth * 0.8)
            if depth < 0.25:
                cv.put(c, r, '.', p.col)
            elif depth < 0.55:
                cv.put(c, r, 'v' if (c + r) % 2 else 'u', p.col)
            elif r % 2 == 0:
                cv.put(c + sway, r, 'U' if c % 2 else 'u', p.col if c % 3 else p.hi)
            else:
                cv.put(c, r, '|' if c % 2 else '/', GREEN2)
    W, WL = RGB(45, 30, 45), RGB(90, 60, 70)           # windmill
    for r in range(8, H + 3):
        w = 1 + (r - 8) // 3
        for o in range(-w, w + 1):
            cv.set_bg(16 + o, r, W)
            cv.put(16 + o, r, '/' if o == -w else '\\' if o == w else ' ', WL)
    cv.put(16, H + 1, '#', RGB(255, 200, 120))
    cv.put(16, 12, 'o', RGB(255, 200, 120))
    cv.text(14, 7, "/^\\", WL)
    rot = progress(f, 1) * TAU / 4
    for k in range(4):
        a = rot + k * TAU / 4
        dx, dy = math.cos(a), math.sin(a)
        px, py = -dy, dx

        def P(along, side):
            return 16 + (dx * along + px * side) / CELL_W, 8 + (dy * along + py * side) / CELL_H

        cv.line(*P(0, 0), *P(38, 0), RGB(60, 40, 50))
        for side in (3, 6, 9):
            cv.line(*P(12, side), *P(38, side), RGB(150, 120, 120), 0 if side == 9 else '#')
    cv.put(16, 8, '@', RGB(200, 170, 150))
    cv.text(52, 1, "~ tulip fields forever ~", RGB(255, 220, 230))


def scene_rose(cv, f):
    cv.clear(RGB(40, 18, 34))
    sw = wave(f, 1) * 0.6
    cv.curve(40 + sw, 20, 38, 30, 40, 39, GREEN2)
    for r in range(23, 39, 4):
        cv.put(41 if r % 8 else 37, r, '>' if r % 8 else '<', GREEN3)
    leaf(cv, 39, 28, 8, -1, LEAF)
    leaf(cv, 40, 33, 8, +1, LEAF)
    cv.text(36, 20, "\\v^v/", GREEN1)
    rose(cv, 40 + sw, 12, 8, RED)
    falling_petals(cv, f, 4, 9, [RED.col, RED.hi, RED.dark], "oc0", 4)
    sparkles(cv, f, 31, 45, RGB(255, 200, 210))
    cv.text(54, 37, "a rose,", PINK.hi)
    cv.text(54, 38, "for my favorite", PINK.hi)
    cv.text(54, 39, "person", PINK.hi)


def scene_sunflowers(cv, f):
    cv.clear(0)
    cv.gradient(RGB(60, 130, 215), RGB(175, 215, 245), 0, 32)
    cv.gradient(RGB(70, 150, 60), RGB(35, 95, 35), 32, ROWS)
    for i in range(3):                                 # clouds
        x = math.fmod(progress(f, 1, i * 0.33) * 110, 110) - 15
        y = 3 + i * 4
        cv.ellipse_bg(x, y, 6, 1.6, RGB(245, 248, 255))
        cv.ellipse_bg(x + 4, y - 0.8, 4, 1.3, RGB(245, 248, 255))
    for r in range(32, ROWS):                          # grass
        for c in range(COLS):
            if hash32(c * 13 + r * 71) % 3:
                continue
            w = wave(f, 2, c * 0.02 - r * 0.015)
            cv.put(c, r, '/' if w > 0.4 else '\\' if w < -0.4 else '|', GREEN3 if r % 2 else GREEN1)
    for i, (fx, fy, fr) in enumerate([(18, 13, 5), (41, 9, 6.5), (63, 15, 4.5)]):
        x = fx + wave(f, 1, i * 0.2) * 0.8
        cv.curve(x, fy + 2, x - 1, 28, fx, ROWS, GREEN2)
        leaf(cv, x - 0.5, 26 - i, 4, -1, LEAF)
        leaf(cv, x + 0.2, 30 - i, 4, +1, LEAF)
        sunflower(cv, x, fy, fr, progress(f, 1) * TAU / 16 * (-1 if i % 2 else 1))
    t = progress(f, 1) * TAU                           # bee
    bx, by = 41 + math.sin(t) * 26, 20 + math.sin(2 * t) * 5
    cv.putf(bx, by, '8', RGB(250, 210, 30))
    cv.putf(bx + (-1 if math.cos(t) > 0 else 1), by, 'o', RGB(30, 30, 30))
    cv.putf(bx, by - 1, '^' if f % 4 < 2 else '-', RGB(255, 255, 255))
    cv.text(2, 1, "you are my sunshine", RGB(255, 250, 200))


def scene_cherry_blossom(cv, f):
    cv.clear(0)
    cv.gradient(RGB(25, 25, 60), RGB(80, 55, 100))
    cv.ellipse_bg(66, 9, 7, 4.6, RGB(70, 65, 110))
    cv.ellipse_bg(66, 9, 5, 3.3, RGB(230, 225, 200))
    cv.ellipse(66, 9, 3.5, 2.2, " .  o", RGB(200, 195, 175))
    sparkles(cv, f, 61, 35, RGB(230, 230, 255), 0, 25)
    B, BL = RGB(85, 55, 50), RGB(130, 90, 75)
    cv.curve_bg(-2, 6, 20, 8, 46, 17, 2.2, 0.8, B)
    cv.curve_bg(18, 9, 25, 2, 34, 1, 1.0, 0.5, B)
    cv.curve_bg(30, 13, 38, 7, 50, 6, 1.0, 0.5, B)
    cv.curve_bg(42, 16, 52, 21, 60, 27, 0.9, 0.5, B)
    cv.curve_bg(8, 8, 10, 14, 16, 18, 0.9, 0.5, B)
    cv.curve(-2, 6, 20, 8, 46, 17, BL, '=')
    blossoms = [(6, 5), (12, 7), (17, 5), (22, 3), (27, 2), (33, 1), (24, 10), (30, 9), (36, 7),
                (42, 6), (48, 5), (39, 13), (45, 15), (50, 19), (55, 22), (59, 26), (11, 13),
                (15, 17), (35, 12), (44, 9), (20, 8), (53, 24), (9, 10)]
    for i, (bx, y) in enumerate(blossoms):
        x = bx + wave(f, 1, i * 0.05) * 0.5
        petal = PINK.hi if i % 3 else PINK.col
        cv.putf(x - 1, y, 'o', petal); cv.putf(x + 1, y, 'o', petal)
        cv.putf(x, y - 1, 'o', petal); cv.putf(x, y + 1, 'o', petal)
        cv.putf(x, y, '*', RGB(255, 240, 120))
    falling_petals(cv, f, 6, 22, [PINK.hi, PINK.col, RGB(255, 230, 240)], "o.'", 10)
    cv.text(4, 37, "softly, like spring", PINK.hi)


def scene_lavender(cv, f):
    H = 15
    cv.clear(0)
    cv.gradient(RGB(60, 40, 110), RGB(245, 155, 115), 0, H)
    cv.ellipse_bg(40, H, 9, 5.5, RGB(255, 185, 120))
    cv.ellipse_bg(40, H, 6, 3.8, RGB(255, 225, 150))
    cv.gradient(RGB(85, 60, 120), RGB(25, 35, 35), H, ROWS)
    for c in range(COLS):
        hill = H - 1 + rnd(math.sin(c * 0.13) * 0.8 + math.sin(c * 0.05 + 1) * 0.8)
        for r in range(hill, H + 1):
            cv.set_bg(c, r, RGB(95, 65, 125))
    sparkles(cv, f, 71, 18, RGB(255, 240, 255), 0, 7, 0, COLS, False)
    rows_y = [17, 18.5, 20.5, 23, 26, 29.5, 33.5, 38.5]
    for row, y in enumerate(rows_y):
        length = 1 + row
        spacing = 1.5 + row * 0.55
        mound = mix(RGB(90, 60, 130), RGB(55, 35, 85), row / 7.0)
        start = math.fmod(row * 0.9, spacing)
        j = 0
        while start + j * spacing < COLS + 1:
            x = start + j * spacing + (hash32(j + row * 1000) % 100) / 100.0 * spacing * 0.4
            lean = wave(f, 1, x * 0.012 + row * 0.05) * (0.3 + row * 0.22)
            if row >= 3:
                cv.line(x, y + 1, x, y + 1 + row * 0.5, GREEN2, '|')
            for k in range(length):
                bx = x + lean * k / length
                alt = (k + row) % 2
                c, r = rnd(bx), rnd(y - k)
                if row >= 2:
                    cv.set_bg(c, r, mound)
                cv.put(c, r, 'o' if alt else '0', PURPLE.col if alt else PURPLE.hi)
                if row >= 5 and alt:
                    cv.put(c - 1, r, '*', PURPLE.col)
                    cv.put(c + 1, r, '*', PURPLE.col)
            j += 1
    t = progress(f, 1) * TAU                           # butterfly
    bx, by = 40 + math.cos(t) * 24, 10 + math.sin(t * 2) * 3
    wings_open = f % 4 < 2
    cv.putf(bx - 1, by, '}' if wings_open else '>', YELLOW.hi)
    cv.putf(bx + 1, by, '{' if wings_open else '<', YELLOW.hi)
    cv.putf(bx, by, ':', RGB(60, 40, 40))
    cv.text(2, 1, "calm like lavender", RGB(255, 225, 245))


def scene_daisy_vase(cv, f):
    cv.clear(RGB(58, 52, 78))
    for r in range(1, 33, 3):                          # wallpaper
        for c in range((r // 3) % 2 * 3, COLS, 6):
            cv.put(c, r, '.', RGB(85, 78, 108))
    cv.fill_bg(0, 34, COLS - 1, ROWS - 1, RGB(105, 70, 45))   # table
    for c in range(COLS):
        cv.put(c, 34, '=', RGB(140, 100, 65))
        if hash32(c) % 4 == 0:
            cv.put(c, 36 + hash32(c) % 3, '-', RGB(130, 90, 60))
    V, VD, VL = RGB(70, 120, 200), RGB(40, 75, 150), RGB(150, 190, 245)
    for r in range(23, 35):                            # vase
        t = (r - 23) / 11.0
        w = 5 if r == 23 else 3.5 if r == 24 else 3.5 + math.sin(t * 3.1) * 4.5
        iw = rnd(w)
        for o in range(-iw, iw + 1):
            cv.set_bg(40 + o, r, V)
            ch = '(' if o == -iw else ')' if o == iw else ('~' if (o + r) % 4 == 0 else ' ')
            cv.put(40 + o, r, ch, VL if o == -iw else VD if o == iw else VL)
    cv.put(37, 28, "'", RGB(255, 255, 255)); cv.put(37, 29, "'", RGB(255, 255, 255))
    ds = [(40, 6, 4.5, True), (27, 9, 4, True), (53, 9, 4, True), (18, 16, 3.2, True),
          (62, 16, 3.2, True), (33, 15, 3, False), (47, 15, 3, False), (40, 14, 2.4, True)]
    for i, (dx, dy, dr, white) in enumerate(ds):
        x = dx + wave(f, 1, i * 0.11) * 0.7
        cv.curve(40 + (i % 3 - 1), 23, 40 + (x - 40) * 0.3, dy + 8, x, dy + 1, GREEN1, 0, 8)
    leaf(cv, 38, 21, 5, -1, LEAF)
    leaf(cv, 42, 20, 5, +1, LEAF)
    for i, (dx, dy, dr, white) in enumerate(ds):
        x = dx + wave(f, 1, i * 0.11) * 0.7
        daisy(cv, x, dy, dr, WHITE if white else YELLOW, YELLOW.col if white else ORANGE.col,
              0.0 if i % 2 else TAU / 24)
    sparkles(cv, f, 81, 25, RGB(255, 250, 220), 0, 33)
    cv.text(60, 38, "fresh daisies", RGB(255, 230, 190))


def scene_moon_garden(cv, f):
    cv.clear(0)
    cv.gradient(RGB(10, 14, 36), RGB(28, 40, 80))
    cv.ellipse_bg(62, 9, 8, 5.3, RGB(24, 30, 62))      # crescent moon
    cv.ellipse(62, 9, 5.5, 3.7, "@0", RGB(255, 245, 190))
    for r in range(4, 15):
        for c in range(58, 71):
            u, v = (c - 65.5) / 4.6, (r - 7.6) / 3.2
            if u * u + v * v <= 1:
                cv.put(c, r, ' ', 0)
    sparkles(cv, f, 91, 90, RGB(220, 230, 255), 0, 28)
    for c in range(COLS):                              # ground
        top = 34 + rnd(math.sin(c * 0.15) * 1.2)
        for r in range(top, ROWS):
            cv.set_bg(c, r, RGB(10, 25, 20))
            if r == top:
                cv.put(c, r, '^' if c % 3 else 'v', RGB(30, 70, 45))
    glowing = Petal(RGB(220, 235, 255), RGB(120, 140, 200), RGB(255, 255, 255))
    ms = [(12, 22, 3), (24, 18, 3.6), (37, 24, 2.6), (50, 20, 3.4), (66, 25, 3), (6, 29, 2), (74, 30, 2)]
    for i, (mx, my, mr) in enumerate(ms):
        x = mx + wave(f, 1, i * 0.17) * 0.6
        cv.curve(x, my + 1, x + 2, 30, mx, 35, RGB(40, 110, 80))
        glow = 0.5 + 0.5 * wave(f, 2, i * 0.13)
        cv.ellipse_bg(x, my, mr + 2, (mr + 2) * 0.67, mix(RGB(20, 30, 65), RGB(55, 70, 120), glow))
        daisy(cv, x, my, mr, glowing, RGB(200, 255, 230), 0)
    rng = Rng(17)                                      # fireflies
    for i in range(14):
        cx = rng.range(4, 76)
        cy = rng.range(14, 33)
        ph = rng.f()
        x = cx + wave(f, 1, ph) * 4
        y = cy + wave(f, 2, ph + 0.25) * 1.5
        p = (f + i * 7) % 24
        if p < 16:
            edge = p < 4 or p > 12
            cv.putf(x, y, '.' if edge else '*', RGB(150, 200, 80) if edge else RGB(230, 255, 120))
    cv.text(3, 2, "goodnight, sweet dreams", RGB(200, 210, 255))


def scene_forget_me_not(cv, f):
    cv.clear(RGB(255, 236, 240))
    beat = 1 + 0.07 * max(0.0, wave(f, 4)) + 0.03 * max(0.0, wave(f, 4, 0.15))
    blues = [RGB(70, 120, 230), RGB(110, 160, 245), RGB(60, 95, 200), RGB(140, 180, 250)]
    for r in range(ROWS):
        for c in range(COLS):
            u = (c - 40) / (17.0 * beat)
            v = -(r - 18) / (14.5 * beat)
            a = u * u + v * v - 1
            if a * a * a - u * u * v * v * v > 0:
                continue
            h = hash32(c * 131 + r * 7)
            if (c + r) % 3 == 0 and h % 3 == 0:
                cv.put(c, r, 'o', RGB(250, 205, 60))
            else:
                cv.put(c, r, '@' if h % 5 == 0 else '*' if h % 5 == 1 else 'o', blues[h % 4])
    floating_hearts(cv, f, 12, 6, RGB(240, 120, 150))
    sparkles(cv, f, 101, 30, RGB(240, 150, 180), 0, 34, 0, COLS, False)
    cv.text_centered(37, "forget me not", RGB(60, 95, 200))


def scene_water_lilies(cv, f):
    cv.clear(0)
    cv.gradient(RGB(12, 50, 62), RGB(22, 85, 92))
    for i in range(3):                                 # koi
        p = progress(f, 1, i * 0.37)
        right = i % 2 == 0
        x = p * 100 - 10 if right else 90 - p * 100
        y = 8 + i * 11 + wave(f, 2, i * 0.3) * 1.5
        col = WHITE.col if i == 1 else ORANGE.col
        if right:
            cv.putf(x - 2, y, '>', col); cv.putf(x - 1, y, '<', col); cv.putf(x, y, '>', col)
        else:
            cv.putf(x, y, '<', col); cv.putf(x + 1, y, '>', col); cv.putf(x + 2, y, '<', col)
    for i, (rx, ry) in enumerate([(20, 12), (58, 26), (44, 6)]):   # ripples
        p = progress(f, 1, i * 0.33)
        cv.ring(rx, ry, 1 + p * 12, (1 + p * 12) * 0.6, mix(RGB(150, 210, 215), RGB(22, 80, 90), p))
    for i in range(40):                                # shimmer
        c, r = hash32(i * 3) % COLS, hash32(i * 5 + 1) % ROWS
        if cv.at(c, r) == 32 and (f // 6 + i) % 4 == 0:
            cv.put(c, r, '~', RGB(70, 140, 150))
    pads = [(14, 26, 6, 1), (32, 31, 5, 0), (50, 14, 6.5, 2), (68, 30, 5.5, 1),
            (66, 8, 4, 0), (28, 9, 4.5, 2), (6, 13, 3.5, 0)]
    for i, (x, py, pr, flower) in enumerate(pads):
        y = py + wave(f, 1, i * 0.21) * 0.4
        rx, ry = pr, pr * 0.55
        for r in range(math.floor(y - ry), math.ceil(y + ry) + 1):
            for c in range(math.floor(x - rx), math.ceil(x + rx) + 1):
                u, v = (c - x) / rx, (r - y) / ry
                if u * u + v * v > 1:
                    continue
                if u > 0.1 and abs(v) < u * 0.45:     # the notch
                    continue
                cv.set_bg(c, r, RGB(40, 120, 55))
                cv.put(c, r, '-' if hash32(c * 7 + r * 13) % 4 == 0 else ' ', RGB(75, 160, 80))
        if flower:
            p = PINK if flower == 1 else WHITE
            fx, fy = x - rx * 0.3, y - 0.5
            cv.putf(fx - 1, fy - 1, '\\', p.col); cv.putf(fx, fy - 1, '|', p.hi); cv.putf(fx + 1, fy - 1, '/', p.col)
            cv.putf(fx - 2, fy, '<', p.col); cv.putf(fx - 1, fy, '(', p.hi)
            cv.putf(fx, fy, '@', YELLOW.col)
            cv.putf(fx + 1, fy, ')', p.hi); cv.putf(fx + 2, fy, '>', p.col)
    cv.text(3, 38, "still waters", RGB(170, 220, 220))


def scene_poppies(cv, f):
    H = 14
    cv.clear(0)
    cv.gradient(RGB(110, 175, 240), RGB(215, 232, 250), 0, H)
    cv.gradient(RGB(95, 160, 70), RGB(40, 100, 40), H, ROWS)
    for c in range(COLS):                              # hills
        hill = H - 2 + rnd(math.sin(c * 0.09 + 2) * 1.6)
        for r in range(hill, H):
            cv.set_bg(c, r, RGB(120, 170, 110))
    for r in range(H, ROWS):                           # grass
        for c in range(COLS):
            if hash32(c * 17 + r * 53) % 3:
                continue
            w = wave(f, 2, c * 0.025 - r * 0.02)
            cv.put(c, r, '/' if w > 0.45 else '\\' if w < -0.45 else '|', GREEN3 if r % 2 else GREEN1)
    rng = Rng(42)
    for i in range(90):                                # poppies
        y = rng.range(H + 1, ROWS - 2)
        x = rng.range(0, COLS)
        depth = (y - H) / (ROWS - H)
        x += wave(f, 2, x * 0.025 - y * 0.02) * depth * 0.8
        p = ORANGE if i % 9 == 0 else RED
        if depth < 0.25:
            cv.putf(x, y, 'o', p.col)
        elif depth < 0.55:
            cv.putf(x - 0.5, y, '@', p.col)
            cv.putf(x + 0.5, y, '@', p.col)
        else:
            rx = 1.2 + depth * 1.8
            cv.line(x, y + 1, x, y + 4, GREEN2, '|')
            cv.ellipse(x, y, rx, rx * 0.6, "@O", p.col)
            cv.putf(x - rx, y, '(', p.dark)
            cv.putf(x + rx, y, ')', p.dark)
            cv.putf(x, y, '#', RGB(30, 20, 30))
    for i in range(2):                                 # butterflies
        t = progress(f, 1, i * 0.5) * TAU
        bx, by = 40 + math.sin(t) * 30, 10 + i * 6 + math.sin(t * 3) * 2
        wings_open = ((f + i * 2) % 4) < 2
        col = BLUE.hi if i else ORANGE.col
        cv.putf(bx - 1, by, '}' if wings_open else '>', col)
        cv.putf(bx + 1, by, '{' if wings_open else '<', col)
        cv.putf(bx, by, ':', RGB(40, 30, 30))
    cv.text(2, 1, "wild & free", RGB(40, 60, 110))


SCENES = [
    ("Wildflower Bouquet", scene_wildflowers),
    ("Tulip Bouquet", scene_tulip_bouquet),
    ("Tulip Field", scene_tulip_field),
    ("Single Rose", scene_rose),
    ("Sunflowers", scene_sunflowers),
    ("Cherry Blossom", scene_cherry_blossom),
    ("Lavender Field", scene_lavender),
    ("Daisies in a Vase", scene_daisy_vase),
    ("Moon Garden", scene_moon_garden),
    ("Forget-Me-Not", scene_forget_me_not),
    ("Water Lilies", scene_water_lilies),
    ("Poppy Meadow", scene_poppies),
]


def render(index, frame=0):
    """Render scene `index` (0-based) at `frame` -> 240x320x3 uint8 image."""
    cv = Canvas()
    SCENES[index][1](cv, frame % LOOP)
    return cv.to_image()


# ==================================================================== main ==
def show_still(indices, frame):
    here = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else os.getcwd()
    images = [render(i, frame) for i in indices]

    if len(indices) > 1:                               # contact sheet of all scenes
        cols = 4
        rows = math.ceil(len(indices) / cols)
        fig, axes = plt.subplots(rows, cols, figsize=(16, 3.4 * rows))
        for ax in np.ravel(axes):
            ax.axis("off")
        for ax, i, img in zip(np.ravel(axes), indices, images):
            ax.imshow(img, interpolation="nearest")
            ax.set_title(f"{i + 1}. {SCENES[i][0]}", fontsize=11)
        fig.suptitle(f"ESP32 Flower Display - frame {frame}", fontsize=14)
        fig.tight_layout()
        plt.show()

    if SHOW_EACH or len(indices) == 1:                 # each at real size (2x zoom)
        for i, img in zip(indices, images):
            fig = plt.figure(figsize=(6.4, 4.8), dpi=100)
            ax = fig.add_axes([0, 0, 1, 1])
            ax.imshow(img, interpolation="nearest")
            ax.axis("off")
            ax.set_title(f"{i + 1}. {SCENES[i][0]}", y=0.96, color="white", fontsize=10)
            plt.show()

    if SAVE_PNG:
        for i, img in zip(indices, images):
            path = os.path.join(here, f"scene_{i + 1:02d}.png")
            plt.imsave(path, img)
            print("saved", path)


def animate(start_index):
    """Pop-up window that plays the animation. SPACE / click = next scene."""
    try:  # Spyder's Plots pane can't animate: switch to a separate window
        get_ipython().run_line_magic("matplotlib", "qt")   # noqa: F821
    except Exception:
        pass
    from matplotlib.animation import FuncAnimation

    state = {"scene": start_index, "frame": 0}
    fig = plt.figure(figsize=(6.4, 5.0))
    ax = fig.add_axes([0, 0, 1, 0.94])
    ax.axis("off")
    im = ax.imshow(render(start_index, 0), interpolation="nearest")
    title = fig.suptitle("")

    def next_scene(_event=None):
        state["scene"] = (state["scene"] + 1) % len(SCENES)
        state["frame"] = 0

    def on_key(event):
        if event.key in (" ", "right"):
            next_scene()
        elif event.key == "left":
            state["scene"] = (state["scene"] - 1) % len(SCENES)
            state["frame"] = 0

    def step(_):
        im.set_data(render(state["scene"], state["frame"]))
        s = state["scene"]
        title.set_text(f"{s + 1}/{len(SCENES)}  {SCENES[s][0]}   (space / click = button)")
        state["frame"] += 1
        return (im,)

    fig.canvas.mpl_connect("key_press_event", on_key)
    fig.canvas.mpl_connect("button_press_event", next_scene)
    fig._flower_anim = FuncAnimation(fig, step, interval=80, cache_frame_data=False)  # keep a reference
    plt.show()
    return fig


if __name__ == "__main__":
    if SCENE == "all":
        chosen = list(range(len(SCENES)))
    else:
        chosen = [int(SCENE) - 1]
    if ANIMATE:
        animate(chosen[0])
    else:
        show_still(chosen, FRAME)
