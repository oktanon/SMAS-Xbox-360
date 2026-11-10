# Run 034 — Rendimiento por juego y aspecto (hito 8)

- **Fecha:** 2026-11-10
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0034; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8a/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes. Cada 10 s el log dice el juego, el aspecto y en qué se va
  el tiempo.
- **Pedir:** jugar cada juego 30-40 s seguidos (sin abrir el menú) en 4:3 y otros 30-40 s en 16:9: SMB1, Lost
  Levels, SMB2, SMB3. Si se puede, un rato de co-op en SMB1. Anotar en qué juego se notó lento.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Resultado:** OK, medido. Filtro Nearest y rewind en todo. Además de la emulación, cada pasada lleva el snapshot
  del rewind (2,8-3,3 ms, uno cada dos frames) y la interfaz y el vídeo (1,4 ms en 4:3, 0,4 ms en 16:9):

| Juego | Aspecto | Emulación por frame | Frames dibujados de 600 | Peor pasada |
|---|---|---|---|---|
| SMB1 | 4:3 | 10,7-11,8 ms | 528-600 | 18-34 ms |
| SMB1 | 16:9 | 13,3-14,1 ms | 490-541 | 25-51 ms |
| Lost Levels | 4:3 | 10,5-11,8 ms | 500-600 | 18-34 ms |
| Lost Levels | 16:9 | 13,9-14,1 ms | 461-513 | 27 ms |
| SMB2 | 4:3 | 11,3-11,5 ms | 587-600 | 18-25 ms |
| SMB2 | 16:9 | 13,8-14,2 ms | 347-460 | 28-36 ms |
| SMB3 | 4:3 | 10,8-12,3 ms | 599-600 | 18-19 ms |
| SMB3 | 16:9 | 13,7-14,2 ms | 263-475 | 28-32 ms |
| SMB1 co-op | 4:3 | 13,1-13,4 ms | 361-447 | 31-35 ms |
| SMB1 co-op | 16:9 | 14,2-14,3 ms | 292-334 | 45-47 ms |

  En 4:3, 11,5 + 3 + 1,4 ms entran justo en 16,7 ms. En 16:9, 14 + 3 ms no: el frame del snapshot se pasa, la
  pasada siguiente hace dos frames y dibuja uno (SMB2 y SMB3, 45 fps a la vista; co-op en 16:9, 30).
- **Logs:** `smas_port.log`, `supermarioallstars.log`.
- **Siguiente:** tarea 0035 (el snapshot del rewind: el CRC era la mayor parte), run 035.
