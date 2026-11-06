# Run 010 — Rendimiento por componente y -O2 / -O3 en la consola (hito 2b)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `0bfe9a5` (tarea 0009); SHA1 en `paquete.sha1`.
- **Qué se probó:** `O2/smas_hito2.xex` y `O3/smas_hito2_o3.xex`: el guion de la run 004, benchmarks con y sin PPU,
  rewind, y el diagnóstico de nombres de fichero.
- **Resultado:**
  - Los dos dan las 22 líneas y las 9 salidas idénticas a x86 (`O2/comparacion.txt`, `O3/comparacion.txt`).
  - **La PPU es el 77-85 % de cada frame** (20-36 ms); sin dibujar, el resto va a ~160 fps. `-O3`: hasta 6 % más
    rápido en total. Tabla en `docs/PERFORMANCE.md` §1.
  - Las capturas salen ya como `.png`: el temporal sin puntos funciona.
  - La regla del USB, corregida: fallan los nombres con dos o más puntos y la primera parte de 4 caracteres
    (`rqst.png.tmp` → `rqst.tmp`, `rqst.pn.tmp` → `rqst.tmp`, `rqst.png.t` → `rqst.tng`); `rqs.pngx.tmp`,
    `rqstu.png.tmp`, `r.q.s.t.tmp` van bien. El listado muestra **dos ficheros `rqst.tmp`** en la misma carpeta.
- **Logs:** `O2/smas_hito2.log`, `O3/smas_hito2_o3.log` (PNG, WAV, states, ROM y `.xex` no se suben).
- **Siguiente:** hito 2b, `docs/PERFORMANCE.md` §3.
