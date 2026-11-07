// Super Mario All-Stars port: the Xbox 360 (hito 5). platform/linux/app_linux.cpp's part for the console: the main
// loop and its pacing, the events, the paths; the application itself is app/app.cpp, the same as on every platform
// (the launcher, the menus, the games), on the layers of platform/xenon: Video (vulkan4xenon), Audio and Input (SDL3).
// - Settings, saves and the ROM are beside the .xex (game:); the ROM is looked for there and in a "roms" folder.
// - No file pickers, browser or network: the ROM and the covers go in by hand; Check for Updates finds nothing.
// - The Guide over the title pauses it and its sound.
// - Back + LB + RB held 3 s quits to the dashboard from anywhere (the launcher's Quit does it too).
// - The log: game:\smas_port.log (the app's, as on Linux) and, kept in memory and written at the Guide, at the end or
//   past 32 KB, every 10 s the passes' times (a line on the USB drive costs milliseconds: run 020).
#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cwctype>
#include <string>

#include "app/app.h"
#include "app/sys.h"
#include "xsdk/splash.h"
#include "xsdk/thread.h"
#include "xsdk/xam.h"

namespace {

// The PPU's line threads (Ppu::renderThreadStart): hardware threads 2 and 4 (run 012); the emulation stays on 0 and
// SDL's sound on 5 (audio_sdl3.cpp).
constexpr uint32_t kRenderHw[] = {2, 4, 3, 1};
void placeRenderThread(int index) { xsdk_thread_move(kRenderHw[index % 4]); }

int lockedRefreshes(double hz) {
  int k = (int)std::lround(hz / Snes::kFrameRate);
  return k >= 1 && std::fabs(hz / k - Snes::kFrameRate) / Snes::kFrameRate < 0.025 ? k : 0;
}

bool hasRomExt(const std::wstring& name) {
  std::wstring lower = name;
  for (wchar_t& c : lower) c = (wchar_t)towlower(c);
  for (const wchar_t* ext : {L".sfc", L".smc", L".swc", L".fig"})
    if (lower.size() > 4 && lower.compare(lower.size() - 4, 4, ext) == 0) return true;
  return false;
}

std::string g_stats;  // (the passes' times: see the top)
void statsf(const char* fmt, ...) {
  char line[512];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(line, sizeof line, fmt, ap);
  va_end(ap);
  g_stats += line;
  g_stats += '\n';
}
void flushStats() {
  if (g_stats.empty()) return;
  if (g_log) {
    std::fputs(g_stats.c_str(), g_log);
    std::fflush(g_log);
  }
  g_stats.clear();
}

uint64_t nowUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

}  // namespace

void openUrl(const char*) {}
bool httpGet(const char*, std::string& out) {
  out.clear();
  return false;
}
void openFolder(const std::wstring&) {}

void App::applyDisplayMode() {}  // (the console's one screen)

bool App::findRom(std::wstring* out) {
  for (const std::wstring& dir : {exeDir, pathJoin(exeDir, L"roms")}) {
    std::vector<std::wstring> names;
    if (!listFiles(dir, names)) continue;
    for (const std::wstring& n : names) {
      if (!hasRomExt(n)) continue;
      std::wstring p = pathJoin(dir, n);
      std::vector<uint8_t> data;
      if (readAll(p, data) && isSmasRom(data, nullptr)) {
        *out = p;
        return true;
      }
    }
  }
  return false;
}

void App::pickRom() {
  menu.notify("Put the Super Mario All-Stars (USA) ROM (.sfc) beside the .xex, or in a roms folder there", 6.0f);
}
void App::pollPick() {}
void App::pickArt(int) {}
void App::pollArtPick() {}

// No touch screen: the on-screen pad stays hidden, nothing is tapped.
void App::touchFrame() {
  touch.layout((float)video.width(), (float)video.height());
  touch.inGame = false;
  touch.visible = false;
}

bool App::init() {
  if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
    std::printf("SDL_Init falló: %s\n", SDL_GetError());
    return false;
  }
  exeDir = L"game:";
  g_log = openFile(pathJoin(exeDir, L"smas_port.log"), "w");
  logf("starting (Xbox 360), version %s", kAppVersion);
  configPath = pathJoin(exeDir, L"smas_port.ini");
  saveDir = pathJoin(exeDir, L"saves");
  shotDir = pathJoin(exeDir, L"screenshots");
  makeDir(saveDir);
  if (!cfg.load(configPath)) cfg.save(configPath);
  cfg.vsync = true;
  if (!video.init()) {
    logf("video init failed");
    return false;
  }
  logf("video ok %dx%d", video.width(), video.height());
  if (!audio.init(cfg.audioLatency)) menu.notify("Audio device unavailable");
  else logf("audio: %d Hz, %d-frame chunks, latency %d ms", audio.rate(), audio.deviceFrames(), cfg.audioLatency);
  input.init();
  menu.setCanvas(video.canvas());
  touchActive = false;

  const bool haveRom = loadRom();
  logf(haveRom ? "rom loaded: %s" : "rom not found%s", narrow(romPath).c_str());
  achStore.load(pathJoin(exeDir, L"achievements.ini"));
  launcher.setVersion(kAppVersion);
  launcher.setAllStarsOnly(cfg.launcherAllStars);
  launcher.selectGame(Launcher::cardGame(cfg.lastGame));
  playing = Launcher::cardGame(cfg.lastGame);
  if (haveRom) romReady();
  applyConfig();
  inLauncher = true;
  if (!achStore.noticeShown) {
    achStore.noticeShown = true;
    achStore.save();
    if (cfg.achievements) achWelcome();
  }
  return true;
}

void App::shutdown() {
  saveSram();
  cfg.save(configPath);
  music.shutdown();
  launcher.releaseGpu();
  menu.setCanvas(nullptr);
  audio.shutdown();
  input.shutdown();
  if (snes) snes->ppu.setRenderThreads(0);
  video.shutdown();
  flushStats();
  if (g_log) fclose(g_log);
  g_log = nullptr;
  SDL_Quit();
}

// One pass per refresh (present waits for it: FIFO); the pacing as app_linux.cpp's, the refresh rate from the
// passes' median interval (the console's 59.94 Hz: a game frame a refresh).
void App::run() {
  const Uint64 freq = SDL_GetPerformanceFrequency();
  Uint64 last = SDL_GetPerformanceCounter();
  const double period = 1.0 / Snes::kFrameRate;
  float intervals[48] = {};
  int nIntervals = 0;
  bool uiOpen = false;
  int quitHeld = 0;
  // (the log's 10 s: passes, their work without the waits, the worst, the refreshes missed)
  uint64_t workUs = 0, worstUs = 0, statsAt = nowUs(), lastShown = 0;
  uint32_t passes = 0, missed = 0, over = 0;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) running = false;
      if (e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED) input.rescanDevices();
    }
    if (!running) break;
    Uint64 now = SDL_GetPerformanceCounter();
    double dt = (double)(now - last) / freq;
    last = now;

    // the Guide over the title: everything waits, the picture stays
    const bool ui = xsdk_system_ui_open();
    if (ui != uiOpen) {
      uiOpen = ui;
      audio.setPaused(ui);
      if (ui) flushStats();
      input.clearKeys();
    }
    if (ui) {
      render();
      video.present();
      continue;
    }

    intervals[nIntervals++ % 48] = (float)dt;
    double hz = 59.94;
    if (nIntervals >= 48) {
      float d[48];
      std::copy(intervals, intervals + 48, d);
      std::nth_element(d, d + 24, d + 48);
      if (d[24] > 0.002f) hz = 1.0 / d[24];
    }
    if (pacing.mode != kPaceVsync || lockedRefreshes(hz) != pacing.perFrame) {
      pacing = Pacing();
      pacing.mode = kPaceVsync;
      pacing.perFrame = lockedRefreshes(hz);
      logf("pacing: %.2f Hz display, %d refreshes per frame (0 = uneven)", hz, pacing.perFrame);
    }
    pacing.hz = hz;
    int frames = 0;
    int vb = std::max(1, (int)std::lround(dt * hz));
    if (vb > 6) {  // stalled (loading, the Guide): resume without fast-forwarding
      vb = 0;
      pacing.vblanks = 0;
      pacing.accum = 0;
      frames = 1;
    }
    if (pacing.perFrame) {
      pacing.vblanks += vb;
      while (pacing.vblanks >= pacing.perFrame) {
        pacing.vblanks -= pacing.perFrame;
        frames++;
      }
    } else {
      pacing.accum += vb / hz;
      while (pacing.accum >= period) {
        pacing.accum -= period;
        frames++;
      }
    }
    frames = std::min(frames, 3);

    // Back + LB + RB, 3 s: out (Back + Start takes a screenshot in the dashboard, run 018)
    const PadState* p0 = input.pad(cfg.padIndex[0]);
    quitHeld = p0 && p0->buttons[6] && p0->buttons[4] && p0->buttons[5] ? quitHeld + 1 : 0;
    if (quitHeld >= 180) running = false;

    const uint64_t t0 = nowUs();
    frame(dt, frames);
    render();
    const uint64_t t1 = nowUs();
    video.present();
    const uint64_t shown = nowUs();
    const Video::Stats vs = video.takeStats();
    const uint64_t work = (t1 - t0) + vs.workUs;
    workUs += work;
    worstUs = std::max(worstUs, work);
    if (work > 16683) over++;
    if (lastShown && shown - lastShown > 25000) missed += (uint32_t)((shown - lastShown) / 16683) - 1;
    lastShown = shown;
    passes++;
    if (shown - statsAt >= 10000000) {
      int dry = 0, low = 0;
      audio.takeStats(&dry, &low);
      statsf("pasadas %s: %u en 10 s, trabajo %.2f ms de media, el peor %.2f ms, %u de más de 16,7 ms, %u refrescos "
             "perdidos; audio vacío %d veces (mínimo %d frames)",
             inLauncher ? "launcher" : menu.open() ? "menú" : "juego", (unsigned)passes,
             passes ? workUs / 1000.0 / passes : 0.0, worstUs / 1000.0, (unsigned)over, (unsigned)missed, dry, low);
      statsAt = shown;
      workUs = worstUs = 0;
      passes = missed = over = 0;
      if (g_stats.size() > 32768) flushStats();
    }
  }
}

int main() {
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  Ppu::renderThreadStart = placeRenderThread;
  std::unique_ptr<App> app = std::make_unique<App>();
  g_app = app.get();
  int rc = 0;
  if (!app->init()) rc = 1;
  else app->run();
  app->shutdown();
  g_app = nullptr;
  return rc;
}
