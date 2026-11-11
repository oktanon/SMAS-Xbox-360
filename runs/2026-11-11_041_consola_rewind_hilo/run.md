# Run 041 — El hilo del rewind y el núcleo de la emulación (hito 8)

- **Fecha:** 2026-11-11
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0042; SHA1 en `paquete.sha1` (`build-xenon/consola/hito8h/SuperMarioAllStars.xex`).
- **Qué se prueba:** copiar el `.xex` sobre el de antes. La composición vuelve a la de la run 039.
- **Pedir:**
  1. SMB3 en 16:9, el mismo nivel: 30-40 s con el rewind activado y después 30-40 s con el rewind desactivado
     (Options). Anotar el orden.
  2. Lo mismo con el co-op de SMB1 en 16:9, si se puede.
  3. Volver a activar el rewind al terminar.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log`.
- **Mirar en el log:**
  - la línea nueva "hilo del rewind: N diferencias, X ms cada una, Y ms por segundo";
  - con el rewind desactivado ("rewind no"), si bajan las fases de la PPU y la CPU del hilo de la emulación.
- **Orden:** SMB3 en 16:9 con rewind y después sin rewind; después, co-op de SMB1 en 16:9 sin rewind y con rewind.
- **Resultado:** OK, medido. El hilo del rewind no es la palanca:
  - trabaja ~3,7 ms por diferencia, unos 110 ms por segundo (11 % del hilo hardware 1);
  - SMB3 en 16:9, frame dibujado: 14,5-14,7 ms con rewind y 14,2-14,4 sin él. PPU 9,4-9,5 frente a 9,2-9,4;
    CPU 5,2 frente a 5,0;
  - co-op de SMB1 en 16:9: 16,0-16,6 ms con rewind y 15,4-16,3 sin él;
  - un hilo que comparte núcleo con la emulación le quita muy poco (~0,3 ms por frame por 110 ms por segundo de
    trabajo): el núcleo in-order deja libres muchos ciclos en las esperas.
- **Siguiente:** tarea 0043: la APU (~20 % del frame del co-op sin dibujar, en x86) en su propio hilo, en el hilo
  hardware 1.
