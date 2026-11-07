# Run 026 — Hotkeys reasignables, Back + RB / LB para states, Jugar directo al juego

- **Fecha:** 2026-11-07
- **Dónde:** Xenia (humo) y consola (RGH)
- **Commit:** tarea 0026; SHA1 en `paquete.sha1` (`build-xenon/consola/hito5c/SuperMarioAllStars.xex`).
- **Qué se prueba:** los cambios de la tarea 0026. Copiar el `.xex` sobre el de antes (el `smas_port.ini` puede
  quedarse: si no tiene atajos de states, toma Back + RB / Back + LB).
- **Pedir en consola:**
  1. Las ayudas de abajo con "A" y "B" (sin "?").
  2. Jugando: Back + RB guarda el state, Back + LB lo carga; Back solo (al soltarlo) abre el menú; RB y LB solos
     siguen siendo L y R del juego.
  3. Options > Hotkeys: elegir una fila con A y pulsar un botón; comprobar que el atajo cambia y funciona.
  4. Jugar desde una tarjeta: debe pasar solo por la pantalla de archivos (con el archivo marcado) hasta el título del
     juego.
- **Devolver:** `smas_port.log` y `SuperMarioAllStars.log` (abrir la Guía o salir antes).
- **Resultado:** Parcial. Con covers el launcher va a 3 ms por pasada (el lag era de las previews en vivo). Jugando
  en widescreen con rewind: 450-530 pasadas cada 10 s en vez de 600, 19-22 ms de "trabajo"; el contador marca 60
  porque cuenta frames emulados. Usuario: "no se siente como 60 fps"; al salir de una tubería en widescreen se ven un
  momento barras negras (el juego vuelve a 4:3 ahí, como en PC, donde lo tapa Side Borders "Blurred Game", que en la
  Xbox no estaba). Tarea 0027.
- **Logs:** `smas_port.log`, `supermarioallstars.log`, `smas_port.ini`, `achievements.ini`, `rompath.txt`.
- **Siguiente:** tarea 0027 y run 027.
