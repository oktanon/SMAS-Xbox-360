# Run 042 — La APU en su propio hilo (hito 8)

- **Fecha:** 2026-11-11
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0043; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8i/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Con el rewind activado: 30-40 s de SMB3 en 16:9, un rato de co-op de SMB1 en 16:9, un poco de SMB2 y SMB1.
  2. Escuchar el sonido: música y efectos como siempre, sin cortes ni desfases. En el co-op de SMB1, morir con un
     jugador y ver que la música vuelve bien.
  3. Guardar y cargar un state, y mantener el rewind un momento.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** "CPU, APU y juegos" y "frame saltado" (antes ~5,0 ms solos y ~7,4 en co-op).
- **Resultado:** pendiente.
- **Siguiente:** según la run.
