# Run 027 — Bordes difuminados, ritmo fijo a 59,94 Hz, rewind en otro hilo

- **Fecha:** 2026-11-07
- **Dónde:** Xenia (humo) y consola (RGH)
- **Commit:** tarea 0027; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5d/SuperMarioAllStars.xex`).
- **Qué se prueba:** los cambios de la tarea 0027. Copiar el `.xex` sobre el de antes.
- **Xenia:** arranca; el ritmo se fija una vez (59,94 Hz, un frame por refresco) y no cambia más.
- **Pedir en consola:**
  1. Widescreen: entrar y salir de tuberías; donde antes había barras negras, ahora el juego difuminado y oscuro.
  2. Jugar un par de minutos en widescreen con rewind activado (y otro rato en 4:3): ¿se siente a 60?
  3. Usar el rewind (LT) un momento, para ver que sigue funcionando.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` (abrir la Guía o salir antes). Las líneas "pasadas" ahora
  dicen cuántos frames se emularon y el trabajo sin las esperas de la GPU.
- **Resultado:** Parcial. Al principio bien (11-15 ms por pasada); después el usuario cambió Pixel Shape a Square
  (1:1), y con el aspecto Auto el widescreen pasa de 46 a 71 columnas por lado: más dibujo. Pasadas de 17-21 ms y
  ~550 por 10 s; el ritmo contaba cada una como un refresco, así que el juego iba ~8 % lento y el sonido se quedaba
  sin cola (10-37 veces cada 10 s); "refrescos perdidos" no lo veía (umbral 25 ms). El rewind ya no aparece como
  picos. El usuario no encontró cómo volver a 4:3. Pide SMAS + Super Mario World (tarea 0028).
- **Logs:** `smas_port.log`, `supermarioallstars.log`, `smas_port.ini`, `achievements.ini`, `rompath.txt`.
- **Siguiente:** tarea 0028 y run 028.
