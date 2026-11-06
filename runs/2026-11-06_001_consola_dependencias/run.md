# Run 001 — Tests de las dependencias en consola (hito 1)

- **Fecha:** 2026-11-06 (compilado; pendiente de ejecutar en consola)
- **Dónde:** Consola (RGH)
- **Commit:** tarea 0004
- **Qué se prueba:** `xsdk_test.xex` (xsdk), `vulkan4xenon_test.xex` y `vkbench.xex` (vulkan4xenon),
  `sdl_check.xex` (SDL-xenon), compilados con el toolchain de WSL Ubuntu. SHA1 en `xex.sha1`.
- **Esperado:** vulkan4xenon_test todo PASS; xsdk_test casi todo PASS (sin perfil, sin `net_host.txt` o lanzado
  desde USB fallan unos pocos, como en BT3: 624 de 630); sdl_check PASS con el mando.
- **Resultado:** pendiente — copiar aquí `xsdk_test.log`, `vulkan4xenon_test.log`, `vkbench.log`,
  `vkbench_summary.csv`, `sdl_check.log`.
- **Siguiente:** con los logs, cerrar el hito 1 (o anotar qué falla y por qué).
