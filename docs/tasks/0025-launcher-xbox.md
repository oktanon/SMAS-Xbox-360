# 0025 — Launcher de Xbox: 4 juegos por defecto, previews más livianas, textos de Xbox

- **Fecha:** 2026-11-07
- **Objetivo:** lo que mostró la run 024: arrancar en la vista de los 4 juegos, quitar el lag de las previews,
  "Xbox 360 Edition", botones de Xbox en las líneas de ayuda, Hotkeys accesibles y con sentido en la consola.
- **Análisis del lag (run 024):** cada tarjeta es una consola SNES entera, como en PC (las previews en vivo de
  upstream). Con 4 tarjetas, 53 ms por pasada: la elegida corría hasta 3 frames por pasada para ponerse al día (una
  pasada lenta pedía más frames en la siguiente) y dibujaba en un solo hilo; las otras tenían un 35 % de cada
  refresco.
- **Hecho** (todo en `#ifdef SMAS_XENON`, el original en `#else`, salvo `app_xenon.cpp`):
  - `config.h`: `launcherAllStars = false` por defecto (las 4 tarjetas).
  - `launcher.cpp`: "Xbox 360 Edition"; las tarjetas no elegidas, un 10 % del refresco (un frame de una de ellas por
    pasada, así que van más lentas); `Launcher::previewThreads()`: la consola de la tarjeta elegida dibuja en los
    hilos de render (hilos hardware 2 y 4), las otras no. Nunca hay dos grupos de hilos de render a la vez (los que
    esperan giran: `Ppu::RenderPool`).
  - `app_xenon.cpp`: un frame por pasada como máximo en el launcher; los hilos de render son de la consola del juego
    al jugar y de la tarjeta elegida en el launcher; las líneas de ayuda con Ⓐ/Ⓑ (según Menu Buttons).
  - `app.cpp`: el aviso "Press Back for the menu"; `bindingsText` con los nombres de Xbox y solo el mando; Options >
    Hotkeys: filas a las que llega el cursor (las de solo lectura al final de la página no entraban en pantalla), sin
    Fullscreen ni Screenshot.
- **Probado:** compila. Xenia: arranca sin `.ini` en las 4 tarjetas, "Xbox 360 Edition". Los tiempos solo valen en
  la consola.
- **Estado:** pendiente de consola (run 025).
- **Siguiente:** run 025. Si las 4 tarjetas siguen pesadas: fotos fijas de las no elegidas o sus consolas en los
  hilos hardware libres (1 y 3).
