# 0017 — Hito 4: mandos y audio con SDL3

- **Fecha:** 2026-11-06
- **Objetivo:** jugar el cartucho en la consola con el mando y con sonido, con capas que la app (hito 5) use tal cual.
- **Hecho:**
  - `src/platform/xenon/audio_sdl3.h/.cpp`: la clase `Audio` de la app (misma interfaz que Windows y Android) sobre un
    `SDL_AudioStream` en la frecuencia del dispositivo, `SDL_AUDIO_S16` nativo (nunca `S16LE`, `docs/ENDIANNESS.md`
    §3.3), el remuestreo Hermite de 32 040 Hz y el control dinámico de latencia de Android.
  - `src/platform/xenon/input_sdl3.h/.cpp`: la clase `Input` de la app sobre `SDL_Gamepad` (hasta 4 mandos, ranuras
    fijas, botones 0-16 y ejes 0-5 como la app), más `rumble()`.
  - `src/platform/xenon/play_main.cpp` → `smas_jugar.xex`: SDL3 + `VideoVk` + `Audio` + `Input`, el cartucho con dos
    jugadores y los botones por defecto de la app (`Config::setDefaults`, como `App::readPlayer`, Back = Select), 2
    hilos de dibujo, pausa con la Guía (`xsdk_system_ui_open`), widescreen con el stick derecho, salida con Back +
    Start 3 s. Log: mandos conectados, primera pulsación de cada botón de la SNES, y cada 600 frames tiempos, cola de
    audio y frames que el kernel recibió en silencio (`xsdk_audio_stats`).
  - `xenon/CMakeLists.txt`: target `smas_jugar` (`find_package(SDL3)` con `SDL3_DIR` relativo; `config.cpp`,
    `utf8.cpp`, `sys_posix.cpp` de la app). SDL-xenon sin tocar.
  - `AGENTS.md` (§2, §3, §5), `docs/ROADMAP.md`.
- **Probado:** Xenia (run 017): arranca y se ve el cartucho; SDL3 abre el audio a 48 kHz.
- **Estado:** pendiente de consola (run 018).
- **Siguiente:** con la run 018, cerrar el hito 4; después el hito 5 (launcher y `Canvas`).
