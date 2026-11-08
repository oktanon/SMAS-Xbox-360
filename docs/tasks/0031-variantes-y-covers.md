# 0031 — Shader del juego en variantes, colores por tabla, covers del selector de la ROM

- **Fecha:** 2026-11-08
- **Objetivo:** lo de la run 030: el juego iba peor con o sin filtros y Mario desaparecía al parpadear; el usuario
  pide de covers por defecto las cajas del selector de juegos de la ROM, recortadas solas la primera vez.
- **Análisis (run 030):** con el menú abierto y nada emulado salían 250-280 pasadas cada 10 s (600 antes de la tarea
  0030) con 1 ms de trabajo: la GPU tardaba más de un refresco por frame. El shader único de la 0030 llevaba todos los
  filtros y colores con `if`; en Xenos sus registros (los del brazo que más usa) dejan pocos píxeles en vuelo para
  esconder las lecturas de textura, así que hasta Nearest era caro. A 30 imágenes por segundo cada una trae 2 frames
  emulados (y el juego quedaba algo lento): el parpadeo de Mario tras un golpe es un frame sí y otro no, y se veía
  siempre el frame en el que no está.
- **Hecho:**
  - `shaders/game.glsl` con `FILTER` (los 6 filtros y el fondo de los bordes), `COLOR` y `CURVE`, y 26 variantes
    mínimas en `shaders/game/` (cada una solo `#define` e `#include`); un pipeline por variante en `video_xenon.cpp`,
    elegido en cada draw. Por defecto (Sharp Bilinear, colores sin tocar, sin curva) el shader es casi el de antes de
    la 0030.
  - Colores: todo `applyColor` (brillo, contraste, saturación, gamma, perfil, temperatura, daltonismo) depende solo del
    color, así que la CPU lo calcula para 32×32×32 colores cuando cambian los ajustes (una textura de 1024×32) y el
    shader lo lee con 2 muestras. La lectura de punto se hace en el centro del texel con el muestreador lineal (queda
    libre el binding para la tabla).
  - El log suma "espera a la GPU" (la espera del fence de cada frame): alta si la GPU es la que frena.
  - Covers por defecto (`app/select_covers.*`, `launcher.*` bajo `SMAS_XENON`): la primera vez, una consola aparte
    corre el cartucho desde el encendido, unos 3 ms por pasada del launcher en el hilo principal (los buffers de
    línea de `ppu.cpp` son estáticos: no puede ir en otro hilo). Toma la pantalla de título (cover de la tarjeta
    All-Stars), pulsa Start y, en el selector, recorta la caja centrada cuando se queda quieta: arriba por la columna
    del medio, derecha y abajo donde empieza la sombra (0x424242, 4 px corrida) y la izquierda 4 px antes de donde
    termina la sombra de abajo (SMB1 y SMB2 80×120, Lost Levels 88×96, SMB3 83×120). Derecha para la siguiente.
    Se guardan en `saves\covers\*.png` (las próximas veces se leen de ahí) y se muestran a 4× píxel a píxel. Las del
    jugador (`covers\` con Game Covers activado) tienen prioridad. Con todas las tarjetas con cover, ninguna emula
    (el launcher iba a 3 ms por pasada con covers).
  - Probado en nativo con `smastool` y un arnés de prueba (fuera del repositorio): cinco imágenes bien en ~230 pasos.
- **Probado:** compila. En Xenia: el launcher genera y guarda las cinco covers y las muestra; en el juego (compilación
  temporal que entraba directo a SMB1, no subida) CRT + curvatura + Sepia por la tabla y los bordes bien.
  Rendimiento: pendiente de consola (run 031).
- **Siguiente:** run 031.
