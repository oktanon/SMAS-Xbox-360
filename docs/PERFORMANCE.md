# Rendimiento en la consola

El emulador tiene que dar 60 fps en la consola con margen para el vídeo, el audio, la interfaz, el rewind y el
co-op. Todo número de rendimiento sale de la consola física: Xenia y qemu no miden nada de esto (AGENTS.md §4).

## 1. Dónde estamos (run 009, 2026-11-06, base v1.1.1, `-O2`)

`bench 600` de `smastool` en la consola, un solo hilo, sin vídeo, audio ni interfaz:

| Escena | Consola | x86 (WSL, referencia) | Consola / tiempo real |
|---|---|---|---|
| Menú de All-Stars | 38,0 fps | 215 fps | 63 % |
| SMB1 1-1 corriendo | 26,5 fps | 149-154 fps | 44 % |
| SMB2 1-1 | 24,0 fps | 146-149 fps | 40 % |
| SMB3 1-1 corriendo | 28,8 fps | 175-178 fps | 48 % |
| SMB3 1-1, widescreen 64 | 25,1 fps | 139-142 fps | 42 % |
| SMB1 co-op | 21,8 fps | 122-126 fps | 36 % |

La consola va unas **5,8 veces más lenta** que el PC de referencia. El rewind también: guardar un frame 6,1 ms (x86
2,5 ms) y volver uno 48 ms (x86 10 ms).

## 2. Dónde se va el tiempo (x86, tarea 0009)

`render off` (`ppu.skipRender`, lo que la app usa en los frames que se saltan) deja correr la CPU 65816, la APU y los
juegos sin que la PPU dibuje:

| Escena (x86) | Dibujando | Sin dibujar | Parte de la PPU |
|---|---|---|---|
| Menú | 215 fps | 743 fps | ~71 % |
| SMB1 1-1 | 149 fps | 672 fps | ~78 % |
| SMB2 1-1 | 146 fps | 681 fps | ~79 % |
| SMB3 1-1 | 178 fps | 709 fps | ~75 % |
| SMB1 co-op | 122 fps | 414 fps | ~70 % |

**La PPU (el dibujo de cada línea, `Ppu::renderLine`) se lleva unas tres cuartas partes del tiempo.** Si la consola
reparte igual, la CPU y la APU solas irían a ~100-130 fps en la consola, y el problema es casi entero del dibujo. La
run 010 lo mide en la consola, y también `-O3` frente a `-O2`.

## 3. Líneas de trabajo (a decidir con la run 010)

1. **Dibujar la PPU en otros hilos hardware.** La consola tiene 6 (3 núcleos × 2). El dibujo no cambia el estado que
   lee el juego (salvo los flags de "time over"/"range over" de OBJ, a revisar), así que el hilo de emulación puede
   guardar el estado de la PPU de cada línea y otros hilos dibujar las líneas en paralelo, como hacen otros
   emuladores. Es la ganancia mayor y la que más trabajo pide; hay que mantener la imagen idéntica (los hashes
   `pic=` de `hashall` lo comprueban, en qemu y en la consola).
2. **Opciones del compilador:** `-O3` (run 010), y después LTO y `-funroll-loops` en `ppu.cpp`, medido siempre en la
   consola.
3. **El código del dibujo en un procesador in-order:** menos saltos difíciles de predecir, evitar load-hit-store
   (40-80 ciclos; BT3 `docs/port/xenon_optimizations.md`) y pasos de enteros a float (31/51 ciclos, xsdk
   `docs/HARDWARE.md`), tablas en vez de ramas, líneas de caché de 128 bytes (`dcbt`).
4. **La APU (SPC700 + DSP) en otro hilo**, si después del punto 1 la CPU + APU no dejan margen. Más delicado: tiene que
   seguir sincronizada con la CPU para dar los mismos hashes.
5. **Rewind:** 6 ms por frame guardado es demasiado en la consola; revisar cuántas veces guarda y si puede comprimir
   en otro hilo.

Cada cambio se mide en la consola (run con `bench`) y se comprueba en qemu y en la consola que los hashes siguen
iguales a los de la run 004 (o a la referencia que la sustituya).
