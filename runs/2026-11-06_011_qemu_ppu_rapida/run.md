# Run 011 — Comparación x86 / qemu-ppc con la PPU rápida

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** tarea 0011 (`SMAS_PPU_FAST` en las dos builds de `build.sh`)
- **Qué se probó:** `compare.sh` con el guion de siempre, la PPU rápida en x86 y en PowerPC big-endian.
- **Resultado:** **Iguales** (22 líneas, 9 archivos, MSU-1). Las 22 líneas de PowerPC son idénticas a las de la x86
  original de la run 004: la PPU rápida no cambia nada tampoco en big-endian. qemu-ppc tardó 157 s (441 s con la PPU
  original).
- **Antes, en x86 (`compare_ppu.sh`):** 9020 frames iguales con y sin `SMAS_PPU_FAST`; el control negativo, distinto.
- **Siguiente:** run 012 en la consola.
