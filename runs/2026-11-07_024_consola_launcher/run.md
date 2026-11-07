# Run 024 — El juego entero con su launcher (hito 5)

- **Fecha:** 2026-11-07 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Xenia (humo, hecho) y consola (RGH, pendiente)
- **Commit:** tarea 0024; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5/SuperMarioAllStars.xex`).
- **Qué se prueba:** `SuperMarioAllStars.xex` es ahora la App de upstream entera (launcher, menús, logros, música)
  sobre `video_xenon` (Canvas con vulkan4xenon, texto con stb_truetype), `audio_sdl3` e `input_sdl3`. Copiar el `.xex`
  sobre el de antes, junto a la ROM.
- **Xenia:** arranca, carga la ROM, dibuja el launcher (logo, títulos en Luckiest Guy, texto en DejaVu, la tarjeta
  redondeada) con la preview en vivo y colores correctos, 40 s sin cerrarse. Sin mando ni tiempos válidos.
- **Pedir en consola:**
  1. Que el launcher se vea como en la foto del PC y se mueva fluido; que suene la música de la preview.
  2. Elegir el juego con A, jugar un rato; abrir el menú (Back) y volver al launcher.
  3. Options > Launcher > "The Four Games" (cuatro tarjetas con previews): ver si va fluido.
  4. Salir con Quit del launcher (o Back + LB + RB 3 s).
- **Devolver:** `smas_port.log` (la app y, cada 10 s, los tiempos: se escriben al abrir la Guía o al salir) y
  `SuperMarioAllStars.log`; fotos si algo se ve mal.
- **Resultado:** pendiente.
- **Siguiente:** —
