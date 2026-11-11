# 0041 — Hito 8: run 039, la composición sin saltos por píxel

- **Fecha:** 2026-11-10
- **Run 039 (consola, tarea 0040):** la PPU en el hilo de la emulación, por fases (tabla en la run). En 16:9, en
  cada frame dibujado:
  - las capas: 3,4-4,3 ms de trabajo propio más ~1 ms de espera;
  - la composición: 2,1-3,3 ms más ~0,6 de espera;
  - lo que va en serie, los ganchos de línea y el final: menos de 1 ms entre todo.

  La parte de la composición del hilo de la emulación son ~30 000 píxeles por frame, a unos **300 ciclos por
  píxel**: demasiados para unas 60-80 instrucciones.
- **Por qué (hipótesis):** en el bucle por capas de cada píxel, el mismo salto (¿la ventana tapa esta capa?,
  ¿`r < best`?) sale distinto según la capa. Un predictor por dirección, como el del núcleo in-order de la
  consola, falla así varias veces por píxel, a ~20 ciclos cada vez. El color math añade saltos que dependen del
  píxel: recorte, si la capa pide math, la mitad, la saturación.
- **Hecho (bajo `SMAS_PPU_FAST`, `core/ppu.cpp`, `composePixels`):**
  - **La capa ganadora sin saltos:** cada capa de la pantalla da su rango a través de la máscara de su ventana (una
    ventana de ceros si no la tapa), y el mínimo se saca con aritmética (`d = r - best; best += d & (d >> 31)`).
    El bucle por capas está desenrollado con una prueba por capa, igual en toda la línea; no es un `switch`,
    porque una tabla de saltos es un salto indirecto.
  - **Sin saltos para el fondo:** la tabla rango → capa da 5 para "ninguna", y el color del fondo se elige con una
    máscara.
  - **Color math:** el recorte, si se aplica, la mitad y la saturación, con máscaras y aritmética. Si la capa pide
    math se mira en una tabla por capa y paleta.
  - **El oscurecido de los márgenes por tramos** (izquierda, 4:3, derecha) en vez de mirarlo en cada píxel.
  - Lo que solo depende de la línea (¿hay math?, ¿resta?, ¿pantalla secundaria?) sigue como salto: se predice
    bien.
  - En el ensamblador PowerPC del `.xex` (GCC de xsdk, `-O3 -mcpu=cell`): `composePixels` baja de 187 a 110 saltos
    condicionales, ninguno indirecto. Los que quedan en el bucle del píxel dependen de la línea o del tramo.
- **Probado:**
  - `compare_ppu.sh` sin hilos y con 2: **9020 frames iguales** las dos veces (frame a frame contra la PPU original).
  - smastool x86 de HEAD contra el de ahora: `guion.txt` **iguales** (22 líneas y 9 archivos), `guion_ppu.txt` **iguales** (9049 líneas).
  - `compare.sh` (x86 contra PowerPC en qemu): **iguales** (22 líneas, 9 archivos).
  - **En x86 va más lento** (bench de un hilo en 16:9: SMB3 +14 %, SMB2 +37 %, co-op +42 % de tiempo por frame): allí los saltos
    apenas cuestan y la versión sin saltos hace siempre todas las cuentas. No afecta al port de PC, que no usa
    `SMAS_PPU_FAST`; solo hace más lentas nuestras pruebas.
  - Compila para la consola sin avisos. Paquete `build-xenon/consola/hito8g/`. **Pendiente de consola** (run 040).
- **Cómo se decide:** la fase "composición" del log, antes 2,1-3,3 ms en 16:9. Si no baja en la consola, se
  revierte la tarea.
- **Siguiente:** las capas (3,4-4,3 ms en el hilo de la emulación), con la misma idea si la composición mejora.
