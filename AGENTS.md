# AGENTS.md — Super Mario All-Stars para Xbox 360 (RGH)

Guía para cualquier agente (o persona) que trabaje en este repositorio. Léela entera antes de tocar nada.

## 0. Regla: nada de rutas absolutas

**No se escriben rutas absolutas en ningún sitio**: ni en documentos (`.md`), ni en código, scripts, CMake, guiones
de prueba, mensajes de commit o `run.md`. Nada de rutas de disco de Windows (`C:` y similares), de `/mnt/`, `/home/`,
`/usr/` ni del disco de la consola (`Hdd1:`). En su lugar:

- rutas relativas a `<raíz>` (la carpeta de este `AGENTS.md`), o relativas a la carpeta del fichero cuando es
  código o un script;
- fuera del repositorio, relativas a `<raíz>` (`../Varios/...`) o descritas ("el home de WSL");
- en el título, en tiempo de ejecución, relativas a `game:\` (la carpeta desde la que se lanzó el `.xex`);
- en comandos, partir de `<raíz>` como carpeta actual; si una herramienta exige una ruta absoluta, construirla al
  ejecutar (`$PWD/...`), nunca escribirla.

Antes de cada commit, estas dos búsquedas (desde `<raíz>`, con lo del commit ya en el índice) no deben encontrar nada
fuera de esta sección §0 y de los `game:\` del título o de los logs de la consola:

```
# lo nuestro entero
git grep --cached -nIE '[A-Za-z]:[\\/][A-Za-z]|/mnt/|/home/|/usr/|~/' -- AGENTS.md CLAUDE.md docs runs SuperMarioAllStars-main/xenon SuperMarioAllStars-main/src/platform/xenon
# lo que añadimos a ficheros de upstream
git diff --cached -U0 -- SuperMarioAllStars-main ':!SuperMarioAllStars-main/xenon' ':!SuperMarioAllStars-main/src/platform/xenon' | grep -E '^\+[^+]' | grep -E '[A-Za-z]:[\\/][A-Za-z]|/mnt/|/home/|/usr/|~/'
```

Los scripts no llevan `#!` (es una ruta absoluta): se ejecutan con `bash <script>`. El código y los documentos de
upstream (el port de PC: sus notas de ingeniería inversa, el README y la versión Linux traen rutas de su autor y de
Linux) no se tocan por esta regla, igual que los de las dependencias (`xsdk-main/`, `vulkan4xenon-main/`,
`SDL-xenon-xenon/`). En un commit que actualiza el port base, la segunda búsqueda muestra las líneas de upstream:
basta con que no aparezca ninguna escrita por nosotros.

## 1. Objetivo

Hacer un **porteo completo** del port de PC *Super Mario All-Stars* (`SuperMarioAllStars-main/`) a una **Xbox 360 con
RGH/JTAG**, como título `.xex` que arranca desde el dashboard (Aurora/FSD/retail). "Completo" significa que la versión de
Xbox tiene todo lo que tiene la de Windows/Android, salvo lo que la consola no puede tener (ver §6):

- el **launcher** (cielo nocturno, estrellas, suelo a cuadros, tarjeta "Super Mario All-Stars" o las cuatro tarjetas
  con vista previa en vivo, botones Options / Achievements / Music Player / Credits / Quit);
- los cuatro juegos (SMB1, The Lost Levels, SMB2, SMB3) y el menú de All-Stars, con partidas de batería;
- el menú de pausa entero: save states, Level Select, Cheats, Sound Select, Music Player, Options, Credits;
- widescreen real, HUDs reanclados, filtros/paleta de color, co-op de SMB1/LL/SMB3;
- rewind y fast forward;
- los **82 logros** del juego (20 SMB1, 19 LL, 20 SMB2, 21 SMB3, 2 de la colección), con su banner, y —como fase
  aparte— su espejo como **logros nativos de Xbox 360** (§7);
- 60 fps estables a 720p **en la consola física**, con audio sin cortes.

Pilas a usar (impuestas por el usuario, no cambiarlas sin preguntar):

| Parte | Herramienta |
|---|---|
| Compilación, kernel, XAM, ficheros, hilos | **xsdk** (`xsdk-main/`) |
| Gráficos | **vulkan4xenon** (`vulkan4xenon-main/`), Vulkan 1.0 sobre la capa GPU de xsdk |
| Controles (y audio, si conviene) | **SDL-xenon** (`SDL-xenon-xenon/`, SDL 3.4.10 rama `xenon`) |
| Pruebas rápidas en PC | **XSDK Xenia** (`XSDK Xenia/xsdk_xenia.exe`) |

El juego **no contiene datos del cartucho**: corre desde la ROM del usuario. La ROM nunca se sube al repositorio.

## 2. Rutas

`<raíz>` es la carpeta que contiene este `AGENTS.md` (el repositorio git). Todas las rutas son relativas a ella
(§0).

| Qué | Ruta (relativa a `<raíz>`) | Notas |
|---|---|---|
| ROM | `Super Mario All-Stars (USA).sfc` | CRC32 `925637C7`, 2 MiB. **Ignorada por git.** |
| Juego (código fuente del port de PC) | `SuperMarioAllStars-main/` | **Base: upstream v1.1.1** (`github.com/Darhkfox/SuperMarioAllStars`, commit `0c49e5e`), con nuestros cambios encima (§5, "Actualizar el port base"). `README.md` describe todas las funciones |
| Núcleo de emulación (65816, SPC700, DSP, PPU) | `SuperMarioAllStars-main/src/core/` | Portable, sin dependencias |
| Lógica de cada juego, logros, paleta | `SuperMarioAllStars-main/src/game/` | Portable |
| App: launcher, menú, config, música, logros | `SuperMarioAllStars-main/src/app/` | `launcher.cpp`, `menu.cpp`, `app.cpp`, `canvas.h`, `achstore.cpp` |
| Capa Windows (referencia) | `src/app/app_win.cpp`, `video.cpp`, `audio.cpp`, `input.cpp`, `canvas_d2d.cpp` | D3D11/D2D/XAudio2/XInput |
| Capa Linux / Steam Deck (**plantilla del bucle de la app**) | `SuperMarioAllStars-main/src/platform/linux/app_linux.cpp` | `main`, `App::init/run/shutdown`, búsqueda de ROM, `openUrl`, `httpGet`; usa las capas de vídeo, audio e input de Android |
| Capa Android (**plantilla de vídeo, audio e input**) | `SuperMarioAllStars-main/src/platform/android/` | SDL2 + GLES 3; `video_gles.cpp` dibuja el texto con FreeType fuera de Android |
| Build Linux (lista de fuentes comunes) | `SuperMarioAllStars-main/CMakeLists.txt`, rama `UNIX AND NOT ANDROID` | Los ficheros de `src/app` que entran en un build no Windows |
| **Capa Xbox 360** | `SuperMarioAllStars-main/src/platform/xenon/` | Ver §5: `app_xenon.cpp` (la App en la consola → `SuperMarioAllStars.xex`), `video_xenon.*` (vídeo y `Canvas`), `fonts_xenon.cpp`, `audio_sdl3.*` (sonido), `input_sdl3.*` (mandos), `shaders/`, `canvas_shaders.*` y `screen_shaders.*`; los títulos de prueba `hito2_main.cpp`, `video_main.cpp` + `video_vk.*` y `play_main.cpp` (→ `smas_jugar.xex`) |
| **Build Xbox 360** | `SuperMarioAllStars-main/xenon/CMakeLists.txt` | Toolchain de xsdk; guiones de consola `hito2_*.txt`, `hito3_demo.txt`; comparación en `xenon/qemu/` |
| xsdk | `xsdk-main/` | `CLAUDE.md`, `docs/GETTING_STARTED.md`, `docs/GUIDE.md`, `docs/PITFALLS.md`, `docs/PARITY.md`, `docs/HARDWARE.md` |
| Toolchain CMake de xsdk | `xsdk-main/cmake/xsdk.cmake` | `xsdk_add_xex(<target>)`, `xsdk_add_shaders()` |
| Empaquetador XEX | `xsdk-main/tools/elf2xex.py` | `--title-id`, `--title-name`; ya escribe un XDBF (§7) |
| Compilador de shaders (xost) | `xsdk-main/xenos-toolchain/` | glslang en `xenos-toolchain/tools/glslang/bin/` |
| vulkan4xenon | `vulkan4xenon-main/` | `include/vulkan4xenon.h`, `docs/SUPPORT.md`, `tools/build_shaders.py` |
| SDL-xenon | `SDL-xenon-xenon/` | `docs/README-xsdk.md` |
| Xenia (fork para xsdk) | `XSDK Xenia/xsdk_xenia.exe` | `xenia_canary.exe` es el mismo binario (mismo MD5) |
| Documentación del port | `docs/` | `docs/ROADMAP.md`, `docs/ENDIANNESS.md`, `docs/PERFORMANCE.md`, `docs/UPSTREAM.md`, `docs/ACHIEVEMENTS_X360.md`, `docs/tasks/` |
| Port de referencia (BT3, PS2 → Xbox 360) | `../Varios/Tenkaichi3Decomp-main/Tenkaichi3Decomp-main` | Solo lectura. `AGENTS.md`, `docs/port/xenon_*.md`, `bt3xbx-main/docs/x360/guia-de-port.md` §5 |
| Runs (pruebas) | `runs/` | Ver §9 |
| Builds | `build-xenon/` (y `build-*` en las dependencias) | Ignorados por git |

Toolchain (WSL, distro **Ubuntu 26.04**, no Debian como dicen los docs de xsdk):

- GCC 15 / binutils / newlib para `powerpc-xsdk-elf` en la carpeta `xsdk-gcc` del home de WSL (donde
  la busca `xsdk-main/cmake/xsdk.cmake`; prefijo `bin/powerpc-xsdk-elf-`).
- `cmake`, `ninja`, `make`, `python3`, `g++`, `qemu-ppc` (qemu-user 10.2) y `powerpc-linux-gnu-g++` 15.2
  (paquete `g++-powerpc-linux-gnu`, instalado 2026-11-06), todos en el `PATH`. Para qemu se enlaza con `-static`, así
  no hace falta indicarle la carpeta de bibliotecas de PowerPC.
- En el home de WSL hay copias viejas (`Xbox-360-SDK`, `vulkan4xenon`): **no son las canónicas**. Se compila
  siempre contra las carpetas de `<raíz>`.
- Llamar a WSL desde PowerShell abierto en `<raíz>`: `wsl.exe -d Ubuntu --exec bash -c '...'`. WSL arranca en la
  misma carpeta, así que los comandos usan rutas relativas. No usar Git Bash para esto (reescribe las rutas).
- La raíz está en OneDrive: si la sincronización bloquea ficheros durante un build, pausarla o compilar en una carpeta
  del home de WSL y copiar el `.xex`.

## 3. Compilar

Orden: xsdk → vulkan4xenon → SDL-xenon → juego. Desde PowerShell, en `<raíz>`:

```powershell
# 1. xsdk (libxsdk.a)
wsl.exe -d Ubuntu --exec bash -c 'cd xsdk-main && make'
# 2. vulkan4xenon (libvulkan4xenon.a); su Makefile busca ../Xbox-360-SDK por defecto
wsl.exe -d Ubuntu --exec bash -c 'cd vulkan4xenon-main && make XSDK=../xsdk-main'
# 3. SDL-xenon (estática)
#    (la configuración tarda >10 min sobre OneDrive; la ruta del toolchain se construye con $PWD)
wsl.exe -d Ubuntu --exec bash -c 'cd SDL-xenon-xenon && cmake -S. -Bbuild-xsdk -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/../xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-xsdk'
# 3b. Título de prueba de SDL -> SDL-xenon-xenon/build-xsdk-check/sdl_check.xex
wsl.exe -d Ubuntu --exec bash -c 'cd SDL-xenon-xenon && cmake -S test/xsdk -B build-xsdk-check -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/../xsdk-main/cmake/xsdk.cmake -DSDL3_DIR=$PWD/build-xsdk && cmake --build build-xsdk-check'
# 4. El port (SuperMarioAllStars-main/xenon/CMakeLists.txt) -> build-xenon/
#    hoy: smas_hito2.xex + guion.txt (hitos 2 y 2b), smas_video.xex + guion_video.txt (hito 3, enlaza
#    vulkan4xenon-main/build/libvulkan4xenon.a), smas_jugar.xex (hito 4: el cartucho sin la App, con SDL3 de
#    SDL-xenon-xenon/build-xsdk) y SuperMarioAllStars.xex, el juego (hito 5: la App entera; ID de título 534D4153,
#    nombre "Super Mario All-Stars"; empaquetado con xenon/tools/elf2xex_spa.py, el elf2xex.py de xsdk con un XDBF
#    como el SPA de un juego). Solo el juego: cmake --build build-xenon --target SuperMarioAllStars
wsl.exe -d Ubuntu --exec bash -c 'cmake -S SuperMarioAllStars-main/xenon -B build-xenon -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-xenon'
# 5. Shaders del port (solo si cambia src/platform/xenon/shaders/*): GLSL -> SPIR-V (glslang de WSL) -> Xenos (xost)
#    -> screen_shaders.c/.h (hito 3) y canvas_shaders.c/.h (el juego), que se suben al repositorio
wsl.exe -d Ubuntu --exec bash -c 'cd SuperMarioAllStars-main && python3 ../vulkan4xenon-main/tools/build_shaders.py --xost ../xsdk-main/xenos-toolchain --glslang "$(command -v glslang)" --output src/platform/xenon/screen_shaders src/platform/xenon/shaders/screen.vert src/platform/xenon/shaders/screen.frag'
wsl.exe -d Ubuntu --exec bash -c 'cd SuperMarioAllStars-main && python3 ../vulkan4xenon-main/tools/build_shaders.py --xost ../xsdk-main/xenos-toolchain --glslang "$(command -v glslang)" --output src/platform/xenon/canvas_shaders src/platform/xenon/shaders/canvas.vert src/platform/xenon/shaders/canvas.frag src/platform/xenon/shaders/game/*.frag'
```

El núcleo se compila en Xbox con `-O3 -ffp-contract=off -fno-strict-aliasing` (la comparación en qemu, con `-O2`; los
resultados son los mismos, runs 010 y 012) y con `SMAS_XENON`, `SMAS_BIG_ENDIAN`, `SMAS_PPU_FAST`, `SMAS_CPU_FAST` y
`SMAS_PORT_FIXES` definidos. El paquete de cada run de consola se arma
en `build-xenon/consola/<hito>/`; para Xenia, una copia en `build-xenon/xenia/<hito>/` (Xenia escribe junto al
`.xex`).

Cada link de xsdk ejecuta `tools/xenia_scan.py`, que lista instrucciones que Xenia no implementa: si aparece una
función del juego, cualquier resultado de Xenia que pase por ella **no vale**.

## 4. Probar: Xenia rápido, la consola decide

**El rendimiento de Xenia no es comparable con la consola física.** XSDK Xenia es un fork experimental para
depuración: sirve para ver si arranca, si crashea, si el launcher se dibuja y si la lógica funciona. **No sirve** para
medir fps, tiempos de frame, latencia de audio ni de input, ni para dar por bueno nada de GPU, memoria o hilos.
**Toda funcionalidad debe verificarse en la consola física antes de darla por terminada.** Una tarea probada solo en
Xenia se marca como "pendiente de consola" en su `.md` y en la run.

Xenia:

```powershell
# desde <raíz>
& ".\XSDK Xenia\xsdk_xenia.exe" --exit_on_crash --break_on_unimplemented_instructions=false ".\build-xenon\xenia\<hito>\SuperMarioAllStars.xex"
```

- `--break_on_unimplemented_instructions=false`: sin él, Xenia se cierra en la primera instrucción que no implementa;
  la App la ejecuta cada frame (`rldic` en `App::menuInput`, que GCC emite con `-mpowerpc64`; ver `xenia_scan`). Con
  él la salta: lo que pase por esas funciones (la navegación de los menús, entre otras) **no vale en Xenia**.
- `xsdk_xenia.exe` lanza Xenia y termina: para cerrarlo, parar los procesos `*xenia*`.

- Se queda abierto al terminar el título: cerrarlo tras unos segundos.
- El log del título cae en la carpeta `cache` de Xenia con el nombre del `.xex`; el `DbgPrint` en `xenia.log` (solo si
  hay un `dbgprint.txt` junto al `.xex`).
- Diferencias conocidas con la consola: `xsdk-main/CLAUDE.md` (reloj a 50 MHz en vez de 49.875, estadísticas de
  memoria, prioridades de hilos, `icbi`, renombrado de ficheros, etc.) y `vulkan4xenon-main/CLAUDE.md` (resolve →
  sample). Este fork emula los registros AVIVO: el swapchain de vulkan4xenon funciona y se ve la imagen (run 015), pero
  la presentación real, los colores y los tiempos solo los confirma la consola. Para mirar la imagen de Xenia desde
  un agente: captura de la pantalla de Windows con PowerShell (`System.Drawing`) mientras corre.

Consola física (RGH del usuario):

1. Copiar por FTP a una carpeta propia en el disco de la consola: `SuperMarioAllStars.xex`, la ROM y los
   ficheros del juego (fuente del título, `README.txt`).
2. Lanzarlo desde el dashboard.
3. El usuario devuelve el log (`game:\SuperMarioAllStars.log`; con `net_log.txt` junto al `.xex` llega por red a
   `xsdk-main/tools/net_log.py`) y, si hace falta, foto/captura.
4. Se guarda todo en la run (§9). Pedir una run en consola cuando un hito o una duda sobre el hardware real lo necesite.
   El paquete para el FTP se arma en `build-xenon/consola/<hito>/`; los logs que vuelven de la consola suelen quedar
   ahí (junto a los `.xex`): buscarlos también en esa carpeta y copiarlos a la run.

Lo que la consola del usuario ya mostró (run 001, 2026-11-06), y que el port debe tener en cuenta:

- Se lanza desde **USB** y sin perfil. Escribir ficheros normales en `game:\` funciona; las partidas en paquete de XAM
  (`xsdk_save_mount`) fallan sin perfil. El port guarda en ficheros normales, como en PC.
- **Una línea de log cuesta ~1,8 ms en USB.** Nada de logs por frame: acumular en memoria y escribir pocas veces, o
  mandarlo por red (`net_log.txt`).
- Salida de vídeo 1080p; xsdk/vulkan4xenon dibujan a 1280x720 y la consola escala.
- Mando Xbox 360 completo por SDL (botones, sticks, gatillos analógicos, vibración); audio por SDL a 48 kHz,
  6 canales, sin cortes.
- Resultados de referencia: `vulkan4xenon_test` 550/550, `vkbench` 17/17, `sdl_check` 18/18, `xsdk_test` 624/630
  (los 6 fallos son del entorno: USB, sin perfil, sin `net_host.txt`).
- **El emulador da los mismos resultados en la consola que en x86** (run 009: 22 líneas de hashes y 9 salidas).
- **Va a 22-38 fps en un hilo** (36-63 % del tiempo real). **La PPU es el 77-85 % de cada frame** (20-36 ms); sin
  dibujar, el resto va a ~160 fps (run 010). Con la PPU rápida (`SMAS_PPU_FAST`), `-O3` y 2 hilos de dibujo
  (hilos hardware 2 y 4): 91-96 fps en juego, 108 en el menú, 81 en widescreen, 67 en co-op (run 014; hito 2b
  cerrado). El núcleo se compila con `-O3` y dibuja con 2 hilos; los hilos hardware 1, 3 y 5 quedan para audio,
  vídeo y rewind. `docs/PERFORMANCE.md`.
- **Audio de SDL:** su hilo arranca en el hilo hardware de quien abre el dispositivo (con prioridad +1). En el 0 le
  quitaba ~5 ms por frame a la emulación (run 018): `Audio::init` lo lleva al 5 con
  `SDL_HINT_AUDIO_XSDK_HARDWARE_THREAD`. Hilos hardware: 0 emulación, 2 y 4 dibujo, 5 audio; libres 1 y 3.
- **Back + Start** saca una captura en el dashboard del usuario: no usarlo como combinación del port. Salir de los
  títulos de prueba: Back + LB + RB 3 s.
- **Nada de escribir en el USB durante el juego**: hasta una línea de log cada 10 s costaba un refresco (run 020).
  El log del bucle va a memoria y se escribe en pausa (Guía) o al salir.
- **Con vídeo (run 016):** colores correctos (prueba de 4 barras leída del front buffer); el vídeo cuesta ~0,3 ms
  por frame; en 4:3 todo a 60. En widescreen, SMB2, SMB3 y el co-op pasan de 16,7 ms: los ganchos de los juegos
  (`fetchWideTile`, casi una llamada por píxel en SMB2) van en el hilo de la emulación. Pendiente en el hito 8.
- **Nombres de fichero en el USB:** un nombre con dos o más puntos y la primera parte de 4 caracteres se crea
  recortado (`menu.png.tmp` → `menu.tmp`, `rqst.png.t` → `rqst.tng`), y puede quedar **duplicado** en la carpeta
  (runs 009 y 010). `writeFileSafely` usa en Xbox un temporal sin puntos (`menu_png.tmp`, `src/core/fileio.cpp`).
  **Regla: el port solo crea nombres de fichero con un punto.**
- Una run de consola dura lo que tarda: la de la run 009, 9,5 minutos con la pantalla negra; el título vuelve al
  dashboard al terminar.

## 5. Arquitectura del port

El port de PC ya separa núcleo y plataforma; Android y Linux demuestran que la app entera (launcher incluido) compila
sin Windows. El port de Xbox sigue exactamente ese modelo: **se reutiliza `src/core`, `src/game` y la parte común de
`src/app`** (la del build Linux: `app.cpp`, `config.cpp`, `menu.cpp`, `launcher.cpp`, `music.cpp`, `achstore.cpp`,
`utf8.cpp`, `sys_posix.cpp`, `imageload.cpp`, `msupack.cpp`, `achexport.cpp`, `updatecheck.cpp`) y se escribe solo la
capa de plataforma en `src/platform/xenon/`. Lo que cada plataforma aporta (ver `app_linux.cpp`): `main` y
`App::init/run/shutdown`, `App::findRom/pickRom/pollPick`, `App::pickArt/pollArtPick`, `App::applyDisplayMode`,
`App::touchFrame`, y de `sys.h` `openUrl`, `httpGet` y `openFolder` (en Xbox: sin selector de ficheros, sin navegador;
`httpGet` devuelve `false` hasta decidir si se usa curl-xenon).

| Fichero | Hace | Base |
|---|---|---|
| `app_xenon.cpp` (**hecho**, hito 5; pendiente de consola) | `main`, bucle de frames y ritmo (el de Linux; FIFO a 59,94 Hz), pausa con la Guía (`xsdk_system_ui_open`), rutas `game:` (ROM junto al `.xex` o en `roms`), funciones de plataforma de arriba (sin selectores ni red), salir con Back + LB + RB 3 s, hilos de dibujo de la PPU en los hilos hardware 2 y 4, tiempos cada 10 s al log en memoria | `platform/linux/app_linux.cpp` |
| `video_vk.cpp/.h` (**hecho**, hito 3) | Swapchain 1280×720 FIFO de vulkan4xenon; el framebuffer de la PPU (704×240) a una textura `R8G8B8A8` por un staging alineado (copia en GPU); un quad 4:3 que se ensancha con los márgenes; `screen.frag` toma el color de `(g, b, a)` (los bytes `00 RR GG BB` en big-endian); lectura de píxeles del front buffer para pruebas; `present()` recibe el rectángulo, `fitLayout`/`wantExtFor` copian el modo "ajustar" de `App::computeLayout` (16:9 con píxeles 8:7 = 46 por lado). Hoy solo lo usan `smas_video.xex` y `smas_jugar.xex`: el juego usa `video_xenon` | vulkan4xenon `tests/harness*.c`, `test_texture.c` |
| `video_xenon.cpp/.h` (**hecho**, hito 5; pendiente de consola) | La clase `Video` de `video_gles.h` y su `Canvas` (`XenonCanvas`, método a método como `GlesCanvas`): un solo pipeline de quads por lotes (`canvas.vert/.frag`: sólido, glifos de un atlas `R8`, rectángulos redondeados por distancia, imágenes `0xAARRGGBB` leídas como `(g, b, a)` y alfa `r`), recortes con el scissor dinámico; lo del frame se graba y `present()` hace un command buffer: copias de texturas (staging por slot, filas de 256 bytes, 4 KB) y una pasada. La imagen del juego es una textura más (704×240), sin filtros ni color de las opciones todavía (hito 6); `capture()` aún no | `GlesCanvas` en `video_gles.cpp`, `video_vk.cpp` |
| texto (**hecho**, hito 5) | Como Linux: `LuckiestGuy-Regular.ttf` (títulos) y DejaVu Sans normal y negrita (UI), dentro del `.xex` (`fonts_xenon.cpp`, `.incbin` de `third_party/fonts`), rasterizadas con `stb_truetype` (`third_party/stb`, dominio público) con las métricas de FreeType (ascenso y alto de línea redondeados arriba, avances en píxeles enteros) | `video_gles.cpp`, `loadFace` |
| `input_sdl3.cpp/.h` (**hecho**, hito 4) | La clase `Input` de la app con SDL3 (`SDL_Gamepad`, hasta 4 mandos, mismos números de botones y ejes, ranuras fijas por mando); `rumble()` añadido (la app no vibra todavía); `stickToDpad()` (el stick como cruceta por sectores, run 019; `App::readPlayer` lo usa bajo `SMAS_XENON`); el teclado no existe en la consola | `platform/android/input_sdl.cpp` (SDL2 → SDL3) |
| `audio_sdl3.cpp/.h` (**hecho**, hito 4) | La clase `Audio` de la app con SDL3: un `SDL_AudioStream` en la frecuencia del dispositivo, `SDL_AUDIO_S16` nativo, el mismo remuestreo Hermite y control de latencia que Android; su hilo, en el hilo hardware 5 (run 018) | `platform/android/audio_sdl.cpp` (SDL2 → SDL3) |
| `ach_xam.cpp/.h` (a crear) | Espejo de logros nativos (fase de §7) | |
| `xenon/CMakeLists.txt` | El núcleo (`smascore`), los títulos de prueba y `SuperMarioAllStars` (las fuentes de `src/app` que compila Linux + `touch.cpp` + la capa xenon, `SDL3::SDL3-static` y `libvulkan4xenon.a`) | rama Linux del `CMakeLists.txt` de upstream |

Puntos de riesgo que hay que tratar explícitamente:

- **Big-endian. Detalle completo en `docs/ENDIANNESS.md` (leerlo antes de tocar el núcleo o la capa de vídeo/audio).**
  Resumen, con lo aprendido de los ports de BT3 a Xbox 360 (`../Varios/Tenkaichi3Decomp-main/Tenkaichi3Decomp-main`):
  - Se compila **nativo big-endian** con el GCC de xsdk, sin el pase LLVM de BT3: la memoria de la SNES son arrays de
    bytes y el núcleo arma los valores byte a byte; no hay `union`s ni casts de punteros a enteros.
  - **Verificación antes de la consola:** `smastool` compilado para x86 y para PowerPC big-endian
    (`powerpc-linux-gnu-g++` bajo `qemu-ppc` en WSL), mismo guion, comparando hashes de WRAM/VRAM/CGRAM/OAM/ARAM y
    del framebuffer. Toda diferencia es un error de bytes (o de float). Así no se gasta una run de consola por
    intento. Xenia no sirve para esto. Scripts en `SuperMarioAllStars-main/xenon/qemu/` (`build.sh`, `compare.sh`,
    `guion.txt`, `guion_control.txt`); cómo usarlos en `docs/ENDIANNESS.md` §2.1. **Pasar `compare.sh` después de
    cualquier cambio en `src/core`, `src/game` o en código común de `src/app` que entre en smastool, y
    `compare_ppu.sh` (frame a frame, con y sin `SMAS_PPU_FAST`) después de tocar la PPU.**
  - **Hito 1b hecho (run 004):** el núcleo y los juegos dan lo mismo en big-endian que en x86 sin cambiar nada; el
    control negativo (run 005) demuestra que la comparación detecta las diferencias. Falta la consola (GCC de xsdk,
    newlib).
  - Puntos conocidos: píxeles `0x00RRGGBB` (comprobar canales con una textura de prueba en consola; arreglar con
    swizzle, no en CPU); audio con `SDL_AUDIO_S16` nativo, nunca `S16LE`; save states con los bytes en el orden del
    host: **los de PC no valen en Xbox**; `loadState` ya los rechaza por el `magic` invertido sin tocar la máquina
    (comprobado en qemu), pero la app debería decir "state de otra plataforma" (hito 7); `.srm`, `.ini` y PNG son portables; `-ffp-contract=off` (GCC PPC funde `a*b+c` en `fmadd`) y la tabla
    del DSP hecha con `std::exp` puede diferir entre libm.
  - Cambios en código común solo dentro de `#if`, con el original en el `#else`. `SMAS_BIG_ENDIAN` para correcciones
    de bytes (valen en qemu y en Xbox); `SMAS_XENON` para comportamiento de consola (solo en el `.xex`);
    `SMAS_PPU_FAST` para la PPU rápida y en hilos (`docs/PERFORMANCE.md` §3.1 y §3.3); `SMAS_CPU_FAST` para las
    lecturas de la CPU por páginas (`docs/PERFORMANCE.md` §8); `SMAS_PORT_FIXES` para
    corregir fallos de upstream (cada uno anotado en `docs/UPSTREAM.md`, para reportarlo). `SMAS_PPU_FAST`,
    `SMAS_CPU_FAST` y `SMAS_PORT_FIXES` valen en qemu y en el `.xex`.
  - Los sanitizers de GCC en WSL (`-fsanitize=thread` con `threads N`, `-fsanitize=address,undefined`) se pasan
    después de tocar los hilos o código delicado; lo que encuentren en upstream va a `docs/UPSTREAM.md`.
  - Un `bswap` solo compila en big-endian (`__BYTE_ORDER__`): uno que corre en cualquier host rompe los datos de la
    consola. Las cargas con bytes invertidos (`lwbrx`) cuestan lo mismo que las normales.
- **CPU.** El emulador corre en un núcleo PPE in-order a 3.2 GHz, mucho más lento por ciclo que un PC. Las vistas
  previas del launcher en modo "The Four Games" ejecutan **cuatro consolas SNES a la vez**: probable que no quepa en
  60 fps. Opciones a medir en consola: repartir previews en hilos hardware (xsdk tiene 6), bajar su tasa, o dejar por
  defecto la tarjeta única. Rewind (snapshot por frame) y fast forward también se miden en consola.
- **Compilador.** `-O2` mínimo incluso en debug (el núcleo lo necesita); respetar `xsdk-main/docs/PITFALLS.md`
  (memoria de GPU con stores `volatile` de 32 bits, sin tail calls, LR de 32 bits, etc.).
- **Memoria.** 512 MB compartidos con la GPU; el buffer de rewind (~1 minuto) debe dimensionarse y medirse en consola.
- **Ficheros.** Todo relativo a `game:\` (`SDL_GetBasePath`): `saves\smas.srm`, `saves\` (states), `smas_port.ini`,
  `achievements.ini`, `rompath.txt`, `screenshots\`. Escritura atómica: Xenia no hace `rename` (ver su CLAUDE.md), así
  que el guardado seguro se verifica en consola.

### Actualizar el port base

El port base es un proyecto vivo (upstream). Para pasar a una versión nueva (hecho de v1.0 a v1.1.1 en la tarea 0008):

1. Clonar upstream en el scratchpad (carpeta nueva, nunca dentro del repositorio).
2. `git diff --binary --full-index <tag actual> <tag nuevo>` y aplicarlo con
   `git apply --3way --directory=SuperMarioAllStars-main`.
3. Comprobar que el resultado es exactamente el tag nuevo más nuestros cambios (comparar el árbol con el del tag).
4. Añadir a `xenon/qemu/build.sh` y `xenon/CMakeLists.txt` las fuentes nuevas del núcleo (`CORE_SOURCES` del
   `CMakeLists.txt` de upstream).
5. Revisar todo código nuevo que lea o escriba binarios (búsqueda de `docs/ENDIANNESS.md` §3.5) y pasar
   `xenon/qemu/compare.sh`. La referencia de la consola (run 004) cambia si cambian el núcleo o los juegos: hacer una
   run de qemu nueva y usarla como referencia.
6. Anotar en §2 la versión base nueva.

Reglas para las dependencias:

- **No modificar SDL-xenon**: su `AGENTS.md` prohíbe código generado por IA en ese proyecto. Se usa tal cual; si hace
  falta un cambio, se documenta y lo escribe una persona.
- xsdk y vulkan4xenon tienen sus propias reglas (`CLAUDE.md`, `CODE_GUIDELINES.md`): sin herramientas ni material de
  Microsoft, cada afirmación sobre el hardware con evidencia (Xenia, libxenon, xost, `docs/HARDWARE.md` o una run en
  consola). Si el port necesita algo que falta en ellas, se propone primero; no se parchean sin acuerdo.
- Por convención del ecosistema, el título arranca con el splash de vulkan4xenon (`xsdk_splash_show`).

## 6. Paridad con PC: qué cambia en Xbox

- **Se porta:** todo lo de §1, incluido el launcher, Options (Display, Graphics, Colour, Audio, Controls), logros y
  música. De la v1.1/1.1.1: música MSU-1 (carpeta `msu\`; ya corregida para big-endian, `docs/ENDIANNESS.md`),
  volúmenes de música y efectos, portadas y fondo del launcher (carpeta `covers\`, PNG/JPG con stb_image), los
  cuatro estilos de menú, NES Brick Bounce, Menu Buttons y la exportación de logros (CSV y PNG en `game:\`).
- **Se adapta:** controles solo de mando (sin teclado): Back = Menú, LT/RT = rewind/fast forward como en PC; F5/F8
  (states) y F12 (captura) pasan a combinaciones o al menú. "Quit to Desktop" vuelve al dashboard (`terminate.h`).
  La Guía pausa el juego.
- **No aplica:** ventana/pantalla completa, monitores, VSync configurable, Black Frame Insertion, High Frame Rate,
  slang shaders/librashader (DLL de Windows), selector de ficheros (ROM y portadas se copian por FTP). Se ocultan esas
  opciones en el menú. La comprobación de actualizaciones (v1.1.1, `updatecheck.cpp`) queda apagada al principio: pide
  HTTPS (curl-xenon + mbedtls-xenon) y red; decidir con el usuario si se porta.
- **ROM:** se busca junto al `.xex`, en `roms\`, o en carpetas padre (igual que en PC). Sin ROM, el launcher muestra
  *ROM not found* con lo que hace falta.

## 7. Logros nativos de Xbox 360: ¿es posible?

**Sí, es técnicamente posible en una consola RGH, pero es la parte más difícil y se hace al final, sin bloquear el
resto.** El sistema de logros del juego (`src/game/achievements.cpp` + `src/app/achstore.cpp`) sigue siendo la fuente
de verdad; los nativos son un espejo de él.

Por qué es viable:

- Los logros del dashboard salen de la base de datos XDBF (SPA) incrustada como recurso en el `.xex`, con el title ID
  como nombre. `xsdk-main/tools/elf2xex.py` **ya escribe un XDBF** (`title_database`: secciones `XSTC`/`XSTR` para el
  nombre del título). Habría que ampliarlo con la sección de logros (`XACH`), sus cadenas por idioma (nombre,
  descripción bloqueado/desbloqueado), las imágenes (`XTHD`/`XITB`, PNG 64×64) y el gamerscore. El formato está en el
  código de Xenia (`xdbf_utils`), así que se puede hacer sin XLAST ni herramientas de Microsoft.
- Desbloquear se hace por XAM (`XamUserWriteAchievements`, vía `XMsgStartIORequest`). xsdk lo tiene listado como
  "No / baja prioridad" en `xsdk-main/docs/PARITY.md` ("Through `XMsgStartIORequest` (xam_msg.cc)"), así que hay que
  añadir esa llamada a xsdk (acordarlo con su alcance) o importarla en el port.

Condiciones y riesgos:

- Necesita un perfil con sesión iniciada; sin perfil, solo cuentan los logros del juego.
- **Title ID del port: `0x534D4153` ("SMAS"), nombre "Super Mario All-Stars"** (desde la tarea 0021, en
  `SuperMarioAllStars.xex`; un solo sitio: `SMAS_TITLE_ID` y `SMAS_TITLE_NAME` en `xenon/CMakeLists.txt`). Cambiarlo
  después deja huérfanos los logros y partidas ya hechos.
  Cambiarlo después deja huérfanos los logros ya desbloqueados.
- 82 logros con 1000 G repartidos (o 0 G si el usuario lo prefiere). Las imágenes deben dibujarse de cero o salir de
  arte propio: no se puede incrustar arte extraído de la ROM en el `.xex` distribuido.
- Un perfil con logros de un título homebrew no debe conectarse a Xbox Live (las RGH no deberían estarlo de todos
  modos). Avisar al usuario.
- Los logros "con progreso" (contadores, conjuntos) no existen como tales en Xbox 360: el nativo se desbloquea al
  completarse; el progreso sigue en el menú del juego.
- Xenia Canary soporta logros parcialmente: solo para humo. **Que aparezcan la notificación y el logro en el perfil se
  verifica en la consola.**

Plan: `docs/ACHIEVEMENTS_X360.md`.

## 8. Registro de trabajo: un commit y un `.md` por tarea

**Toda tarea, por pequeña que sea, termina con:**

1. Un fichero en `docs/tasks/NNNN-descripcion-corta.md` (numeración correlativa de 4 dígitos) con: fecha, objetivo,
   qué se hizo, ficheros tocados, cómo se probó (Xenia / consola / ninguno), estado (hecho, pendiente de consola,
   bloqueado) y siguientes pasos.
2. Una línea nueva en `docs/CHANGELOG.md`.
3. **Un commit** que incluya el código y ese `.md`, con mensaje `NNNN: descripción corta` (en español o inglés, pero
   consistente). Nunca dejar trabajo sin commitear al terminar una tarea. No hacer push sin que el usuario lo pida.

Nunca se commitea: la ROM (`*.sfc`, `*.smc`), saves, states, builds, binarios de Xenia, ni las carpetas de dependencias (`xsdk-main/`, `vulkan4xenon-main/`, `SDL-xenon-xenon/`,
`XSDK Xenia/`), que son repos externos. Tampoco lo que sale de la ROM al ejecutarla en las runs de qemu: save states
(`*.state`), volcados de memoria (`*.wram`, `*.vram`, ...), audio (`*.wav`) y capturas PNG; de eso se suben los
hashes (`hashes.txt`, `archivos.sha1`). `.gitignore` ya lo cubre.

## 9. Runs

Cada ejecución de prueba (qemu, Xenia o consola) se guarda en su propia carpeta dentro de `runs/`:

```
runs/AAAA-MM-DD_NNN_<qemu|xenia|consola>_<descripcion>/
    run.md          # obligatorio, corto
    *.log           # log del título, xenia.log, net_log
    *.png / *.jpg   # capturas o fotos (opcional)
```

`NNN` es correlativo global (001, 002, ...). `run.md` es **corto** (5–15 líneas), con la plantilla de
`runs/_PLANTILLA/run.md`: qué se probó, dónde (Xenia o consola), commit probado, resultado, y qué sigue. Las runs se
commitean junto con la tarea a la que pertenecen (los logs sí; nada que contenga la ROM).

## 10. Orden de trabajo

El plan por hitos está en `docs/ROADMAP.md`. Resumen: compilar dependencias → núcleo big-endian comparado con x86 en
qemu-ppc → núcleo headless en Xbox → vídeo con vulkan4xenon → input/audio con SDL3 → launcher y menús → guardados y opciones →
rendimiento en consola → logros nativos. Un hito solo se cierra con una run en consola.
