# 0043 — README del repositorio

- **Fecha:** 2026-11-10
- **Objetivo:** un `README.md` en la raíz que explique de qué trata el port, cómo se construyó, el estado actual,
  cómo hacer una build, cómo instalarlo en la consola, y que sirva de índice de los documentos con sus conexiones.
- **Hecho:** `README.md` nuevo, **en inglés** (el resto de la documentación sigue en español; distinto del de `SuperMarioAllStars-main/`, que es el de upstream): descripción y
  pilas (con diagrama), historia de la construcción por etapas, tabla de hitos y limitaciones conocidas, mapa del
  repositorio, índice de documentos con lo que conecta cada uno, build paso a paso, instalación en la consola,
  controles, niveles de prueba (qemu, Xenia, consola), reglas de trabajo y licencias. Fila nueva en la tabla de
  rutas de `AGENTS.md` §2.
- **Ficheros:** `README.md`, `AGENTS.md`, `docs/CHANGELOG.md`, este fichero.
- **Probado:** ninguno (solo documentación); los enlaces relativos del README se comprobaron uno a uno y las
  búsquedas de rutas absolutas de `AGENTS.md` §0 no encuentran nada.
- **Estado:** hecho.
- **Siguiente:** actualizar el README al cerrar cada hito (estado y rendimiento); en el hito 10, sustituir el paso de
  "compilar" por el paquete de release y añadir el aviso de licencias.
