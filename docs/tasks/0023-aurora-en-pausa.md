# 0023 — Nombre en Aurora: en pausa

- **Fecha:** 2026-11-07
- **Objetivo:** dejar registrado el estado de la tarea del nombre en Aurora (tareas 0021-0022).
- **Resultado (run 023):** con el XDBF como un SPA de juego (`XTHD`, `XACH`, `XSTC`, `XSTR`) Aurora sigue mostrando el
  nombre del archivo. Xenia y la propia consola (`XexGetModuleSection`) leen el nombre bien.
- **Decisión del usuario:** pausar la tarea y seguir con el launcher (hito 5).
- **Para retomarla:** (a) añadir el XLAST (`XSRC`) y la imagen del título, que no caben en el sitio que da
  `elf2xex.py` (`TITLE_NAME_END`): cambio en xsdk, proponerlo antes; (b) renombrar a mano en Aurora; (c) llamar al
  archivo `Super Mario All-Stars.xex` (ojo: FAT y nombres con espacios, AGENTS.md §4).
- **Siguiente:** hito 5.
