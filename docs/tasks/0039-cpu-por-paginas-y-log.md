# 0039 — Hito 8: run 037, las lecturas de la CPU por páginas (co-op) y el log por frame dibujado

- **Fecha:** 2026-11-10
- **Run 037 (consola, tarea 0038):** de cada 600 frames en 16:9 se dibujan 547-588 en SMB2 (antes 347-460: el
  usuario lo nota), 511-540 en SMB3 y 301-358 en el co-op de SMB1 (casi igual). El snapshot del rewind baja a
  ~0,95 ms. La "emulación por frame" del log no baja, pero mezcla frames dibujados y saltados, y los saltados
  cuestan mucho menos: no dice cuánto cuesta un frame dibujado.
- **Perfil en x86 (gprof, un hilo, 16:9 de 142 px):**
  - Sin dibujar, el co-op de SMB1 cuesta 3,9 ms por frame y SMB1 solo, 2,1. El co-op corre la CPU al doble de
    velocidad (`setCpuFast`: las rutinas del jugador dos veces por frame): 278 M llamadas a `Snes::tick` frente a
    158 M en 2400 frames. **El problema del co-op es la CPU, no la PPU** (ya tardaba 13 ms en 4:3, run 034).
  - SMB3 en 16:9 frente a 4:3: 1,1 ms más por frame en un hilo. En la consola eso va repartido entre tres hilos, y
    el log nuevo dirá cuánto queda en el de la emulación.
- **Hecho:**
  - **`SMAS_CPU_FAST`** (macro nueva; `xenon/CMakeLists.txt`, `xenon/qemu/build.sh`, `compare_ppu.sh`,
    `AGENTS.md`). En `core/snes.h` y `core/snes.cpp`:
    - `cpuRead` lee la memoria normal (WRAM, ROM, área de parches, SRAM) con una tabla de 2048 páginas de 8 KB: el
      puntero y el tiempo de acceso, con FastROM y sin él. Se salta la decodificación de `busRead` y
      `accessTime`. La E/S, y cualquier lectura con un vigilante puesto, van por el original (`cpuReadSlow`).
    - `tick` (inline) solo actualiza los contadores cuando ningún evento de la línea cae dentro de esos ciclos, como
      haría el bucle original; si cae alguno, el bucle (`tickSlow`).
    - La tabla se hace en `loadRom` y se rehace si se mueven los vectores de la ROM o del área de parches (se mira
      al empezar cada frame). `Snes` no se puede copiar con la macro (la tabla apunta dentro del objeto).
  - **El log** (bajo `SMAS_XENON`, `app.cpp`, `app_xenon.cpp`, y `Ppu::profile` bajo `SMAS_PPU_FAST`): una línea
    más por juego cada 10 s, con:
    - el tiempo de un frame dibujado;
    - la PPU en el hilo de la emulación, y cuánto de eso espera a los hilos de dibujo;
    - la CPU con la APU y los juegos (el resto);
    - el tiempo de un frame saltado.
- **Probado:**
  - smastool x86 de HEAD (sin la macro) contra el de ahora (con ella): `guion.txt` **iguales** (22 líneas y 9
    archivos), `guion_ppu.txt` **iguales** (9049 líneas: los cuatro juegos, widescreen, co-op).
  - x86, co-op de SMB1: sin dibujar, de 1,10 a **0,71 ms** por frame (−35 %); dibujando, de 1,92 a 1,56.
  - `compare.sh` (x86 contra PowerPC en qemu): **iguales** (22 líneas, 9 archivos). `HILOS=2 compare_ppu.sh`: **9020 frames iguales**.
  - Compila para la consola; el único aviso es un falso positivo de GCC 15 en `msu1.cpp` (upstream, sin tocar,
    `-Wstringop-overflow` en un `push_back`). Paquete `build-xenon/consola/hito8e/`. **Pendiente de consola** (run 038).
- **Siguiente:** con la línea nueva de la run 038, decidir para SMB3 en 16:9 entre la PPU que queda en el hilo de la
  emulación y la CPU. Para la CPU, lo siguiente son las escrituras. Ahora no tienen camino rápido porque `SmasGame`
  pone siempre vigilantes en cuatro rangos de la WRAM; se podría marcar por página dónde cae alguno.
