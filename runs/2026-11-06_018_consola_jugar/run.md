# Run 018 — Jugar con mando y sonido en la consola (hito 4)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB, 59,94 Hz.
- **Commit:** `8d16ba9` (tarea 0017); SHA1 en `paquete.sha1`.
- **Qué se probó:** `smas_jugar.xex` jugado por el usuario: SMB1 (uno y dos jugadores), SMB3, widescreen y 4:3, Guía.
- **Resultado:** **los mandos y el sonido funcionan; tirones de imagen notables.**
  - Mandos: los dos conectados; todos los botones probados en su sitio (el log anota la primera pulsación de cada
    uno). Dos jugadores, bien. La Guía pausa (`guía abierta: pausa`).
  - Sonido: "se escucha bien" (usuario); el kernel no recibió ningún frame en silencio.
  - **Tirones:** la emulación tarda más que sin sonido: SMB1 en 4:3 15,6 ms por frame (10,4 en la run 014), SMB1 ancho
    14,5-15,5 ms, co-op ancho 18 ms (600 de 600 frames de más), SMB3 ancho 16-20 ms. Cada frame de más pierde un
    refresco (FIFO): el tirón. Cuando la emulación va más lenta que el tiempo real, la cola de audio se vacía (hasta
    68 veces cada 600 frames) y SDL rellena con silencio.
  - **Causa:** el hilo del dispositivo de audio de SDL arranca en el hilo hardware de quien abre el dispositivo (el 0,
    el de la emulación) con prioridad +1 (SDL-xenon `docs/README-xsdk.md`) y le quita tiempo. SDL-xenon tiene el hint
    `SDL_HINT_AUDIO_XSDK_HARDWARE_THREAD` para moverlo.
  - Back + Start saca una captura en el dashboard del usuario: no sirve para salir.
- **Logs:** `smas_jugar.log`.
- **Siguiente:** run 019 con el hilo de audio en el hilo hardware 5 (tarea 0018).
