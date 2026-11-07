# 0027 — Bordes difuminados, ritmo fijo, rewind en otro hilo

- **Fecha:** 2026-11-07
- **Objetivo:** lo de la run 026: barras negras un instante al salir de tuberías en widescreen, y "no se siente
  como 60 fps" aunque el contador diga 60.
- **Análisis:**
  - Barras: el juego vuelve a 4:3 a propósito en algunas transiciones (salas cerradas, salida de tubería:
    `Smb1Engine::frameStart`, `show`). En PC la imagen mantiene su tamaño y los costados los cubre Side Borders
    "Blurred Game" (por defecto); en la Xbox `Video::draw` aún no dibujaba los bordes.
  - Lag: jugando en widescreen con rewind, 450-530 pasadas cada 10 s (de 600): muchas de más de 16,7 ms; cuando una
    se pasa, la siguiente emula 2 frames y dibuja solo el último, así que el contador (frames emulados) dice 60 y se
    ven menos. Dos causas medidas o vistas: guardar un snapshot de rewind cuesta 5,8 ms en el hilo principal cada 2
    frames (hito 2b), y el ritmo de `app_xenon` medía la pantalla con la mediana de las pasadas, que con pasadas
    lentas se iba de 60 Hz y cambiaba entre "un frame por refresco" y "el tiempo debido" (89 cambios en el log),
    con pasadas de 0 y 2 frames: tirones.
- **Hecho:**
  - `video_xenon.cpp`: Side Borders "Blurred Game": la imagen promediada en la CPU a 48×27 (4×4 muestras por celda),
    difuminada (1 2 1), oscurecida ×0,32 y con viñeta como `video_gles.cpp`, dibujada estirada con filtro lineal
    detrás de la imagen cuando esta no cubre la pantalla. "Dark Frame" ya lo dibuja la app.
  - `src/core/rewind.cpp/.h` (bajo `SMAS_XENON`, el original en `#else`): `push` entrega el snapshot a un hilo propio
    (hilo hardware 1, `Rewind::threadStart` desde `app_xenon.cpp`) que calcula la diferencia; `clear`, `stepBack` y
    `newest` esperan a que termine; `snapshots()` no espera (contador atómico). En el hilo principal queda
    `saveState()` (copias de memoria).
  - `app_xenon.cpp`: ritmo fijo a 59,94 Hz (el modo único de la consola), un frame por refresco; el log de 10 s cuenta
    los frames emulados, los snapshots del rewind y el trabajo sin la espera de la GPU (`Video::Stats::fenceUs`).
- **Probado:** compila (también `smas_hito2`). Xenia: arranca y el ritmo se fija una sola vez. Bordes, fluidez y
  rewind: pendientes de consola (run 027). Pendiente aparte (hito 8): el widescreen de SMB2/SMB3 sigue pesado.
- **Siguiente:** run 027.
