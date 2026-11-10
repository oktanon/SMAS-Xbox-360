# Run 033 — Save & Quit al launcher, estilo de las partidas, carga (hito 7)

- **Fecha:** 2026-11-09
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0033; SHA1 en `paquete.sha1` (`build-xenon/consola/hito7b/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes (la carpeta `saves` se queda).
- **Pedir:**
  1. La pantalla de partidas en cada estilo (Options, el estilo de los menús): ¿colores, letra y bordes como el
     menú?
  2. Entrar a un juego dos veces seguidas: ¿la pantalla de carga muestra el juego, el mundo y el archivo? ¿La
     segunda carga tarda menos que la primera (unos 7 s en vez de 12)?
  3. Entrar con un archivo que tenga progreso: ¿empieza en su mundo?
  4. En el juego, Save & Quit desde su propio menú de pausa: ¿vuelve al launcher? ¿El archivo muestra el progreso
     guardado?
  5. Level Select desde el menú: ¿la carga muestra el nivel?
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`; fotos de la pantalla de partidas y de la carga.
- **Resultado:** OK. El usuario: archivos, estilos y Save & Quit al launcher funcionan (el log lo dice 4 veces:
  "left for the All-Stars menu"); probó los filtros en SMB1. Rendimiento justo en SMB2 y SMB3 y en 16:9 (en 4:3
  bien): trabajo de 17-22 ms por pasada en buena parte del juego, 28-38 ms en un tramo (250-390 pasadas en 10 s); la
  GPU, 3 ms por frame. Salida de vídeo a 1080p esta vez. El log no dice qué juego ni qué aspecto: lo agrega la 0034.
- **Logs:** `smas_port.log`, `SuperMarioAllStars.log` (los `0xC0000034`/`0xC000003A` son covers que no existen).
- **Siguiente:** cerrar el hito 7; hito 8 (tarea 0034, run 034).
