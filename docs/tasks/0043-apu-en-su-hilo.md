# 0043 — Hito 8: run 041, la APU en su propio hilo

- **Fecha:** 2026-11-11
- **Run 041 (consola, tarea 0042):** el hilo del rewind trabaja ~3,7 ms por diferencia (~110 ms por segundo), pero
  a la emulación solo le quita ~0,3 ms por frame (SMB3 en 16:9: 14,5-14,7 ms con rewind y 14,2-14,4 sin él). Un
  hilo en el mismo núcleo que la emulación le quita poco: el núcleo in-order deja muchos ciclos libres en las
  esperas. Moverlo no compensa.
- **Por qué la APU:** en x86, la APU (SPC700 y DSP) es ~20 % del frame del co-op sin dibujar, y corre entera en el
  hilo de la emulación. Es el paso 5 del plan de `PERFORMANCE.md` §3.
- **Hecho (macro nueva `SMAS_APU_THREAD`; `xenon/CMakeLists.txt`, `xenon/qemu/build.sh`, `compare_ppu.sh`):**
  - **`core/snes.cpp`, `core/snes.h`:** con `Snes::setApuThread(true)`, la APU corre en un hilo propio hasta un
    objetivo que publica la CPU:
    - en cada línea (`newLine`), su tiempo: la APU va como mucho una línea por detrás;
    - en cada acceso a los puertos `$2140-$2143` y al final del frame (`syncApu`), su tiempo exacto, y espera a
      que la APU llegue.
    - `Apu::runTo` avanza instrucción a instrucción sin efectos al empezar ni al terminar, así que partir el camino
      en tramos da la misma secuencia: el primer límite de instrucción a partir de cada objetivo es el mismo.
  - **Lo que toca la APU desde fuera espera a que esté quieta:** los states (`saveState`, `loadState`), `reset` y
    `takeAudio`. El volumen de la app se cambia entre frames, cuando ya lo está.
  - **El objetivo de 64 bits** va en dos mitades de 32 bits bajo un contador de secuencia (seqlock), todo con
    atómicos de 32 bits: el PowerPC de 32 bits no tiene atómicos de 64.
  - **Espera:** el hilo que solo espera baja su prioridad SMT (`or 1,1,1`, y `or 2,2,2` para volver). Así el otro
    hilo hardware del núcleo se lleva el núcleo, también la CPU cuando espera a la APU.
  - **Los ganchos del co-op** (`smb1coop.cpp`, `smb3coop.cpp`), que guardan y restauran la APU a mitad de frame,
    llaman antes a `Snes::apuSyncNow()`: la APU hasta "ahora" y quieta. Antes la veían donde la dejó la última
    sincronía, que con el hilo ya no es un punto fijo. **Es el único cambio de comportamiento**, va igual con hilo
    y sin hilo, y solo mueve el instante en que el co-op guarda la música.
  - **Consola (`app_xenon.cpp`):** el hilo de la APU va en el hilo hardware 1, el núcleo de la emulación (como el
    rewind, run 041), y se enciende en el juego y se apaga en el launcher, como los hilos de dibujo. smastool:
    `apuhilo 0|1`.
- **Probado:**
  - El mismo smastool x86 con y sin `apuhilo 1`: `guion.txt` **iguales** (22 líneas, con el hash del audio; 9
    archivos) y `guion_ppu.txt` **iguales** (9049 líneas).
  - Sin el hilo, igual que HEAD en los dos guiones.
  - ThreadSanitizer con la APU en su hilo y 2 hilos de dibujo (`guion_ppu.txt` entero): 0 avisos,
    9020 PIC y las mismas líneas que sin hilo. GCC avisa de que TSan no sigue `atomic_thread_fence`; los campos del
    seqlock son atómicos, así que no se le escapan accesos sin sincronizar.
  - `compare.sh` (x86 contra PowerPC en qemu), sin hilo y con `apuhilo 1`: IGUALES en los dos casos (22 líneas,
    9 archivos). La primera versión, con atómicos de 64 bits, no enlazaba en PowerPC (`__atomic_load_8`); de ahí el
    seqlock.
  - Compila para la consola sin avisos. Paquete `build-xenon/consola/hito8i/`. **Pendiente de consola** (run 042).
- **Lo esperado:** "CPU, APU y juegos" baja en todo (en x86 la APU era ~1/4 de esa parte), y un poco "frame
  saltado".
- **Siguiente:** según la run 042.
