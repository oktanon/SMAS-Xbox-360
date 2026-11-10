# Run 039 — La PPU por fases y la composición a partes iguales (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0040; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8f/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:** como en la run 038: 30-40 s de SMB3 en 16:9, un rato de co-op de SMB1 en 16:9, un poco de SMB2 y de
  SMB1 en 16:9.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** la línea nueva "PPU: preparar la línea y ganchos ..., capas ... (espera ...), en serie ...,
  composición ... (espera ...), final ...". Los ganchos `beginLine` pasan a contar en la PPU: la cifra "CPU, APU y
  juegos" baja lo que antes llevaban ellos.
- **Resultado:** pendiente.
- **Siguiente:** la fase que más pese.
