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
- **Resultado:** OK, medido. La PPU en el hilo de la emulación, por frame dibujado en 16:9 (ms):

  | Escena | Preparar y ganchos | Capas (espera) | En serie | Composición (espera) | Final | Frame dibujado |
  |---|---|---|---|---|---|---|
  | SMB3 | 0,08-0,49 | 3,4-4,0 (0,8-1,5) | 0,56-1,6 | 2,1-3,3 (0,5-0,8) | 0,18-0,26 | 13,3-15,1 |
  | SMB2 | 0,11-0,13 | 3,9-5,2 (1,0-1,4) | 0,23-0,34 | 2,5-2,7 (0,4-0,7) | 0,17-0,20 | 13,7-15,3 |
  | SMB1 | 0,08-0,10 | 3,5-3,9 (0,7-1,1) | 0,46-0,73 | 2,1-2,9 (0,5-0,7) | 0,16-0,19 | 12,3-14,1 |
  | Co-op SMB1 | 0,10-0,12 | 3,7-4,3 (0,3-1,0) | 0,46-0,52 | 2,2-2,9 (0,4-0,7) | 0,16-0,19 | 14,9-16,4 |

  - La composición del hilo de la emulación son ~133 px × 224 líneas, unos 30 000 píxeles, en ~2,7 ms: unos 300
    ciclos por píxel, demasiados para lo que hace.
  - El usuario no reporta cambios de imagen.
- **Siguiente:** tarea 0041: la composición sin saltos por píxel.
