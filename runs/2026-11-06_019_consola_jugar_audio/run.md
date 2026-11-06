# Run 019 — Jugar con el hilo de audio fuera del de la emulación (hito 4)

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), desde USB.
- **Commit:** `318b4b1` (tarea 0018); SHA1 en `paquete.sha1`.
- **Qué se probó:** `smas_jugar.xex` con el hilo de audio de SDL en el hilo hardware 5; SMB1 (1 y 2 jugadores) y SMB3,
  en 4:3 y widescreen 64. El usuario mandó una foto de la tele (SMB1 1-2 en widescreen, con el menú de pausa del
  juego).
- **Resultado:** **mejoró, no del todo.**
  - **4:3 con un jugador: arreglado.** SMB1 9,4-11,8 ms por frame, 0 frames de más (antes 15,6 ms). Audio 0,4 ms,
    mandos 0,15-0,3 ms.
  - **Co-op:** 16,7 ms en 4:3 (266 de 600 de más), 18,3 ms en widescreen (600 de 600). SMB3 en widescreen 64: 16-20
    ms. (Perfil en x86, tarea 0019: en co-op la CPU emula ~40 % más; la PPU, como siempre.)
  - **Widescreen 64 se cortaba** (en la foto, TIME cortado a la derecha): 384 píxeles × 3,75 = 1440 > 1280; y como se
    centraba la parte 4:3 con márgenes desiguales (56 + 72), la imagen entera se movía y con ella el HUD.
  - **Stick en diagonal:** con un poco de abajo Mario se agacha y deja de andar (SMB1 para con Abajo); el PC toma cada
    eje por su lado.
- **Logs:** `smas_jugar.log`.
- **Siguiente:** run 020 (tarea 0019: widescreen como el PC, 16:9 con píxeles 8:7 = 46 por lado; stick por sectores).
