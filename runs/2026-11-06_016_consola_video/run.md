# Run 016 — El vídeo en la consola (hito 3)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB; salida a 59,94 Hz.
- **Commit:** `fc0a785` (tarea 0015); SHA1 en `paquete.sha1`.
- **Qué se probó:** `smas_video.xex`: prueba de color (4 barras leídas del front buffer), el guion de la run 004 con 2
  hilos de dibujo y una demostración en widescreen 64, 4:3 y co-op, cada frame en la pantalla (FIFO).
- **Resultado:** **OK en 4:3; widescreen y co-op en widescreen, por encima del presupuesto.**
  - **Prueba de color: PASS en las cuatro barras** en la consola: el orden de canales `(g, b, a)` es el correcto.
  - Las 22 líneas y las 9 salidas idénticas a x86 (`comparacion.txt`), con vídeo y 2 hilos.
  - Vídeo (copia + envío): 0,25-0,33 ms por frame.
  - 4:3: 9-12 ms por frame en los cuatro juegos, 0-2 frames de 600 por encima de 16,7 ms (escrituras de PNG/states:
    ~330 ms cada una).
  - Widescreen 64: SMB1 12,6-13,7 ms (0 de más); **SMB2 17-19 ms (400-477 de 600 de más)**; **SMB3 15,2 ms (235 de
    más)**; **co-op en SMB1 17,6 ms (568 de más)**. Las cargas de SMB2 (pantalla negra): 22-32 ms por frame.
  - Sin foto de la tele (opcional).
- **Causa (perfil en x86, tarea 0016):** en SMB2 widescreen, los fondos que el juego pide enteros con `wideFetchAll`
  y los ganchos `fetchWideTile` (~67 000 llamadas por frame, casi una por píxel tras los tiles en blanco) van en el
  hilo de la emulación; además un fondo con mosaico de tamaño 1 iba por el camino lento (corregido en la tarea 0016).
- **Logs:** `smas_video.log`, `comparacion.txt`.
- **Siguiente:** widescreen y co-op a 60 con vídeo (`docs/PERFORMANCE.md` §4); hito 4.
