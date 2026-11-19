// ============================================================================
//  FlowerDisplay  -  animated ASCII-art flowers on a 2.0" ST7789V (320x240)
//
//  Libraries (Arduino IDE -> Tools -> Manage Libraries):
//     * Adafruit ST7735 and ST7789 Library   (installs Adafruit GFX too)
//  Board: any ESP32 (Tools -> Board -> esp32 -> "ESP32 Dev Module" etc.)
//
//  Wiring + options: config.h      Graphics: scenes.h      Engine: art.h
//  Preview the graphics on your computer first: see dev/ (README.md).
// ============================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>

#include "config.h"
#include "scenes.h"

using namespace art;

Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

static Canvas cur, prev;                  // what we're drawing / what's on screen
static uint16_t rowBuf[SCREEN_W * CELL_H]; // one text row of pixels (3.8 KB)

int scene = 0;
uint32_t frame = 0;
bool slideshow = START_IN_SLIDESHOW;
bool needFullRedraw = true;
uint32_t lastFrameMs = 0, lastSlideMs = 0;

// ------------------------------------------------------------ drawing ----
static inline void fixColours(uint16_t* p, int n) {
#if TFT_SWAP_RB
  for (int i = 0; i < n; i++) {
    uint16_t c = p[i];
    p[i] = (uint16_t)(((c & 0x1F) << 11) | (c & 0x07E0) | (c >> 11));
  }
#else
  (void)p; (void)n;
#endif
}

void pushFull() {
  for (int r = 0; r < ROWS; r++) {
    renderRow(cur, r, rowBuf);
    fixColours(rowBuf, SCREEN_W * CELL_H);
    tft.drawRGBBitmap(0, r * CELL_H, rowBuf, SCREEN_W, CELL_H);
  }
}

// only send the cells that changed since last frame (in horizontal runs)
void pushChanges() {
  for (int r = 0; r < ROWS; r++) {
    int c = 0;
    while (c < COLS) {
      if (cur.cell[r][c] == prev.cell[r][c]) { c++; continue; }
      int start = c;
      while (c < COLS && cur.cell[r][c] != prev.cell[r][c]) c++;
      int w = (c - start) * CELL_W;
      for (int k = start; k < c; k++) renderCell(cur.cell[r][k], rowBuf + (k - start) * CELL_W, w);
      fixColours(rowBuf, w * CELL_H);
      tft.drawRGBBitmap(start * CELL_W, r * CELL_H, rowBuf, w, CELL_H);
    }
  }
}

void drawFrame() {
  scenes::SCENES[scene].draw(cur, frame);
  if (needFullRedraw) { pushFull(); needFullRedraw = false; }
  else pushChanges();
  memcpy(&prev, &cur, sizeof(Canvas));
}

void showScene(int s) {
  scene = (s % scenes::SCENE_COUNT + scenes::SCENE_COUNT) % scenes::SCENE_COUNT;
  frame = 0;
  needFullRedraw = true;
  lastSlideMs = millis();
  prefs.putInt("scene", scene);
  Serial.printf("Scene %d/%d: %s\n", scene + 1, scenes::SCENE_COUNT, scenes::SCENES[scene].name);
}

// tiny on-screen notice (e.g. "slideshow on") for one second
void flashMessage(const char* msg) {
  scenes::SCENES[scene].draw(cur, frame);
  int len = (int)strlen(msg) + 4;
  int c0 = (COLS - len) / 2;
  for (int c = c0; c < c0 + len; c++) { cur.setBg(c, 18, RGB(0, 0, 0)); cur.put(c, 18, ' ', 0); }
  cur.textCentered(18, msg, RGB(255, 255, 255));
  pushChanges();
  memcpy(&prev, &cur, sizeof(Canvas));
  delay(1000);
}

// ------------------------------------------------------------- button ----
enum BtnEvent { NONE, SHORT_PRESS, LONG_PRESS };

BtnEvent readButton() {
  static bool stable = HIGH, lastRead = HIGH, longFired = false;
  static uint32_t changedAt = 0, downAt = 0;
  bool now = digitalRead(BUTTON_PIN);
  uint32_t t = millis();
  if (now != lastRead) { lastRead = now; changedAt = t; }
  if (t - changedAt > 25 && now != stable) {          // debounced edge
    stable = now;
    if (stable == LOW) { downAt = t; longFired = false; }
    else if (!longFired) return SHORT_PRESS;          // released before long press
  }
  if (stable == LOW && !longFired && t - downAt >= LONG_PRESS_MS) {
    longFired = true;
    return LONG_PRESS;
  }
  return NONE;
}

// --------------------------------------------------------------- main ----
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#if TFT_BL >= 0
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setSPISpeed(TFT_SPI_HZ);
  tft.setRotation(TFT_ROTATION);
  tft.invertDisplay(TFT_INVERT);
  tft.fillScreen(0);

  prefs.begin("flowers", false);
  showScene(prefs.getInt("scene", 0));   // remembers the last picture after power-off
}

void loop() {
  switch (readButton()) {
    case SHORT_PRESS: showScene(scene + 1); break;
    case LONG_PRESS:
      slideshow = !slideshow;
      flashMessage(slideshow ? "slideshow on" : "slideshow off");
      lastSlideMs = millis();
      break;
    default: break;
  }
  if (slideshow && millis() - lastSlideMs >= SLIDESHOW_MS) showScene(scene + 1);

  if (millis() - lastFrameMs >= FRAME_MS) {
    lastFrameMs = millis();
    drawFrame();
    frame++;
  }
}
