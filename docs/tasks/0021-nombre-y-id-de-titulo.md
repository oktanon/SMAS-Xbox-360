# 0021 — El juego con nombre y ID de título propios

- **Fecha:** 2026-11-06
- **Objetivo:** que la consola reconozca el título como un juego con nombre fijo (el usuario: Aurora toma el nombre del
  archivo). El usuario también dice que jugar a un jugador se siente mucho más estable (run 021 opcional, sin log).
- **Análisis:** los títulos de prueba usaban el ID por defecto de xsdk (`5853444B`, compartido con `xsdk_test` y los
  demás) y un nombre de prueba. xsdk mete el nombre en un recurso XDBF con el nombre del ID de título, como los juegos
  (xsdk `docs/HARDWARE.md`, "A title's name in its .xex"); la guía de xsdk pide un ID propio y su nombre.
- **Hecho:**
  - `xenon/CMakeLists.txt`: el target del juego pasa a `SuperMarioAllStars` (`SuperMarioAllStars.xex`, su log
    `SuperMarioAllStars.log`) con `TITLE_ID 0x534D4153` ("SMAS", la propuesta de AGENTS.md §7), `NAME "Super Mario
    All-Stars"` y `VERSION 0.4.0.0`, definidos una vez (`SMAS_TITLE_ID`, `SMAS_TITLE_NAME`, `SMAS_VERSION`).
  - `play_main.cpp`: al arrancar lee su propio recurso (`XexGetModuleSection`, como `cmake_check` de xsdk) y escribe
    `PASS`/`FAIL` con el nombre.
  - `AGENTS.md` (§2, §3, §7: ID de título decidido), `docs/ACHIEVEMENTS_X360.md`.
- **Probado:** Xenia: la ventana muestra `[534D4153 v0.4.0.0] Super Mario All-Stars`; `PASS: título 534D4153 "Super
  Mario All-Stars" en el .xex (123 bytes)`.
- **Estado:** pendiente de ver en Aurora (run 022). Si Aurora siguiera tomando el nombre del archivo, el siguiente paso
  es completar el XDBF como un SPA (cabecera `XTHD`, imagen), lo que toca `xsdk-main/tools/elf2xex.py`: proponerlo
  antes (reglas de xsdk).
- **Siguiente:** hito 8 adelantado (rendimiento en widescreen, co-op, batalla), si el usuario lo confirma.
