# Run 013 — Comparación x86 / qemu-ppc con 2 hilos de dibujo

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** tarea 0013 (`SMAS_PPU_FAST`, `SMAS_PORT_FIXES`)
- **Qué se probó:** `compare.sh` con `threads 2` delante del guion de siempre (como lo ejecutará la consola).
- **Resultado:** **Iguales** (22 líneas, 9 archivos, MSU-1), y las 22 líneas idénticas a la run 004. Con hilos de
  verdad en paralelo (qemu-user) y en big-endian.
- **Antes, en x86:** `HILOS=1|2|3 compare_ppu.sh`, 9020 frames iguales; ThreadSanitizer sin avisos; ASan + UBSan sin
  avisos en los 9020 frames con 2 hilos (salvo el desplazamiento del DSP, `docs/UPSTREAM.md` #2).
- **Siguiente:** run 014 en la consola.
