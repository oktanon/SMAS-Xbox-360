# Fallos encontrados en el port base (upstream)

El port base es `github.com/Darhkfox/SuperMarioAllStars` (hoy v1.1.1, AGENTS.md §2). Lo que se encuentre mal en su
código va aquí, con su corrección en el port y si se reportó. Las correcciones van bajo `#ifdef SMAS_PORT_FIXES`
(definida en `xenon/CMakeLists.txt`, `xenon/qemu/build.sh` y `compare_ppu.sh`), con el código original intacto, para
que PC y Android sigan igual que upstream y se pueda reportar cada una.

| # | Dónde | Qué pasa | Riesgo | Corrección | Reportado |
|---|---|---|---|---|---|
| 1 | `src/game/smb2.cpp`, `Smb2Engine::playerReachesEdge` (~línea 1013) | En el recorrido de los tiles abiertos, la columna `cc` puede valer -1 o 16 y no se comprueba: en la primera fila (izquierda) o la última (derecha), `seen[rr * 16 + cc]` lee fuera del vector, y si lo que hay allí vale 0, escribe `seen[...] = 1` fuera de él (heap). | Alto: lectura y escritura fuera de límites (comportamiento indefinido, corrupción del heap); el resultado puede depender de la memoria y cambiar entre plataformas. ThreadSanitizer lo hizo fallar (SEGV) en SMB2 1-1 con widescreen 64, también sin hilos (tarea 0013). | Un paso cuyo índice cae fuera de `seen` se toma como visto; los demás, como el original. | No (pendiente: abrir un issue con la descripción de esta fila) |
| 2 | `src/core/dsp.cpp:153` | Desplazamiento a la izquierda de un número negativo (UBSan: "left shift of negative value -2"). | Bajo: comportamiento indefinido en C++17, pero GCC lo compila como desplazamiento en complemento a dos en x86 y PowerPC (C++20 lo define así), y las comparaciones x86 / qemu-ppc / consola dan lo mismo. | Ninguna. | No |
| 3 | `src/core/dsp.cpp`, `Dsp::saveState` (~línea 361) | `DspFlat f;` sin inicializar se copia entero al state, relleno incluido: esos bytes salen de lo que hubiera en la pila, así que dos states de la misma partida pueden diferir en un byte (y en el CRC). Visto comparando states (tarea 0038). | Bajo: `loadState` no lee el relleno; solo impide comparar states byte a byte. | Ninguna (bastaría `DspFlat f{};`). | No |

## Cómo se buscan

- ThreadSanitizer (`-fsanitize=thread`) con los hilos de dibujo (`threads N`) — tarea 0013.
- AddressSanitizer + UBSan (`-fsanitize=address,undefined`) sobre `xenon/qemu/guion_ppu.txt` — tarea 0013.
- La búsqueda estática de `docs/ENDIANNESS.md` §3.5 y la comparación x86 / qemu-ppc.
