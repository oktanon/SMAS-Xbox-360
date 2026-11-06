# 0019 — Run 019: widescreen como el PC (16:9, 8:7) y stick por sectores

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 019 (el usuario: mejoró pero no del todo; el stick en diagonal entorpece; en 16:9 se
  mueve mucho la interfaz y el tiempo se ve cortado; foto de la tele) y corregir.
- **Análisis:** 4:3 con un jugador, arreglado (9,4-11,8 ms, sin frames de más). Co-op 16,7 ms en 4:3 y 18,3 ms ancho;
  SMB3 ancho 16-20 ms (hito 8; el perfil en x86 del co-op muestra ~40 % más de CPU emulada, nada fácil). El
  widescreen fijo de 64 por lado medía 1440 px a 3,75 por píxel y se cortaba, y centrar la parte 4:3 con márgenes
  desiguales (56 + 72) movía la imagen entera, HUD incluido. El stick: cualquier diagonal daba Abajo, y SMB1 para a
  Mario con Abajo.
- **Hecho:**
  - `VideoVk::fitLayout` (el modo "ajustar" de `App::computeLayout`: la imagen entera centrada, píxeles 8:7 por defecto
    como el PC, escala de la imagen ancha completa, el último píxel rellenado si el aspecto coincide) y
    `VideoVk::wantExtFor` (el margen para un aspecto: 16:9 con 8:7 = 46 por lado). `present()` recibe el rectángulo
    (la app le dará el suyo en el hito 5). `video_main.cpp` y `play_main.cpp` adaptados (la prueba de color mide en
    el rectángulo; el widescreen pide 46).
  - `Input::stickToDpad`: el stick como cruceta por sectores (vertical más allá de 40°, horizontal hasta 65°), en
    `play_main.cpp`; en el hito 5, en `App::readPlayer` bajo `#ifdef SMAS_XENON`.
  - `docs/ROADMAP.md`, `AGENTS.md` §5, runs 019 y 020.
- **Probado:** compila. Pendiente de consola (run 020).
- **Siguiente:** con la run 020, cerrar el hito 4 (los mandos y el sonido ya funcionan; lo que queda del co-op y SMB3
  ancho es del hito 8).
