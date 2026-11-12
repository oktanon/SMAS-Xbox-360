# 0047 — Run 043: instalación limpia, hito 10 cerrado, versión 1.0

- **Fecha:** 2026-11-12
- **Run 043 (consola):** el paquete de la tarea 0046 en una carpeta nueva.
  - **Aurora muestra "Super Mario All-Stars"** (usuario): el XDBF de xsdk, como Stick Fight, en una carpeta que Aurora
    escanea por primera vez.
  - Arranca sin archivos previos (crea `saves\`, corta las portadas de la ROM, encuentra la ROM); SMB1, The Lost
    Levels, SMB2 y SMB3 en 4:3 y 16:9 con rewind.
  - Sin el hilo de la APU, "CPU, APU y juegos" vuelve a ~4,8-5,2 ms y el snapshot del rewind a ~0,95 ms.
- **Hecho:** la run 043 registrada con sus logs; el hito 10 cerrado en el roadmap; los commits subidos a `origin/main`
  (rebase sobre la versión del commit del README que había en GitHub).
- **Nota:** el log dice "version 1.1.1", la versión de upstream (`kAppVersion`); la del port, 1.0.0.0, está en la
  cabecera del `.xex` y en el release.
- **Siguiente:** publicar el release 1.0 (`publish_release.sh 1.0`, borrador para revisar en GitHub), cuando el
  usuario lo pida.
