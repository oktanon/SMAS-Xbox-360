# Hoja de ruta del port a Xbox 360

Cada hito se cierra solo con una run en la consola física (`runs/`). Xenia sirve para llegar antes, no para cerrar.

| # | Hito | Cómo se verifica | Estado |
|---|---|---|---|
| 0 | AGENTS.md, estructura de docs/runs, repositorio git | — | Hecho (tarea 0001) |
| 1 | Compilar xsdk, vulkan4xenon (`XSDK=../xsdk-main`) y SDL-xenon con el toolchain de WSL Ubuntu; correr `xsdk_test.xex`, `vulkan4xenon_test.xex` y `test/xsdk` de SDL en consola | Logs de los tres tests en consola | **Hecho** 2026-11-06 (run 001, tareas 0004-0005): v4x 550/550, vkbench 17/17, sdl_check 18/18, xsdk_test 624/630 (6 fallos del entorno) |
| 1b | Núcleo big-endian en WSL: `smastool` para x86 y para `powerpc-linux-gnu` bajo `qemu-ppc`, guion fijo de los cuatro juegos, comparación automática de hashes de memoria y framebuffer (`ENDIANNESS.md` §2); corregir toda diferencia | Hashes idénticos x86 vs qemu-ppc (runs `qemu`); no necesita consola | **Hecho** 2026-11-06 (runs 002-005, tarea 0006): iguales sin cambiar el núcleo; control negativo detecta las diferencias |
| 2 | `xenon/CMakeLists.txt` + núcleo headless: carga la ROM desde `game:\`, ejecuta `xenon/qemu/guion.txt` y escribe las mismas líneas `HASHALL`/`ACH`/`WAITLOAD` en el log | Mismas líneas que la run 004 (con el GCC de xsdk y newlib: `std::exp` del DSP y float) | Run 007 (v1.0): las 9 salidas idénticas a x86, sin log; run 009 (v1.1.1, tarea 0008) esperando la consola |
| 3 | Vídeo: swapchain 720p de vulkan4xenon, framebuffer SNES como textura, aspecto, widescreen | Imagen correcta y 60 fps en consola | Pendiente |
| 4 | Input (SDL3 gamepad, 4 mandos, vibración) y audio (SDL3 a 48 kHz) | Jugable sin cortes de audio en consola | Pendiente |
| 5 | `Canvas` con vulkan4xenon + `stb_truetype`: launcher completo (tarjeta única y cuatro tarjetas con previews) | Launcher idéntico al de PC; fps del modo cuatro tarjetas medidos en consola | Pendiente |
| 6 | Menú de pausa completo: states, Level Select, Cheats, Sound Select, Music Player, Options (sin las de solo Windows), Credits | Recorrido de todas las páginas en consola | Pendiente |
| 7 | Guardados: `.srm`, states, `smas_port.ini`, `achievements.ini`, escritura atómica, Guía pausa el juego, volver al dashboard | Guardar, apagar, volver a cargar en consola | Pendiente |
| 8 | Rendimiento: rewind, fast forward, co-op, previews en hilos hardware; perfil en consola | Tabla de tiempos de frame de consola en la run | Pendiente |
| 9 | Logros nativos (ver `ACHIEVEMENTS_X360.md`) | Notificación y logro en el perfil, en consola | Pendiente |
| 10 | Paquete de release: `.xex`, README para Xbox, avisos de licencias (xsdk, vulkan4xenon, SDL, fuentes) | Instalación limpia desde cero en consola | Pendiente |
