# Run 022 — El nombre del juego en Aurora (hito 4)

- **Fecha:** 2026-11-06 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Consola (RGH), Aurora
- **Commit:** tarea 0021; SHA1 en `paquete.sha1`.
- **Qué se prueba:** `SuperMarioAllStars.xex` (antes `smas_jugar.xex`) con su ID de título `534D4153` y el nombre
  "Super Mario All-Stars" en el recurso XDBF del `.xex`. El log empieza con `PASS: título 534D4153 ...` si la consola ve
  el nombre.
- **Esperado:** Aurora muestra "Super Mario All-Stars" (puede hacer falta quitar la entrada vieja y volver a escanear:
  Aurora guarda lo que escaneó). En Xenia (tarea 0021) la ventana ya muestra `[534D4153 v0.4.0.0] Super Mario
  All-Stars` y la comprobación pasa.
- **Resultado:** pendiente — qué nombre muestra Aurora y `SuperMarioAllStars.log` (con medidas de juego, si se juega).
- **Siguiente:** —
