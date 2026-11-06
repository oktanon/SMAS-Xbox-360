# Run 009 — Hito 2 en la consola con la base v1.1.1

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `db401cb` (tarea 0008); SHA1 del `.xex` y del guion en `paquete.sha1`.
- **Qué se probó:** `smas_hito2.xex` (GCC de xsdk, newlib, base v1.1.1): el guion de la run 004, benchmarks y un
  diagnóstico del guardado seguro.
- **Resultado:** **hito 2 hecho.**
  - Las 22 líneas iguales a la run 004 y las 9 salidas idénticas a las de x86 (`comparacion.txt`).
  - Velocidad (un hilo, `-O2`): 38 fps el menú, 22-29 fps en juego (36-48 % del tiempo real). `docs/PERFORMANCE.md`.
  - Los `.tmp`: la consola crea `menu.png.tmp` como `menu.tmp` (lo muestra su propio listado, no es el FTP) y el
    `rename` falla (`0xC0000034`). Pasa con `menu`, `smb1`, `smb2` y `smb3.png`: la parte antes del último punto
    mide 8 caracteres y lleva un punto. `rename` sobre un destino que existe funciona. Corregido para Xbox en la tarea
    0009 (temporal `menu_png.tmp`); la run 010 lo comprueba.
- **Logs:** `smas_hito2.log`, `comparacion.txt` (PNG, WAV, `.tmp` y states no se suben).
- **Siguiente:** run 010 (rendimiento por componente, `-O2` frente a `-O3`, guardado corregido).
