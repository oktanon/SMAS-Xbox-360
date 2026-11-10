# 0036 — El archivo que arranca el cartucho, en el log

- **Fecha:** 2026-11-10
- **Run 035 (consola):**
  - El snapshot del rewind baja a 1,2-1,5 ms (tarea 0035). En 16:9, SMB1 y Lost Levels ya casi no pierden frames;
    SMB2 y SMB3 sí.
  - Fallo: el usuario eligió en SMB1 un archivo de un jugador en el mundo 4 y entró en otro, de dos jugadores en el
    mundo 1.
- **Probado en x86 (`smastool`):** la batería del usuario, con un archivo B de dos jugadores en el mundo 1 añadido
  (A, mundo 4, un jugador; último jugado: B).
  - Elegir A o B en el primer arranque, y en el segundo (desde la foto del título) en los cuatro órdenes, y A con
    co-op: siempre entra en el archivo elegido. A empieza en el mundo 4 con un jugador; B, en el mundo 1 con dos.
  - El cursor del panel (`$00D1`) lo pone el cartucho en el último archivo jugado cuando acepta Start en el título,
    mucho antes de que el port lo mueva. El panel acepta Abajo cada 12 frames, y el port lo lleva hasta el archivo y
    pulsa Start ahí.
  - No se reproduce.
- **Hecho (bajo `SMAS_XENON`):** al terminar un arranque con archivo, una línea en `smas_port.log` con el archivo
  pedido, el archivo donde estaba el cursor del panel cuando se pulsó Start, el juego al que entró, y si el co-op
  estaba activado. `SmasGame::startedFile`.
- **Probado:** compila sin avisos. En consola, run 036.
- **Siguiente:** con esa línea, ver si el cartucho recibió otro archivo o si fue otra cosa; luego, SMB2 y SMB3 en
  16:9.
