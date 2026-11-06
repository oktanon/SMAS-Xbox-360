# Run 003 — Guion completo, x86 / qemu-ppc

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** `b89ef02` + cambios de la tarea 0006 (guion con `waitload`)
- **Qué se probó:** los cuatro juegos cargados por `warp` hasta jugarse, save/load state, rewind, widescreen, trucos,
  paleta, co-op, reset; 8 capturas.
- **Resultado:** Iguales (22 líneas, 8 capturas idénticas byte a byte). Las capturas de PowerPC muestran juego real. x86
  tardó 50 s y qemu-ppc 422 s.
- **Siguiente:** arreglo del WAV de smastool y run final (004).
