# Run 023 — Un SPA de juego en el .xex, para el nombre en Aurora

- **Fecha:** 2026-11-07
- **Dónde:** Consola (RGH), Aurora
- **Commit:** tarea 0022; SHA1 en `paquete.sha1`.
- **Qué se prueba:** `SuperMarioAllStars.xex` empaquetado con `xenon/tools/elf2xex_spa.py`: el XDBF lleva `XTHD`
  (ID de título, juego completo, versión 0.4.0.0), `XACH` vacío, `XSTC` y `XSTR` con "Super Mario All-Stars".
- **Esperado:** Aurora muestra "Super Mario All-Stars" (si guardó la entrada vieja: quitarla y volver a escanear). Si
  muestra "SuperMarioAllStars" (el archivo), Aurora no lo toma del SPA: ver la tarea 0022.
- **Resultado:** Aurora sigue mostrando el nombre del archivo; el juego arranca y funciona igual.
- **Siguiente:** tarea en pausa por decisión del usuario (tarea 0023); se pasa al hito 5 (launcher).
