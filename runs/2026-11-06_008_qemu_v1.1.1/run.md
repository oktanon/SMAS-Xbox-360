# Run 008 — Comparación x86 / qemu-ppc con la base v1.1.1

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** tarea 0008 (port base actualizado a upstream v1.1.1, corrección de MSU-1)
- **Qué se probó:** `compare.sh` con el guion de siempre y la prueba nueva de MSU-1 (`msu1_test`).
- **Resultado:** **Iguales**: 22 líneas, las 9 salidas del guion y la prueba MSU-1. Las 22 líneas y los 9 SHA1 son
  además idénticos a los de la run 004 (v1.0): para este guion el núcleo de v1.1.1 se comporta igual, así que la
  run 004 sigue siendo la referencia de la consola. (`resumen.txt` dice 10 archivos porque contó `msu1.txt`; ya
  excluido en `compare.sh`.) x86 52 s, qemu-ppc ~7,5 min.
- **Siguiente:** `smas_hito2.xex` con v1.1.1 en la consola (run 009).
