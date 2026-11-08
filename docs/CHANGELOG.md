# Registro de tareas

Una línea por tarea; el detalle está en `docs/tasks/`.

- 0001 (2026-11-06) — AGENTS.md, roadmap, plan de logros nativos, estructura de runs, repositorio git.
- 0002 (2026-11-06) — Estrategia de orden de bytes tomada de los ports de BT3: `docs/ENDIANNESS.md`, comparación
  x86 vs qemu-ppc (hito 1b).
- 0003 (2026-11-06) — Regla de no usar rutas absolutas (`AGENTS.md` §0); rutas de los documentos pasadas a
  relativas; compilador PowerPC para qemu comprobado.
- 0004 (2026-11-06) — Hito 1: xsdk, vulkan4xenon y SDL-xenon compilados; cuatro títulos de prueba listos para la
  consola (run 001, pendiente de logs).
- 0005 (2026-11-06) — Hito 1 hecho en consola: v4x 550/550, vkbench 17/17, sdl_check 18/18, xsdk_test 624/630
  (6 fallos del entorno).
- 0006 (2026-11-06) — Hito 1b hecho: smastool x86 y PowerPC big-endian (qemu) dan lo mismo en todo el guion sin cambiar
  el núcleo; `hashall`, `waitload`, WAV portable; scripts en `xenon/qemu/`; control negativo.
- 0007 (2026-11-06) — Hito 2: primer build de Xbox (`xenon/CMakeLists.txt`), `smas_hito2.xex` con el guion y
  benchmarks; humo en Xenia OK (run 006); esperando la consola (run 007).
- 0008 (2026-11-06) — Port base actualizado a upstream v1.1.1 (sin conflictos); error de orden de bytes de MSU-1
  corregido y probado en qemu; run 007 (consola): salidas idénticas a x86, falta el log; run 009 preparada.
- 0009 (2026-11-06) — Hito 2 hecho en consola (run 009); nombres de temporales en el USB corregidos para Xbox;
  sonda de rendimiento (PPU sí/no, -O2/-O3) preparada (run 010); docs/PERFORMANCE.md.
- 0010 (2026-11-06) — Run 010: la PPU es el 77-85 % de cada frame en la consola, `-O3` +6 %, resultados iguales;
  perfil en x86 (`renderBg` + `composeLine` = 69 %); plan del hito 2b; regla de nombres del USB corregida.
- 0011 (2026-11-06) — Paso 1 del rendimiento: `renderBg` y `composeLine` rápidas bajo `SMAS_PPU_FAST`, idénticas
  frame a frame (9020 frames, control negativo), 2-3× en x86; qemu-ppc igual (run 011); run 012 preparada.
- 0012 (2026-11-06) — Run 012: la PPU rápida es 2,4-4,3× más rápida en la consola, resultados idénticos; con `-O3`
  63-65 fps en juego (50 en widescreen y co-op); el núcleo pasa a `-O3`.
- 0013 (2026-11-06) — Paso 2 del rendimiento: el dibujo de cada línea en varios hilos (`threads N`), idéntico frame
  a frame, ThreadSanitizer y ASan/UBSan limpios; fallo de upstream en SMB2 corregido (`SMAS_PORT_FIXES`,
  `docs/UPSTREAM.md`); qemu con 2 hilos igual (run 013); run 014 preparada.
- 0014 (2026-11-06) — Run 014: con 2 hilos de dibujo, 91-96 fps en juego, 81 en widescreen, 67 en co-op, hashes
  iguales. Hito 2b cerrado; el port dibuja con 2 hilos (hilos hardware 2 y 4).
- 0015 (2026-11-06) — Hito 3: vídeo con vulkan4xenon (`video_vk`, shaders, `smas_video.xex`) con prueba de color leída
  del front buffer; en Xenia se ve el juego y las 4 barras pasan (run 015); run 016 preparada; documentación al día.
- 0016 (2026-11-06) — Run 016: colores correctos en consola, hashes iguales, vídeo 0,3 ms, 60 fps en 4:3; widescreen de
  SMB2/SMB3 y co-op ancho por encima de 16,7 ms (→ hito 8). Hito 3 hecho en 4:3. Mosaico de tamaño 1 por el camino
  rápido.
- 0017 (2026-11-06) — Hito 4: `Audio` e `Input` de la app con SDL3 (`audio_sdl3`, `input_sdl3`) y `smas_jugar.xex`,
  el cartucho jugable con mando y sonido; arranca en Xenia (run 017); run 018 preparada.
- 0018 (2026-11-06) — Run 018: mandos y sonido bien, tirones porque el hilo de audio de SDL corría en el hilo hardware
  de la emulación: movido al 5 (hint de SDL-xenon); salir con Back + LB + RB; run 019 preparada.
- 0019 (2026-11-06) — Run 019: 4:3 con un jugador ya sin tirones; widescreen como el PC (16:9 con píxeles 8:7, 46 por
  lado, la imagen entera centrada: nada cortado, el HUD quieto) y el stick por sectores; run 020 preparada.
- 0020 (2026-11-06) — Run 020: sticks y salida bien; tirones en 16:9, SMB3 y la batalla de SMB3 (40-55 ms). Hito 4
  hecho. El log del juego, en memoria (la escritura periódica en el USB costaba un refresco); refrescos perdidos en el
  log; run 021 opcional.
- 0021 (2026-11-06) — El juego con ID de título `534D4153` y nombre "Super Mario All-Stars" (`SuperMarioAllStars.xex`,
  antes `smas_jugar.xex`), comprobado al arrancar; Xenia lo muestra; run 022 para Aurora.
- 0022 (2026-11-07) — Aurora seguía mostrando el nombre del archivo (run 022): el `.xex` lleva ahora un XDBF como el
  SPA de un juego (`XTHD`, `XACH`, `XSTC`, `XSTR`) con un envoltorio de `elf2xex.py` (xsdk sin cambios); Xenia lo lee
  entero; run 023 para Aurora.
- 0023 (2026-11-07) — Run 023: Aurora sigue mostrando el nombre del archivo; la tarea queda en pausa (opciones
  anotadas en la tarea) y se pasa al hito 5 (launcher).
- 0024 (2026-11-07) — Hito 5 en marcha: `SuperMarioAllStars.xex` es la App entera (launcher, menús, logros, música)
  con `video_xenon` (Canvas con vulkan4xenon, texto con stb_truetype y las fuentes dentro del `.xex`) y `app_xenon`;
  el launcher se dibuja en Xenia; run 024 pendiente de consola.
- 0025 (2026-11-07) — Run 024: el launcher funciona pero con 4 tarjetas iba a 53 ms por pasada. Ahora arranca en los
  4 juegos, la tarjeta elegida dibuja en los hilos de render y sin ponerse al día con varios frames, las otras con un
  10 % del refresco; "Xbox 360 Edition", botones Ⓐ/Ⓑ en las ayudas, Hotkeys accesibles; run 025 pendiente.
- 0026 (2026-11-07) — Run 025: launcher de 53 a 21-25 ms, juego sin refrescos perdidos. Hotkeys reasignables desde
  Options, Back + RB / Back + LB para guardar/cargar state, A/B en vez de Ⓐ/Ⓑ ("?"), Jugar llega solo al título del
  juego; investigación de packs de gráficos (`docs/TEXTURE_PACKS.md`); run 026 pendiente.
- 0027 (2026-11-07) — Run 026: con covers el launcher va a 3 ms; jugando en widescreen con rewind se perdían
  refrescos. Side Borders "Blurred Game" en la Xbox, ritmo fijo a 59,94 Hz (tirones al cambiar de modo), diferencias
  del rewind en el hilo hardware 1; run 027 pendiente.
- 0028 (2026-11-08) — Run 027: con Pixel Shape en Square el widescreen dibuja 71 columnas por lado y las pasadas
  subieron a 17-21 ms; el ritmo ahora cuenta refrescos en fracciones (el juego no se frena), el stick derecho cambia
  entre 4:3 y widescreen, sin "Display Mode" en la Xbox. SMAS + SMW: analizado, sin implementar. Run 028 pendiente.
