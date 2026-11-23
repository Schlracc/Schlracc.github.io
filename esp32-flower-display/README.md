# ESP32 Flower Display 🌷

Animated ASCII-art flowers (like the "wild flowers made by hand" bouquet) for the
**AITRIP 2.0" ST7789V 240x320 TFT**, drawn in **landscape (320x240)**.
Press the button to switch pictures.

| # | Scene | # | Scene |
|---|-------|---|-------|
| 1 | Wildflower Bouquet | 7 | Lavender Field |
| 2 | **Tulip Bouquet** 🌷 | 8 | Daisies in a Vase |
| 3 | **Tulip Field** (windmill) 🌷 | 9 | Moon Garden |
| 4 | Single Rose | 10 | Forget-Me-Not heart |
| 5 | Sunflowers | 11 | Water Lilies |
| 6 | Cherry Blossom | 12 | Poppy Meadow |

All of them animate: things sway, sparkle, twinkle, fall, fly or swim.

```
FlowerDisplay/          <- the Arduino sketch (open FlowerDisplay.ino)
  config.h              <- PINS + settings  (edit this first)
  scenes.h              <- the 12 graphics
  art.h, font3x5.h      <- tiny drawing engine + 3x5 pixel font
dev/                    <- DEV MODE: preview the graphics on your computer
tools/gen_font.py       <- regenerates font3x5.h if you edit glyphs
```

## 1. Dev mode: preview on your computer

You don't need the ESP32 for this. It compiles the **same** `art.h`/`scenes.h` the board uses
and writes animated GIFs plus a viewer page with a fake button.

* **Windows:** double-click `dev\preview.bat` (needs `g++`, e.g. from MSYS2 or
  `winget install BrechtSanders.WinLibs.POSIX.UCRT`)
* **macOS / Linux:** `./dev/preview.sh`

Then open `dev/out/index.html`. Click the red button or press **Space** (or the arrow keys)
to change pictures, the same as the real button.
Options: `--scene 3` renders only scene 3, `--scale 3` makes the GIFs bigger.

### Python / Spyder version

`dev/flower_preview.py` is a Python copy of the same drawing code. It needs only numpy and
matplotlib, which Spyder already includes. Open it in Spyder and press **F5**: all 12 pictures
appear in the **Plots** pane. The settings at the top of the file control what you see:

```python
SCENE = "all"      # or 1-12 for just one picture
FRAME = 30         # animation frame 0-95
ANIMATE = False    # True = pop-up window that plays it; SPACE / click = next picture
SAVE_PNG = False   # True = save scene_XX.png files
```

It matches the C++ version pixel-for-pixel except for a few rounding cells.

## 2. Wiring (defaults, change in `config.h`)

| Display pin | ESP32 |
|---|---|
| GND | GND |
| VCC | 3V3 |
| SCL | GPIO 18 |
| SDA | GPIO 23 |
| RES | GPIO 4 |
| DC  | GPIO 2 |
| CS  | GPIO 5 |
| BL  | GPIO 32 (or straight to 3V3, then set `TFT_BL -1`) |
| **Button** | between `BUTTON_PIN` and GND. The default is GPIO 0, which is the board's own **BOOT** button, so it works without wiring anything. |

Any ESP32 (classic, S2, S3, C3) works. Just put your board's GPIO numbers in `config.h`.

## 3. Upload

1. Arduino IDE → *Boards Manager* → install **esp32 by Espressif**.
2. *Library Manager* → install **Adafruit ST7735 and ST7789 Library** (say yes to Adafruit GFX).
3. Open `FlowerDisplay/FlowerDisplay.ino`, pick your board + port, Upload.

**Button:** a short press goes to the next picture. Holding it for about 0.7 s turns the
slideshow on or off (it changes picture every 20 s).
The board remembers the last picture after it loses power.

## Troubleshooting

| Problem | Fix in `config.h` |
|---|---|
| Colours look like a photo negative | `TFT_INVERT 0` |
| Red and blue are swapped | `TFT_SWAP_RB 1` |
| Picture is upside-down | `TFT_ROTATION 3` |
| Screen stays black | Check BL wiring / `TFT_BL` pin, VCC is 3V3 |
| Random glitches or noise | `TFT_SPI_HZ 27000000`, use shorter wires |

## Making your own picture

The screen is an 80 x 40 grid of character cells, each 4x6 pixels. In `scenes.h`, write
`static void sceneMine(Canvas& cv, uint32_t f)`, add it to `SCENES[]`, and run dev mode.
The helpers you'll use most:
`cv.text()`, `cv.line()`, `cv.curve()`, `cv.ellipse()`, `cv.gradient()`, `tulip()`,
`daisy()`, `rose()`, `sparkles()`, `floatingHearts()`.
To make an animation loop, use `wave(f, cycles)` / `progress(f, cycles)`.
