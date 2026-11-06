# 0012 — La PPU rápida medida en la consola (run 012)

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 012 y decidir lo siguiente del hito 2b.
- **Resultado:** resultados idénticos a la run 004 con `-O2` y `-O3`. La PPU es 2,4-4,3 veces más rápida que en la
  run 010. Con `-O3`: menú 95 fps, juegos 63-65, widescreen 50, co-op 51; `-O3` gana ahora un 12-14 % sobre `-O2`.
  Rewind: volver un frame 22-24 ms (antes 48), guardar ~6 ms.
- **Decisión:** el núcleo del port se compila con `-O3`.
- **Hecho:** `docs/PERFORMANCE.md` (tabla de la consola, §3.2 siguiente), `docs/ROADMAP.md`, `AGENTS.md` §4, run 012.
- **Probado:** consola (run 012).
- **Siguiente:** el margen para vídeo, audio e interfaz: la PPU en otro hilo hardware (paso 2), el widescreen y el
  co-op, y el rewind (`docs/PERFORMANCE.md` §3.2).
