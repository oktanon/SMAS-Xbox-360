# 0044 — Hito 8 cerrado: run 042, el hilo de la APU apagado

- **Fecha:** 2026-11-12
- **Run 042 (consola, tarea 0043):** el hilo de la APU **empeora** frente a la run 041:
  - "CPU, APU y juegos": ~5,75 ms frente a ~5,2 con un jugador, ~8,6 frente a ~7,5 en el co-op de SMB1;
  - el snapshot del rewind: ~1,9 ms frente a ~1,0 (espera a que la APU esté quieta);
  - la emulación y la APU comparten núcleo, y la CPU espera a la APU en los puertos, en los ganchos del co-op y en
    los states: más esperas que trabajo en paralelo. El rewind sale barato porque nadie lo espera.
- **Hecho:**
  - **`app_xenon.cpp`:** `setApuThread(false)` en la consola. El código queda bajo `SMAS_APU_THREAD` (smastool:
    `apuhilo 1`); sin el hilo, el núcleo es el de la run 041 (comprobado en la tarea 0043: igual que HEAD).
  - **Hito 8 cerrado** con la tabla de `PERFORMANCE.md` §12: SMB1, SMB2 y SMB3 en 16:9 con rewind a ~14,5-15 ms
    por frame dibujado, co-op de SMB1 ~16 ms. Suficiente para una primera versión con el co-op de SMB1 y SMB2.
  - **Hitos 5 y 6 cerrados:** el launcher y el menú de pausa se usan en la consola en todas las runs desde la 024 y
    la 031.
  - **Hito 9 (logros nativos) pospuesto**, por decisión del usuario: se pasa al hito 10.
  - **Hito 10:** el usuario confirma en la consola que la Guía pausa el juego y que se vuelve al dashboard. El
    fallo pospuesto de SMB1 (run 035, tarea 0036) no existió: fue un error al leer la prueba.
- **Probado:** compila para la consola sin avisos. Sin run propia: la run de instalación limpia del hito 10 lo
  cubre.
- **Siguiente:** hito 10: escritura atómica de los guardados, README en inglés más corto con los avisos de
  licencias, paquete de release.
