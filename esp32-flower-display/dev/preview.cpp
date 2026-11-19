// ============================================================================
//  DEV MODE  -  preview the ESP32 graphics on your computer.
//
//  It compiles the exact same art.h / scenes.h the ESP32 uses and writes:
//     dev/out/scene_XX.gif   animated preview of every scene (2x size)
//     dev/out/index.html     viewer: click the fake button / press SPACE
//
//  Build + run (needs any C++ compiler, no libraries):
//     Windows:  dev\preview.bat          macOS/Linux:  ./dev/preview.sh
//  or manually:
//     g++ -O2 -std=c++17 dev/preview.cpp -o dev/preview && ./dev/preview
//
//  Options:  preview [--scale N] [--scene N] [--out DIR]
// ============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0755)
#endif

#include "../FlowerDisplay/config.h"
#include "../FlowerDisplay/scenes.h"

using namespace art;

// ----------------------------------------------------------- GIF writer ---
struct GifWriter {
  std::vector<uint8_t> out;
  void b(uint8_t v) { out.push_back(v); }
  void w16(int v) { b(v & 255); b((v >> 8) & 255); }
  void str(const char* s) { while (*s) b((uint8_t)*s++); }

  void header(int w, int h, const std::vector<uint32_t>& pal) {
    str("GIF89a");
    w16(w); w16(h);
    b(0xF7);  // global colour table, 256 entries
    b(0); b(0);
    for (int i = 0; i < 256; i++) {
      uint32_t c = i < (int)pal.size() ? pal[i] : 0;
      b((c >> 16) & 255); b((c >> 8) & 255); b(c & 255);
    }
    // loop forever
    b(0x21); b(0xFF); b(11); str("NETSCAPE2.0"); b(3); b(1); w16(0); b(0);
  }
  void frame(int w, int h, const std::vector<uint8_t>& idx, int delayCs) {
    b(0x21); b(0xF9); b(4); b(0x04); w16(delayCs); b(0); b(0);  // graphic control
    b(0x2C); w16(0); w16(0); w16(w); w16(h); b(0);              // image descriptor
    lzw(idx);
  }
  void end() { b(0x3B); }

  // LZW, min code size 8
  void lzw(const std::vector<uint8_t>& data) {
    const int minCode = 8, clear = 256, eoi = 257;
    b(minCode);
    std::vector<uint8_t> blk;
    uint32_t bitBuf = 0; int bitCnt = 0;
    auto emit = [&](int code, int size) {
      bitBuf |= (uint32_t)code << bitCnt; bitCnt += size;
      while (bitCnt >= 8) {
        blk.push_back(bitBuf & 255); bitBuf >>= 8; bitCnt -= 8;
        if (blk.size() == 255) { b(255); out.insert(out.end(), blk.begin(), blk.end()); blk.clear(); }
      }
    };
    std::unordered_map<uint32_t, int> dict;
    int codeSize = minCode + 1, next = eoi + 1;
    emit(clear, codeSize);
    int prefix = data[0];
    for (size_t i = 1; i < data.size(); i++) {
      uint32_t key = ((uint32_t)prefix << 8) | data[i];
      auto it = dict.find(key);
      if (it != dict.end()) { prefix = it->second; continue; }
      emit(prefix, codeSize);
      if (next < 4096) {
        dict[key] = next++;
        if (next > (1 << codeSize) && codeSize < 12) codeSize++;
      } else {
        emit(clear, codeSize);
        dict.clear(); codeSize = minCode + 1; next = eoi + 1;
      }
      prefix = data[i];
    }
    emit(prefix, codeSize);
    emit(eoi, codeSize);
    if (bitCnt > 0) emit(0, 8 - bitCnt);
    if (!blk.empty()) { b((uint8_t)blk.size()); out.insert(out.end(), blk.begin(), blk.end()); }
    b(0);
  }
};

static uint32_t to888(uint16_t c) {
  uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
  return ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
}

static void renderFrame(Canvas& cv, std::vector<uint16_t>& px) {
  px.resize(SCREEN_W * SCREEN_H);
  for (int r = 0; r < ROWS; r++) renderRow(cv, r, &px[r * CELL_H * SCREEN_W]);
}

int main(int argc, char** argv) {
  int scale = 2, only = -1;
  std::string outDir = "dev/out";
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--scale") && i + 1 < argc) scale = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--scene") && i + 1 < argc) only = atoi(argv[++i]) - 1;
    else if (!strcmp(argv[i], "--out") && i + 1 < argc) outDir = argv[++i];
    else { printf("usage: preview [--scale N] [--scene N] [--out DIR]\n"); return 1; }
  }
  if (scale < 1) scale = 1;
  MKDIR(outDir.c_str());
  static Canvas cv;
  std::vector<uint16_t> px;
  const int W = SCREEN_W * scale, H = SCREEN_H * scale;

  for (int s = 0; s < scenes::SCENE_COUNT; s++) {
    if (only >= 0 && s != only) continue;
    // render all frames, build palette
    std::vector<std::vector<uint16_t>> frames;
    std::map<uint16_t, int> colors;
    for (int f = 0; f < LOOP; f++) {
      scenes::SCENES[s].draw(cv, (uint32_t)f);
      renderFrame(cv, px);
      for (uint16_t c : px) colors[c] = 0;
      frames.push_back(px);
    }
    // reduce precision until it fits in 256 colours (rarely needed)
    const uint16_t masks[] = {0xFFFF, 0xF7DE, 0xE79C, 0xC718, 0x8610};
    uint16_t mask = 0xFFFF;
    for (uint16_t m : masks) {
      std::map<uint16_t, int> reduced;
      for (auto& kv : colors) reduced[kv.first & m] = 0;
      mask = m;
      if (reduced.size() <= 256) { colors = reduced; break; }
    }
    std::vector<uint32_t> pal;
    int n = 0;
    for (auto& kv : colors) { kv.second = n++; pal.push_back(to888(kv.first)); }

    GifWriter gif;
    gif.header(W, H, pal);
    std::vector<uint8_t> idx(W * H);
    for (auto& fr : frames) {
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          idx[y * W + x] = (uint8_t)colors[fr[(y / scale) * SCREEN_W + x / scale] & mask];
      gif.frame(W, H, idx, FRAME_MS / 10);
    }
    gif.end();
    char name[512];
    snprintf(name, sizeof name, "%s/scene_%02d.gif", outDir.c_str(), s + 1);
    FILE* fp = fopen(name, "wb");
    if (!fp) { printf("cannot write %s\n", name); return 1; }
    fwrite(gif.out.data(), 1, gif.out.size(), fp);
    fclose(fp);
    printf("  %2d. %-20s -> %s (%zu colours)\n", s + 1, scenes::SCENES[s].name, name, pal.size());
  }

  // viewer page
  std::string html = outDir + "/index.html";
  FILE* fp = fopen(html.c_str(), "w");
  if (!fp) return 1;
  fprintf(fp, "<!doctype html><html><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
              "<title>Flower Display Preview</title><style>"
              "body{margin:0;background:#15151c;color:#ddd;font:15px system-ui,sans-serif;text-align:center;padding:16px}"
              ".dev{display:inline-block;background:#1f4f9e;padding:18px 18px 10px;border-radius:10px;box-shadow:0 8px 30px #0008}"
              ".dev img{display:block;width:min(640px,86vw);image-rendering:pixelated;border:6px solid #111}"
              "button{margin-top:14px;width:46px;height:46px;border-radius:50%%;border:4px solid #333;background:#c33;cursor:pointer;box-shadow:0 3px #000}"
              "button:active{transform:translateY(2px);box-shadow:none}"
              ".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(240px,1fr));gap:12px;margin-top:28px}"
              ".grid img{width:100%%;image-rendering:pixelated;cursor:pointer;border:2px solid #333}.grid div{font-size:13px}"
              "</style></head><body><h2>Flower Display &mdash; dev preview</h2>"
              "<div class=dev><img id=v><button id=b title='button (or press space)'></button>"
              "<div id=n style='color:#fff;margin-top:6px'></div></div>"
              "<p style='color:#888'>Click the red button or press <b>Space</b> / arrow keys, same as the real button.</p><div class=grid>");
  for (int s = 0; s < scenes::SCENE_COUNT; s++)
    fprintf(fp, "<div><img src='scene_%02d.gif' onclick='go(%d)'><div>%d. %s</div></div>", s + 1, s, s + 1,
            scenes::SCENES[s].name);
  fprintf(fp, "</div><script>const N=[");
  for (int s = 0; s < scenes::SCENE_COUNT; s++) fprintf(fp, "%s'%s'", s ? "," : "", scenes::SCENES[s].name);
  fprintf(fp, "];let i=0;function go(k){i=(k+N.length)%%N.length;v.src='scene_'+String(i+1).padStart(2,'0')+'.gif';"
              "n.textContent=(i+1)+' / '+N.length+'  -  '+N[i];scrollTo(0,0)}"
              "b.onclick=()=>go(i+1);onkeydown=e=>{if(e.key==' '||e.key=='ArrowRight'){e.preventDefault();go(i+1)}"
              "if(e.key=='ArrowLeft')go(i-1)};go(0)</script></body></html>\n");
  fclose(fp);
  printf("\nOpen %s in your browser.\n", html.c_str());
  return 0;
}
