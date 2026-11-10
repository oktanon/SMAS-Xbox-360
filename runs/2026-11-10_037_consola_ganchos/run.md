# Run 037 — Los ganchos del widescreen en los hilos de dibujo (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0038; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8d/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Como en las runs 034 y 035: 30-40 s de SMB2 y de SMB3 en 16:9 (si es posible, una zona vertical de SMB2), un
     rato de SMB1 en 16:9 y de co-op de SMB1 en 16:9.
  2. Ver que la imagen de los márgenes está bien (sin tiles rotos ni parpadeos).
  3. Mantener el rewind un momento, y cargar un state de antes.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** "emulación ... ms por frame" en 16:9 (antes 14-15 ms) y "snapshot del rewind" (antes 1,2-1,4).
- **Resultado:** pendiente.
- **Siguiente:** según el resultado, el co-op o lo que queda en serie en el hilo de la emulación (tarea 0038).
