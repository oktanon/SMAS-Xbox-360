# Run 022 — El nombre del juego en Aurora (hito 4)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), Aurora
- **Commit:** `1e4b8d1` (tarea 0021); SHA1 en `paquete.sha1`.
- **Qué se probó:** `SuperMarioAllStars.xex` con ID de título `534D4153` y el nombre en el XDBF de xsdk (solo `XSTC` y
  `XSTR`).
- **Resultado:** **Aurora sigue mostrando el nombre del archivo** (usuario; sin log). La consola sí ve el recurso
  (comprobado en Xenia con `XexGetModuleSection`); Aurora necesita algo más.
- **Siguiente:** run 023 con un XDBF como el SPA de un juego (tarea 0022).
