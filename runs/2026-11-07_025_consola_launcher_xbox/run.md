# Run 025 — Launcher de Xbox: 4 juegos por defecto, previews más livianas, textos de Xbox

- **Fecha:** 2026-11-07 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Xenia (humo, hecho) y consola (RGH, pendiente)
- **Commit:** tarea 0025; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5b/SuperMarioAllStars.xex`).
- **Qué se prueba:** los cambios de la tarea 0025 sobre la run 024. Copiar el `.xex` sobre el de antes. Para ver el
  arranque por defecto en los 4 juegos, borrar antes `smas_port.ini` de la consola (si no, vale lo que guardó).
- **Xenia:** sin `smas_port.ini` arranca con las 4 tarjetas; el título dice "Xbox 360 Edition".
- **Pedir en consola:**
  1. El launcher con las 4 tarjetas: ¿va fluido al moverse entre tarjetas? Las que no están elegidas pueden ir más
     lentas (a propósito).
  2. Abajo: "←→ Choose  Ⓐ Play  Ⓑ Quit"; en los menús, "Ⓐ Confirm  Ⓑ Back".
  3. Options: bajar hasta el final y ver la sección Hotkeys (Menu, Save state, Load state, Fast forward, Rewind).
  4. Jugar un minuto (para medir el juego sin la carga) y volver al launcher.
- **Devolver:** `smas_port.log` (abrir la Guía o salir antes, para que se escriban los tiempos) y
  `SuperMarioAllStars.log`.
- **Resultado:** pendiente.
- **Siguiente:** —
