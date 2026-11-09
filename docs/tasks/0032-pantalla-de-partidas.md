# 0032 — Hito 7: pantalla de partidas, states por juego, GPU en el log

- **Fecha:** 2026-11-08
- **Objetivo:** el usuario pasa al hito 7 (guardados). Pide una pantalla de partidas entre la tarjeta y el juego (la
  Xbox pasaba sola el panel de archivos del cartucho). Elige pantalla propia con los 4 archivos del cartucho y los
  states, y states separados por juego.
- **Run 031 (consola, logs del usuario):** el juego va a 599 pasadas cada 10 s con 580 frames (60 estables). Con
  el menú de pausa abierto y el juego quieto, 300 pasadas justas (30 fps) con 1 ms
  de trabajo; en la run 027 eran 600. La espera del fence da 0: la espera está en `vkAcquireNextImageKHR`. Lo más
  probable es la GPU (juego + fondo de los bordes + capa del menú pasan de un refresco), sin medir. Esta tarea agrega
  la medida.
- **Batería del cartucho (descifrada en su código con `smastool`, rutinas en `$00:8C6E`, `$00:8D83` y las de guardado
  de cada juego en `$00:8F08`, `$00:90B5`, `$00:917F`, `$00:9239`):**
  - 4 archivos de 0x120 bytes desde `$0010`. Cada uno tiene una entrada por juego, en este orden: SMB1 9 bytes, Lost
    Levels 8, SMB2 9, SMB3 0x106.
  - Cada entrada son sus datos y una palabra que deja en 0 la suma de los bytes.
  - El byte 0 es el mundo, base 0 (0xFF: archivo nuevo). En Lost Levels el byte 1 es el nivel. En SMB1 el byte 5 es
    la segunda vuelta (la estrella) y el byte 6, dos jugadores.
  - Valores por defecto en la ROM, en `$00:8DE3`.
  - Comprobado escribiendo valores y fotografiando el panel del cartucho: "3w 2P", "4w-3", "Dw-4", la estrella.
  - El cursor del panel es `$00D1` (0-3).
- **Hecho:**
  - `SmasGame::requestTitle(g, true, file)` (bajo `SMAS_XENON`): en el panel baja el cursor una pulsación a la vez
    hasta el archivo y luego pulsa Start como antes. Probado en nativo con el archivo C de los 4 juegos: SMB1 empieza
    en 5-1, Lost Levels en 5-2.
  - `app/file_select.*` (nuevo): lee y borra entradas de la batería (borrar copia los valores por defecto de la ROM y
    recalcula el checksum) y dibuja la pantalla:
    - a la izquierda, la cover de la tarjeta y el nombre del juego;
    - pestaña Files: A-D con el mundo, 1P/2P y la estrella de la segunda vuelta, o "New Game";
    - pestaña Save States: 3×3, con la foto y la fecha de cada ranura;
    - controles: LB/RB cambian de pestaña, A juega o carga, X borra un archivo (con confirmación), B vuelve.
  - App (bajo `SMAS_XENON`):
    - la tarjeta de un juego abre la pantalla; la de All-Stars arranca como siempre;
    - `MenuInput::aux` es el botón X;
    - los states van en `saves\states\<juego>\stateN.sst`, con `stateN.png` (128×112) al lado. El juego es el que
      corre: `allstars` si es el menú del cartucho;
    - cargar un state deja los 4 archivos de la batería como están (`$0010`-`$048F`), igual que el rewind. Así un
      state viejo no borra el progreso guardado después, en ese juego o en otro;
    - cargar un state desde la pantalla arranca el juego como `launchGame` y carga el state.
  - Log: una línea más cada 10 s, con el tiempo de GPU de un frame de cada 30 (desde que se envía hasta que la GPU lo
    termina) y la espera al refresco. Con eso se ve si el menú a 30 fps es la GPU.
- **Probado:**
  - Compila.
  - En nativo (`smastool` con el código del juego compilado con `SMAS_XENON` y un test fuera del repositorio):
    - el arranque elige el archivo pedido;
    - la lectura de la batería coincide con el panel del cartucho;
    - borrar las 16 entradas deja los archivos byte a byte como al primer encendido.
  - Xenia: arranca y abre la pantalla (log), pero no hubo captura: la sesión de Windows estaba bloqueada.
  - La pantalla y los states: pendientes de consola (run 032).
- **Nota:** los states viejos (`saves\stateN.sst`) ya no se ven. Para usar uno, se mueve a la carpeta de su juego.
- **Siguiente:**
  - Run 032.
  - Con su log, decidir qué hacer con el menú a 30 fps.
  - Después, el resto del hito 7: escritura atómica, `.srm` e ini al salir, la Guía pausa el juego, volver al
    dashboard.
