# Run 028 — 4:3 con el stick derecho, ritmo que mantiene la velocidad

- **Fecha:** 2026-11-08
- **Dónde:** Xenia (el usuario no tenía la consola; logs en `xenia/`)
- **Commit:** tarea 0028; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5e/SuperMarioAllStars.xex`).
- **Qué se prueba:** los cambios de la tarea 0028. Copiar el `.xex` sobre el de antes.
- **Pedir en consola:**
  1. Jugando, pulsar el stick derecho: cambia entre 4:3 y widescreen (aviso en pantalla). También en Options >
     Display > Aspect Ratio > "4:3 (Original)".
  2. Options > Display > Pixel Shape en "CRT (8:7)" (el de por defecto) y jugar en widescreen; luego en "Square" para
     comparar.
  3. ¿El sonido se corta menos y el juego va a su velocidad?
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` (abrir la Guía o salir antes).
- **Resultado:** OK en Xenia, pendiente de consola. El stick derecho cambia entre 4:3 y widescreen; Pixel Shape
  probado en los dos modos; sin problemas de sonido notados. Tiempos de Xenia: no valen para la consola (AGENTS.md §4).
- **Logs:** `xenia/smas_port.log`, `xenia/SuperMarioAllStars.log`, `xenia/smas_port.ini`.
- **Siguiente:** tarea 0029 (hito 6) y run 029; repetir la 028 en consola cuando se pueda.
