# 0020 — Run 020; hito 4 hecho; el log del juego, en memoria

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 020 (el usuario: sticks más responsivos, salir con Back + LB + RB funciona, siguen los
  tirones incluso con un jugador, la batalla de SMB3 a dos es lo peor) y decidir.
- **Análisis:** 4:3 con un jugador sin frames de más (9,6-12 ms); 16:9 SMB1 13-14,5 ms con tramos de 13-55 frames de
  más; SMB3 16:9 98-168 de más; batalla de SMB3 en 16:9 40-55 ms por frame. En casi cada tramo la cola de audio se vacía
  exactamente una vez: la línea del log escrita cada 600 frames en el USB costaba un refresco que el log no veía.
- **Hecho:**
  - `src/platform/xenon/play_main.cpp`: el log del bucle va a memoria (`logf`) y se escribe al abrir la Guía, al salir
    o pasados 32 KB; cada tramo cuenta los refrescos perdidos (intervalos entre presents de más de 1,5 refrescos).
  - Movido el log de la run 020 a su carpeta y recuperado el de la 019.
  - `docs/ROADMAP.md` (hito 4 hecho; el hito 8 con la batalla de SMB3 y la medida de refrescos perdidos),
    `AGENTS.md` (§4: no escribir en el USB durante el juego), runs 020 y 021.
- **Decisión:** el hito 4 queda hecho: mandos de uno y dos jugadores, stick por sectores, sonido sin cortes propios,
  Guía, salida. Los tirones son del rendimiento de la PPU y de los ganchos de widescreen de los juegos en el hilo de la
  emulación (`docs/PERFORMANCE.md` §4), y del co-op: hito 8.
- **Probado:** compila. La run 021 (opcional) mide con el log arreglado.
- **Siguiente:** propuesta al usuario: adelantar el hito 8 (widescreen, co-op, batalla) antes del launcher.
