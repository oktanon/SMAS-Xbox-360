# 0040 — Hito 8: run 038, la PPU por fases en el log y la composición a partes iguales

- **Fecha:** 2026-11-10
- **Run 038 (consola, tarea 0039):**
  - El usuario nota mejora en SMB3 y en el co-op de SMB1; SMB2, igual de bien.
  - Co-op de SMB1 en 16:9: 442-571 frames dibujados de 600 (antes 301-358).
  - La línea nueva del log, por frame dibujado (tabla en la run): en 16:9 la PPU ocupa ~9,5 ms del hilo de la
    emulación, y de eso solo ~1,5 son espera; la CPU con la APU y los juegos, ~5,5 (~7 en co-op).
  - SMB3 en 16:9 sigue al límite: trabajo medio de 16,4-16,8 ms.
- **Perfil en x86 (gprof, un hilo, SMB3 en 16:9):** `composePixels` es casi la mitad de la PPU (27 % del total);
  `renderBgStretch`, 14 %. Lo que va en serie (`renderLine` en sí, `computeWindows`, `applyRemap`) ronda el 10 %
  de la PPU.
- **Hecho (bajo `SMAS_PPU_FAST`, `core/ppu.cpp`):**
  - **La composición a partes iguales:** cada tramo se redondeaba a 32 px. En 16:9 (398 px entre 3 hilos) salían
    160 + 160 + 78, y la línea esperaba a un tramo de 160 en vez de a uno de 133. Ahora se redondea a 8 px.
  - **Sin pantalla secundaria cuando no hace falta:** la pantalla secundaria solo alimenta el color math. En una
    línea donde ninguna capa lo pide (`CGADSUB` bits 0-5 a cero) o donde nunca se aplica (modo 3 de `CGWSEL`), su
    píxel no se usa, así que no se busca.
  - **El log, por fases** (`Ppu::profNs`, que sustituye a `profLineNs`/`profWaitNs` de la tarea 0039). Una línea
    más con lo que tarda cada fase en el hilo de la emulación:
    - preparar la línea con los ganchos `beginLine` (antes contaba como CPU);
    - su parte de las capas, y la espera a los hilos de dibujo;
    - lo que va en serie (remapeos, ventanas);
    - su parte de la composición, y la espera;
    - el final (`composeTail`).
- **Probado:**
  - smastool x86 de HEAD contra el de ahora: `guion.txt` **iguales** (22 líneas y 9 archivos), `guion_ppu.txt`
    **iguales** (9049 líneas).
  - Bench x86 en un hilo, 16:9: SMB3 +2 %, SMB2 +1 %, co-op +1,5 %. El reparto equilibrado solo se nota con hilos.
  - `HILOS=2 compare_ppu.sh`: **9020 frames iguales**. `compare.sh` (x86 contra PowerPC en qemu): **iguales** (22 líneas, 9 archivos).
  - Compila para la consola sin avisos. Paquete `build-xenon/consola/hito8f/`. **Pendiente de consola** (run 039).
- **Siguiente:** con las fases de la run 039, atacar la fase que más pese en 16:9: la composición (`composePixels`),
  los fondos (`renderBgStretch`) o los ganchos de línea.
