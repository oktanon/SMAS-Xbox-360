# Orden de bytes: de little-endian (PC) a big-endian (Xenon)

Estrategia del port para el orden de bytes, con lo aprendido en los ports de Budokai Tenkaichi 3 a Xbox 360
(`../Varios/Tenkaichi3Decomp-main/Tenkaichi3Decomp-main`, relativo a la raíz del repositorio; abajo **BT3**):

- **BT3/port/xenon** (`AGENTS.md` "Byte order", `docs/port/xenon_plan.md` X1, `docs/port/xenon_optimizations.md`):
  xsdk + vulkan4xenon, el código del juego compilado con un pase LLVM que invierte cada load/store.
- **BT3/bt3xbx-main** (`docs/x360/guia-de-port.md` §2 y §5, `docs/x360/decisoes.md`): build PowerPC big-endian
  headless bajo `qemu-ppc` comparada automáticamente con la build x86.

## 1. Por qué aquí es más simple que en BT3

BT3 es una decompilación de PS2: el código del juego da por hecho memoria little-endian (punning, uniones, campos de
bits sobre paquetes de GPU, datos del disco leídos como structs). Por eso tuvieron que mantener la memoria del juego
en little-endian con un pase LLVM, o convertir cada formato.

Nuestro caso es un **emulador**: la RAM, VRAM, CGRAM, OAM y ARAM de la SNES son arrays de `uint8_t`, y el núcleo
arma los valores de 16 bits byte a byte (`lo | hi << 8`), lo que da el mismo resultado en cualquier host. Una búsqueda
inicial (2026-11-06) no encontró `union`, casts `*(uint16_t*)` ni `reinterpret_cast` a enteros en `src/core`,
`src/game` ni `src/app`. **Decisión: compilar todo nativo big-endian con el GCC de xsdk, sin pase LLVM**, y corregir
solo los puntos concretos de la lista de §3. Si la comparación de §2 encontrara demasiados problemas, se revisa esta
decisión (no antes).

## 2. Cómo se verifica: build PowerPC big-endian bajo qemu (lección de bt3xbx, fase F1)

> "A F1 existe porque testar ordem de bytes no console custa uma rodada manual por tentativa. No qemu roda uma luta
> inteira em 20 minutos, com comparação automática." (bt3xbx `docs/x360/guia-de-port.md` §3)

Antes de tocar la consola, el núcleo se compila dos veces en WSL Ubuntu y se compara:

| Build | Compilador | Corre en |
|---|---|---|
| Referencia LE | `g++` nativo x86-64 | WSL |
| BE | `powerpc-linux-gnu-g++ -static` (paquete `g++-powerpc-linux-gnu` 15.2, instalado 2026-11-06) | `qemu-ppc` (`qemu-user` 10.2); con `-static` no necesita la carpeta de bibliotecas de PowerPC |

Las dos ejecutan `tools/smastool.cpp` (headless, ya portable) con el mismo guion de entradas y escriben hashes cada
N frames. Cualquier diferencia solo puede venir del orden de bytes (o del float, ver §3.6). Esto es mucho más rápido
y fiable que Xenia para la lógica, y no gasta runs de consola. Las runs de qemu también se guardan en `runs/` (tipo
`qemu`).

Reglas de comparación (bt3xbx §5.4):

- **Comparar la memoria, no solo lo impreso.** Hash de WRAM (128 KB), VRAM, CGRAM, OAM, ARAM del SPC700, registros
  de CPU/PPU/DSP y del framebuffer, no solo de la posición de Mario. (En BT3 la pelea "coincidía" en 11 palabras con
  el heap desplazado.)
- **Hashear bytes, o valores, pero nunca la memoria cruda de arrays de `uint32_t`/`int16_t`**: el framebuffer
  (`uint32_t` 0x00RRGGBB) y el audio (`int16_t`) tienen bytes distintos en LE y BE aunque los valores sean iguales.
  Hashear valor a valor (`v & 0xFF`, `v >> 8`, ...). El `hash` actual de smastool (FNV-1a de WRAM, byte a byte) vale.
- **Estampar cada build con su commit** y comparar solo builds del mismo commit.
- **Borrar las salidas de la run anterior** antes de la siguiente (en bt3xbx un comando que ni llegó a correr dejó el
  `hash.txt` viejo y la comparación "pasó").
- Guion fijo: los cuatro juegos desde power-on, `warp` a niveles concretos, `rewindtest`, save/load state, cheats,
  `wide`, co-op, `ach on` (los eventos de logros deben salir idénticos en las dos builds).

### 2.1 Cómo se ejecuta (hito 1b, hecho 2026-11-06)

Desde la raíz del repositorio, en WSL (desde PowerShell: `wsl.exe -d Ubuntu --exec bash -c '...'`):

```
bash SuperMarioAllStars-main/xenon/qemu/build.sh all            # build-qemu/x86/smastool y build-qemu/ppc/smastool
bash SuperMarioAllStars-main/xenon/qemu/compare.sh runs/<run>   # guion.txt en las dos; sale 0 si todo coincide
bash SuperMarioAllStars-main/xenon/qemu/compare.sh runs/<run> SuperMarioAllStars-main/xenon/qemu/guion_control.txt
```

- `smastool` tiene para esto `hashall NOMBRE` (FNV-1a por valor de WRAM, VRAM, CGRAM, OAM, ARAM, SRAM, imagen, CPU y
  de todo el audio desde el arranque) y `waitload MAX` (corre frames hasta que termina la carga de un `warp`, que
  tarda 500-1500 frames).
- `compare.sh` compara las líneas `HASHALL`, `ACH` y `WAITLOAD` y los SHA1 de todos los archivos que deja el guion
  (PNG, WAV, volcados) salvo los save states. Exige que las dos builds sean del mismo commit y borra antes las
  salidas de la run anterior.
- El guion tarda ~50 s en x86 y ~7,5 min en qemu-ppc.
- **Control negativo** (`guion_control.txt`, run 005): `dump` vuelca la VRAM y la CGRAM (arrays de `uint16_t`) en
  crudo, y la comparación debe dar DISTINTOS solo en esos dos ficheros. Repetirlo si se cambia `compare.sh`.

**Resultado (run 004):** las dos builds dan lo mismo en todo el guion **sin cambiar nada del núcleo, de los juegos ni
de la app**. Solo `smastool` tenía un fallo de bytes: escribía la cabecera y las muestras del WAV con `fwrite` de
enteros nativos (WAV es little-endian); ahora escribe byte a byte y en x86 sale el mismo archivo que antes (mismo SHA1).
`dump` sigue volcando en el orden del host (es el control negativo). Un state de x86 cargado en la build PowerPC se
rechaza (`loadstate failed`, el `magic` invertido) y deja la máquina como estaba.

Lo que qemu **no** cubre y queda para la consola (hito 2): el GCC de xsdk y newlib en vez del GCC de Linux y glibc
(`std::exp` de la tabla del DSP, el resto del float), y el rendimiento. En la run 007 (consola, base v1.0) las 8
capturas y el WAV del guion salieron idénticos a los de x86: el GCC de xsdk y newlib calculan igual, audio incluido.

**Base v1.1.1 (tarea 0008):** `compare.sh` también ejecuta `msu1_test` (una pista MSU-1 sintética) en las dos builds.
Upstream añadió `src/core/msu1.cpp`, que tenía el único error de orden de bytes encontrado en el código del juego
(§3.4).

## 3. Lista de puntos a revisar

1. **Píxeles para la GPU.** La PPU escribe `uint32_t` 0x00RRGGBB (`src/core/ppu.cpp:1259`). En memoria LE son los
   bytes B,G,R,X (por eso PC usa `DXGI_FORMAT_B8G8R8A8_UNORM`); en BE son X,R,G,B. vulkan4xenon toma los formatos
   *packed* "en el orden de bytes de la CPU" (`vulkan4xenon-main/docs/SUPPORT.md`, "On xsdk") y xsdk sube texels "in
   the CPU's byte order" (`xsdk-main/docs/GUIDE.md`). **No suponer:** el primer paso del hito de vídeo es una textura
   de 4 píxeles (rojo, verde, azul, blanco) verificada en Xenia y en consola. Si los canales salen cambiados, se
   arregla con el swizzle del sampler o del shader (gratis), no invirtiendo en CPU. El byte X vale 0: el shader debe
   ignorar alfa.
2. **Vértices, índices y uniforms** en el orden de la CPU, tal cual (GUIDE.md: "Indices are big-endian, as the CPU
   writes them"). En BT3 había que invertirlos porque sus datos eran LE; los nuestros nacen nativos, así que no.
   Memoria de GPU: solo stores `volatile` de 32 bits (un `std` desalineado cuelga la consola; PITFALLS.md).
3. **Audio.** El DSP produce `int16_t` nativos; SDL-xenon los convierte a los floats big-endian de xsdk. Pedir
   `SDL_AUDIO_S16` (nativo, = `S16BE` en Xenon), **nunca** `S16LE` fijo.
4. **Ficheros escritos en PC y leídos en Xbox:**
   - `.srm` (SRAM cruda, bytes): compatible tal cual.
   - ROM: bytes; la cabecera interna se lee byte a byte. Verificar `fileio.cpp` / detección de CRC.
   - Save states: el `Writer`/`Reader` de `snes.cpp:676-700` copia los bytes de cada valor en el orden del host, y
     `apu.cpp`/`dsp.cpp` hacen `memcpy` de structs enteras (con su relleno). Un state de PC leído en Xbox tendría
     todos los números invertidos. Lo mismo vale para los snapshots de rewind (en memoria: sin problema) y para el
     bloque de logros de los states (`AchievementTracker::save`). Lección de bt3xbx §5.1.7: una suma de
     comprobación de bytes **no** detecta otro orden. Detectarlo por el `magic` (`kStateMagic`): si coincide
     invertido, rechazar con mensaje claro ("state de otra plataforma"). Opcional y más adelante: hacer los states
     portables escribiendo LE explícito.
   - `smas_port.ini`, `achievements.ini`, `rompath.txt`: texto, compatibles. Ojo a `wchar_t`: en xsdk/newlib es de
     32 bits; las conversiones UTF-8 de `utf8.cpp` deben funcionar igual (probar en qemu).
   - PNG (`png.cpp`): portable, escribe cabeceras y CRC con desplazamientos (`be32`) y lee los píxeles como valores.
   - Datos incrustados (`logo_data.h`, fuentes TTF): bytes, sin problema; si alguna tabla es de `uint16_t`/`uint32_t`
     generada como bytes, revisarla.
   - **Pistas MSU-1** (`.pcm`, v1.1): "MSU1", el bucle de 32 bits y muestras estéreo de 16 bits, todo little-endian.
     La cabecera se lee byte a byte, pero `Msu1::nextSample` leía las muestras como `int16_t` del host (su comentario:
     "x86 and ARM are little-endian"): en Xbox, ruido. **Corregido** (tarea 0008) con un `bswap16` bajo
     `__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__`. Lo comprueba `xenon/qemu/msu1_test.cpp` dentro de `compare.sh`; sin la
     corrección, PowerPC da otras muestras (control hecho a mano en la tarea 0008). Los states de MSU-1 se escriben
     byte a byte (`put32`/`get`).
   - Portadas (`imageload.cpp`, stb_image) y exportación de logros (`achexport.cpp`, `writePng`): bytes y valores,
     portables.
5. **Búsqueda estática** (bt3xbx §5.2, encontró 7 casos que su detector no veía). Repetirla antes de cada hito y
   tras cambios grandes:
   ```
   grep -rnE '\*\((const )?(u?int(16|32|64)_t|short|int|long|float)\s*\*\)' src tools
   grep -rnE '\(\((const )?(u?int(16|32|64)_t)\s*\*\)&' src tools
   grep -rnE '\bunion\b|reinterpret_cast<|#pragma pack|__attribute__\(\(packed' src tools
   grep -rnE 'memcpy\([^;]*&[a-zA-Z_]+[,)]' src tools        # memcpy de un escalar a/desde bytes
   grep -rnE 'fwrite\(&|fread\(&|\.write\(\(const char\*\)&' src tools
   ```
   Campos de bits (`int x : 3;`): GCC big-endian los coloca desde el bit alto. Solo importan si la struct se guarda
   en disco o se superpone a bytes; hoy no hay en los headers del núcleo.
6. **Float.** El núcleo usa `double` para la tabla de interpolación gaussiana del DSP (`dsp.cpp:35-43`, con
   `std::exp`) y hay floats en `palette.cpp` y los módulos de juego. Dos riesgos para la comparación (no son de
   orden de bytes, pero la ensucian):
   - el `exp` de newlib (Xbox) o de glibc-PPC puede diferir en el último bit del de glibc x86 o del CRT de MSVC → la
     tabla y el audio difieren. Comprobar la tabla en qemu y en consola; si difiere, generarla una vez y dejarla
     como constante en el código;
   - GCC en PowerPC funde `a*b+c` en `fmadd` (un solo redondeo) y x86 no (bt3xbx §5.4): compilar la comparación y
     el `.xex` con `-ffp-contract=off` al menos en `src/core` y `src/game`. PC compila con `/fp:fast`: la referencia
     de comparación es la build de WSL, no la de MSVC.
   - Coste en Xenon: mover un float entre registros enteros y de coma flotante pasa por memoria (31 ciclos un load,
     51 un store; xsdk `docs/HARDWARE.md`). Evitar mezclar enteros y floats en bucles calientes (PPU/DSP).
7. **Orden de evaluación de argumentos** (bt3xbx §5.1.5): `f(read(), read())` da distinto en GCC, clang y MSVC.
   Si una comparación diverge sin explicación de bytes, buscar esto.
8. **Inversiones condicionales.** Si se añade un `bswap`, debe compilar solo en big-endian
   (`#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__`): un swap que corre en cualquier host vuelve a romper los datos de
   la consola (xsdk PITFALLS.md, "Big-endian data", caso Dusklight). Las cargas/almacenamientos con bytes invertidos
   cuestan lo mismo que las normales en Xenon (`lwbrx`/`lhbrx`/`stwbrx`, 1,6 ciclos; xsdk `docs/HARDWARE.md`), así
   que `__builtin_bswap16/32` en datos LE no es un problema de rendimiento.

## 4. Reglas de código (de bt3xbx `AGENTS.md`)

- **El código común solo cambia dentro de `#if`**, dejando el original en el `#else`, para que Windows y Android
  sigan compilando igual. Única excepción: reescribir un escritor o lector de un formato fijo byte a byte (como el
  WAV de smastool), siempre que en little-endian dé exactamente los mismos bytes que antes, comprobado con un SHA1
  antes y después y anotado en la tarea.
- Dos macros distintas, para que la comparación en qemu siga siendo válida:
  - `SMAS_BIG_ENDIAN` (o `__BYTE_ORDER__`): correcciones de orden de bytes. Valen en qemu-ppc y en la Xbox.
  - `SMAS_XENON`: comportamiento propio de la consola (rutas `game:\`, Guía, XAM). Solo en el `.xex`.
  Un cambio de comportamiento bajo la macro de big-endian haría divergir qemu y x86 sin ser un error de bytes.
- Toda corrección de orden de bytes se documenta en la tarea (`docs/tasks/`) con el fichero, la línea y la run de
  qemu que la demuestra.

## 5. Avisos de BT3 que también nos tocan

- El toolchain de xsdk compila sin cambios en **WSL Ubuntu 26.04** (BT3 xenon_plan X0, 2026-10-27).
- `xsdk_test` dio 624 de 630 en consola en BT3: los 6 fallos eran del entorno (lanzado desde USB, sin perfil, sin
  `net_host.txt`), no del SDK. No confundirlos con fallos del port.
- GCC y clang pliegan distinto una constante float que desborda (clang da infinito): la referencia es la build de
  WSL con el mismo compilador de la familia GCC.
- LLVM emite `mcrf`, que Xenia ignora en silencio; GCC casi no (xsdk reserva CR2-CR7). Otra razón para no basarse en
  Xenia para lógica fina.
- Rendimiento en Xenon (BT3 `xenon_optimizations.md`): load-hit-store de 40-80 ciclos, líneas de caché de 128 bytes
  (`dcbt`/`dcbz` para copias grandes, como la subida del framebuffer), copias con VMX hacia memoria de GPU. Útil en
  el hito de rendimiento, medido siempre en consola.
