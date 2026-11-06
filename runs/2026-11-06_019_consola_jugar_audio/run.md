# Run 019 — Jugar con el hilo de audio fuera del de la emulación (hito 4)

- **Fecha:** 2026-11-06 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0018; SHA1 en `paquete.sha1`.
- **Qué se prueba:** `smas_jugar.xex` con el hilo de audio de SDL en el hilo hardware 5; el log da el tiempo de cada
  parte (mandos, frame, audio). Salir: Back + LB + RB 3 s.
- **Esperado:** en 4:3, la emulación cerca de los 10-11 ms de las runs 014/016 y sin tirones; la cola de audio sin
  vaciarse. En widescreen, SMB2/SMB3 y el co-op siguen justos (hito 8).
- **Resultado:** pendiente — `smas_jugar.log` y cómo se siente (tirones en 4:3 y en widescreen, sonido).
- **Siguiente:** —
