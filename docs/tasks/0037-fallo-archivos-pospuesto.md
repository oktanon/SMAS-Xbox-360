# 0037 — El fallo de los archivos de SMB1, pospuesto

- **Fecha:** 2026-11-10
- **Qué:** el usuario pospone el fallo de la run 035 (en SMB1, elegir el archivo de un jugador en el mundo 4 entró en
  el de dos jugadores en el mundo 1). Queda anotado en `ROADMAP.md` (hito 10) y la run 036 sin correr.
- **Para retomarlo:**
  - En x86 no se reproduce (tarea 0036: la batería del usuario con un archivo B añadido, los dos órdenes, desde la
    foto del título y con co-op).
  - El paquete `hito8c` escribe en `smas_port.log` una línea `launch:` con el archivo pedido, el del cursor del panel
    al pulsar Start, el juego al que entró y el co-op. Correr la run 036 con dos archivos y mirar esa línea.
  - Pista: la sesión de la run 035 empezó con co-op activado.
- **Código:** sin cambios.
