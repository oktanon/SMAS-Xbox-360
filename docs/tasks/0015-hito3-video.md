# 0015 — Hito 3: el vídeo con vulkan4xenon

- **Fecha:** 2026-11-06
- **Objetivo:** el juego en la pantalla de la consola: swapchain 720p, el framebuffer de la PPU como textura, aspecto
  4:3 y widescreen, a 60 fps; y comprobar el orden de los canales de color (`docs/ENDIANNESS.md` §3.1).
- **Hecho:**
  - `src/platform/xenon/video_vk.h/.cpp`: `VideoVk` (instancia, dispositivo, superficie de pantalla, swapchain 1280×720
    FIFO de 2 imágenes, render pass, pipeline, textura `R8G8B8A8` de 704×240, sampler nearest, dos staging alternos con
    su fence en memoria de tipo 0 alineada a 4 KB para que vulkan4xenon copie en la GPU, un quad con push constants:
    4:3 para los 256 píxeles, ensanchado por los márgenes, centrado). `screenPixel()` lee el front buffer (con
    `tiled_index.h` de las pruebas de vulkan4xenon). Estadísticas por frame (copia, envío, esperas).
  - `src/platform/xenon/shaders/screen.vert/.frag` y `screen_shaders.c/.h` (generados con
    `vulkan4xenon-main/tools/build_shaders.py`, glslang de WSL y xost): el color sale de `(g, b, a)` de la textura,
    porque los `0x00RRGGBB` de la PPU quedan en memoria como `00 RR GG BB` en big-endian.
  - `src/platform/xenon/video_main.cpp` → `smas_video.xex`: prueba de color (4 barras escritas como la PPU, leídas del
    front buffer, `PASS`/`FAIL` en el log), luego el guion con cada frame en pantalla y líneas `VIDEO` cada 600 frames
    (emulación, vídeo, peor frame, frames de más de 16,7 ms).
  - `tools/smastool.cpp`: `g_smastoolFrameHook`, llamado tras cada frame si está puesto.
  - `xenon/hito3_demo.txt` (widescreen 64 en los cuatro juegos, 4:3, co-op) y el target `smas_video` en
    `xenon/CMakeLists.txt` (lenguaje C añadido por `screen_shaders.c`; `V4X_DIR` relativo).
  - Documentación al día: `AGENTS.md` (§2 capa y build de Xbox, §3 build actual con `-O3` y las cuatro macros, cómo
    regenerar los shaders, §4 Xenia sí presenta y cómo capturarla, §5 tabla de ficheros con lo hecho y lo pendiente),
    `docs/ENDIANNESS.md` §3.1, `docs/ROADMAP.md`.
- **Probado:** Xenia (run 015): prueba de color PASS en las cuatro barras, el juego se ve en 4:3, hashes iguales a la
  run 004.
- **Estado:** pendiente de consola (run 016).
- **Siguiente:** con la run 016, cerrar el hito 3 (o corregir); después el hito 4 (mandos y audio con SDL3).
