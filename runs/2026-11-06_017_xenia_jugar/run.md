# Run 017 — smas_jugar.xex en Xenia (hito 4)

- **Fecha:** 2026-11-06
- **Dónde:** XSDK Xenia (no es evidencia; solo para ver que arranca)
- **Commit:** tarea 0017
- **Qué se probó:** `smas_jugar.xex` con la ROM al lado, 40 s, sin mando; una captura de la pantalla de Windows (no se
  sube).
- **Resultado:** arranca: SDL3 (audio y mandos), vídeo y el cartucho en tiempo real; la pantalla de título de
  All-Stars en color. Audio: 48 kHz, bloques de 1024 frames en Xenia (en la consola, `sdl_check` dio 256). Los
  vaciados de la cola y los 14 ms por frame son de la lentitud de Xenia: no valen.
- **Logs:** `smas_jugar.log`.
- **Siguiente:** run 018 en la consola, con mando.
