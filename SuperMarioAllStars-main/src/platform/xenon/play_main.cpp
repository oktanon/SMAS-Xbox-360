// Hito 4 (docs/ROADMAP.md): mandos y audio con SDL3. smas_jugar.xex arranca el cartucho (el menú de All-Stars) y se
// juega con el mando: vídeo con VideoVk (hito 3), sonido con Audio (audio_sdl3), mandos con Input (input_sdl3), los
// dos últimos con la misma interfaz que usa la app, y los botones como la app los asigna por defecto
// (Config::setDefaults, App::readPlayer: posiciones de Xbox, abajo = B de SNES, derecha = A; el stick izquierdo
// también mueve; Back = Select, ya que aquí no hay menú). Dos jugadores (mandos 1 y 2).
// - El stick derecho pulsado cambia entre 4:3 y widescreen (el margen que pide la pantalla 16:9 con píxeles 8:7,
//   como el aspecto Auto del port de PC: 46 por lado), con la imagen como la coloca el PC (VideoVk::fitLayout).
// - La Guía abierta pausa el juego y el sonido.
// - Back + LB + RB 3 segundos: salir al dashboard (Back + Start saca una captura en el dashboard, run 018).
// El log (game:\smas_jugar.log): cada mando que se conecta, la primera vez que se pulsa cada botón de la SNES en
// cada jugador (para comprobar la asignación), y cada 600 frames los tiempos y el sonido (veces que la cola se quedó
// vacía, su mínimo, y los frames que el kernel recibió en silencio, xsdk_audio_stats).
#include <SDL3/SDL.h>

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <fstream>
#include <iterator>
#include <vector>

#include "app/config.h"
#include "core/snes.h"
#include "game/smas.h"
#include "platform/xenon/audio_sdl3.h"
#include "platform/xenon/input_sdl3.h"
#include "platform/xenon/video_vk.h"
#include "xsdk/audio.h"
#include "xsdk/splash.h"
#include "xsdk/thread.h"
#include "xsdk/xam.h"

namespace {

constexpr uint32_t kRenderHw[] = {2, 4, 3, 5, 1};
void placeRenderThread(int index) { xsdk_thread_move(kRenderHw[index % 5]); }

uint64_t nowUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

VideoVk g_video;
Audio g_audio;

// The log while playing goes to memory and out at the Guide, at the end or past 32 KB: a line written to the USB
// drive costs milliseconds (run 001), and each one, every 10 s, cost a refresh and emptied the sound's queue (run 020).
std::string g_log;
void logf(const char* fmt, ...) {
  char line[512];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(line, sizeof line, fmt, ap);
  va_end(ap);
  g_log += line;
}
void flushLog() {
  if (g_log.empty()) return;
  std::fputs(g_log.c_str(), stdout);
  std::fflush(stdout);
  g_log.clear();
}
Input g_input;
Config g_cfg;

// The player's SNES buttons as App::readPlayer makes them, without the menu: the bindings, the left stick as the
// D-pad, Back (Select's binding) as the game's Select.
uint16_t readPlayer(int p) {
  const float dz = g_cfg.deadzone / 100.0f;
  const int padIdx = g_cfg.padIndex[p];
  bool act[kActCount];
  for (int a = 0; a < kActCount; a++) act[a] = g_input.active(g_cfg.pad[p][a], padIdx, dz);
  if (const PadState* ps = g_input.pad(padIdx)) {
    bool l, r, u, d;  // (by sectors: Input::stickToDpad)
    Input::stickToDpad(ps->axes[0], ps->axes[1], dz, &l, &r, &u, &d);
    act[kActLeft] |= l;
    act[kActRight] |= r;
    act[kActUp] |= u;
    act[kActDown] |= d;
  }
  uint16_t v = 0;
  if (act[kActUp]) v |= kPadUp;
  if (act[kActDown]) v |= kPadDown;
  if (act[kActLeft]) v |= kPadLeft;
  if (act[kActRight]) v |= kPadRight;
  if (act[kActA]) v |= kPadA;
  if (act[kActB]) v |= kPadB;
  if (act[kActX]) v |= kPadX;
  if (act[kActY]) v |= kPadY;
  if (act[kActL]) v |= kPadL;
  if (act[kActR]) v |= kPadR;
  if (act[kActStart]) v |= kPadStart;
  if (act[kActSelect]) v |= kPadSelect;
  if ((v & kPadUp) && (v & kPadDown)) v &= ~(kPadUp | kPadDown);
  if ((v & kPadLeft) && (v & kPadRight)) v &= ~(kPadLeft | kPadRight);
  return v;
}

const char* snesButtonName(uint16_t bit) {
  switch (bit) {
    case kPadUp: return "arriba";
    case kPadDown: return "abajo";
    case kPadLeft: return "izquierda";
    case kPadRight: return "derecha";
    case kPadA: return "A";
    case kPadB: return "B";
    case kPadX: return "X";
    case kPadY: return "Y";
    case kPadL: return "L";
    case kPadR: return "R";
    case kPadStart: return "Start";
    case kPadSelect: return "Select";
  }
  return "?";
}

}  // namespace

int main()
{
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  Ppu::renderThreadStart = placeRenderThread;
  std::printf("smas_jugar: hito 4, mandos y audio (Back + LB + RB 3 s para salir)\n");

  if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
    std::printf("FIN: SDL_Init falló: %s\n", SDL_GetError());
    return 1;
  }
  if (!g_video.init()) {
    std::printf("FIN: el vídeo no arrancó\n");
    return 1;
  }
  if (!g_audio.init(64)) std::printf("audio: no arrancó (%s); se sigue sin sonido\n", SDL_GetError());
  else std::printf("audio: %d Hz, bloques de %d frames, latencia 64 ms\n", g_audio.rate(), g_audio.deviceFrames());
  g_input.init();
  g_cfg.setDefaults();

  std::ifstream romFile("Super Mario All-Stars (USA).sfc", std::ios::binary);
  std::vector<uint8_t> rom((std::istreambuf_iterator<char>(romFile)), std::istreambuf_iterator<char>());
  static Snes snes;
  if (rom.empty() || !snes.loadRom(rom)) {
    std::printf("FIN: no se pudo leer la ROM (Super Mario All-Stars (USA).sfc junto al .xex)\n");
    return 1;
  }
  static SmasGame game(snes);
  game.install();
  snes.reset(true);
  snes.ppu.setRenderThreads(2);

  std::vector<int16_t> samples;
  uint16_t seen[2] = {0, 0};
  int wide = 0;
  bool prevRs = false, uiOpen = false;
  int quitHeld = 0;
  uint64_t emuUs = 0, videoUs = 0, worstUs = 0, inputUs = 0, runUs = 0, pushUs = 0;
  uint32_t frames = 0, over = 0, totalFrames = 0, missed = 0;
  uint64_t lastShown = 0;  // (when the last present returned: an interval of more than 1.5 refreshes is a refresh missed)
  XsdkAudioStats lastStats{};
  xsdk_audio_stats(&lastStats);

  for (bool running = true; running;) {
    const uint64_t tIn = nowUs();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) running = false;
      if (e.type == SDL_EVENT_GAMEPAD_ADDED || e.type == SDL_EVENT_GAMEPAD_REMOVED) {
        g_input.rescanDevices();
        logf("mando %s (id %u)\n", e.type == SDL_EVENT_GAMEPAD_ADDED ? "conectado" : "desconectado",
                    (unsigned)e.gdevice.which);
      }
    }
    g_input.poll();

    // the Guide over the game: pause it and its sound
    const bool ui = xsdk_system_ui_open();
    if (ui != uiOpen) {
      uiOpen = ui;
      g_audio.setPaused(ui);
      logf("guía %s\n", ui ? "abierta: pausa" : "cerrada: sigue");
      if (ui) flushLog();  // (paused: writing costs nothing seen)
    }
    if (ui) {
      g_video.present(snes.ppu.frame, snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(),
                    VideoVk::fitLayout(snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(), wide, true));
      continue;
    }

    const uint64_t t0 = nowUs();
    for (int p = 0; p < 2; p++) {
      const uint16_t v = readPlayer(p);
      snes.input[p] = v;
      for (uint16_t bit = 1; bit; bit <<= 1)
        if ((v & bit) && !(seen[p] & bit)) {
          seen[p] |= bit;
          logf("jugador %d: botón %s de la SNES (primera vez)\n", p + 1, snesButtonName(bit));
        }
    }
    const PadState* pad0 = g_input.pad(0);
    const bool rs = pad0 && pad0->buttons[9];
    if (rs && !prevRs) {
      wide = wide ? 0 : VideoVk::wantExtFor(16.0 / 9.0, true, SmasGame::kMaxSupportedExt);
      game.setWidescreen(wide);
      logf("widescreen %d\n", wide);
    }
    prevRs = rs;
    // Back + LB + RB (Back + Start takes a screenshot in the user's dashboard, run 018)
    quitHeld = pad0 && pad0->buttons[6] && pad0->buttons[4] && pad0->buttons[5] ? quitHeld + 1 : 0;
    if (quitHeld >= 180) running = false;

    const uint64_t tRun = nowUs();
    game.beforeFrame();
    snes.runFrame();
    game.afterFrame();
    const uint64_t tPush = nowUs();
    samples.clear();
    snes.takeAudio(samples);
    g_audio.push(samples.data(), samples.size() / 2, Snes::kSampleRate);
    const uint64_t t1 = nowUs();
    g_video.present(snes.ppu.frame, snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(),
                    VideoVk::fitLayout(snes.ppu.extL, snes.ppu.extR, snes.ppu.frameHeight(), wide, true));
    const VideoVk::Stats vs = g_video.takeStats();
    const uint64_t shown = nowUs();
    if (lastShown && shown - lastShown > 25000) missed += (uint32_t)((shown - lastShown) / 16683) - 1;
    lastShown = shown;
    inputUs += tRun - tIn;
    runUs += tPush - tRun;
    pushUs += t1 - tPush;
    (void)t0;

    const uint64_t emu = t1 - tIn, work = emu + vs.copyUs + vs.submitUs;
    emuUs += emu;
    videoUs += vs.copyUs + vs.submitUs;
    if (work > worstUs) worstUs = work;
    if (work > 16683) over++;
    totalFrames++;
    if (++frames == 600) {
      int underruns = 0, minQueued = 0;
      g_audio.takeStats(&underruns, &minQueued);
      XsdkAudioStats st{};
      xsdk_audio_stats(&st);
      logf("JUGAR f%u (%s, wide %d): emulación %.2f ms (mandos %.2f, frame %.2f, audio %.2f), vídeo %.2f ms "
                  "por frame, el peor %.2f ms, %u de 600 de más, %u refrescos perdidos; audio: cola vacía %d veces, mínimo %d frames, ahora "
                  "%d; kernel: %u frames, %u en silencio\n",
                  (unsigned)totalFrames, game.describe().substr(0, game.describe().find('\n')).c_str(), wide,
                  emuUs / 600000.0, inputUs / 600000.0, runUs / 600000.0, pushUs / 600000.0, videoUs / 600000.0,
                  worstUs / 1000.0, (unsigned)over, (unsigned)missed, underruns, minQueued,
                  g_audio.queuedFrames(), (unsigned)(st.played - lastStats.played),
                  (unsigned)(st.silent - lastStats.silent));
      lastStats = st;
      emuUs = videoUs = worstUs = inputUs = runUs = pushUs = 0;
      frames = over = missed = 0;
      if (g_log.size() > 32768) flushLog();
    }
  }

  logf("FIN: %u frames jugados\n", (unsigned)totalFrames);
  flushLog();
  snes.ppu.setRenderThreads(0);
  g_audio.shutdown();
  g_input.shutdown();
  g_video.shutdown();
  SDL_Quit();
  std::printf("FIN del título\n");
  std::fflush(stdout);
  return 0;
}
