// ============================================================================
//  config.h  -  wiring + settings. Change pins here once you know your board.
// ============================================================================
#pragma once

// ---- Display: AITRIP 2.0" ST7789V 240x320 SPI module -----------------------
// Module pin   ->  ESP32 GPIO (defaults for a classic ESP32 DevKit / WROOM-32)
//   GND        ->  GND
//   VCC        ->  3V3
//   SCL (SCK)  ->  18
//   SDA (MOSI) ->  23
//   RES (RST)  ->  4
//   DC         ->  2
//   CS         ->  5
//   BL (BLK)   ->  32   (or tie to 3V3 and set TFT_BL to -1)
#define TFT_SCK   18
#define TFT_MOSI  23
#define TFT_RST    4
#define TFT_DC     2
#define TFT_CS     5
#define TFT_BL    32

#define TFT_ROTATION   1          // 1 = landscape, 3 = landscape upside-down
#define TFT_SPI_HZ     40000000   // drop to 27000000 if you see glitches
#define TFT_INVERT     1          // most of these modules need inversion; set 0 if colours look like a negative
#define TFT_SWAP_RB    0          // set 1 if red and blue are swapped

// ---- Button -----------------------------------------------------------------
// Wire a small push button between this GPIO and GND (internal pull-up used).
// GPIO 0 is the on-board "BOOT" button, so it works with no wiring at all.
#define BUTTON_PIN     0
#define LONG_PRESS_MS  700        // hold the button this long to toggle slideshow

//  short press  -> next picture
//  long press   -> slideshow on/off (auto-advance every SLIDESHOW_MS)
#define SLIDESHOW_MS   20000
#define START_IN_SLIDESHOW 0

// ---- Animation --------------------------------------------------------------
#define FRAME_MS       80         // ~12 fps
