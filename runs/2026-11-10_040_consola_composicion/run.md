# Run 040 — La composición sin saltos por píxel (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0041; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8g/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:** como en la run 039: 30-40 s de SMB3 en 16:9, un rato de co-op de SMB1 en 16:9, un poco de SMB2 y de
  SMB1 en 16:9; mirar que la imagen esté bien (colores, transparencias, el HUD, los márgenes).
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:** "composición" en la línea "PPU:" (antes 2,1-3,3 ms en 16:9). Si no baja, se revierte la
  tarea 0041.
- **Resultado:** Falla (la hipótesis). La fase "composición" **sube** en vez de bajar, por frame dibujado en 16:9:

  | Escena | Run 039 | Run 040 |
  |---|---|---|
  | SMB3 | 2,1-3,3 ms | 3,0-4,8 ms |
  | SMB1 | 2,1-2,9 ms | 3,9-4,3 ms |
  | SMB2 | 2,5-2,7 ms | 4,0-4,3 ms |
  | Co-op SMB1 | 2,2-2,9 ms | 3,7-4,3 ms |

  - Los saltos no eran lo que costaba: la versión sin saltos hace más instrucciones y va más lenta, como en x86.
  - El usuario no reporta fallos de imagen.
- **Siguiente:** tarea 0042: se revierte la 0041.
