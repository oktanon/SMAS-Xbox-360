# 0001 — AGENTS.md y estructura del proyecto

- **Fecha:** 2026-11-06
- **Objetivo:** dejar escrito el objetivo del port a Xbox 360, las rutas, las reglas de trabajo, y preparar el
  repositorio.
- **Hecho:**
  - `AGENTS.md` (objetivo, rutas, compilación, pruebas Xenia vs consola, arquitectura, logros nativos, registro).
  - `CLAUDE.md` que remite a `AGENTS.md`.
  - `docs/ROADMAP.md`, `docs/ACHIEVEMENTS_X360.md`, `docs/CHANGELOG.md`.
  - `runs/README.md` y `runs/_PLANTILLA/run.md`.
  - `.gitignore` (ROM, builds, dependencias externas, Xenia) y `git init`.
- **Comprobado:** CRC32 de la ROM = `925637C7`; toolchain presente en WSL Ubuntu (carpeta `xsdk-gcc` del home); `libxsdk.a` ya
  compilada en `xsdk-main/build/`; vulkan4xenon y SDL-xenon aún sin compilar.
- **Probado:** no aplica (solo documentación). Sin run.
- **Siguiente:** hito 1 del roadmap (compilar dependencias y correr sus tests en consola).
