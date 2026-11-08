# 0028 — Volver a 4:3, ritmo que mantiene la velocidad, SMAS + SMW (consulta)

- **Fecha:** 2026-11-08
- **Objetivo:** lo de la run 027: empeoró tras cambiar opciones; no se encontraba cómo volver a 4:3; consulta sobre
  Super Mario All-Stars + Super Mario World.
- **Análisis (run 027):** el usuario puso Pixel Shape en Square (1:1); con el aspecto Auto (16:9) el margen pasa de
  46 a 71 columnas por lado (`App::computeLayout`: 224 × 16/9 / par), un 14 % más de imagen que dibujar, y las
  pasadas de juego subieron a 17-21 ms. El ritmo de la tarea 0027 contaba cada pasada como un refresco entero: a
  ~18 ms el juego corría ~8 % lento y la cola del sonido se vaciaba. El "4:3 (Original)" está en Options > Display >
  Aspect Ratio; en la Xbox no había atajo (en `smas_jugar.xex` era el stick derecho).
- **Hecho** (`app_xenon.cpp`; `app.cpp` bajo `SMAS_XENON`):
  - Ritmo: un frame por cada refresco transcurrido contado en fracciones (con un cuarto de margen), así las pasadas
    largas a veces corren dos frames y el juego mantiene su velocidad; nunca más de 3.
  - Stick derecho pulsado, jugando: 4:3 ↔ el widescreen elegido (Auto si no hay), con aviso y guardado.
  - Options > Display: sin "Display Mode" (ventana o pantalla completa) en la Xbox.
- **SMAS + SMW:** la ROM (2,5 MB, cabecera "ALL_STARS + WORLD") no la acepta `isSmasRom`. Todo lo propio del port
  (widescreen, co-op, logros, selector de niveles, carrusel, arranque directo, previews) engancha direcciones del
  código de la ROM USA de SMAS; en esa ROM los juegos están en otro lugar, y hay un quinto juego. Opciones: (a)
  jugarla como SNES sin mejoras (aceptarla y no instalar los ganchos): el emulador es genérico, trabajo moderado,
  ninguna mejora en ningún juego; (b) con las mejoras: localizar cada gancho de los 4 juegos en esa ROM y hacer el de
  SMW desde cero (widescreen, etc.): un proyecto del tamaño del port original, de upstream. Sin implementar.
- **Probado:** compila; Xenia arranca. Pendiente de consola (run 028).
- **Siguiente:** run 028.
