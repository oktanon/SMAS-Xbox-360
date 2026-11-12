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
- **Orden:** SMB3, SMB2 y SMB1 en 16:9 con rewind; después, co-op de SMB1 en 16:9 con rewind (varios filtros).
- **Resultado:** OK, medido. **El hilo de la APU empeora** (frente a la run 041):
  - "CPU, APU y juegos" con un jugador: ~5,75 ms frente a ~5,2;
  - en el co-op de SMB1: ~8,6 ms frente a ~7,5; frame dibujado ~17,6 ms frente a ~16,0;
  - el snapshot del rewind: ~1,9 ms frente a ~1,0 (espera a que la APU esté quieta);
  - tramos con el audio vacío alguna vez: 28 frente a 20.
  - La emulación y la APU comparten núcleo, la CPU espera a la APU en cada acceso a los puertos y los ganchos del
    co-op la sincronizan varias veces por frame: más esperas, poco paralelo de verdad.
- **Siguiente:** tarea 0044: el hilo de la APU apagado en la consola; el hito 8 cerrado.
