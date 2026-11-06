# Rendimiento en la consola

El emulador tiene que dar 60 fps en la consola con margen para el vídeo, el audio, la interfaz, el rewind y el
co-op. Todo número de rendimiento sale de la consola física: Xenia y qemu no miden nada de esto (AGENTS.md §4).

## 1. Dónde estamos (runs 009 y 010, 2026-11-06, base v1.1.1)

`bench 600` de `smastool` en la consola, un solo hilo, sin vídeo, audio ni interfaz. "Sin PPU" es `render off`
(`ppu.skipRender`, lo que la app usa en los frames que se saltan): la CPU 65816, la APU y los juegos sin dibujar.

| Escena | `-O2` | `-O2` sin PPU | PPU (ms/frame) | Resto (ms/frame) | % PPU | `-O3` | `-O3` PPU / resto (ms) |
|---|---|---|---|---|---|---|---|
| Menú de All-Stars | 38,0 fps | 164,6 fps | 20,2 | 6,1 | 77 % | 37,1 fps | 21,4 / 5,5 |
| SMB1 1-1 corriendo | 26,5 fps | 161,3 fps | 31,5 | 6,2 | 84 % | 27,1 fps | 31,4 / 5,5 |
| SMB2 1-1 | 24,0 fps | 161,3 fps | 35,5 | 6,2 | 85 % | 25,0 fps | 34,5 / 5,5 |
| SMB3 1-1 corriendo | 28,8 fps | 159,7 fps | 28,5 | 6,3 | 82 % | 30,5 fps | 27,1 / 5,7 |
| SMB3 1-1, widescreen 64 | 25,1 fps | — | — | — | — | 26,8 fps | — |
| SMB1 co-op | 21,8 fps | 96,3 fps | 35,5 | 10,4 | 77 % | 22,3 fps | 35,9 / 9,0 |

- **La PPU es el 77-85 % de cada frame (20-36 ms).** Sin dibujar, la CPU, la APU y los juegos cuestan ~6 ms
  (160 fps); 10 ms en co-op.
- `-O3` baja el resto un 12 % y no mejora la PPU: hasta 6 % en total. Los resultados son idénticos con las dos.
- La consola va ~6 veces más lenta que x86 en la PPU y ~4 veces en el resto: el código del dibujo encaja
  especialmente mal en el procesador in-order.
- Rewind: guardar un frame 6,0 ms (`-O3` 5,8) y volver uno 48 ms.
- Objetivo: 16,7 ms por frame con margen para vídeo, audio e interfaz. La PPU tiene que bajar de ~32 ms a menos de
  ~8 ms en el hilo principal.

## 2. Dónde se va el tiempo dentro (perfil en x86, gprof, SMB1 + SMB2 + SMB3)

| Función | % del total |
|---|---|
| `Ppu::renderBg` (fondos, por línea y capa) | 36,6 % |
| `Ppu::composeLine` (mezcla de capas, ventanas, color math, salida) | 31,9 % |
| `Ppu::renderObj` (sprites) | 2,8 % |
| CPU 65816 (`Snes::tick`, `cpuRead`, `busRead`, `step`...) | ~12 % |
| APU (`Dsp::runSample`, `interpolate`, `Apu::step`...) | ~7 % |

Dos funciones de `src/core/ppu.cpp` concentran el 69 % del tiempo.

## 3. Plan (hito 2b)

Estado: paso 1 hecho y medido en consola (run 012); siguen los pasos 2-5 y lo que queda de la PPU (§3.2).

En orden; cada paso se mide en la consola (`bench`) y se comprueba que los hashes no cambian (qemu y consola).

1. **`renderBg` y `composeLine` para un procesador in-order, en un hilo.** Sin cambiar resultados: menos ramas por
   píxel (decidir por línea o por tile, no por píxel), tablas en vez de condiciones, nada de load-hit-store (40-80
   ciclos; BT3 `docs/port/xenon_optimizations.md`) ni de pasos entre enteros y float (31/51 ciclos, xsdk
   `docs/HARDWARE.md`), escritura del framebuffer por palabras y `dcbt` para las líneas de 128 bytes. Es lo más
   seguro y beneficia también a PC y Android; los cambios van con `#if` si solo sirven para Xbox.
2. **Dibujo diferido en otros hilos hardware.** El hilo de emulación guarda por línea lo que la PPU necesita (los
   registros de la línea y lo que fijan los ganchos del juego, `hooks->beginLine`) y los hilos auxiliares dibujan las
   líneas en paralelo mientras la CPU sigue. Lo delicado: la VRAM, la CGRAM y la OAM pueden cambiar a mitad de frame
   (HDMA, degradados de color), los ganchos del juego por línea y lo que el dibujo escribe y el juego lee (flags de
   OBJ "time over"/"range over", `objMoves`, las líneas de High Frame Rate). Con 6 hilos hardware (3 núcleos × 2,
   que comparten el pipeline) la ganancia realista es de 3-4 veces en el dibujo, así que hace falta también el punto 1.
3. **Compilador:** `-O3` para el núcleo (ya medido: un poco mejor, sin cambios en los resultados); probar LTO.
4. **Rewind:** 6 ms por frame guardado no cabe; revisar cada cuánto guarda la app y llevar la compresión a otro hilo.
5. **La APU en otro hilo** solo si después de lo anterior falta margen.

### 3.1 Hecho (tarea 0011): `renderBg` y `composeLine` rápidas, bajo `SMAS_PPU_FAST`

`src/core/ppu.cpp`, con los cuerpos originales intactos (en el build rápido se compilan como `renderBgGeneric` y
`composeLineGeneric`; sin la macro, todo es el original, como en PC y Android):

- `renderBg`: los fondos por tiles de 8 píxeles en vez de píxel a píxel: lo que es igual en toda la línea
  (desplazamientos, fila del mapa) se calcula una vez, los bitplanes pasan a píxeles con tablas (`kPlane`) en vez de un
  bucle de bits, y los píxeles del tile se sacan de registros (nada de guardar y releer: load-hit-store). Los márgenes
  del widescreen también, llamando al gancho del juego (`fetchWideTile`) exactamente cuando lo hacía el original.
  Van por el original solo offset-per-tile (modos 2/4/6), mosaico y color directo.
- `composeLine`: por línea, cada (capa, prioridad) recibe su puesto en la lista de prioridades como rango; cada píxel
  toma la capa de rango menor (5 lecturas y mínimos sin saltos) en vez de recorrer la lista con saltos. Las
  decisiones de clip y color math que dependen solo de la línea, en tablas.

**Comprobación:** `xenon/qemu/compare_ppu.sh` compila smastool x86 con y sin `SMAS_PPU_FAST` y compara el hash de
la imagen (con las intermedias del High Frame Rate) **frame a frame** en `guion_ppu.txt`: 9020 frames de los cuatro
juegos (16 niveles y mapas, widescreen, High Frame Rate 2 y 3, trucos, paleta, co-op, reset): **iguales**. Control
negativo (`compare_ppu.sh <carpeta> control`, un bit de color en un píxel de BG1): **distintos**. En qemu-ppc (run
011) las 22 líneas son las mismas que la x86 original de la run 004.

**x86 (ms de PPU por frame, original → rápida):** menú 1,35 → 0,46-0,55; SMB1 2,04 → ~0,87; SMB2 2,15 → ~0,90;
SMB3 1,72 → ~0,94; widescreen 64: 3,11 → 1,96 ms el frame entero; co-op 1,58 → ~0,77. Entre 2 y 3 veces. La consola
lo dirá (run 012); el `composeLine` sin saltos está pensado para ella (en x86 rinde como la primera versión).

**En la consola (run 012):** la PPU es 2,4-4,3 veces más rápida (más que en x86: el `composeLine` sin saltos rinde
en el procesador in-order), y los resultados siguen idénticos a la run 004.

| Escena | Antes `-O2` | Ahora `-O2` | Antes `-O3` | **Ahora `-O3`** | PPU `-O3` (ms/frame) antes → ahora |
|---|---|---|---|---|---|
| Menú de All-Stars | 38,0 | 84,8 | 37,1 | **95,1 fps** | 21,4 → 5,0 |
| SMB1 1-1 corriendo | 26,5 | 57,3 | 27,1 | **65,3 fps** | 31,4 → 9,9 |
| SMB2 1-1 | 24,0 | 55,7 | 25,0 | **62,9 fps** | 34,5 → 10,5 |
| SMB3 1-1 corriendo | 28,8 | 55,1 | 30,5 | **63,0 fps** | 27,1 → 10,2 |
| SMB3 1-1, widescreen 64 | 25,1 | 44,2 | 26,8 | **50,4 fps** | — |
| SMB1 co-op | 21,8 | 44,7 | 22,3 | **51,4 fps** | 35,9 → 10,5 |

- Con `-O3` los cuatro juegos pasan de 60 fps en un hilo; `-O3` ahora gana un 12-14 % sobre `-O2`, así que **el
  núcleo se compila con `-O3`** en el port.
- Falta margen: el frame tiene que dejar sitio al vídeo, el audio y la interfaz (y al rewind), y el widescreen y el
  co-op siguen por debajo de 60. La PPU sigue siendo ~65 % del frame (~10 ms de ~15,5).
- El rewind: volver un frame bajó de 48 a 22-24 ms (redibuja); guardar uno sigue en ~6 ms.

Herramientas: `smastool` (`bench`, `render on|off`, `hashall`, `pichash`), `xenon/qemu/compare_ppu.sh`, `xenon/qemu/compare.sh` para los resultados,
`xenon/hito2_bench.txt` para la consola, gprof en x86 para encontrar el código caliente (la proporción es parecida,
pero los números valen solo de la consola).

### 3.2 Siguiente

1. **Lo que queda de la PPU en un hilo:** el widescreen (los márgenes) y el co-op son los peores casos; en el perfil de
   x86 quedan `composeLine` (la parte por píxel del color math y la salida), `renderObj` y `computeWindows`.
2. **El dibujo en otros hilos hardware** (paso 2): con la PPU en ~10 ms y el resto en ~5,5 ms, pasar el dibujo a otro
   hilo dejaría el frame del hilo principal en ~6 ms; es lo que da el margen para vídeo, audio, interfaz, rewind,
   widescreen y co-op.
3. **Rewind:** ~6 ms por frame guardado.

### 3.3 El dibujo en varios hilos (tarea 0013)

`Ppu::setRenderThreads(n)` (bajo `SMAS_PPU_FAST`; `threads N` en smastool): cada línea visible se dibuja entre el
hilo de la emulación y `n` hilos más. La emulación sigue esperando cada línea, así que la imagen se dibuja del mismo
estado que en el original (nada de dibujo diferido: ver abajo por qué).

- **Reparto:** los hilos auxiliares toman la parte 4:3 de cada fondo (`renderBgStretch`), el modo 7 y los sprites
  (`renderObj`); el hilo de la emulación, lo que llama a los ganchos de los juegos (los márgenes del widescreen, los
  fondos que un gancho pide enteros con `wideFetchAll`, los dibujados por el original: mosaico, offset-per-tile, color
  directo), además de `applyRemap` (usa un búfer `static`) y las ventanas. Después, la composición
  (`composePixels`) en tantos tramos como hilos, y `composeTail` en el de la emulación.
- **Por qué los ganchos se quedan en el hilo de la emulación:** `fetchWideTile` lee el estado de cada motor (cámara,
  columnas, copias) y la WRAM a través de funciones que no se han auditado para hilos; ninguno escribe en el `Ppu`
  (comprobado), pero así no hace falta que sean seguros entre hilos.
- **Reparto de tareas:** una palabra atómica con la ronda (24 bits) y el índice de la siguiente tarea (8 bits); un hilo
  toma una tarea con un compare-and-swap que solo vale en la ronda que vio. Dos conjuntos de tareas alternan por ronda.
  Los auxiliares esperan dando vueltas (con `yield` tras ~65 000 vueltas sin trabajo).
- **Colocación:** la plataforma da `Ppu::renderThreadStart`; en la consola, `hito2_main.cpp` lleva los hilos a los
  hilos hardware 2, 4, 3, 5, 1 (`xsdk_thread_move`); la emulación va en el 0.
- **Comprobado:** `HILOS=1|2|3 compare_ppu.sh`: 9020 frames iguales al original; ThreadSanitizer sin avisos (los
  cuatro juegos, widescreen, High Frame Rate, co-op, cambios de número de hilos); qemu-ppc con 2 hilos (run 013).
- **En x86 no gana** (la línea dura ~4 µs y sincronizar entre núcleos, bajo WSL, se come la ganancia); en la consola
  una línea dura ~45 µs, así que la sincronización pesa unas 10 veces menos: lo mide la run 014.
- **El dibujo diferido** (dibujar la línea N mientras la CPU sigue) daría más, pero pide guardar por línea el estado
  de la PPU, la VRAM/CGRAM/OAM que cambian a mitad de frame y el de los juegos que leen los ganchos. Queda como
  siguiente paso si esto no basta.

**En la consola (run 014, `-O3`, PPU rápida):** resultados idénticos a la run 004 dibujando con 2 hilos. fps según
los hilos de dibujo además del de la emulación (en los hilos hardware 2, 4 y 3):

| Escena | 0 | 1 | **2** | 3 | ms/frame, 0 → 2 |
|---|---|---|---|---|---|
| Menú de All-Stars | 97,2 | 106,3 | **107,6** | 105,3 | 10,3 → 9,3 |
| SMB1 1-1 corriendo | 66,4 | 87,5 | **95,8** | 92,3 | 15,1 → 10,4 |
| SMB2 1-1 | 63,1 | 82,0 | **90,6** | 86,4 | 15,8 → 11,0 |
| SMB3 1-1 corriendo | 64,6 | 89,8 | **96,4** | 92,4 | 15,5 → 10,4 |
| SMB3 1-1, widescreen 64 | 52,7 | 72,6 | **81,0** | 78,5 | 19,0 → 12,3 |
| SMB1 co-op | 51,5 | 62,7 | **66,9** | 65,6 | 19,4 → 14,9 |

- **2 hilos es lo mejor**; el tercero (hilo hardware 3) comparte núcleo con el primero y resta. El port dibuja con 2
  hilos en los hilos hardware 2 y 4, y deja libres el 1, el 3 y el 5 (audio, vídeo, rewind).
- Sin dibujar, el resto va a 184 fps (5,4 ms).
- Margen por frame hasta 16,7 ms: ~6 ms en juego, 4,4 ms en widescreen, **1,8 ms en co-op**.
- **Rewind:** guardar un snapshot cuesta 5,8 ms y la app guarda uno cada 2 frames (`app.cpp`, `rewindCount & 1`):
  ~2,9 ms de media por frame, picos de 5,8 ms. No cabe en co-op; candidato a ir a otro hilo (hito 8). Volver un
  frame: 17 ms (antes 22-48).

**Hito 2b cerrado** con esto (tarea 0014). El margen del co-op y el rewind se miden con el juego entero (vídeo, audio,
interfaz) en el hito 8.

