# 0014 — El dibujo en hilos medido en la consola; hito 2b cerrado

- **Fecha:** 2026-11-06
- **Objetivo:** analizar la run 014 y decidir.
- **Resultado:** resultados idénticos a la run 004 dibujando con 2 hilos; los hilos en los hilos hardware pedidos.
  Con 2 hilos de dibujo: 91-96 fps en juego, 108 en el menú, 81 en widescreen, 67 en co-op. El tercer hilo (hilo
  hardware 3, en el núcleo del primero) resta. Rewind: guardar un snapshot 5,8 ms (la app guarda cada 2 frames),
  volver uno 17 ms.
- **Decisiones:** el port dibuja con 2 hilos en los hilos hardware 2 y 4 (libres el 1, el 3 y el 5). Hito 2b cerrado:
  60 fps o más en las cuatro escenas, widescreen y co-op, con los hashes iguales. El margen del co-op (1,8 ms en el
  bench) y el rewind pasan al hito 8, medidos con el juego entero.
- **Hecho:** `docs/PERFORMANCE.md` (tabla de la run 014, conclusiones), `docs/ROADMAP.md` (hito 2b hecho, hito 8
  ampliado), `AGENTS.md` §4, run 014.
- **Probado:** consola (run 014).
- **Siguiente:** hito 3, el vídeo con vulkan4xenon.
