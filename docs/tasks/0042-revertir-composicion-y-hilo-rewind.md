# 0042 — Hito 8: run 040, se revierte la composición sin saltos; el hilo del rewind en el log

- **Fecha:** 2026-11-11
- **Run 040 (consola, tarea 0041):** la composición sin saltos **va más lenta**. Por frame dibujado en 16:9 pasa
  de 2,1-3,3 ms a 3,0-4,8 en SMB3, de 2,1-2,9 a 3,9-4,3 en SMB1 y de 2,5-2,7 a 4,0-4,3 en SMB2 (tabla en la run).
  La hipótesis de la tarea 0041 era falsa: los saltos mal predichos no eran lo que costaba. Las instrucciones de
  más pesan más, igual que en x86, y apuntan a un núcleo in-order limitado por cadenas de instrucciones
  dependientes.
- **Hecho:**
  - **Revertida la tarea 0041:** `core/ppu.cpp` vuelve exactamente al de la tarea 0040 (commit `cd300ba`), que ya
    se había comparado (frame a frame con 2 hilos, `compare.sh`).
  - **El hilo del rewind en el log** (bajo `SMAS_XENON`, `core/rewind.h`, `core/rewind.cpp`, `app_xenon.cpp`):
    cuántas diferencias calculó cada 10 s, cuánto tardó cada una y cuántos ms por segundo. El hilo va en el hilo
    hardware 1, **que comparte núcleo con el de la emulación** (el 0): mientras trabaja, la emulación tiene la
    mitad del núcleo.
- **Probado:** compila para la consola sin avisos. Paquete `build-xenon/consola/hito8h/`. El núcleo y la PPU no
  cambian respecto a la tarea 0040. **Pendiente de consola** (run 041, con el rewind activado y desactivado en el
  mismo nivel).
- **Siguiente:** si con el rewind desactivado bajan las fases del hilo de la emulación, mover el hilo del rewind
  al hilo hardware 3 (comparte núcleo con un hilo de dibujo) o abaratar las diferencias. Para la composición, el
  próximo intento tiene que bajar instrucciones o cadenas dependientes, no saltos: por ejemplo, recorrer la línea
  capa a capa en vez de píxel a píxel.
