# Run 005 — Control negativo de la comparación

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** tarea 0006
- **Qué se probó:** `compare.sh` con `guion_control.txt`, que vuelca las memorias en crudo (`dump`). La VRAM y la CGRAM
  son arrays de `uint16_t`, así que en big-endian sus ficheros deben salir distintos.
- **Resultado:** **Distintos, como se esperaba**: solo `control.vram` y `control.cgram`. WRAM, OAM, ARAM y SRAM (bytes)
  y el `hashall` (por valor) iguales. La comparación detecta los errores de orden de bytes.
- **Siguiente:** —
