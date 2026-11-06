# Run 020 — Widescreen 16:9 como el PC y stick por sectores (hito 4)

- **Fecha:** 2026-11-06 (preparada; pendiente de ejecutar en la consola)
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0019; SHA1 en `paquete.sha1`.
- **Qué se prueba:** `smas_jugar.xex`: el stick derecho cambia entre 4:3 y widescreen 16:9 (46 píxeles por lado, píxeles
  8:7, la imagen entera centrada como en el PC: `VideoVk::fitLayout`); el stick izquierdo por sectores
  (`Input::stickToDpad`). Salir: Back + LB + RB 3 s.
- **Esperado:** en widescreen nada cortado y el HUD quieto en los bordes; el stick en diagonal ligera no agacha a Mario;
  4:3 a 60 sin tirones; widescreen más barato que antes (46 en vez de 64 por lado). El co-op y SMB3 ancho pueden
  seguir justos (hito 8).
- **Resultado:** pendiente — `smas_jugar.log`, cómo se siente, y si se puede una foto en widescreen.
- **Siguiente:** —
