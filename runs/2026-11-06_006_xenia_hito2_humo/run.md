# Run 006 — Prueba de humo de smas_hito2.xex en Xenia

- **Fecha:** 2026-11-06
- **Dónde:** XSDK Xenia (no es evidencia; solo para ver que arranca)
- **Commit:** tarea 0007
- **Qué se probó:** `smas_hito2.xex` con la ROM y `guion.txt` al lado, 75 s y cerrado a mano.
- **Resultado:** arranca, lee la ROM y el guion de `game:\` y avanza por menú, SMB1 y Lost Levels. Las 7 líneas que dio
  tiempo a escribir coinciden con la run 004, y `menu.png`, `menu.wav`, `smb1.png` y `ll.png` (de una primera prueba
  igual) son idénticos a los de x86 (`archivos_xenia.sha1`). Es decir, el GCC de xsdk y newlib dan lo mismo en Xenia,
  audio incluido. En la primera prueba el log quedó vacío por el búfer de `stdout`: ahora va línea a línea.
- **Aviso:** `Smb3Engine::planFill` tiene 10 instrucciones que Xenia ignora (xenia_scan): en Xenia, SMB3 no es fiable.
- **Siguiente:** run 007 en la consola.
