# 0002 — Estrategia de orden de bytes (aprendida de los ports de BT3)

- **Fecha:** 2026-11-06
- **Objetivo:** revisar el port de Budokai Tenkaichi 3 a Xbox 360 del usuario
  (`C:\Users\estan\OneDrive\Escritorio\Varios\Tenkaichi3Decomp-main\Tenkaichi3Decomp-main`) y traer lo útil sobre
  little-endian → big-endian.
- **Revisado:** `AGENTS.md` ("Byte order"), `docs/port/xenon_plan.md` (X0, X1, X3), `docs/port/xenon_optimizations.md`,
  `bt3xbx-main/AGENTS.md`, `bt3xbx-main/docs/x360/guia-de-port.md` (§2 herramientas, §5 aprendizajes de bytes);
  además `xsdk-main/docs/PITFALLS.md` ("Big-endian data"), `xsdk-main/docs/GUIDE.md` y
  `vulkan4xenon-main/docs/SUPPORT.md` (formatos en el orden de la CPU).
- **Conclusiones:**
  - El pase LLVM de BT3 (memoria LE en Xenon) no hace falta aquí: el emulador trabaja con arrays de bytes. Se compila
    nativo big-endian.
  - Se adopta la fase F1 de bt3xbx: build PowerPC big-endian de `smastool` bajo `qemu-ppc`, comparada con x86.
    Nuevo hito 1b en el roadmap y tipo de run `qemu`.
  - Puntos concretos del código: framebuffer `0x00RRGGBB` (`ppu.cpp:1259`), save states con bytes del host
    (`snes.cpp:676-700`, `memcpy` en `apu.cpp`/`dsp.cpp`), tabla del DSP con `std::exp` (`dsp.cpp:35-43`), `fmadd`.
    `png.cpp` ya es portable.
- **Ficheros:** `docs/ENDIANNESS.md` (nuevo), `AGENTS.md` (§2 rutas y herramientas, §5 riesgo big-endian, §9, §10),
  `docs/ROADMAP.md` (hito 1b), `runs/README.md`, `runs/_PLANTILLA/run.md`, `docs/CHANGELOG.md`.
- **Comprobado en WSL Ubuntu:** `qemu-ppc` y `g++` instalados; `g++-powerpc-linux-gnu` no instalado (disponible, 15.2).
- **Probado:** no aplica (documentación). Sin run.
- **Siguiente:** que el usuario instale `g++-powerpc-linux-gnu`; luego hito 1b.
