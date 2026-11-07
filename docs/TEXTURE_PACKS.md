# Packs de gráficos (investigación, sin implementar)

Pregunta (run 025): ¿se pueden cargar encima de los gráficos de All-Stars los de los juegos originales de NES, como
un pack de texturas?

## Cómo dibuja el port

No hay texturas: el port emula la PPU de la SNES (`src/core/ppu.cpp`). Los gráficos son tiles de 8×8 a 4 bits por
píxel (16 colores de una paleta de 256) que el juego copia a la VRAM por DMA desde la ROM, mapas de tiles y sprites
(OAM). La imagen sale línea a línea de eso. Un "pack" tiene que entrar en alguno de esos puntos.

## Formas posibles

1. **Parche a la ROM (IPS/BPS).** Cambiar los tiles y paletas en la ROM. Es lo que hacen los hacks de la comunidad;
   el port ya aplica parches a su copia en memoria (`applyRomPatch`, para MSU-1). Costo de ejecución: cero. Límite: los
   juegos de NES usan tiles de 2 bits (4 colores) y fondos de una capa; All-Stars usa 16 colores, fondos con varias
   capas y parallax, y bloques compuestos de otra forma. Un parche que "parezca NES" cambia tiles, paletas y a veces
   mapas: es un hack de ROM entero, no un reemplazo de imágenes.
2. **Reemplazo de tiles al cargar la VRAM.** Enganchar las escrituras de DMA a la VRAM (el core ya tiene
   `onVramWrite`), calcular un hash de cada bloque de tiles y, si está en el pack, escribir el reemplazo (misma
   resolución, mismo formato de 4 bits). Barato: solo cuando el juego sube gráficos, no por píxel. Mismo límite de
   arte que el 1, pero el pack se reparte aparte de la ROM, como un pack de texturas.
3. **Reemplazo en alta resolución al dibujar** (como los "HD packs" de Mesen para NES): por cada tile dibujado, buscar
   su hash con su paleta y pintar una imagen más grande. Exige una PPU que dibuje a más resolución que 256×224 y una
   búsqueda por tile en cada línea: en el núcleo in-order de la Xbox, a 60 fps, muy probablemente no entra (el dibujo
   ya es el 77-85 % del frame sin esto, run 010). Descartado en la consola.

## Si se hiciera

- La opción 2 es la que encaja en la Xbox: un formato de pack propio (una carpeta `packs/<nombre>` con un índice de
  hashes → tiles), cargado al arrancar, y un interruptor en Options. Probarlo primero en PC (smastool) y con la
  comparación de hashes de siempre, luego en la consola.
- **Derechos:** los gráficos de los juegos originales son de Nintendo. Como la ROM, el pack lo tiene que poner el
  usuario: el port no puede traerlo ni descargarlo.
- Trabajo grande (formato, herramienta para crear packs, hooks en el core bajo una macro nueva). Para después del
  hito 10, si se decide.
