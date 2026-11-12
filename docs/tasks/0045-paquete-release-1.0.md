# 0045 — Hito 10: paquete de release 1.0, README en inglés más corto, avisos de licencias

- **Fecha:** 2026-11-12
- **Lo que el usuario confirmó en la consola:** la Guía pausa el juego, se vuelve al dashboard y los guardados van
  bien. El fallo pospuesto de SMB1 (tarea 0036) no existió.
- **Escritura atómica de los guardados: ya estaba.** `writeFileSafely` (`core/fileio.cpp`) escribe a un temporal y lo
  renombra; el `rename` de xsdk (`xsdk-main/src/path.c`) usa `NtSetInformationFile` con `replace_if_exists`: el
  archivo viejo se reemplaza de una vez. Pasan por ahí el `.srm`, los states, `smas_port.ini`, `achievements.ini` y
  `rompath.txt`. Comprobado en la consola en las runs 009 y 014 (los nombres que el USB recortaba). Sin cambios.
- **Hecho:**
  - **`xenon/release/README.txt`:** el README para jugadores, en inglés: instalar, controles, rendimiento y
    limitaciones, licencias.
  - **`xenon/release/make_release.sh`:** arma `build-xenon/release/SuperMarioAllStars-x360-<versión>/` y su `.zip`
    con el `.xex`, el `README.txt` y `THIRD_PARTY_NOTICES.txt`. Los avisos salen de los archivos de licencia de cada
    proyecto, tal cual: el port de PC (MIT), xsdk (MIT, más los avisos BSD de libxenon y Xenia que su
    `THIRD_PARTY_NOTICES.md` pide llevar), vulkan4xenon (MIT), SDL 3 (zlib), DejaVu Sans, Luckiest Guy (Apache 2.0),
    stb (dominio público) y el runtime de GCC y newlib.
  - **`README.md` de la raíz:** de 450 a ~140 líneas, en inglés: estado de la 1.0, instalar, controles, en qué se
    diferencia del port de PC, compilar, probar, licencias. Fuera la historia por tareas, el mapa del repositorio y
    el índice de documentos (están en `AGENTS.md` y `docs/`).
  - **`xenon/CMakeLists.txt`:** `SMAS_VERSION` 1.0.0.0 (estaba en 0.5.0.0, del hito 5); el XTHD del `.xex` la lleva.
  - **`AGENTS.md` §3:** cómo se arma el paquete de release.
- **Probado:** compila sin avisos; el paquete se arma con sus cuatro archivos; el XTHD del `.xex` dice 1.0.0.0. El
  núcleo no cambia desde la tarea 0044.
- **Siguiente:** run 043, instalación limpia en la consola.
