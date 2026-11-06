# 0004 — Hito 1: compilar las dependencias y sus tests

- **Fecha:** 2026-11-06
- **Objetivo:** compilar xsdk, vulkan4xenon y SDL-xenon con el toolchain de WSL Ubuntu y preparar sus títulos de
  prueba para la consola.
- **Hecho:**
  - `make` en `xsdk-main` → `build/libxsdk.a`, `build/xsdk_test.xex` (y las sondas). `xenia_scan` avisa de 44
    instrucciones que Xenia ignora (en libstdc++/newlib/xsc; ninguna del port todavía).
  - `make XSDK=../xsdk-main` en `vulkan4xenon-main` → `libvulkan4xenon.a`, `vulkan4xenon_test.xex`, `vkbench.xex`.
  - SDL-xenon (CMake + Ninja) → `build-xsdk/libSDL3.a`; `test/xsdk` → `build-xsdk-check/sdl_check.xex`.
  - Los cuatro `.xex` copiados a `build-xenon/consola/hito1/` (ignorado por git) para el FTP; SHA1 en la run 001.
  - Logs de compilación de SDL en `build-xenon/logs/`.
  - `AGENTS.md` §3: comando de SDL con la ruta del toolchain construida con `$PWD` (relativa no la toma) y el de
    `sdl_check`; aviso de que la configuración tarda más de 10 minutos sobre OneDrive.
- **Probado:** solo compilación. Xenia no se usó (no decide nada en este hito).
- **Estado:** pendiente de consola (run `runs/2026-11-06_001_consola_dependencias/`).
- **Siguiente:** el usuario ejecuta los cuatro títulos en la consola y devuelve los logs; mientras, hito 1b (qemu).
