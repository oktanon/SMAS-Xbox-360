# Run 020 — Widescreen 16:9 como el PC y stick por sectores (hito 4)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `bf950fc` (tarea 0019); SHA1 en `paquete.sha1`. (El log llegó en la carpeta de la run 019; movido aquí y el
  de la 019 recuperado de git.)
- **Qué se probó:** `smas_jugar.xex`: SMB1 en 4:3 y en 16:9 (46 por lado), SMB3, y por error el modo batalla de SMB3 a
  dos jugadores, en 16:9.
- **Resultado:** **los mandos, el stick por sectores ("se sienten más responsivos") y salir con Back + LB + RB
  funcionan; siguen los tirones.**
  - SMB1 4:3: 9,6-12 ms por frame, sin frames de más. SMB1 16:9: 13,2-14,5 ms, casi todo sin frames de más, algunos
    tramos con 13-55. SMB3 16:9: 13,5-14,9 ms con 98-168 de 600 de más. **Batalla de SMB3 a dos en 16:9: 40-55 ms
    por frame** (393-600 de 600 de más).
  - En casi cada tramo de 600 frames, la cola de audio se vacía una vez: la línea del log escrita cada 600 frames en
    el USB (milisegundos, fuera de lo medido) costaba un refresco. Corregido en la tarea 0020.
  - El juego elige márgenes desiguales en SMB1 (20 + 72): con la imagen entera centrada, el HUD queda quieto.
- **Logs:** `smas_jugar.log`.
- **Siguiente:** hito 4 hecho; los tirones del widescreen, el co-op y la batalla, al hito 8. Run 021 para medir con
  el log arreglado.
