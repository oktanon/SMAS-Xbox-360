// Hito 3 (docs/ROADMAP.md): el vídeo con vulkan4xenon. smas_video.xex:
// 1. Una imagen de prueba hecha como la escribe la PPU (0x00RRGGBB): cuatro barras, rojo, verde, azul y un color
//    mezclado (128, 192, 64), mostrada 3 s; luego lee esos píxeles del front buffer y escribe en el log si cada barra
//    salió de su color (docs/ENDIANNESS.md §3.1: el orden de los canales en la GPU).
// 2. El emulador con el guion de siempre (guion.txt: el de la run 004 con 2 hilos de dibujo, más una parte para ver
//    el juego en widescreen), mostrando cada frame en la pantalla (el gancho g_smastoolFrameHook de smastool) a la
//    velocidad de la pantalla (FIFO). Las líneas HASHALL siguen comparándose con la run 004; cada 600 frames, el log
//    dice cuánto tardaron la emulación y el vídeo, y cuántos frames pasaron de 16,7 ms.
// Al terminar vuelve al dashboard.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "core/ppu.h"
#include "core/snes.h"
#include "platform/xenon/video_vk.h"
#include "xsdk/splash.h"
#include "xsdk/thread.h"

int smastool_main(int argc, char** argv);
extern void (*g_smastoolFrameHook)(Snes& snes);

namespace {

VideoVk g_video;

constexpr uint32_t kRenderHw[] = {2, 4, 3, 5, 1};
void placeRenderThread(int index) { xsdk_thread_move(kRenderHw[index % 5]); }

uint64_t nowUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// ---- 1. the test pattern
bool testPattern() {
  struct Bar {
    const char* name;
    uint32_t value;
    int r, g, b;
  };
  const Bar bars[4] = {{"rojo", 0x00ff0000, 255, 0, 0},
                       {"verde", 0x0000ff00, 0, 255, 0},
                       {"azul", 0x000000ff, 0, 0, 255},
                       {"mezcla (128,192,64)", 0x0080c040, 128, 192, 64}};
  std::vector<uint32_t> frame((size_t)kLineW * kFrameH, 0);
  for (int y = 0; y < 224; y++)
    for (int x = 0; x < 256; x++) frame[(size_t)y * kLineW + kMaxExt + x] = bars[x / 64].value;
  const VideoVk::Rect rect = VideoVk::fitLayout(0, 0, 224, 0, true);
  for (int i = 0; i < 180; i++)
    if (!g_video.present(frame.data(), 0, 0, 224, rect)) return false;
  // each bar's middle, half way down
  bool all = true;
  for (int k = 0; k < 4; k++) {
    const int sx = (int)(rect.x + (k * 64 + 32) * rect.w / 256.0f), sy = (int)(rect.y + rect.h / 2);
    int r = -1, g = -1, b = -1;
    const bool read = g_video.screenPixel(sx, sy, &r, &g, &b);
    const bool good = read && std::abs(r - bars[k].r) <= 2 && std::abs(g - bars[k].g) <= 2 &&
                      std::abs(b - bars[k].b) <= 2;
    all = all && good;
    std::printf("%s: prueba de color, barra %s: leído (%d, %d, %d) en (%d, %d), esperado (%d, %d, %d)%s\n",
                good ? "PASS" : "FAIL", bars[k].name, r, g, b, sx, sy, bars[k].r, bars[k].g, bars[k].b,
                read ? "" : " (el front buffer mostrado no es del swapchain)");
  }
  return all;
}

// ---- 2. each emulated frame on the screen
uint64_t g_lastHook = 0, g_emuUs = 0, g_videoUs = 0, g_worstUs = 0;
uint32_t g_frames = 0, g_over = 0;

void showFrame(Snes& snes) {
  const uint64_t t0 = nowUs();
  const uint64_t emu = g_lastHook ? t0 - g_lastHook : 0;
  const VideoVk::Rect r = VideoVk::fitLayout(snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(),
                                       (snes.ppu.extL + snes.ppu.extR) / 2, true);
  g_video.present(snes.ppu.frame, snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(), r);
  const uint64_t t1 = nowUs();
  const VideoVk::Stats vs = g_video.takeStats();
  // the frame's own work: the emulation since the last frame, the copy and the submit; not the waits (vblank)
  const uint64_t work = emu + vs.copyUs + vs.submitUs;
  g_emuUs += emu;
  g_videoUs += vs.copyUs + vs.submitUs;
  if (work > g_worstUs) g_worstUs = work;
  if (work > 16683) g_over++;
  if (++g_frames == 600) {
    std::printf("VIDEO f%u: emulación %.2f ms, vídeo (copia + envío) %.2f ms por frame; el peor %.2f ms; %u de 600 "
                "pasaron de 16,7 ms\n",
                (unsigned)snes.frameCount(), g_emuUs / 600000.0, g_videoUs / 600000.0, g_worstUs / 1000.0,
                (unsigned)g_over);
    g_emuUs = g_videoUs = g_worstUs = 0;
    g_frames = g_over = 0;
  }
  g_lastHook = t1;
}

}  // namespace

int main()
{
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  Ppu::renderThreadStart = placeRenderThread;
  std::printf("smas_video: hito 3, el vídeo (unos minutos; al terminar vuelve al dashboard)\n");
  if (!g_video.init()) {
    std::printf("FIN: el vídeo no arrancó\n");
    return 1;
  }
  const bool colours = testPattern();
  std::printf("prueba de color: %s\n", colours ? "las cuatro barras de su color" : "ALGUNA BARRA MAL");

  g_smastoolFrameHook = showFrame;
  char name[] = "smas_video";
  char rom[] = "Super Mario All-Stars (USA).sfc";
  char script[] = "guion.txt";
  char* args[] = {name, rom, script, nullptr};
  const auto t0 = std::chrono::steady_clock::now();
  const int status = smastool_main(3, args);
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  g_smastoolFrameHook = nullptr;
  std::printf("FIN: smastool_main devolvió %d en %.1f s\n", status, s);
  g_video.shutdown();
  std::printf("FIN del título\n");
  std::fflush(stdout);
  return status;
}
