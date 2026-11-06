# Run 016 — El vídeo en la consola (hito 3)

- **Fecha:** 2026-11-06 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0015; SHA1 en `paquete.sha1`.
- **Qué se prueba:** `smas_video.xex`: la prueba de color (4 barras, leídas del front buffer), luego el guion de la
  run 004 con 2 hilos de dibujo y una demostración en widescreen 64 y co-op, cada frame en la pantalla (FIFO, 60 Hz).
- **Esperado:** `PASS` en las 4 barras; las 22 líneas iguales a la run 004; la imagen bien en la tele (4:3 con bandas,
  luego widescreen); en las líneas `VIDEO`, la emulación + vídeo por debajo de 16,7 ms y pocos o ningún frame pasado.
- **Resultado:** pendiente — copiar aquí `smas_video.log` y, si se puede, una foto de la tele en 4:3 y en widescreen.
- **Siguiente:** —
