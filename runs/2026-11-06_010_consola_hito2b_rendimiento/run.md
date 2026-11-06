# Run 010 — Rendimiento por componente y -O2 / -O3 en la consola (hito 2b)

- **Fecha:** 2026-11-06 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0009; SHA1 en `paquete.sha1`.
- **Qué se prueba:** dos títulos, cada uno en su carpeta con la ROM y `guion.txt`: `O2/smas_hito2.xex` y
  `O3/smas_hito2_o3.xex`. Cada uno ejecuta el guion de la run 004 y benchmarks con la PPU dibujando y sin dibujar
  (`render off`), más el rewind y el diagnóstico de nombres de fichero.
- **Esperado:** las 22 líneas iguales a la run 004 en los dos; cuánto de cada frame es la PPU en la consola; si `-O3`
  gana; que las capturas salgan como `.png` (ya no `.tmp`); las líneas `DIAG fopen` para confirmar la regla del USB.
- **Resultado:** pendiente — copiar aquí `smas_hito2.log` y `smas_hito2_o3.log` (y, si se quiere, lo demás).
- **Siguiente:** decidir la línea de trabajo de `docs/PERFORMANCE.md` §3.
