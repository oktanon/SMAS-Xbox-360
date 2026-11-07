# 0024 — El launcher y la App entera en la consola (hito 5)

- **Fecha:** 2026-11-07
- **Objetivo:** portar el launcher: que `SuperMarioAllStars.xex` sea la App de upstream (`src/app`), como la compila
  Linux, con capas de Xbox.
- **Hecho:**
  - `src/platform/xenon/video_xenon.h/.cpp`: la clase `Video` de `video_gles.h` (sin ventana) y `XenonCanvas`, que
    implementa `Canvas` método a método como `GlesCanvas`. Un pipeline de quads por lotes (`shaders/canvas.vert/.frag`
    → `canvas_shaders.c/.h` con xost: selects en lugar de ramas), índices de 16 bits, el color del vértice como bytes
    `R8G8B8A8`, recortes con scissor dinámico. Lo que la App pide en el frame se graba y `present()` hace un command
    buffer: las copias de las texturas que cambiaron (staging por slot, filas de 256 bytes, alineado a 4 KB) y una
    pasada. Texturas liberadas: se destruyen dos frames después. Imágenes `0xAARRGGBB`: color `(g, b, a)`, alfa `r`.
    La imagen del juego es una textura más (704×240), dibujada con su `fade`; filtros, color y bordes difuminados
    de `VideoParams` → hito 6; `capture()` → hito 7.
  - Texto: `stb_truetype` (`third_party/stb/stb_truetype.h`, v1.26, dominio público / MIT) con las métricas de
    FreeType; fuentes dentro del `.xex` (`fonts_xenon.cpp`, `.incbin`): Luckiest Guy (ya en upstream) y DejaVu Sans y
    Sans Bold (`third_party/fonts`, con su licencia, `DejaVu-LICENSE.txt`).
  - `src/platform/xenon/app_xenon.cpp`: de `app_linux.cpp`: `main`, `App::init/run/shutdown`, ritmo, rutas `game:`,
    ROM junto al `.xex` o en `roms`, sin selectores ni red (`httpGet` falso), Guía = pausa, Back + LB + RB 3 s = salir,
    hilos de dibujo en los hilos hardware 2 y 4, tiempos cada 10 s en un log en memoria.
  - `src/app/app.h`: con `SMAS_XENON`, las cabeceras de Xbox. `src/app/app.cpp` (bajo `SMAS_XENON`, el original en
    `#else`): el stick por sectores (`Input::stickToDpad`) y `setRenderThreads(2)` al crear la consola del juego.
  - `xenon/CMakeLists.txt`: `SuperMarioAllStars` = las fuentes de `src/app` de Linux + `touch.cpp` + la capa xenon,
    versión 0.5.0.0; el título del hito 4 pasa a `smas_jugar.xex`.
  - `AGENTS.md` §2, §3, §4 (Xenia) y §5.
- **Probado:** compila sin avisos. Xenia (run 024): el launcher se dibuja entero con la preview en vivo y colores
  correctos, sin cerrarse. Xenia se cerraba sin `--break_on_unimplemented_instructions=false`: `App::menuInput`
  lleva `rldic`, que Xenia no implementa (la consola sí): la navegación de menús no vale en Xenia.
- **Estado:** pendiente de consola (run 024): imagen, fluidez del launcher (una tarjeta y cuatro), sonido, entrar a
  jugar y volver.
- **Siguiente:** la run 024; luego el hito 6 (menús y opciones de Xbox, filtros).
