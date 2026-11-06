# 0005 — Hito 1: resultados en consola

- **Fecha:** 2026-11-06
- **Objetivo:** revisar los logs de la consola de la run 001 y cerrar el hito 1.
- **Hecho:**
  - Logs copiados de `build-xenon/consola/hito1/` (donde los dejó la consola) a
    `runs/2026-11-06_001_consola_dependencias/`; `run.md` con el resultado.
  - `docs/ROADMAP.md`: hito 1 hecho.
  - `AGENTS.md` §4: dónde suelen quedar los logs, y lo que mostró la consola (USB sin perfil, coste del log en USB,
    salida 1080p, mando y audio por SDL, resultados de referencia).
- **Resultado:** `vulkan4xenon_test` 550/550, `vkbench` 17/17, `sdl_check` 18/18, `xsdk_test` 624/630. Los 6 fallos
  de xsdk_test son del entorno: una línea de log en USB cuesta 1758 µs (el test pide < 1500), 4 de XAM/saves en
  paquete sin perfil ni disco duro enumerado, y la red sin `net_host.txt`. Son los mismos 6 que vio BT3.
- **Probado:** consola física.
- **Estado:** hito 1 hecho.
- **Siguiente:** hito 1b (comparación x86 / qemu-ppc del núcleo).
