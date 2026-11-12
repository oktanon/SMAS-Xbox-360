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
- **Resultado:** pendiente.
- **Siguiente:** si va bien, cerrar el hito 10 y la versión 1.0.
