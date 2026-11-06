# Run 004 — Hito 1b: núcleo big-endian igual a x86

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu); `powerpc-linux-gnu-g++` 15.2 `-static` frente a `g++` x86-64, los dos con
  `-O2 -ffp-contract=off`.
- **Commit:** tarea 0006 (`xenon/qemu/guion.txt` definitivo)
- **Qué se probó:** `compare.sh` con el guion completo: menú y 5 s de su música en WAV, SMB1, Lost Levels, SMB2 y SMB3
  jugados, save/load state, `rewindtest`, widescreen, trucos, paleta, co-op, reset, logros activos (sale
  `smb3_items`).
- **Resultado:** **Iguales**: 22 líneas (hashes de WRAM, VRAM, CGRAM, OAM, ARAM, SRAM, imagen, CPU y audio; esperas
  de carga; eventos de logros) y 9 archivos (8 PNG y 1 WAV) idénticos byte a byte. x86 52 s, qemu-ppc 441 s.
- **Logs:** `resumen.txt`, `x86|ppc/salida.txt`, `hashes.txt`, `archivos.sha1` (los PNG, WAV y states no se suben).
- **Siguiente:** hito 2 (el mismo guion en la consola, con el GCC de xsdk y newlib).
