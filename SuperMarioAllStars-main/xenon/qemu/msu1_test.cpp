// Prueba del MSU-1 (src/core/msu1.cpp) para la comparación x86 / qemu-ppc (docs/ENDIANNESS.md §2).
// Escribe una pista sintética little-endian (msu1test-1.pcm: cabecera "MSU1", punto de bucle, muestras con valores
// positivos y negativos que cambian en los dos bytes), la reproduce por los registros como un juego y mezcla 2 s a la
// frecuencia de la consola. Imprime un FNV-1a por valor de lo mezclado y algunas muestras: las dos builds deben dar lo
// mismo. Se ejecuta en una carpeta de trabajo vacía.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "core/msu1.h"

int main()
{
  FILE* f = std::fopen("msu1test-1.pcm", "wb");
  if (!f) return 1;
  const uint32_t frames = 44100, loop = 1000;
  const uint8_t head[8] = {'M', 'S', 'U', '1', uint8_t(loop), uint8_t(loop >> 8), uint8_t(loop >> 16), uint8_t(loop >> 24)};
  std::fwrite(head, 1, 8, f);
  for (uint32_t i = 0; i < frames; i++) {
    const int16_t l = int16_t((i * 977) % 60000 - 30000), r = int16_t(-(int32_t)((i * 331) % 50000) + 25000);
    const uint8_t b[4] = {uint8_t(l), uint8_t(uint16_t(l) >> 8), uint8_t(r), uint8_t(uint16_t(r) >> 8)};
    std::fwrite(b, 1, 4, f);
  }
  std::fclose(f);

  Msu1 msu;
  msu.open(L"msu1test");
  msu.write(4, 1);  // pista 1
  msu.write(5, 0);  // (carga)
  msu.write(6, 255);
  msu.write(7, 3);  // reproducir, en bucle
  std::vector<int16_t> out(32040 * 2 * 2, 0);
  msu.mix(out.data(), out.size() / 2, 32040.0);

  uint32_t h = 2166136261u;
  for (int16_t v : out) {
    h = (h ^ uint8_t(v)) * 16777619u;
    h = (h ^ uint8_t(uint16_t(v) >> 8)) * 16777619u;
  }
  std::printf("MSU1 status=%02x playing=%d hash=%08x s[100]=%d,%d s[50000]=%d,%d\n", msu.read(0), (int)msu.playing(), h,
              out[200], out[201], out[100000], out[100001]);
  return 0;
}
