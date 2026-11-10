# Run 035 — El snapshot del rewind más rápido (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0035; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8b/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Cargar un state guardado con la versión anterior: ¿carga? (El formato no cambia.)
  2. Guardar un state nuevo y cargarlo.
  3. Mantener el rewind un momento: ¿va bien?
  4. Como en la run 034: 30-40 s de SMB2 y de SMB3 en 16:9, y un rato de co-op de SMB1 en 16:9.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Resultado:** Parcial.
  - El snapshot del rewind baja de ~3 ms a 1,2-1,5 ms. En 16:9 se dibujan 500-600 de cada 600 frames en SMB1 y
    Lost Levels, 400-595 en SMB3 y 200-510 en SMB2, que sigue lento. La emulación en 16:9 sigue en 14-15 ms.
  - **Fallo que reporta el usuario:** en SMB1 tenía un archivo de un jugador en el mundo 4 y otro de dos jugadores
    en el mundo 1. Eligió el del mundo 4 y entró en el de dos jugadores.
  - La batería que dejó (`saves`, no se sube) tiene ahora solo el archivo A de SMB1, mundo 4, un jugador. El último
    archivo jugado es el B. La sesión empezó con co-op activado (venía de la run 034).
- **Logs:** `smas_port.log`, `supermarioallstars.log`.
- **Siguiente:** tarea 0036, run 036.
