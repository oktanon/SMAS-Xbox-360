# 0011 — Hito 2b, paso 1: renderBg y composeLine rápidas

- **Fecha:** 2026-11-06
- **Objetivo:** acelerar las dos funciones de la PPU que se llevan el 69 % del tiempo (`docs/PERFORMANCE.md` §2) sin
  cambiar ni un píxel.
- **Hecho:**
  - `src/core/ppu.cpp` / `ppu.h`: `renderBg` y `composeLine` nuevas bajo `#ifdef SMAS_PPU_FAST`. Los cuerpos
    originales no cambian: en el build rápido se compilan como `renderBgGeneric` y `composeLineGeneric` (macros
    `SMAS_RENDER_BG`/`SMAS_COMPOSE_LINE`), y la rápida usa `renderBgGeneric` para offset-per-tile, mosaico y color
    directo. Sin la macro (PC, Android) todo es el original. Detalle en `docs/PERFORMANCE.md` §3.1.
  - Primera versión: fondos por tiles y tablas de bitplanes; `composeLine` con la lista de prioridades reducida a las
    capas activas. Segunda: los márgenes del widescreen también por tiles (con el gancho del juego llamado como en el
    original, incluso píxel a píxel tras un "blank") y `composeLine` por rangos, sin saltos por píxel.
  - `tools/smastool.cpp`: `pichash N [NOMBRE]`, el hash de la imagen (con las intermedias del High Frame Rate)
    después de cada frame.
  - `xenon/qemu/guion_ppu.txt` y `compare_ppu.sh` (comparación frame a frame con y sin `SMAS_PPU_FAST`, con control
    negativo `SMAS_PPU_FAST_CONTROL`).
  - `SMAS_PPU_FAST` en `xenon/qemu/build.sh` y en `xenon/CMakeLists.txt`.
  - `docs/PERFORMANCE.md` §3.1, `AGENTS.md` (pasar `compare_ppu.sh` al tocar la PPU), `docs/ROADMAP.md`.
- **Probado:**
  - x86, `compare_ppu.sh`: 9020 frames iguales (las dos versiones); control negativo distinto.
  - x86, benchmarks: la PPU 2-3 veces más rápida (§3.1).
  - qemu-ppc (run 011): iguales, y las 22 líneas idénticas a la run 004.
  - Xenia: humo del `.xex` nuevo (mismos hashes al arrancar). `xenia_scan` no marca nada de la PPU.
- **Estado:** pendiente de consola (run 012).
- **Siguiente:** medir en la consola; según el resultado, más PPU (`renderObj`, `computeWindows`, la parte por píxel
  de `composeLine`) o el paso 2 (dibujo en otros hilos).
