# 0034 — Cierre del hito 7; el log por juego para el hito 8

- **Fecha:** 2026-11-10
- **Run 033 (consola, logs del usuario):** archivos, estilos y Save & Quit al launcher, bien. El usuario propone
  cerrar el hito 7 y seguir con el 8: rendimiento justo en SMB2, SMB3 y 16:9.
- **Hito 7 cerrado.** Quedan sin hacer la escritura atómica de los guardados, que la Guía pause el juego y volver al
  dashboard: pasan al hito 10 (release).
- **Lo que dice el log de la run 033:** trabajo de 17-22 ms por pasada en buena parte del juego (más de 16,7 ms en
  250-450 pasadas de cada 500-600) y de 28-38 ms en un tramo; la GPU, unos 3 ms por frame. El trabajo es de la CPU,
  pero el log no dice en qué juego ni con qué aspecto, ni qué parte del trabajo es la emulación.
- **Hecho (bajo `SMAS_XENON`):** cada 10 s, con el juego corriendo, una línea más en `smas_port.log`:
  - el juego, los píxeles de más del widescreen (4:3 o 16:9), co-op, filtro y rewind;
  - la emulación por frame (la CPU, la PPU y los ganchos de los juegos; cuántos frames y cuántos dibujados);
  - el snapshot del rewind (se toma en el hilo de la emulación) y cuántos;
  - la interfaz y el vídeo (`render`) por pasada.
- **Probado:** compila sin avisos. La línea, en la run 034.
- **Siguiente:** run 034, los cuatro juegos en 4:3 y en 16:9; con eso, decidir dónde atacar (los ganchos de SMB2,
  `PERFORMANCE.md` §4; el rewind a otro hilo; el co-op).
