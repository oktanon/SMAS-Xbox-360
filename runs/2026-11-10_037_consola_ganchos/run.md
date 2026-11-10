# Run 037 — Los ganchos del widescreen en los hilos de dibujo (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0038; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8d/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Como en las runs 034 y 035: 30-40 s de SMB2 y de SMB3 en 16:9 (si es posible, una zona vertical de SMB2), un
     rato de SMB1 en 16:9 y de co-op de SMB1 en 16:9.
  2. Ver que la imagen de los márgenes está bien (sin tiles rotos ni parpadeos).
  3. Mantener el rewind un momento, y cargar un state de antes.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** "emulación ... ms por frame" en 16:9 (antes 14-15 ms) y "snapshot del rewind" (antes 1,2-1,4).
- **Resultado:** Parcial. Frames dibujados de cada 600 en 16:9 (run 034 → 037):
  - SMB2: 347-460 → **547-588**; el usuario lo nota.
  - SMB3: 263-475 → 511-540, con el trabajo medio en 18,3-19,4 ms: sigue pasándose.
  - Co-op de SMB1: 292-334 → 301-358: casi igual.
  - Snapshot del rewind: 1,2-1,4 → **0,92-0,99 ms**.
  - La "emulación por frame" del log no baja (14,8-15,3 ms), pero es la media de los frames dibujados y los
    saltados, y los saltados cuestan mucho menos: al dibujar más frames, la media sube. No dice cuánto cuesta un
    frame dibujado.
  - El usuario no reporta fallos de imagen. La línea `launch:` (run 036) dice que el archivo B se eligió y se jugó: bien esta vez.
- **Logs:** `smas_port.log`, `supermarioallstars.log`.
- **Siguiente:** tarea 0039. En x86 el co-op cuesta casi el doble **sin dibujar** (la CPU emulada corre las rutinas
  del jugador dos veces): su problema es la CPU, no la PPU. El log separará frames dibujados y saltados, y la PPU
  de la CPU.
