# 0030 — Hito 6: filtros y colores del juego

- **Fecha:** 2026-11-08
- **Objetivo:** lo que faltaba del hito 6. Options > Graphics (Filter, Scanline / Effect Strength, CRT Curve) y
  Options > Colour (perfil, brillo, contraste, saturación, gamma, temperatura, daltonismo) no hacían nada en la Xbox:
  el juego se dibujaba con muestreo de punto. Pregunta del usuario: ¿se pueden poner de covers por defecto texturas
  sacadas de la ROM? (respuesta en el chat y abajo).
- **Hecho:**
  - `shaders/game.frag`: el `kGameFS` de `video_gles.cpp` escrito para xost. Filtros Nearest, Sharp Bilinear,
    Bilinear, CRT, CRT con rejilla de apertura y LCD Grid; scanlines, curvatura y los 10 perfiles de color, brillo,
    contraste, saturación, gamma, temperatura y daltonismo. Las funciones auxiliares con `if` van dentro de `main`
    (xost no acepta control de flujo en una auxiliar), las matrices como productos escalares y los enteros como
    floats. Cada `if` lee solo push constants: en Xenos el brazo que no se toma no ejecuta nada, así que cada filtro
    cuesta solo lo suyo (vulkan4xenon `docs/SUPPORT.md`, "if/else").
  - `video_xenon.cpp`: un segundo pipeline (canvas.vert + game.frag, sin blending, como en GLES) con su layout de
    112 bytes de push constants (la de canvas.vert y 6 vec4 de parámetros). La lista de lotes marca los del juego y
    `present()` cambia de pipeline y empuja los parámetros de cada uno. El juego usa su textura dos veces (lineal en
    el binding 0 y de punto en el 1).
  - Side Borders "Blurred Game": el fondo chico se sigue promediando en la CPU, pero ahora lo dibuja la pasada 1 del
    mismo shader: toma los colores del juego (un perfil Pocket tiñe también los bordes), se oscurece (× 0,32) y lleva
    la viñeta, como en GLES.
  - Slang Shader: no; es solo de Windows (librashader).
- **Probado:** compila; xost acepta el shader. En Xenia, con una compilación temporal que entraba directo a SMB1 (no
  subida): CRT con rejilla, curvatura 40 y bordes difuminados; LCD Grid con el perfil Pocket (Green); se ven bien.
  Rendimiento: pendiente de consola (run 030); el CRT es el más caro (4 lecturas lineales y dos `exp` por píxel).
- **Covers desde la ROM (consulta):** la ROM no trae las carátulas de las cajas. Lo que sí trae es la pantalla de
  selección de All-Stars con un recuadro por juego, y la pantalla de título de cada uno. Sin decodificar gráficos de
  la ROM, se puede dejar que el emulador las dibuje: correr la consola de la tarjeta All-Stars hasta el selector,
  recortar los cuatro recuadros y usarlos de cover por defecto (o una foto fija de la pantalla de título de cada
  juego). Así las tarjetas que no están elegidas dejan de emular y el launcher iría como con covers (3 ms por
  pasada). Sin implementar; propuesto al usuario.
- **Siguiente:** run 030.
