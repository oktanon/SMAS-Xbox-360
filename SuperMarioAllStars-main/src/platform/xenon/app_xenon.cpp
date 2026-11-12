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
  // (the Save and Load State chords, Back + RB / LB: a settings file from before them had none, and the console
  // has no Delete to clear one, so none means the default)
  if (cfg.pad[0][kActSaveState].type == Binding::None && cfg.pad[0][kActLoadState].type == Binding::None) {
    Config d;
    d.setDefaults();
    cfg.pad[0][kActSaveState] = d.pad[0][kActSaveState];
    cfg.pad[0][kActLoadState] = d.pad[0][kActLoadState];
  }
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
  files.releaseGpu();
  menu.setCanvas(nullptr);
  audio.shutdown();
  input.shutdown();
  if (snes) snes->ppu.setRenderThreads(0);
  if (snes) snes->setApuThread(false);
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
  bool uiOpen = false, prevRs = false;
  int wideAspect = kAspectAuto;  // (the widescreen the right stick goes back to)
  int quitHeld = 0;
  // (the log's 10 s: passes, their work without the waits, the worst, the refreshes missed)
  uint64_t workUs = 0, worstUs = 0, gpuUs = 0, statsAt = nowUs(), lastShown = 0, renderUs = 0;
  uint32_t passes = 0, missed = 0, over = 0, emulated = 0;
  uint64_t frameGpuUs = 0, vsyncUs = 0, frameGpuWorst = 0;  // (Video::Stats' GPU samples; waits for the refresh)
  uint32_t gpuSamples = 0;
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

    // The console's one display mode: 59.94 Hz, a game frame a refresh. (Measured from the passes, as on Linux, slow
    // passes moved the median off 60 Hz and the pacing between "a frame a refresh" and "the time owed", which ran
    // two frames on some refreshes and none on others: judder, at 60 fps counted, run 026.)
    const double hz = 59.94;
    if (pacing.mode != kPaceVsync || lockedRefreshes(hz) != pacing.perFrame) {
      pacing = Pacing();
      pacing.mode = kPaceVsync;
      pacing.perFrame = lockedRefreshes(hz);
      logf("pacing: %.2f Hz display, %d refreshes per frame (0 = uneven)", hz, pacing.perFrame);
    }
    pacing.hz = hz;
    // A game frame for each refresh gone by, counted in fractions: a pass of 18 ms is 1.08 refreshes, and every
    // dozenth such pass runs two frames, so the game keeps its speed and its sound fed when passes run long (counted
    // whole, each was one refresh: the game ran 8 % slow and the sound's queue emptied, run 027). The quarter's slack
    // keeps a pass a hair short of a refresh from running none and the next two.
    (void)period;
    int frames = 0;
    if (dt * hz > 6) {  // stalled (loading, the Guide): resume without fast-forwarding
      pacing.vbResidual = 0;
      frames = 1;
    } else {
      pacing.vbResidual += dt * hz;
      frames = std::clamp((int)std::floor(pacing.vbResidual + 0.25), 0, 3);
      pacing.vbResidual = std::min(pacing.vbResidual - frames, 1.0);  // (past three: not owed later)
    }

    // Back + LB + RB, 3 s: out (Back + Start takes a screenshot in the dashboard, run 018)
    const PadState* p0 = input.pad(cfg.padIndex[0]);
    quitHeld = p0 && p0->buttons[6] && p0->buttons[4] && p0->buttons[5] ? quitHeld + 1 : 0;
    if (quitHeld >= 180) running = false;
    // The right stick pressed, playing: 4:3 or the widescreen chosen in Options > Display > Aspect Ratio (Auto if
    // none), as smas_jugar.xex had it (run 027: the way back to 4:3 wasn't found)
    const bool rs = p0 && p0->buttons[9];
    if (rs && !prevRs && !inLauncher && !menu.open()) {
      if (cfg.aspect != kAspect4x3) {
        wideAspect = cfg.aspect;
        cfg.aspect = kAspect4x3;
      } else {
        cfg.aspect = wideAspect;
      }
      menu.notify(cfg.aspect == kAspect4x3 ? "4:3 (Original)" : "Widescreen", 1.5f);
      cfg.save(configPath);
    }
    prevRs = rs;

    // One pool of render threads at a time (idle ones spin): the game's console while playing, the chosen card's in
    // the launcher (run 024).
    if (snes) snes->ppu.setRenderThreads(inLauncher ? 0 : 2);
    // (task 0044: the APU stays in the emulation's thread. Its own thread on hardware thread 1, task 0043, made the
    // CPU 0.6 ms slower per frame, 1 ms in co-op, and doubled the rewind's snapshot: same core, many syncs, run 042)
    if (snes) snes->setApuThread(false);
    launcher.previewThreads(inLauncher);
    // The keys' lines with the controller's buttons (the menus' face buttons by position, as Menu Buttons sets; as
    // letters: the fonts have no circled ones, run 025)
    const char* ok = cfg.confirmRight ? "B" : "A";
    const char* back = cfg.confirmRight ? "A" : "B";
    launcher.setHint(std::string("\xE2\x86\x90\xE2\x86\x92 Choose    ") + ok + " Play    " + back + " Quit");
    menu.keysHint = widenUtf8(std::string("\xE2\x86\x91\xE2\x86\x93 Select      \xE2\x86\x90\xE2\x86\x92 Change      ") +
                              ok + " Confirm      " + back + " Back");

    const int emulatedBefore = fpsFrames;
    const uint64_t t0 = nowUs();
    // (the launcher: one frame a pass at most. Catching up three made a slow pass slower: 53 ms a pass with the four
    // cards, run 024; behind, the previews run slower instead)
    frame(dt, inLauncher ? std::min(frames, 1) : frames);
    const uint64_t tRender = nowUs();
    render();
    const uint64_t t1 = nowUs();
    renderUs += t1 - tRender;
    video.present();
    const uint64_t shown = nowUs();
    const Video::Stats vs = video.takeStats();
    // (the work without waits: the GPU's frame two before, waited for at this frame's first drawing, is a wait)
    const uint64_t work = (t1 - t0) - std::min<uint64_t>(vs.fenceUs, t1 - t0) + vs.workUs;
    emulated += (uint32_t)(fpsFrames >= emulatedBefore ? fpsFrames - emulatedBefore : fpsFrames);  // (reset each second)
    workUs += work;
    gpuUs += vs.fenceUs;  // (waiting for the GPU: high when it, not the CPU, holds the frames back)
    vsyncUs += vs.waitUs - std::min(vs.waitUs, vs.fenceUs);
    frameGpuUs += vs.gpuUs;
    gpuSamples += vs.gpuSamples;
    if (vs.gpuSamples) frameGpuWorst = std::max(frameGpuWorst, vs.gpuUs / vs.gpuSamples);
    worstUs = std::max(worstUs, work);
    if (work > 16683) over++;
    if (lastShown && shown - lastShown > 25000) missed += (uint32_t)((shown - lastShown) / 16683) - 1;
    lastShown = shown;
    passes++;
    if (shown - statsAt >= 10000000) {
      int dry = 0, low = 0;
      audio.takeStats(&dry, &low);
      statsf("pasadas %s: %u en 10 s (%u frames emulados), trabajo %.2f ms de media, el peor %.2f ms, %u de más de "
             "16,7 ms, %u refrescos perdidos, espera a la GPU %.2f ms de media; rewind %u snapshots; audio vacío %d veces "
             "(mínimo %d frames)",
             inLauncher ? "launcher" : menu.open() ? "menú" : "juego", (unsigned)passes, (unsigned)emulated,
             passes ? workUs / 1000.0 / passes : 0.0, worstUs / 1000.0, (unsigned)over, (unsigned)missed,
             passes ? gpuUs / 1000.0 / passes : 0.0,
             (unsigned)rewind.snapshots(), dry, low);
      statsf("  GPU %.2f ms por frame (%u muestras, la peor %.2f ms), espera al refresco %.2f ms de media",
             gpuSamples ? frameGpuUs / 1000.0 / gpuSamples : 0.0, (unsigned)gpuSamples, frameGpuWorst / 1000.0,
             passes ? vsyncUs / 1000.0 / passes : 0.0);
      {  // (task 0042: the rewind's thread, on hardware thread 1, the emulation's core)
        uint64_t us = 0;
        uint32_t n = 0;
        rewind.takeWork(&us, &n);
        if (n) statsf("  hilo del rewind: %u diferencias, %.2f ms cada una, %.2f ms por segundo", (unsigned)n, us / 1000.0 / n, us / 1000.0 / 10.0);
      }
      // (hito 8: what the game passes did, see App::prof)
      if (prof.frames) {
        static const char* const kFilters[] = {"Nearest", "Sharp", "Bilinear", "CRT Scanlines", "CRT Aperture", "LCD"};
        const int f = cfg.filter;
        statsf("  %s, %d px de más (%s), co-op %s, filtro %s, rewind %s: emulación %.2f ms por frame (%u, %u "
               "dibujados), snapshot del rewind %.2f ms (%u), interfaz y vídeo %.2f ms por pasada",
               SmasGame::gameName(prof.game), prof.ext, prof.ext ? "16:9" : "4:3", cfg.coop ? "sí" : "no",
               f >= 0 && f < 6 ? kFilters[f] : "otro", cfg.rewind ? "sí" : "no", prof.emuUs / 1000.0 / prof.frames,
               (unsigned)prof.frames, (unsigned)prof.drawn, prof.states ? prof.stateUs / 1000.0 / prof.states : 0.0,
               (unsigned)prof.states, passes ? renderUs / 1000.0 / passes : 0.0);
        // (task 0039: a drawn frame's cost on its own, and where it goes)
        const uint32_t skipped = prof.frames - prof.drawn;
        if (prof.drawn) {
          double ppu[Ppu::kProfPhases], line = 0;  // (ms per drawn frame)
          for (int k = 0; k < Ppu::kProfPhases; k++) line += ppu[k] = prof.ppuNs[k] / 1e6 / prof.drawn;
          statsf("    frame dibujado %.2f ms: PPU en este hilo %.2f ms (de ellos esperando a los hilos de dibujo %.2f ms), "
                 "CPU, APU y juegos %.2f ms; frame saltado %.2f ms",
                 prof.drawnUs / 1000.0 / prof.drawn, line, ppu[Ppu::kProfWait1] + ppu[Ppu::kProfWait2],
                 prof.drawnUs / 1000.0 / prof.drawn - line, skipped ? (prof.emuUs - prof.drawnUs) / 1000.0 / skipped : 0.0);
          // (task 0040: the PPU's phases on this thread, Ppu::profNs)
          statsf("    PPU: preparar la línea y ganchos %.2f, capas %.2f (espera %.2f), en serie %.2f, composición %.2f "
                 "(espera %.2f), final %.2f ms",
                 ppu[Ppu::kProfHooks], ppu[Ppu::kProfLayers], ppu[Ppu::kProfWait1], ppu[Ppu::kProfSerial],
                 ppu[Ppu::kProfCompose], ppu[Ppu::kProfWait2], ppu[Ppu::kProfTail]);
        }
      }
      prof = Prof();
      renderUs = 0;
      frameGpuUs = vsyncUs = frameGpuWorst = 0;
      gpuSamples = 0;
      statsAt = shown;
      workUs = worstUs = gpuUs = 0;
      passes = missed = over = emulated = 0;
      if (g_stats.size() > 32768) flushStats();
    }
  }
}

int main() {
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  Ppu::renderThreadStart = placeRenderThread;
  // (task 0043: the APU's thread, if turned on, on hardware thread 1; off since task 0044)
  Snes::apuThreadStart = [] { xsdk_thread_move(1); };
  Rewind::threadStart = [] { xsdk_thread_move(1); };  // (the rewind's differences: hardware thread 1, free)
  std::unique_ptr<App> app = std::make_unique<App>();
  g_app = app.get();
  int rc = 0;
  if (!app->init()) rc = 1;
  else app->run();
  app->shutdown();
  g_app = nullptr;
  return rc;
}
