# 0022 — Un XDBF como el SPA de un juego, para el nombre en Aurora

- **Fecha:** 2026-11-07
- **Objetivo:** que Aurora muestre "Super Mario All-Stars" (run 022: sigue mostrando el nombre del archivo).
- **Análisis:** el XDBF que escribe `xsdk-main/tools/elf2xex.py` solo lleva el idioma por defecto (`XSTC`) y la tabla
  de cadenas (`XSTR`, cadena `0x8000` = nombre); la consola lo ve (`XexGetModuleSection`, PASS). Un SPA de juego
  (free60.org, "SPA" y "XDBF") lleva además la cabecera del título `XTHD` (ID, tipo de título, versión), los logros
  `XACH`, la imagen del título (PNG con ID `0x8000`) y el XLAST (`XSRC`). Hipótesis: Aurora trata el recurso como el
  SPA de un juego solo si tiene `XTHD` (no hay documentación pública de lo que lee Aurora).
- **Hecho:**
  - `SuperMarioAllStars-main/xenon/tools/elf2xex_spa.py`: carga el `elf2xex.py` de xsdk sin cambiarlo y sustituye su
    `title_database` por uno con `XACH` (vacío), `XTHD` (título 534D4153, tipo 1 = juego completo, versión 0.4.0.0),
    `XSTC` y `XSTR`. Cabe en el sitio que xsdk le da junto a las cabeceras PE (`TITLE_NAME_END` = 0x800): 217 bytes. La
    imagen del título (un PNG de varios KB) no cabe ahí: haría falta cambiar xsdk.
  - `xenon/CMakeLists.txt`: `smas_add_xex()` (como `xsdk_add_xex`, con el envoltorio) para `SuperMarioAllStars`.
  - `AGENTS.md` §3, runs 022 y 023.
- **Probado:** Xenia: lee el nombre ("Title name: Super Mario All-Stars"), la tabla de logros (vacía) y la ventana
  muestra `[534D4153 v0.4.0.0] Super Mario All-Stars`; la comprobación del título pasa (217 bytes). Avisa de que no hay
  XLAST (solo para Xbox Live).
- **Estado:** pendiente de Aurora (run 023). Si no basta: (a) el XLAST (`XSRC`) y la imagen del título, que piden más
  sitio en el `.xex` y por tanto un cambio en `elf2xex.py` de xsdk (proponerlo antes); (b) Aurora permite renombrar a
  mano; (c) llamar al archivo como el juego.
- **Siguiente:** hito 8 adelantado (rendimiento), si el usuario lo confirma.
