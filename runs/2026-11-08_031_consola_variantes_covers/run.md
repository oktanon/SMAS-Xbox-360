# Run 031 — Shader en variantes, colores por tabla, covers del selector

- **Fecha:** 2026-11-08
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0031; SHA1 en `paquete.sha1` (`build-xenon/consola/hito6c/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes.
- **Pedir:**
  1. Launcher: la primera vez, en unos segundos las tarjetas pasan de la vista previa a las cajas del selector de
     la ROM (y quedan en `saves\covers\`). Al volver a abrir, salen de entrada. ¿El launcher va fluido?
  2. Jugar un rato con el filtro por defecto (Sharp Bilinear): ¿va a velocidad normal? Recibir un golpe con Mario
     grande: ¿parpadea bien?
  3. Probar los demás filtros (sobre todo CRT y CRT aperture) y algún perfil de color: ¿alguno da lag?
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`; fotos de lo que se vea mal.
- **Resultado:** Parcial. Juego a 60 estables (599 pasadas / 580 frames cada 10 s); el usuario da por bien lo del
  hito. El menú de pausa abierto va a 30 fps justos (300 pasadas, 1 ms de trabajo; 600 en la
  run 027): falta medir la GPU (tarea 0032).
- **Siguiente:** run 032 (hito 7, con la GPU en el log).
