// The app's video on the Xbox 360 (hito 5): platform/android/video_gles.h's class Video on vulkan4xenon, so app.cpp
// draws the launcher, the menus and the game as on Linux. The 1280x720 swapchain of VideoVk (hito 3), and one
// pipeline that draws everything as batched quads, as video_gles.cpp's GlesCanvas does: the UI (solid, gradients,
// rounded rectangles by distance, glyphs from an R8 atlas, pictures) and the game picture, a picture of its own.
// What app.cpp asks for in a frame (upload, draw, the canvas's drawing) is recorded, and present() makes one command
// buffer of it: the pictures' copies first, then one render pass.
// Text: stb_truetype, from the fonts built into the .xex (Luckiest Guy for titles, DejaVu Sans and Sans Bold).
// The game picture has pipelines of its own, one for each variant of shaders/game.glsl (video_gles.cpp's kGameFS):
// its filters (Nearest, Sharp Bilinear, Bilinear, CRT, CRT with an aperture grille, LCD Grid), scanlines, curve,
// the colour settings (VideoParams) from a table worked out on the CPU, and Side Borders "Blurred Game" behind it.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "app/canvas.h"

struct VideoParams {
  int srcW = 256, srcH = 224;  // active source size (pixels)
  IRect dest{};                // destination rect in screen pixels
  int filter = 1;
  float scanlines = 0.45f;
  float curve = 0.0f;
  int colorProfile = 0;
  float brightness = 1, contrast = 1, saturation = 1, gamma = 1;
  int colorblind = 0;
  float temperature = 0;  // -1 cool .. +1 warm
  int pillarbox = 1;      // 0 black, 1 blurred, 2 dark frame
  float fade = 1.0f;
  unsigned frame = 0;
};

class XenonCanvas;
struct VideoDevice;

class Video {
public:
  Video();
  ~Video();
  bool init();
  void shutdown();
  void resize(int, int) {}  // (the console's one screen)
  int width() const { return 1280; }
  int height() const { return 720; }

  void upload(const uint32_t* px, int w, int h, int stride);
  void draw(const VideoParams& p);
  void clear();  // black, no game picture (the launcher)
  Canvas* canvas();
  void beginUi();
  void endUi();
  void present();
  bool capture(std::vector<uint32_t>& out, int* w, int* h);

  // For the log: time spent waiting (for the GPU and the refresh) and recording, since the last call.
  struct Stats {
    uint64_t waitUs = 0, workUs = 0;
    uint64_t fenceUs = 0;  // (of waitUs: for the GPU to finish the frame two before, at the frame's first drawing)
    uint32_t frames = 0, quads = 0, copies = 0;
  };
  Stats takeStats();

private:
  std::unique_ptr<VideoDevice> dev_;
  std::unique_ptr<XenonCanvas> canvas_;
};
