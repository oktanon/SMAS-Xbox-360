# Hoja de ruta del port a Xbox 360

Cada hito se cierra solo con una run en la consola física (`runs/`). Xenia sirve para llegar antes, no para cerrar.

| # | Hito | Cómo se verifica | Estado |
|---|---|---|---|
| 0 | AGENTS.md, estructura de docs/runs, repositorio git | — | Hecho (tarea 0001) |
| 1 | Compilar xsdk, vulkan4xenon (`XSDK=../xsdk-main`) y SDL-xenon con el toolchain de WSL Ubuntu; correr `xsdk_test.xex`, `vulkan4xenon_test.xex` y `test/xsdk` de SDL en consola | Logs de los tres tests en consola | Pendiente |
| 1b | Núcleo big-endian en WSL: `smastool` para x86 y para `powerpc-linux-gnu` bajo `qemu-ppc`, guion fijo de los cuatro juegos, comparación automática de hashes de memoria y framebuffer (`ENDIANNESS.md` §2); corregir toda diferencia | Hashes idénticos x86 vs qemu-ppc (runs `qemu`); no necesita consola | Pendiente |
| 2 | `xenon/CMakeLists.txt` + núcleo headless: carga la ROM desde `game:\`, corre N frames de cada juego y escribe hashes de frame y de WRAM en el log | Mismos hashes que `smastool` en PC (detecta errores de endianness) | Pendiente |
| 3 | Vídeo: swapchain 720p de vulkan4xenon, framebuffer SNES como textura, aspecto, widescreen | Imagen correcta y 60 fps en consola | Pendiente |
| 4 | Input (SDL3 gamepad, 4 mandos, vibración) y audio (SDL3 a 48 kHz) | Jugable sin cortes de audio en consola | Pendiente |
| 5 | `Canvas` con vulkan4xenon + `stb_truetype`: launcher completo (tarjeta única y cuatro tarjetas con previews) | Launcher idéntico al de PC; fps del modo cuatro tarjetas medidos en consola | Pendiente |
| 6 | Menú de pausa completo: states, Level Select, Cheats, Sound Select, Music Player, Options (sin las de solo Windows), Credits | Recorrido de todas las páginas en consola | Pendiente |
| 7 | Guardados: `.srm`, states, `smas_port.ini`, `achievements.ini`, escritura atómica, Guía pausa el juego, volver al dashboard | Guardar, apagar, volver a cargar en consola | Pendiente |
| 8 | Rendimiento: rewind, fast forward, co-op, previews en hilos hardware; perfil en consola | Tabla de tiempos de frame de consola en la run | Pendiente |
| 9 | Logros nativos (ver `ACHIEVEMENTS_X360.md`) | Notificación y logro en el perfil, en consola | Pendiente |
| 10 | Paquete de release: `.xex`, README para Xbox, avisos de licencias (xsdk, vulkan4xenon, SDL, fuentes) | Instalación limpia desde cero en consola | Pendiente |
