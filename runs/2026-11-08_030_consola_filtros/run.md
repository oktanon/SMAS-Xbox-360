# Run 030 — Filtros y colores del juego (hito 6)

- **Fecha:** 2026-11-08
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0030; SHA1 en `paquete.sha1` (`build-xenon/consola/hito6b/SuperMarioAllStars.xex`).
- **Qué se prueba:** Options > Graphics y Options > Colour. Copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Options > Graphics > Filter: pasar por Nearest, Sharp Bilinear, Bilinear, CRT, CRT (aperture) y LCD Grid
     jugando un rato con cada uno. ¿Se nota lag o tirones con alguno (sobre todo los CRT)?
  2. Scanline / Effect Strength y CRT Curve con un CRT.
  3. Options > Colour: algún perfil (Sepia, Pocket), brillo, contraste, temperatura.
  4. En 4:3 (stick derecho) con Side Borders en "Blurred Game": los bordes toman los colores del perfil.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` (abrir la Guía o salir antes); fotos de lo que se vea mal.
- **Resultado:** Falla (rendimiento). Con o sin filtros, el juego va peor; Mario desaparece al parpadear tras un golpe. El log: con el menú abierto y nada emulado, 250-280 pasadas cada 10 s (antes 600) con 1 ms de trabajo: la GPU tarda más de un refresco por frame, la imagen va a 30 fps y cada imagen trae 2 frames emulados (480-520 de 600 en 10 s: el juego más lento). El parpadeo de Mario es un frame sí y otro no: a 2 frames por imagen se muestra siempre el mismo, en el que no está. Arreglo: tarea 0031.
- **Logs:** `smas_port.log`, `SuperMarioAllStars.log`.
- **Siguiente:** tarea 0031 (variantes del shader, covers del selector) y run 031.
