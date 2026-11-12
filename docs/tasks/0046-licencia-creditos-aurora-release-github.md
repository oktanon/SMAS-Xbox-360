# 0046 — Licencia MIT, créditos de Ttano, el nombre en Aurora y el release de GitHub

- **Fecha:** 2026-11-12
- **Pedido del usuario:** licencia MIT; subir solo el `.xex` a los releases de GitHub, con versión y notas en
  inglés; el nombre en Aurora como en su proyecto Stick Fight; en Credits, "Ttano" en lugar del usuario del PC.
- **Hecho:**
  - **`LICENSE`** en la raíz: MIT, Copyright (c) 2026 Ttano, con una nota de alcance (el port de PC es de Darhkfox, con
    su MIT; las dependencias y las fuentes, con las suyas). `make_release.sh` lo pone primero en los avisos.
  - **`src/app/app.cpp` (Credits):** "Ported by" dice "Ttano". Es una línea que solo existe en Xbox (la sección "Xbox
    360 port" de la tarea 0029).
  - **El nombre en Aurora:**
    - Stick Fight (`xsdk_add_xex(... TITLE_ID ... NAME ... VERSION ...)`) lleva el XDBF que escribe xsdk: solo `XSTC`
      y `XSTR`. Las cabeceras de los dos `.xex` tienen la misma forma; el XDBF de Stick Fight es el mismo que tuvo
      este `.xex` en la run 022, cuando Aurora mostró el nombre del archivo.
    - Así que el formato no explica la diferencia. Lo más probable: Aurora guardó la entrada de la carpeta del juego
      cuando el `.xex` aún no tenía nombre (antes de la tarea 0021) y no la volvió a leer al sobrescribirlo; Stick
      Fight se escaneó por primera vez ya con nombre. Sin documentación pública de Aurora: lo dirá la run 043, en una
      carpeta nueva.
    - **`xenon/CMakeLists.txt`:** el juego se empaqueta con `xsdk_add_xex`, como Stick Fight; fuera
      `xenon/tools/elf2xex_spa.py` (tarea 0022, que no cambió nada en la run 023). El XDBF es ahora el de Stick Fight
      byte a byte salvo el nombre y el ID; la versión 1.0.0.0 sigue en la cabecera del `.xex`.
  - **Release de GitHub:**
    - Un workflow de GitHub Actions no puede compilar el `.xex`: el GCC de xsdk, xsdk, vulkan4xenon y SDL-xenon no
      están en el repositorio. El release se hace desde la máquina que compila.
    - **`xenon/release/publish_release.sh <versión> [--publish]`** (Git Bash, `gh`): crea el release `v<versión>`
      sobre el commit actual, que tiene que estar en `origin/main`, con solo el `.xex`, el título "Super Mario
      All-Stars for Xbox 360 v<versión>", las notas de `xenon/release/notes/<versión>.md` y el SHA-1 del `.xex`. Sin
      `--publish` queda como borrador.
    - **`xenon/release/notes/1.0.md`:** las notas de la 1.0, en inglés: qué trae, rendimiento, instalar, controles,
      limitaciones, licencias.
    - **`THIRD_PARTY_NOTICES.txt`** en la raíz (lo regenera `make_release.sh`): el release lleva solo el `.xex`, y el
      aviso BSD de libxenon y Xenia pide acompañar al binario; las notas lo enlazan.
  - `README.md`, el `README.txt` del paquete, `AGENTS.md` §3: la licencia, el release; fuera la limitación del nombre
    en Aurora (vuelve si la run 043 la ve).
- **Probado:** compila sin avisos; el XDBF del `.xex` tiene solo `XSTC` y `XSTR` con "Super Mario All-Stars"; el
  paquete se arma con los avisos (la licencia de Ttano primero). `publish_release.sh` no se ha ejecutado: publicar
  espera a la run 043 y al usuario.
- **Siguiente:** run 043 (instalación limpia, el nombre en Aurora, Credits); después, subir los commits y el release.
