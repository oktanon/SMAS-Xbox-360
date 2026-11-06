# Run 014 — El dibujo en varios hilos en la consola (hito 2b, paso 2)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `6cef916` (tarea 0013); SHA1 en `paquete.sha1`.
- **Qué se probó:** `smas_hito2.xex` (`-O3`, PPU rápida, `SMAS_PORT_FIXES`): el guion de la run 004 dibujando con 2
  hilos y benchmarks de cada escena con 0-3 hilos de dibujo.
- **Resultado:** **OK, hito 2b cerrado.**
  - Las 22 líneas y las 9 salidas idénticas a x86 dibujando con 2 hilos (`comparacion.txt`).
  - Los hilos quedaron en los hilos hardware pedidos (2, 4, 3); la emulación en el 0.
  - Con 2 hilos: menú 107,6 fps, SMB1 95,8, SMB2 90,6, SMB3 96,4, widescreen 81,0, co-op 66,9 (sin hilos: 51-97).
    El tercer hilo resta. Tabla en `docs/PERFORMANCE.md` §3.3.
  - Rewind: guardar 5,8 ms, volver 17 ms.
- **Logs:** `smas_hito2.log`, `comparacion.txt`.
- **Siguiente:** hito 3 (vídeo con vulkan4xenon).
