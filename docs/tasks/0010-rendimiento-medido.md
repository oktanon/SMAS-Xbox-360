# 0010 — Rendimiento medido por componente (run 010) y plan del hito 2b

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 010 (consola: `-O2` y `-O3`, con y sin PPU) y fijar el plan de rendimiento.
- **Resultados de la run 010:**
  - Las dos variantes dan las 22 líneas y las 9 salidas idénticas a x86.
  - La PPU es el 77-85 % de cada frame (20-36 ms). Sin dibujar, la CPU, la APU y los juegos van a ~160 fps (6 ms;
    10 ms en co-op). `-O3` baja ese resto un 12 % y no mejora la PPU (hasta 6 % en total). Rewind: 6 ms guardar un
    frame, 48 ms volver uno.
  - El temporal sin puntos funciona: todas las capturas salen como `.png`.
  - La regla del USB era otra: fallan los nombres con dos o más puntos y la primera parte de 4 caracteres, y pueden
    quedar duplicados en la carpeta. Regla del port: nombres con un solo punto.
- **Perfil en x86** (gprof, SMB1 + SMB2 + SMB3): `Ppu::renderBg` 36,6 % y `Ppu::composeLine` 31,9 %; la CPU ~12 %, la
  APU ~7 %, `renderObj` 2,8 %.
- **Hecho:** `docs/PERFORMANCE.md` reescrito (tabla de la consola, perfil, plan en 5 pasos: optimizar `renderBg` y
  `composeLine` en un hilo; dibujo diferido en otros hilos hardware; `-O3`/LTO; rewind; APU en otro hilo si hace
  falta). `AGENTS.md` §4, comentario de `src/core/fileio.cpp` (la regla buena), `docs/ROADMAP.md`, run 010.
- **Probado:** consola (run 010).
- **Siguiente:** paso 1 del plan: `renderBg` y `composeLine` sin cambiar resultados, medidos en la consola.
