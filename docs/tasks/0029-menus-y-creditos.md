# 0029 — Hito 6: créditos del port a Xbox 360 y opciones de solo PC

- **Fecha:** 2026-11-08
- **Objetivo:** cerrar el hito 6 (menú de pausa completo). Según el usuario ya funcionan los states, un cheat de SMB1,
  el Music Player y Options; pide un crédito del port a Xbox 360.
- **Hecho** (`app.cpp`, todo bajo `SMAS_XENON` con el original en `#else`):
  - Credits: subtítulo "on the Xbox 360"; sección "Xbox 360 port" (Ported by: estan) y "Built with" (xsdk,
    vulkan4xenon, SDL-xenon, stb_truetype, DejaVu Sans con la mención de Bitstream que pide su licencia); los
    enlaces de Darhkfox se muestran sin abrir nada (no hay navegador; antes decía "Opened … in your browser").
  - Fuera en la Xbox: la sección Updates (sin red; `checkForUpdate` no hace nada) y "Take Screenshot" (`capture()`
    aún no está).
  - "Open Covers Folder" / "Open MSU-1 Folder" → "Create Covers Folder" / "Create MSU-1 Folder" (crean la carpeta
    junto al `.xex`); los textos de las covers dicen dónde van.
- **Run 028 (Xenia):** el stick derecho y Pixel Shape funcionan; sin problemas de sonido notados. Repetir en consola.
- **SMW (consulta):** pedir la ROM de Super Mario World aparte (la suelta, no la combinada) como un quinto juego que
  corre en su propia consola SNES, sin las mejoras del port: posible; ver la respuesta en el chat y la tarea 0028.
- **Probado:** compila. Level Select y Sound Select: pendientes (run 029). Filtros del juego: pendientes.
- **Siguiente:** run 029.
