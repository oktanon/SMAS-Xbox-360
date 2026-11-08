# Run 029 — Recorrido de los menús (hito 6)

- **Fecha:** 2026-11-08
- **Dónde:** Xenia (el log dice "1280x720 shown 1:1 to the 1280x720 output"; la consola del usuario sale a 1080p)
- **Commit:** tarea 0029; SHA1 en `paquete.sha1` (`build-xenon/consola/hito6/SuperMarioAllStars.xex`).
- **Qué se prueba:** el menú de pausa entero. Copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Level Select: elegir un mundo y nivel en cada juego (los logros deben estar desactivados) y ver que llega.
  2. Sound Select: reproducir una música y un efecto.
  3. Cheats: probar uno en cada juego.
  4. Credits: la sección "Xbox 360 port" y "Built with".
  5. Options: que no aparezcan Updates, "Display Mode" ni "Take Screenshot"; "Create Covers Folder" y "Create MSU-1
     Folder" crean las carpetas.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`; fotos de lo que se vea mal.
- **Resultado:** OK en Xenia: el usuario verificó los cinco puntos (Level Select, Sound Select, Cheats, Credits,
  Options). Pendiente de consola.
- **Logs:** `xenia/smas_port.log`, `xenia/SuperMarioAllStars.log`, `xenia/achievements.ini`.
- **Siguiente:** los filtros del juego (lo que falta del hito 6).
