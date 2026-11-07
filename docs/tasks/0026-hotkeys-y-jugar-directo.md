# 0026 — Hotkeys reasignables, states con Back + RB / LB, Jugar directo al juego

- **Fecha:** 2026-11-07
- **Objetivo:** lo que pidió el usuario tras la run 025.
- **Hecho** (en `#ifdef SMAS_XENON`, el original en `#else`, salvo `app_xenon.cpp` y `video_xenon.cpp`):
  - Ⓐ/Ⓑ salían como "?": DejaVu Sans no tiene letras en círculo. Las ayudas usan "A"/"B" y el Canvas dibuja Ⓐ/Ⓑ como
    A/B donde aparezcan (`findGlyph`).
  - Options > Hotkeys: cada fila, con A, espera un botón del mando (`startBind`); el texto de captura de Xbox no habla
    de Esc ni Delete. Save State y Load State son combinaciones: Back + su botón (Back + RB y Back + LB por defecto,
    `config.cpp`); al asignarlas, Back no cuenta como el botón. Un `smas_port.ini` sin ninguna de las dos toma las de
    por defecto (`app_xenon.cpp`).
  - `App::readPlayer`: con Back apretado, sus botones no llegan al juego; Back solo abre el menú al soltarlo (si no
    formó una combinación ni se pulsó en el menú o el launcher).
  - Jugar desde una tarjeta: `requestTitle(g, true)`, como las previews: pasa la pantalla de archivos con el archivo
    marcado y llega al título del juego. En PC se para en esa pantalla para elegir el archivo.
  - `docs/TEXTURE_PACKS.md`: investigación de packs de gráficos (pregunta del usuario; sin implementar).
- **Respuestas al usuario:** los states son de la consola entera (`saves/stateN.sst`, uno por ranura, el mismo para
  los 4 juegos: cargar uno guardado en otro juego vuelve a ese juego). Las covers: carpeta `covers` junto al `.xex`,
  `smb1`, `lostlevels`, `smb2`, `smb3`, `allstars`, `background` en `.png` o `.jpg`, con Options > Launcher > Game
  Covers activado.
- **Probado:** compila; Xenia arranca y dibuja el launcher. Combinaciones, captura y el arranque directo: pendientes
  de consola (run 026).
- **Siguiente:** run 026.
