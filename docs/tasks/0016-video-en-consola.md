# 0016 — El vídeo medido en la consola (run 016); hito 3 hecho en 4:3

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 016 y cerrar el hito 3.
- **Resultado:** prueba de color PASS en las cuatro barras en la consola (el orden de canales `(g, b, a)` es el
  correcto); las 22 líneas y las 9 salidas iguales a la run 004 con vídeo y 2 hilos; el vídeo cuesta 0,25-0,33 ms por
  frame; en 4:3 los cuatro juegos van a 9-12 ms por frame. En widescreen 64, SMB2 (17-19 ms), SMB3 (15,2 ms) y el
  co-op (17,6 ms) pasan del presupuesto; las cargas de SMB2 van a 22-32 ms (pantalla negra).
- **Causa (perfil en x86 de SMB2 widescreen):** los ganchos `fetchWideTile` (~67 000 llamadas por frame) y los fondos
  que el juego pide enteros van en el hilo de la emulación; y un fondo con mosaico de tamaño 1 iba por el camino lento.
- **Hecho:**
  - `src/core/ppu.cpp`, `bgDrawnGeneric`: el mosaico manda al camino original solo si su tamaño pasa de 1 (a tamaño 1
    el original dibuja igual). `compare_ppu.sh`: 9020 frames iguales con 0 y con 2 hilos. En x86, SMB2 ancho con un
    hilo de 162 a 170 fps.
  - `docs/PERFORMANCE.md` §4, `docs/ROADMAP.md` (hito 3 hecho en 4:3; el widescreen y el co-op ancho, con prioridad en
    el hito 8), `AGENTS.md` §4, run 016.
- **Decisión:** el hito 3 queda hecho en lo que es del vídeo (imagen y colores correctos, coste mínimo, 60 en 4:3). El
  rendimiento del widescreen de SMB2/SMB3 y del co-op ancho es trabajo de la PPU y de los ganchos de los juegos, y se
  hace en el hito 8; no bloquea el hito 4.
- **Probado:** consola (run 016); la corrección del mosaico, en x86 frame a frame (falta medirla en consola: va en la
  próxima run).
- **Siguiente:** hito 4 (mandos y audio con SDL3).
