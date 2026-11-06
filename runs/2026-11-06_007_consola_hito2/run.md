# Run 007 — Hito 2: el guion de comparación en la consola

- **Fecha:** 2026-11-06
- **Dónde:** Consola (RGH), lanzado desde USB.
- **Commit:** `1c18445` (tarea 0007, base v1.0); SHA1 del `.xex` y del guion en `paquete.sha1`.
- **Qué se probó:** `smas_hito2.xex` (GCC de xsdk, newlib) con el guion de la run 004 más benchmarks.
- **Resultado:** Parcial.
  - **Las 9 salidas que volvieron son idénticas byte a byte a las de x86 (run 004):** `menu.wav` y las 8 capturas,
    hasta la de co-op, que es la última del guion. El guion terminó, y el GCC de xsdk y newlib dan lo mismo que x86,
    audio incluido (la tabla del DSP con `std::exp`).
  - **No volvió `smas_hito2.log`** (las 22 líneas de hashes y los fps de los benchmarks): pedido al usuario.
  - **Problema en la consola:** 4 capturas llegaron como `menu.tmp`, `smb1.tmp`, `smb2.tmp`, `smb3.tmp` (con el
    contenido correcto), y no volvió `smb1.state`. `writeFileSafely` escribe `<nombre>.png.tmp` y lo renombra; en la
    consola el temporal quedó con otro nombre y sin renombrar en 4 de 8 casos. Por investigar con el log y el listado
    de la carpeta tal como está en la consola (hito 7: guardado seguro).
- **Ficheros:** `menu.wav`, PNG y `.tmp` (no se suben; comparados con `runs/…004…/x86/archivos.sha1`).
- **Siguiente:** el log y el listado de la carpeta en la consola; repetir con la base v1.1.1 (tarea 0008).
