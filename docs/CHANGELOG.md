# Registro de tareas

Una línea por tarea; el detalle está en `docs/tasks/`.

- 0001 (2026-11-06) — AGENTS.md, roadmap, plan de logros nativos, estructura de runs, repositorio git.
- 0002 (2026-11-06) — Estrategia de orden de bytes tomada de los ports de BT3: `docs/ENDIANNESS.md`, comparación
  x86 vs qemu-ppc (hito 1b).
- 0003 (2026-11-06) — Regla de no usar rutas absolutas (`AGENTS.md` §0); rutas de los documentos pasadas a
  relativas; compilador PowerPC para qemu comprobado.
- 0004 (2026-11-06) — Hito 1: xsdk, vulkan4xenon y SDL-xenon compilados; cuatro títulos de prueba listos para la
  consola (run 001, pendiente de logs).
- 0005 (2026-11-06) — Hito 1 hecho en consola: v4x 550/550, vkbench 17/17, sdl_check 18/18, xsdk_test 624/630
  (6 fallos del entorno).
- 0006 (2026-11-06) — Hito 1b hecho: smastool x86 y PowerPC big-endian (qemu) dan lo mismo en todo el guion sin cambiar
  el núcleo; `hashall`, `waitload`, WAV portable; scripts en `xenon/qemu/`; control negativo.
