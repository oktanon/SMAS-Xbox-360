# Run 038 — Las lecturas de la CPU por páginas y el log por frame dibujado (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0039; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8e/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Como en la run 037: 30-40 s de SMB3 en 16:9 y un rato de co-op de SMB1 en 16:9 (y, si se puede, en 4:3);
     un poco de SMB2 en 16:9.
  2. Que todo juegue igual: un nivel con co-op, un state guardado y cargado, el rewind un momento.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** la línea nueva "frame dibujado ... ms: PPU en este hilo ... (esperando ...), CPU, APU y
  juegos ...; frame saltado ...".
- **Resultado:** OK. El usuario nota mejora en SMB3 y en el co-op de SMB1; SMB2 igual de bien. La línea nueva, por
  frame dibujado:

  | Escena | Frame dibujado | PPU en el hilo de la emulación (espera) | CPU, APU y juegos | Dibujados de 600 |
  |---|---|---|---|---|
  | SMB3 16:9 | 15,0 ms | 9,3-9,7 (1,3-1,7) | 5,5-5,7 | casi todo 518-600 |
  | SMB2 16:9 | 14,6 ms | 9,5 (1,6-1,7) | 4,9-5,1 | 576-600 |
  | Co-op SMB1 16:9 | 15,4-16,3 ms | 8,3-9,1 (0,8-1,5) | 7,1-7,2 | 442-571 (antes 301-358) |
  | Co-op SMB1 4:3 | 12,2-13,1 ms | 5,3-5,9 (0,7-0,9) | 6,9-7,3 | 554-600 |
  | SMB1 16:9 | 12,6 ms | 7,8 (1,1) | 4,8 | 474 de 497 |

  - Frame saltado: ~5 ms (co-op, ~7). El co-op gasta ~2 ms más de CPU que un jugador.
  - La PPU es lo que más pesa en 16:9: el hilo de la emulación trabaja ~8 ms en ella y espera solo ~1,5.
  - El trabajo medio en SMB3 16:9 sigue en 16,4-16,8 ms: al límite.
- **Siguiente:** tarea 0040: la PPU por fases en el log, y la composición repartida a partes iguales.
