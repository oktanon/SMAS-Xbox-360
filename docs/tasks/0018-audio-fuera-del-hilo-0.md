# 0018 — Run 018: tirones por el hilo de audio; se mueve al hilo hardware 5

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 018 (el usuario jugó: sonido bien, mandos bien, dos jugadores bien, tirones de
  imagen molestos, SMB3 lo peor; Back + Start saca una captura en su dashboard) y corregirlo.
- **Análisis:** con sonido la emulación tarda ~5 ms más por frame que en las runs 014/016 (SMB1 4:3: 15,6 ms frente a
  10,4). El hilo del dispositivo de audio de SDL-xenon arranca en el hilo hardware del que abre el dispositivo (el 0,
  el de la emulación) con prioridad +1. Los frames de más de 16,7 ms pierden un refresco (los tirones); con la
  emulación más lenta que el tiempo real, la cola de audio se vacía. En widescreen se suma lo ya conocido de SMB2/SMB3
  y el co-op (`docs/PERFORMANCE.md` §4).
- **Hecho:**
  - `src/platform/xenon/audio_sdl3.cpp`: `SDL_HINT_AUDIO_XSDK_HARDWARE_THREAD = "5"` antes de abrir el dispositivo (el
    hint es de SDL-xenon; no se toca su código). Así lo hereda también la app.
  - `src/platform/xenon/play_main.cpp`: salir con Back + LB + RB 3 s; el log separa mandos, frame y audio.
  - `AGENTS.md` §4 y §5, `docs/PERFORMANCE.md` §5, run 018.
- **Probado:** compila; arranca en Xenia. Pendiente de consola (run 019).
- **Siguiente:** con la run 019, cerrar el hito 4 o seguir buscando.
