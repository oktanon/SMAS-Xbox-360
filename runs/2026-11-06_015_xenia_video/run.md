# Run 015 — smas_video.xex en Xenia (hito 3)

- **Fecha:** 2026-11-06
- **Dónde:** XSDK Xenia (no es evidencia; solo para ver que arranca y que se ve)
- **Commit:** tarea 0015
- **Qué se probó:** `smas_video.xex` con la ROM y su guion; 40-75 s y cerrado a mano; capturas de la pantalla de
  Windows (no se suben: imagen del juego).
- **Resultado:** el swapchain de vulkan4xenon funciona en este fork. **La prueba de color pasa:** las cuatro barras
  (rojo, verde, azul, mezcla 128/192/64) se leen del front buffer con su color, así que `screen.frag` lee bien los
  canales `(g, b, a)`. El juego se ve: la pantalla de título de All-Stars en 4:3, centrada, con bandas negras a los
  lados. Los hashes del guion, iguales a la run 004. Tiempos de Xenia (no valen): emulación ~5-6 ms y vídeo ~0,4-0,6
  ms por frame.
- **Logs:** `smas_video.log`.
- **Siguiente:** run 016 en la consola.
