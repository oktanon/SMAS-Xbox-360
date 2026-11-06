# Runs

Una carpeta por ejecución de prueba: `AAAA-MM-DD_NNN_<qemu|xenia|consola>_<descripcion>/`, con un `run.md` corto
(copiar `_PLANTILLA/run.md`) y los logs/capturas. `NNN` es correlativo global. qemu solo compara la lógica y el orden
de bytes, y Xenia no mide rendimiento: solo una run de `consola` cierra un hito.
