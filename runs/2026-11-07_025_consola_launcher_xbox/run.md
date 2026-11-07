# Run 025 — Launcher de Xbox: 4 juegos por defecto, previews más livianas, textos de Xbox

- **Fecha:** 2026-11-07
- **Dónde:** Xenia (humo) y consola (RGH)
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
- **Resultado:** Parcial. Mejor que la 024: el launcher con 4 tarjetas pasa de 53 a 21-25 ms por pasada (50-70
  refrescos perdidos cada 10 s; aún no llega a 60). Jugando, 0 refrescos perdidos (los 16,6 ms de "trabajo" incluyen
  la espera de la GPU dentro del frame). Los states funcionan. Usuario: Ⓐ/Ⓑ salen como "?" (DejaVu no tiene letras
  en círculo); pide hotkeys reasignables y Back + RB / Back + LB para guardar/cargar state; Jugar lleva a la pantalla
  de archivos del juego original (como en PC); preguntas sobre states, covers y packs de texturas (tarea 0026).
- **Logs:** `smas_port.log`, `supermarioallstars.log`, `smas_port.ini`, `achievements.ini`, `rompath.txt`.
- **Siguiente:** tarea 0026 y run 026.
