# 0038 — Hito 8: los ganchos del widescreen en los hilos de dibujo, y el snapshot sin copias de más

- **Fecha:** 2026-11-10
- **De dónde sale (run 035, `smas_port.log`):** en 16:9 la emulación tarda 14-15 ms por frame frente a 10,5-12 en
  4:3; con el snapshot del rewind (1,2-1,4 ms cada dos frames) y la interfaz, el trabajo medio queda en 16,5-17,3 ms:
  justo en el límite, y SMB2 y SMB3 pierden frames. Los márgenes de cada fondo (142 px por línea en 16:9) y los
  fondos que un gancho pide enteros (`wideFetchAll`, SMB2 en las zonas verticales, SMB3 en los muros) se dibujaban
  **solo en el hilo de la emulación**, llamando a `fetchWideTile` (`PERFORMANCE.md` §3.3 y §4), mientras los dos
  hilos de dibujo hacían la parte 4:3.
- **Auditoría de los ganchos:** `fetchWideTile` de `SmasGame` (desfile, sala del trono, batalla, selección de SMB2,
  tarjeta de SMB2), `Smb1Engine`, `Smb2Engine` y `Smb3Engine`, y todo lo que llaman (`colEntry`, `bg2Entry`,
  `parserPos`, `jarTile`, `openVerticalTile`, `openLayerTile`, `metaEntry`, `bg1Entry`, `bgLayerEntry`,
  `carriesEnd`, `tileMeta`, `standTile`, `selectLattice`, `cardTile`, `Snes::peek`, `Ppu::tileEntry`...): **solo
  leen**. Los auxiliares son `const`; los ganchos de SMB3 y de `SmasGame` no lo son pero no escriben ningún miembro
  ni `static`. Lo que sí escribe (`beginLine`, `learnCard`, `findWalledVertical`...) va antes de dibujar la línea, en
  el hilo de la emulación. Durante el dibujo de una línea la emulación está parada esperando, así que nada cambia lo
  que leen.
- **Hecho (bajo `SMAS_PPU_FAST`, `core/ppu.cpp`, `renderLineThreaded`):**
  - Los márgenes (izquierdo y derecho de cada fondo) y la parte 4:3 de un fondo con `wideFetchAll` son tareas del
    reparto, como la parte 4:3 normal: las toma cualquier hilo, también el de la emulación. Van primero en la lista
    (son las más lentas por píxel), luego los sprites y la parte 4:3 normal.
  - Los cortes son los de siempre (inicio de margen, de la parte 4:3, del otro margen): el original vuelve a pedir el
    tile al empezar cada tramo, así que cada píxel sale de la misma llamada al gancho con los mismos argumentos.
  - Los fondos que el original dibuja a su manera (mosaico, offset-per-tile, color directo) siguen en el hilo de la
    emulación.
  - `Ppu::hookTasks` (por defecto encendido) vuelve a lo de antes si hiciera falta; `hooktasks 0|1` en smastool.
- **Hecho (bajo `SMAS_XENON`, el snapshot):** `Snes::saveState` escribe el estado de la PPU directamente en el state
  (antes, un vector propio a ceros y luego copiado), y `Apu::saveState` reserva su tamaño de entrada (antes crecía a
  trozos y los 64 KB de la ARAM se copiaban de nuevo).
- **Probado:**
  - `HILOS=2 compare_ppu.sh`: **9020 frames iguales** a la PPU original (los cuatro juegos, widescreen, High Frame
    Rate, co-op, trucos).
  - ThreadSanitizer (smastool x86 con `threads 2`, `guion_ppu.txt` entero): **sin avisos** (9020 frames).
  - States (smastool x86, `snes.cpp` y `apu.cpp` con y sin `SMAS_XENON`; menú, SMB1, SMB2, SMB3 y uno tras cargar):
    los 279 049 bytes iguales salvo **un byte de relleno de `DspFlat`** (y el CRC, que lo cubre). `Dsp::saveState`
    declara `DspFlat f;` sin inicializar y copia su relleno, que sale de lo que hubiera en la pila: ya pasaba antes,
    no cambia nada al cargar (anotado en `docs/UPSTREAM.md`).
  - `compare.sh` (x86 contra PowerPC en qemu): **IGUALES** (22 líneas, 9 archivos).
  - Compila para la consola sin avisos. Paquete `build-xenon/consola/hito8d/`.
  - En x86 no se mide la ganancia (las líneas son 10 veces más cortas y la sincronización se come lo que se gana,
    §3.3). **Pendiente de consola** (run 037).
- **Lo esperado en la run 037:** la emulación en 16:9 baja hacia la de 4:3 (en la línea del log), sobre todo en SMB2
  y SMB3; el snapshot del rewind, algo por debajo de 1,2 ms.
- **Siguiente:** si 16:9 llega, el co-op (13-14 ms en 4:3); si no, `computeWindows`/`applyRemap`/`composeTail`
  (en serie en el hilo de la emulación) y el dibujo diferido (§3.3).
