# Run 043 — Instalación limpia del paquete de release 1.0 (hito 10)

- **Fecha:** 2026-11-12
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0046; SHA1 en `paquete.sha1` (`build-xenon/release/SuperMarioAllStars-x360-1.0/`, también en el
  `.zip`).
- **Qué se prueba:** el paquete tal como lo recibe un jugador, desde cero. El hilo de la APU, apagado (tarea 0044).
- **Pedir:**
  1. Una carpeta **nueva y vacía** en la consola (no la de las runs): solo el `.xex` del paquete y la ROM. Sin
     `saves\`, `smas_port.ini` ni `achievements.ini` de antes.
  2. En Aurora, agregar la carpeta nueva (ruta de escaneo nueva o "Scan" de la que la contiene) y mirar el nombre:
     tiene que decir "Super Mario All-Stars", no el del archivo (tarea 0046). Arrancar: el launcher sale, las
     portadas se cortan de la ROM la primera vez.
  3. Jugar un rato SMB1 en 16:9 con el rewind activado, el co-op de SMB1 en 16:9 y SMB2. Guardar en el cartucho,
     guardar y cargar un state.
  4. Salir al dashboard (Back + LB + RB), volver a entrar y ver que la partida y la configuración siguen ahí.
  5. Credits: "Ported by Ttano".
  6. Leer el `README.txt` del paquete y decir si falta o sobra algo.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` de la carpeta nueva.
- **Mirar en el log:** que arranca sin archivos previos; "CPU, APU y juegos" de vuelta a ~5,2 ms (~7,5 en co-op).
- **Orden:** carpeta nueva; SMB1, The Lost Levels, SMB2 (4:3 y 16:9) y SMB3 en 16:9, con rewind y filtro Sharp.
- **Resultado:** OK. **Aurora muestra "Super Mario All-Stars"** (usuario).
  - Arranca sin archivos previos: crea `saves`, corta las portadas de la ROM (las primeras pasadas del launcher,
    con refrescos perdidos mientras las corta) y encuentra la ROM.
  - Sin el hilo de la APU, "CPU, APU y juegos" vuelve a ~4,8-5,2 ms y el snapshot del rewind a ~0,95 ms.
  - En juego, muchos tramos de 10 s con 0 refrescos perdidos; los demás pierden 3-34, con picos aislados (el peor
    de cada tramo, 20-570 ms: cargas, menús, cambios de juego).
  - No se jugó el co-op en esta run (medido en la 041: ~16 ms en 16:9).
- **Siguiente:** hito 10 cerrado: subir los commits y el release 1.0 (`publish_release.sh`).
