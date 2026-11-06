# 0013 — Hito 2b, paso 2: el dibujo de la PPU en varios hilos

- **Fecha:** 2026-11-06
- **Objetivo:** repartir el dibujo de cada línea entre varios hilos hardware sin cambiar la imagen.
- **Decisión de diseño:** paralelizar dentro de cada línea (la emulación espera la línea, como siempre), no dibujo
  diferido: los ganchos de los juegos (`fetchWideTile`) leen el estado de cada motor y la WRAM, que la emulación
  cambia, y el diferido pediría guardar todo eso por línea. Los ganchos se llaman solo en el hilo de la emulación.
  Detalle en `docs/PERFORMANCE.md` §3.3.
- **Hecho:**
  - `src/core/ppu.cpp` / `ppu.h` (bajo `SMAS_PPU_FAST`): `renderBg` partido en `bgDrawnGeneric` + `renderBgStretch`;
    `composeLine` en `composePixels` + `composeTail`; `RenderPool` (hilos persistentes, reparto por una palabra atómica
    ronda+índice con CAS, dos conjuntos de tareas alternos, `count` atómico); `renderLineThreaded`;
    `Ppu::setRenderThreads(n)`, `renderThreads()`, `Ppu::renderThreadStart`; `Ppu` no copiable en ese build.
  - `renderLine`: el camino en hilos bajo `#ifdef`, el original intacto en el `else`.
  - `tools/smastool.cpp`: `threads N`. `xenon/qemu/compare_ppu.sh`: `HILOS=N`, ruta de salida absoluta, borra los
    resultados viejos.
  - `src/platform/xenon/hito2_main.cpp`: coloca los hilos de dibujo en los hilos hardware 2, 4, 3, 5, 1 y lo escribe en
    el log.
  - `xenon/CMakeLists.txt`: solo la variante `-O3`; el guion de la consola empieza con `threads 2`
    (`xenon/hito2_hilos.txt`); `xenon/hito2_bench.txt` mide cada escena con 0-3 hilos.
  - **Fallo de upstream encontrado por ThreadSanitizer** (también sin hilos): `Smb2Engine::playerReachesEdge` lee y
    puede escribir fuera de un vector. Corregido bajo `SMAS_PORT_FIXES` (macro nueva, en el `.xex`, qemu y
    `compare_ppu.sh`). `docs/UPSTREAM.md` nuevo con este y el desplazamiento de negativos del DSP (benigno).
  - `docs/PERFORMANCE.md` §3.3, `AGENTS.md` (macros, sanitizers, `docs/UPSTREAM.md`).
- **Probado:** `HILOS=1|2|3 compare_ppu.sh` (9020 frames iguales); ThreadSanitizer sin avisos; ASan + UBSan sin
  avisos (9020 frames, 2 hilos, frames iguales a la referencia); qemu-ppc con 2 hilos (run 013, igual a la run 004);
  humo en Xenia (2 hilos, mismos hashes). En x86 los hilos no ganan (línea de ~4 µs).
- **Estado:** pendiente de consola (run 014).
- **Siguiente:** medir en la consola; si no da margen, el dibujo diferido.
