# Run 038 — Las lecturas de la CPU por páginas y el log por frame dibujado (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0039; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8e/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Como en la run 037: 30-40 s de SMB3 en 16:9 y un rato de co-op de SMB1 en 16:9 (y, si se puede, en 4:3);
     un poco de SMB2 en 16:9.
  2. Que todo juegue igual: un nivel con co-op, un state guardado y cargado, el rewind un momento.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** la línea nueva "frame dibujado ... ms: PPU en este hilo ... (esperando ...), CPU, APU y
  juegos ...; frame saltado ...".
- **Resultado:** pendiente.
- **Siguiente:** según dónde se vaya el frame dibujado de SMB3 y del co-op.
