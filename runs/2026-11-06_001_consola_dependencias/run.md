# Run 001 — Tests de las dependencias en consola (hito 1)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), lanzado desde USB, sin perfil, sin `net_host.txt`; salida de vídeo 1080p.
- **Commit:** `47e7f86` (tarea 0004); SHA1 de los `.xex` en `xex.sha1`.
- **Resultado:** OK.
  - `vulkan4xenon_test`: 550/550. `vkbench`: 17/17 (frames de ~16,7 ms, sincronizados a 60 Hz).
  - `sdl_check`: 18/18 (mando Xbox 360 con botones, sticks, gatillos y vibración; audio 6 canales a 48 kHz sin frames
    en silencio; reloj de 201 ns; base path `game:\`).
  - `xsdk_test`: 624/630. Los 6 fallos son del entorno, los mismos que vio BT3: coste de una línea de log en USB
    (1758 µs > 1500), 4 de XAM/saves en paquete (sin perfil, el enumerador no da el disco duro) y la red sin
    `net_host.txt`.
- **Logs:** `xsdk_test.log`, `vulkan4xenon_test.log`, `vkbench.log`, `vkbench_summary.csv`, `vkbench_frames.csv`,
  `sdl_check.log`.
- **Siguiente:** hito 1b (comparación x86 / qemu-ppc).
