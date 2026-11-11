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
- **Resultado:** pendiente.
- **Siguiente:** si el rewind le quita tiempo a la emulación, moverlo a otro hilo hardware (el 3) o abaratarlo.
