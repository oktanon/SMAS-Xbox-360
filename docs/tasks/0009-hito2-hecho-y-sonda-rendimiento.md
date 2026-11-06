# 0009 — Hito 2 hecho; guardado en USB corregido; sonda de rendimiento

- **Fecha:** 2026-11-06
- **Objetivo:** revisar la run 009 (consola, v1.1.1), corregir lo que muestre y preparar la siguiente.
- **Run 009:**
  - **Hito 2 hecho:** 22 líneas iguales a la run 004 y 9 salidas idénticas a x86 (`compare_consola.sh`).
  - **Velocidad:** 22-38 fps en un hilo, ~5,8 veces más lento que x86. Tabla en `docs/PERFORMANCE.md` §1.
  - **Los `.tmp`:** el log y el listado de `game:\` que hace la propia consola muestran que el USB crea
    `menu.png.tmp` como `menu.tmp`, y el `rename` no lo encuentra (`0xC0000034`). Fallan los nombres cuya parte antes
    del último punto mide 8 caracteres y lleva un punto (`menu.png`, `smb1.png`...); `ll.png.tmp`,
    `smb3_mapa.png.tmp`, `smb1.state.tmp` y los `diag_*` funcionan. `rename` sobre un destino que existe funciona.
- **Hecho:**
  - `src/core/fileio.cpp`: en Xbox (`SMAS_XENON`), el temporal de `writeFileSafely` es el nombre con los puntos
    cambiados por `_` más `.tmp` (`menu_png.tmp`). Fuera de Xbox sigue siendo `<nombre>.tmp`. Es el único sitio del
    código que crea temporales.
  - `hito2_main.cpp`: el diagnóstico crea con `fopen` nombres alrededor del caso que falla (para confirmar la regla) y
    prueba `writeFileSafely` con `menu_diag.png`, `abcd.png`, `abc.defg`, `smb9.png`.
  - `tools/smastool.cpp`: `render on|off` (`ppu.skipRender`).
  - `xenon/hito2_bench.txt`: cada juego con la PPU dibujando y sin dibujar, y el rewind.
  - `xenon/CMakeLists.txt`: dos variantes, `smas_hito2.xex` (`-O2`) y `smas_hito2_o3.xex` (`-O3`; un aviso
    `-Wstringop-overflow` en `Msu1::saveState` → `vector::push_back`, falso positivo conocido de GCC a `-O3`).
  - `xenon/compare_consola.sh`: nombre del `.xex` como segundo argumento.
  - `docs/PERFORMANCE.md` (nuevo), `docs/ROADMAP.md` (hito 2 hecho; hito 2b de rendimiento antes del vídeo),
    `AGENTS.md` §4 (lo que mostró la consola).
- **Medido en x86:** sin dibujar, el emulador va 4-5 veces más rápido: la PPU es ~75 % del tiempo
  (`docs/PERFORMANCE.md` §2). Las 22 líneas siguen iguales a la run 004 con el guion nuevo.
- **Probado:** x86; humo de `-O3` en Xenia. Pendiente de consola (run 010).
- **Siguiente:** con la run 010, elegir la línea de trabajo del hito 2b.
