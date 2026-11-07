# Run 027 — Bordes difuminados, ritmo fijo a 59,94 Hz, rewind en otro hilo

- **Fecha:** 2026-11-07 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Xenia (humo, hecho) y consola (RGH, pendiente)
- **Commit:** tarea 0027; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5d/SuperMarioAllStars.xex`).
- **Qué se prueba:** los cambios de la tarea 0027. Copiar el `.xex` sobre el de antes.
- **Xenia:** arranca; el ritmo se fija una vez (59,94 Hz, un frame por refresco) y no cambia más.
- **Pedir en consola:**
  1. Widescreen: entrar y salir de tuberías; donde antes había barras negras, ahora el juego difuminado y oscuro.
  2. Jugar un par de minutos en widescreen con rewind activado (y otro rato en 4:3): ¿se siente a 60?
  3. Usar el rewind (LT) un momento, para ver que sigue funcionando.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` (abrir la Guía o salir antes). Las líneas "pasadas" ahora
  dicen cuántos frames se emularon y el trabajo sin las esperas de la GPU.
- **Resultado:** pendiente.
- **Siguiente:** —
