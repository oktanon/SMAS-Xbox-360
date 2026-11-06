# 0006 — Hito 1b: núcleo big-endian comparado con x86 en qemu-ppc

- **Fecha:** 2026-11-06
- **Objetivo:** compilar el emulador sin ventana (`smastool`) para x86 y para PowerPC big-endian, ejecutar el mismo
  guion en los dos y corregir toda diferencia (`docs/ENDIANNESS.md` §2).
- **Hecho:**
  - `SuperMarioAllStars-main/xenon/qemu/build.sh`: `g++` y `powerpc-linux-gnu-g++ -static`, `-O2 -ffp-contract=off
    -fno-strict-aliasing`, con sello del commit. El núcleo, los juegos y smastool compilaron en PowerPC sin cambios.
  - `tools/smastool.cpp`:
    - `hashall NOMBRE`: FNV-1a por valor de WRAM, VRAM, CGRAM, OAM, ARAM, SRAM, imagen, registros de CPU y todo el
      audio desde el arranque (`hashall reset` lo reinicia).
    - `waitload MAX`: corre frames hasta que termina la carga de un `warp` (500-1500 frames).
    - `writeWav`: escribe little-endian byte a byte. Antes hacía `fwrite` de enteros nativos y en big-endian el WAV
      salía roto. En x86 da el mismo archivo que antes (SHA1 `d84818ba…` antes y después) y en PowerPC ahora coincide.
      Es la excepción a la regla del `#if` descrita en `docs/ENDIANNESS.md` §4.
  - `xenon/qemu/compare.sh`, `guion.txt` (los cuatro juegos, states, rewind, widescreen, trucos, paleta, co-op,
    reset, logros, audio) y `guion_control.txt` (control negativo).
  - `.gitignore`: states, volcados, WAV y PNG de las runs de qemu (contenido derivado de la ROM).
  - `AGENTS.md` (§5 big-endian, §8), `docs/ENDIANNESS.md` (§2.1 nuevo, §4), `docs/ROADMAP.md`.
- **Runs:**
  - 002: el guion tenía errores (no esperaba las cargas y una etiqueta chocaba con un subcomando). No vale.
  - 003: guion corregido, iguales.
  - 004: definitiva, **iguales**: 22 líneas y 9 archivos.
  - 005: control negativo, distinto solo en VRAM y CGRAM volcadas en crudo, como se esperaba.
- **Comprobado además:** un state de x86 cargado en la build PowerPC se rechaza (`loadstate failed`) y deja la máquina
  como estaba. La búsqueda estática de §3.5 solo encuentra la serialización de states (en el orden del host, ya
  rechazada entre plataformas), `statefix.h` (copia un byte a un `bool`) y el WAV de smastool (corregido).
- Los scripts no llevan `#!` (sería una ruta absoluta, AGENTS.md §0): se ejecutan con `bash <script>`.
- **Probado:** qemu-ppc. Pendiente de consola (hito 2: GCC de xsdk, newlib, `std::exp` del DSP).
- **Siguiente:** hito 2, el mismo guion dentro de un `.xex` en la consola.
