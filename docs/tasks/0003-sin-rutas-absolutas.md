# 0003 — Regla: sin rutas absolutas

- **Fecha:** 2026-11-06
- **Objetivo:** prohibir las rutas absolutas en todo el proyecto y quitar las que ya había en los documentos.
- **Hecho:**
  - `AGENTS.md` §0: la regla, qué usar en su lugar (relativas a la raíz, `game:\` en el título, `$PWD` construido al
    ejecutar) y la búsqueda `git grep` que se pasa antes de cada commit.
  - `AGENTS.md`: raíz descrita como "la carpeta de este `AGENTS.md`"; comandos de WSL relativos (WSL arranca en la
    carpeta actual de PowerShell); Xenia lanzado con `.\XSDK Xenia\xsdk_xenia.exe`; BT3 como
    `../Varios/Tenkaichi3Decomp-main/Tenkaichi3Decomp-main`; toolchain como "carpeta `xsdk-gcc` del home de WSL";
    sin rutas del disco de la consola.
  - `docs/ENDIANNESS.md`, `docs/tasks/0001-…`, `docs/tasks/0002-…`: rutas pasadas a relativas.
  - `docs/ENDIANNESS.md` y `AGENTS.md`: `powerpc-linux-gnu-g++` 15.2 consta como instalado; enlazar con `-static` para
    qemu (así no hace falta la ruta de las bibliotecas de PowerPC).
- **Comprobado:** la búsqueda de §0 solo encuentra la propia §0 y un `game:\`. `powerpc-linux-gnu-g++ -static` +
  `qemu-ppc` ejecuta un programa de prueba que se identifica como big-endian; el mismo con `g++` como little-endian.
- **Probado:** no aplica (documentación). Sin run.
- **Siguiente:** hito 1 (dependencias) y 1b (comparación x86 / qemu-ppc).
