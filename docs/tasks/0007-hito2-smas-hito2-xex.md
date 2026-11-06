# 0007 — Hito 2: el emulador sin ventana como .xex para la consola

- **Fecha:** 2026-11-06
- **Objetivo:** ejecutar en la consola el mismo guion que la comparación x86 / qemu-ppc (run 004), con el GCC de xsdk y
  newlib, y medir la velocidad del emulador.
- **Hecho:**
  - `SuperMarioAllStars-main/xenon/CMakeLists.txt`: primer build de Xbox. Biblioteca `smascore` (núcleo y juegos, los
    mismos ficheros que Windows y Android, `-O2 -ffp-contract=off -fno-strict-aliasing`, `SMAS_XENON`,
    `SMAS_BIG_ENDIAN`) y el título `smas_hito2.xex` ("SMAS hito 2").
  - `src/platform/xenon/hito2_main.cpp`: splash de vulkan4xenon y luego `smastool_main` (smastool.cpp compilado con
    `-Dmain=smastool_main`, sin tocarlo) con la ROM y `guion.txt` de `game:\`. `stdout` va al log de xsdk
    (`game:\smas_hito2.log`), línea a línea. Al terminar vuelve al dashboard.
  - `xenon/hito2_bench.txt`: benchmarks (`bench 600`) del menú, SMB1, SMB2, SMB3, SMB3 en widescreen y SMB1 en co-op.
    El build concatena `qemu/guion.txt` y este fichero en `build-xenon/guion.txt`. Los benchmarks van al final y no
    cambian las 22 líneas que se comparan (comprobado en x86).
  - `xenon/compare_consola.sh`: compara el log de la consola con `runs/…004…/x86/hashes.txt` y los PNG/WAV traídos con
    `archivos.sha1`, avisa si el log se corta y lista los benchmarks.
  - Paquete en `build-xenon/consola/hito2/`: `smas_hito2.xex`, `guion.txt` y la ROM.
  - `.gitignore`: ningún PNG ni SRAM de las runs.
  - `AGENTS.md` §3 (comando real del build de Xbox), `docs/ROADMAP.md`.
- **Probado:**
  - x86: el guion de consola da las mismas 22 líneas que la run 004. Benchmarks de referencia en x86 (WSL): 126-215 fps.
  - Xenia (run 006): arranca y avanza por menú, SMB1 y Lost Levels; las líneas y los PNG/WAV que dio tiempo a hacer
    coinciden con x86, audio incluido.
  - `xenia_scan`: en el código del port solo `Smb3Engine::planFill` usa instrucciones que Xenia ignora (SMB3 no es
    fiable en Xenia).
- **Estado:** pendiente de consola (run 007).
- **Siguiente:** con el log de la consola, cerrar el hito 2 o corregir; los fps deciden el trabajo de rendimiento.
