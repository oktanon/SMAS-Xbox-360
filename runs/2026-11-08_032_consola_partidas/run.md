# Run 032 — Pantalla de partidas y states por juego (hito 7)

- **Fecha:** 2026-11-08
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0032; SHA1 en `paquete.sha1` (`build-xenon/consola/hito7a/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes (la carpeta `saves` se queda).
- **Pedir:**
  1. A en la tarjeta de un juego: se abre la pantalla de partidas, con la cover, los archivos A-D (mundo, 1P/2P,
     estrella o "New Game") y, con LB/RB, los states. ¿Los mundos coinciden con lo que muestra el panel del cartucho
     (tarjeta All-Stars)?
  2. Elegir un archivo con progreso y otro nuevo: ¿entra al juego con ese archivo?
  3. En el juego, guardar un state (Back + RB), volver al launcher y entrar por los states: ¿aparece con su foto y
     fecha, y lo carga?
  4. Borrar un archivo con X (en uno que no importe): pide confirmación y queda "New Game".
  5. Abrir el menú de pausa en el juego unos 20 s quieto (para la medida de la GPU).
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`; fotos de la pantalla de partidas.
- **Resultado:** pendiente.
- **Siguiente:** según el resultado.
