# Run 002 — Primera comparación x86 / qemu-ppc

- **Fecha:** 2026-11-06
- **Dónde:** qemu-ppc (WSL Ubuntu)
- **Commit:** `b89ef02` + cambios de la tarea 0006 (primera versión del guion)
- **Qué se probó:** `compare.sh` con la primera versión de `guion.txt`.
- **Resultado:** Iguales (14 líneas), pero **no vale como prueba**: el guion tenía dos errores. Los `warp` desde SMB2 no
  esperaban a que terminara la carga (VRAM e imagen fijas en la pantalla negra) y `hashall reset` chocaba con el
  subcomando que reinicia el hash de audio, así que esa etiqueta no se imprimió.
- **Siguiente:** comando `waitload` en smastool y guion corregido (run 003).
