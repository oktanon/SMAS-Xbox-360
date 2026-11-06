# AGENTS.md — Super Mario All-Stars para Xbox 360 (RGH)

Guía para cualquier agente (o persona) que trabaje en este repositorio. Léela entera antes de tocar nada.

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

Raíz del proyecto (repositorio git): `C:\Users\estan\OneDrive\Escritorio\SuperMarioAllStars-main`
En WSL: `/mnt/c/Users/estan/OneDrive/Escritorio/SuperMarioAllStars-main` (abajo, `<raíz>`).

| Qué | Ruta (relativa a `<raíz>`) | Notas |
|---|---|---|
| ROM | `Super Mario All-Stars (USA).sfc` | CRC32 `925637C7`, 2 MiB. **Ignorada por git.** |
| Juego (código fuente del port de PC) | `SuperMarioAllStars-main/` | `README.md` describe todas las funciones |
| Núcleo de emulación (65816, SPC700, DSP, PPU) | `SuperMarioAllStars-main/src/core/` | Portable, sin dependencias |
| Lógica de cada juego, logros, paleta | `SuperMarioAllStars-main/src/game/` | Portable |
| App: launcher, menú, config, música, logros | `SuperMarioAllStars-main/src/app/` | `launcher.cpp`, `menu.cpp`, `app.cpp`, `canvas.h`, `achstore.cpp` |
| Capa Windows (referencia) | `src/app/app_win.cpp`, `video.cpp`, `audio.cpp`, `input.cpp`, `canvas_d2d.cpp` | D3D11/D2D/XAudio2/XInput |
| Capa Android (**plantilla del port**) | `SuperMarioAllStars-main/src/platform/android/` | SDL2 + GLES 3; misma estructura que tendrá Xbox |
| Build Android (plantilla de CMake) | `SuperMarioAllStars-main/android/app/src/main/cpp/CMakeLists.txt` | Lista exacta de fuentes comunes |
| **Capa Xbox 360 (a crear)** | `SuperMarioAllStars-main/src/platform/xenon/` | Ver §5 |
| **Build Xbox 360 (a crear)** | `SuperMarioAllStars-main/xenon/CMakeLists.txt` | Toolchain de xsdk |
| xsdk | `xsdk-main/` | `CLAUDE.md`, `docs/GETTING_STARTED.md`, `docs/GUIDE.md`, `docs/PITFALLS.md`, `docs/PARITY.md`, `docs/HARDWARE.md` |
| Toolchain CMake de xsdk | `xsdk-main/cmake/xsdk.cmake` | `xsdk_add_xex(<target>)`, `xsdk_add_shaders()` |
| Empaquetador XEX | `xsdk-main/tools/elf2xex.py` | `--title-id`, `--title-name`; ya escribe un XDBF (§7) |
| Compilador de shaders (xost) | `xsdk-main/xenos-toolchain/` | glslang en `xenos-toolchain/tools/glslang/bin/` |
| vulkan4xenon | `vulkan4xenon-main/` | `include/vulkan4xenon.h`, `docs/SUPPORT.md`, `tools/build_shaders.py` |
| SDL-xenon | `SDL-xenon-xenon/` | `docs/README-xsdk.md` |
| Xenia (fork para xsdk) | `XSDK Xenia/xsdk_xenia.exe` | `xenia_canary.exe` es el mismo binario (mismo MD5) |
| Documentación del port | `docs/` | `docs/ROADMAP.md`, `docs/ACHIEVEMENTS_X360.md`, `docs/tasks/` |
| Runs (pruebas) | `runs/` | Ver §9 |
| Builds | `build-xenon/` (y `build-*` en las dependencias) | Ignorados por git |

Toolchain (WSL, distro **Ubuntu 26.04**, no Debian como dicen los docs de xsdk):

- GCC 15 / binutils / newlib para `powerpc-xsdk-elf` en `~/xsdk-gcc` (prefijo `~/xsdk-gcc/bin/powerpc-xsdk-elf-`).
- `cmake`, `ninja`, `make`, `python3` en `/usr/bin`.
- En el home de WSL hay copias viejas (`~/Xbox-360-SDK`, `~/vulkan4xenon`): **no son las canónicas**. Se compila
  siempre contra las carpetas de `<raíz>`.
- Llamar a WSL desde PowerShell: `wsl.exe -d Ubuntu --exec bash -c '...'` (Git Bash reescribe rutas absolutas).
- La raíz está en OneDrive: si la sincronización bloquea ficheros durante un build, pausarla o compilar en una carpeta
  de WSL (`~/smas-build`) y copiar el `.xex`.

## 3. Compilar

Orden: xsdk → vulkan4xenon → SDL-xenon → juego. Desde PowerShell:

```powershell
# 1. xsdk (libxsdk.a)
wsl.exe -d Ubuntu --exec bash -c 'cd "/mnt/c/Users/estan/OneDrive/Escritorio/SuperMarioAllStars-main/xsdk-main" && make'
# 2. vulkan4xenon (libvulkan4xenon.a); su Makefile busca ../Xbox-360-SDK por defecto
wsl.exe -d Ubuntu --exec bash -c 'cd "/mnt/c/Users/estan/OneDrive/Escritorio/SuperMarioAllStars-main/vulkan4xenon-main" && make XSDK=../xsdk-main'
# 3. SDL-xenon (estática)
wsl.exe -d Ubuntu --exec bash -c 'cd "/mnt/c/Users/estan/OneDrive/Escritorio/SuperMarioAllStars-main/SDL-xenon-xenon" && cmake -S. -Bbuild-xsdk -GNinja -DCMAKE_TOOLCHAIN_FILE=../xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-xsdk'
# 4. El juego -> build-xenon/SuperMarioAllStars.xex
wsl.exe -d Ubuntu --exec bash -c 'cd "/mnt/c/Users/estan/OneDrive/Escritorio/SuperMarioAllStars-main" && cmake -S SuperMarioAllStars-main/xenon -B build-xenon -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/xsdk-main/cmake/xsdk.cmake -DCMAKE_BUILD_TYPE=Release -DSDL3_DIR=$PWD/SDL-xenon-xenon/build-xsdk -DV4X_DIR=$PWD/vulkan4xenon-main && cmake --build build-xenon'
```

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
& "C:\Users\estan\OneDrive\Escritorio\SuperMarioAllStars-main\XSDK Xenia\xsdk_xenia.exe" --exit_on_crash "<raíz>\build-xenon\SuperMarioAllStars.xex"
```

- Se queda abierto al terminar el título: cerrarlo tras unos segundos.
- El log del título cae en la carpeta `cache` de Xenia con el nombre del `.xex`; el `DbgPrint` en `xenia.log` (solo si
  hay un `dbgprint.txt` junto al `.xex`).
- Diferencias conocidas con la consola: `xsdk-main/CLAUDE.md` (reloj a 50 MHz en vez de 49.875, estadísticas de
  memoria, prioridades de hilos, `icbi`, renombrado de ficheros, etc.) y `vulkan4xenon-main/CLAUDE.md` (resolve →
  sample). Este fork emula los registros AVIVO, así que el swapchain sí puede funcionar, pero la presentación real solo
  la confirma la consola.

Consola física (RGH del usuario):

1. Copiar por FTP a una carpeta propia (p. ej. `Hdd1:\Homebrew\SMAS\`): `SuperMarioAllStars.xex`, la ROM y los
   ficheros del juego (fuente del título, `README.txt`).
2. Lanzarlo desde el dashboard.
3. El usuario devuelve el log (`game:\SuperMarioAllStars.log`; con `net_log.txt` junto al `.xex` llega por red a
   `xsdk-main/tools/net_log.py`) y, si hace falta, foto/captura.
4. Se guarda todo en la run (§9). Pedir una run en consola cuando un hito o una duda sobre el hardware real lo necesite.

## 5. Arquitectura del port

El port de PC ya separa núcleo y plataforma; Android demuestra que la app entera (launcher incluido) compila sin
Windows. El port de Xbox sigue exactamente ese modelo: **se reutiliza `src/core`, `src/game` y la parte común de
`src/app`** (`app.cpp`, `config.cpp`, `menu.cpp`, `launcher.cpp`, `music.cpp`, `achstore.cpp`, `utf8.cpp`,
`sys_posix.cpp`) y se escribe solo la capa de plataforma en `src/platform/xenon/`:

| Fichero (a crear) | Hace | Base |
|---|---|---|
| `app_xenon.cpp` | `main`, bucle de frames, pausa con la Guía (`xsdk_system_ui_open`), rutas `game:\` | `platform/android/app_android.cpp` |
| `video_vk.cpp/.h` | Swapchain 1280×720 de vulkan4xenon, sube el framebuffer SNES (256 px o ancho widescreen) como textura, escalado/aspecto/filtros, paleta de color | `platform/android/video_gles.cpp` |
| `canvas_vk.cpp` | Implementa `Canvas` (`src/app/canvas.h`) para launcher, menú y música: rectángulos, redondeados, gradientes, polígonos, formas guardadas, imágenes de vista previa, texto | `GlesCanvas` en `video_gles.cpp` |
| texto | `stb_truetype` (a añadir en `third_party/stb/`) con `LuckiestGuy-Regular.ttf` (títulos) y una fuente UI libre empaquetada (Android usa la del sistema vía JNI; aquí no existe) | |
| `input_sdl3.cpp/.h` | Mandos con SDL3 (`SDL_Gamepad`, hasta 4, vibración); mapeo como el de Xbox en Windows | `platform/android/input_sdl.cpp` (SDL2 → SDL3) |
| `audio_sdl3.cpp/.h` | Audio con `SDL_AudioStream` a 48 kHz (SDL-xenon lo lleva al driver de xsdk) | `platform/android/audio_sdl.cpp` (SDL2 → SDL3) |
| `ach_xam.cpp/.h` | Espejo de logros nativos (fase de §7) | |
| `xenon/CMakeLists.txt` | Fuentes comunes + capa xenon, enlaza `SDL3::SDL3-static` y `libvulkan4xenon.a`, `xsdk_add_xex`, shaders con `tools/build_shaders.py` | `android/app/src/main/cpp/CMakeLists.txt` |

Puntos de riesgo que hay que tratar explícitamente:

- **Big-endian.** El Xenon es PowerPC big-endian; el port solo se ha probado en x86/ARM little-endian. No hay `union`s
  ni casts de punteros en el núcleo, pero hay que auditar: empaquetado de píxeles (orden de bytes de BGRA/RGBA al subir
  texturas), lectura de cabeceras de la ROM, `png.cpp`, save states (son `memcpy` de structs: **no serán compatibles con
  los de PC**, aceptable pero documentarlo), `achievements.ini`, `smas_port.ini`, `.srm` (bytes crudos de SRAM: sí
  compatibles). Añadir pruebas con `tools/smastool.cpp` comparando frames/hashes contra el build de PC.
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

Reglas para las dependencias:

- **No modificar SDL-xenon**: su `AGENTS.md` prohíbe código generado por IA en ese proyecto. Se usa tal cual; si hace
  falta un cambio, se documenta y lo escribe una persona.
- xsdk y vulkan4xenon tienen sus propias reglas (`CLAUDE.md`, `CODE_GUIDELINES.md`): sin herramientas ni material de
  Microsoft, cada afirmación sobre el hardware con evidencia (Xenia, libxenon, xost, `docs/HARDWARE.md` o una run en
  consola). Si el port necesita algo que falta en ellas, se propone primero; no se parchean sin acuerdo.
- Por convención del ecosistema, el título arranca con el splash de vulkan4xenon (`xsdk_splash_show`).

## 6. Paridad con PC: qué cambia en Xbox

- **Se porta:** todo lo de §1, incluido el launcher, Options (Display, Graphics, Colour, Audio, Controls), logros y
  música.
- **Se adapta:** controles solo de mando (sin teclado): Back = Menú, LT/RT = rewind/fast forward como en PC; F5/F8
  (states) y F12 (captura) pasan a combinaciones o al menú. "Quit to Desktop" vuelve al dashboard (`terminate.h`).
  La Guía pausa el juego.
- **No aplica:** ventana/pantalla completa, monitores, VSync configurable, Black Frame Insertion, High Frame Rate,
  slang shaders/librashader (DLL de Windows), selector de ficheros. Se ocultan esas opciones en el menú.
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
- Title ID propio que no choque con ningún juego comercial (propuesta provisional: `0x534D4153`, "SMAS"; confirmar).
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
`XSDK Xenia/`), que son repos externos. `.gitignore` ya lo cubre.

## 9. Runs

Cada ejecución de prueba (Xenia o consola) se guarda en su propia carpeta dentro de `runs/`:

```
runs/AAAA-MM-DD_NNN_<xenia|consola>_<descripcion>/
    run.md          # obligatorio, corto
    *.log           # log del título, xenia.log, net_log
    *.png / *.jpg   # capturas o fotos (opcional)
```

`NNN` es correlativo global (001, 002, ...). `run.md` es **corto** (5–15 líneas), con la plantilla de
`runs/_PLANTILLA/run.md`: qué se probó, dónde (Xenia o consola), commit probado, resultado, y qué sigue. Las runs se
commitean junto con la tarea a la que pertenecen (los logs sí; nada que contenga la ROM).

## 10. Orden de trabajo

El plan por hitos está en `docs/ROADMAP.md`. Resumen: compilar dependencias → núcleo headless en Xbox (endianness,
comparar frames con PC) → vídeo con vulkan4xenon → input/audio con SDL3 → launcher y menús → guardados y opciones →
rendimiento en consola → logros nativos. Un hito solo se cierra con una run en consola.
