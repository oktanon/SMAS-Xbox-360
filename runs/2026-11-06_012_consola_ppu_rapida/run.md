# Run 012 — La PPU rápida en la consola (hito 2b, paso 1)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `9e1163e` (tarea 0011); SHA1 en `paquete.sha1`.
- **Qué se probó:** `O2/smas_hito2.xex` y `O3/smas_hito2_o3.xex` con `SMAS_PPU_FAST`: el guion de la run 004, los
  benchmarks con y sin PPU y el rewind.
- **Resultado:** **OK.**
  - Los dos dan las 22 líneas y las 9 salidas idénticas a x86 (`O2/comparacion.txt`, `O3/comparacion.txt`).
  - La PPU, 2,4-4,3 veces más rápida que en la run 010. Con `-O3`: menú 95 fps, SMB1 65, SMB2 63, SMB3 63,
    widescreen 50, co-op 51 (antes 22-38). Tabla en `docs/PERFORMANCE.md` §3.1.
  - Rewind: volver un frame 22-24 ms (antes 48); guardar uno ~6 ms.
- **Logs:** `O2/smas_hito2.log`, `O3/smas_hito2_o3.log`.
- **Siguiente:** margen para vídeo, audio e interfaz: `docs/PERFORMANCE.md` §3.2.
