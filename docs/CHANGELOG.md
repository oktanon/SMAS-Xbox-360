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
- 0007 (2026-11-06) — Hito 2: primer build de Xbox (`xenon/CMakeLists.txt`), `smas_hito2.xex` con el guion y
  benchmarks; humo en Xenia OK (run 006); esperando la consola (run 007).
- 0008 (2026-11-06) — Port base actualizado a upstream v1.1.1 (sin conflictos); error de orden de bytes de MSU-1
  corregido y probado en qemu; run 007 (consola): salidas idénticas a x86, falta el log; run 009 preparada.
- 0009 (2026-11-06) — Hito 2 hecho en consola (run 009); nombres de temporales en el USB corregidos para Xbox;
  sonda de rendimiento (PPU sí/no, -O2/-O3) preparada (run 010); docs/PERFORMANCE.md.
- 0010 (2026-11-06) — Run 010: la PPU es el 77-85 % de cada frame en la consola, `-O3` +6 %, resultados iguales;
  perfil en x86 (`renderBg` + `composeLine` = 69 %); plan del hito 2b; regla de nombres del USB corregida.
- 0011 (2026-11-06) — Paso 1 del rendimiento: `renderBg` y `composeLine` rápidas bajo `SMAS_PPU_FAST`, idénticas
  frame a frame (9020 frames, control negativo), 2-3× en x86; qemu-ppc igual (run 011); run 012 preparada.
- 0012 (2026-11-06) — Run 012: la PPU rápida es 2,4-4,3× más rápida en la consola, resultados idénticos; con `-O3`
  63-65 fps en juego (50 en widescreen y co-op); el núcleo pasa a `-O3`.
