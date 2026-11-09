# 0033 — Hito 7: Save & Quit al launcher, partidas con el estilo de los menús, carga más rápida

- **Fecha:** 2026-11-09
- **Run 032 (consola, logs y fotos del usuario):** los archivos A-D y los states funcionan. El menú de pausa va a
  559 pasadas en 10 s con 10 ms de GPU (en la run 031 eran 300): el 30 fps no se repitió.
- **Pedido:** al elegir Save & Quit en el menú de pausa del propio juego, volver al launcher y no al menú de
  All-Stars; la pantalla de partidas con el estilo de los menús (colores, fuentes, bordes, según el estilo elegido);
  cargar más rápido o mostrar en la carga el juego y el mundo.
- **Medido (`smastool` nativo):**
  - Un arranque desde el launcher son ~770 frames (unos 12 s), 806 en SMB2. ~500 son el encendido, el logo de
    Nintendo y la espera del título hasta que acepta Start.
  - Save & Quit en SMB1: el juego escribe su entrada de la batería al confirmar, todavía como SMB1 (`$7FFF00` = 2).
    Unos 60 frames después pasa al menú (`$7FFF00` = 0).
  - El cartucho llena una tabla del progreso de los archivos en `$0080`-`$00CF` al aceptar Start en el título
    (rutina en `$00:A324`). Por eso una foto del menú de juegos mostraba los archivos de antes.
- **Hecho (todo bajo `SMAS_XENON`):**
  - `SmasGame`: el primer arranque después de un reset guarda una foto (state en memoria) del título en el Start
    que se aceptó (antes de que se llene la tabla). Los siguientes arranques la cargan, con la batería y la corrida de
    logros de ahora, y siguen pulsando Start como entonces. Arranque: ~415 frames en vez de ~770.
  - Los frames del arranque no se dibujan (`skipRender`; la pantalla ya era negra). El último tampoco: esa pasada
    se muestra negra (`loadEnded`).
  - Save & Quit (o el reset del juego) en un juego que arrancó el launcher: cuando el menú de All-Stars lleva 20
    pasadas, vuelve al launcher (`returnToLauncher` guarda la batería). No con la tarjeta All-Stars, ni con un state
    del menú cargado.
  - `Menu::skin`, `drawFrame`, `drawBox`, `drawRule`: el panel, el título, la selección y las reglas de cada estilo,
    para pantallas propias.
  - La pantalla de partidas los usa: Storybook (tarjeta crema, selección amarilla), Retro (negro, ladrillos, letra
    de píxeles), Themed Dark (el dibujo del juego tras el título) y Classic. El color es el acento del menú (el de la
    tarjeta), y el título es el nombre del juego.
  - Pantalla de carga: un panel en el mismo estilo, con la cover y el nombre del juego, el mundo del archivo
    elegido ("World 5", "New Game") o el nivel del Level Select, "File C" debajo y tres estrellas saltando. Sustituye
    el "Loading..." de la esquina.
- **Probado:**
  - Compila sin avisos.
  - En nativo:
    - arranques con la foto en los cuatro juegos: 409-428 frames;
    - el archivo C cambiado en la batería después del primer arranque se juega en el mundo 5 (con la foto del menú
      de juegos salía el mundo 1; con la del título, bien).
  - En Xenia (con un disparador de prueba que no queda en el código): la pantalla de partidas en Storybook, Retro y
    Themed Dark, la pestaña de states, y la tarjeta de carga.
  - Save & Quit en la app y la velocidad en consola: pendientes (run 033).
- **Siguiente:** run 033; después, escritura atómica, la Guía pausa el juego, volver al dashboard.
