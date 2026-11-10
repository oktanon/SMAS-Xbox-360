# 0035 — Hito 8: el snapshot del rewind, de 3 ms a menos de 1

- **Fecha:** 2026-11-10
- **Run 034 (consola, la línea nueva del log, `PERFORMANCE.md` §6):**
  - En 4:3 la emulación tarda 10,5-12,3 ms por frame; en 16:9, 13,7-14,3 ms; el co-op de SMB1, 13,1-14,3 ms.
  - El snapshot del rewind tarda ~3 ms, uno cada dos frames, en el hilo de la emulación. La interfaz y el vídeo,
    1,4 ms en 4:3 y 0,4 ms en 16:9.
  - En 16:9 la emulación y el snapshot pasan de 16,7 ms, así que se dibujan 263-475 de cada 600 frames en SMB2 y
    SMB3, y ~300 en co-op con 16:9.
- **Medido en x86 (programa de prueba fuera del repositorio):**
  - `Snes::saveState` tarda 1,6 ms, y 1,3 ms de eso es el CRC-32 del final, calculado byte a byte (279 KB).
  - El resto, copiar las memorias y el bloque de los juegos, apenas cuenta: 0,04 ms el bloque, 0,01 ms un `memcpy`
    del tamaño de un state.
- **Hecho (bajo `SMAS_XENON`, en `core/snes.cpp`):**
  - `stateCrc` calcula el mismo CRC-32 con ocho tablas, ocho bytes por vuelta ("slicing-by-8"). Los bytes se juntan
    a mano, así que da lo mismo en cualquier orden de bytes y los states de antes cargan.
  - `saveState` reserva 320 KB de entrada en vez de crecer a trozos.
- **Probado:**
  - x86: `saveState` baja de 1,6 a 0,38 ms. El CRC guardado es igual al byte a byte (`C94EFE14`) y el state carga.
  - Compila para la consola sin avisos.
  - En consola, pendiente (run 035). Lo esperado: el snapshot baja de ~3 ms a menos de 1, y un state viejo carga.
- **Siguiente:**
  - Si 16:9 sigue sin llegar: los 2,5 ms de más de la emulación en 16:9 (`PERFORMANCE.md` §4: los ganchos de los
    juegos en el hilo de la emulación).
  - El co-op.
  - La interfaz en 4:3 (1 ms más que en 16:9).
