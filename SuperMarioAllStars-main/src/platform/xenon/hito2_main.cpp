// Hito 2 (docs/ROADMAP.md): el emulador sin ventana en la consola. Ejecuta tools/smastool.cpp (compilado con su main
// renombrado a smastool_main) con la ROM y el guion de game:\, el mismo guion que la comparación x86 / qemu-ppc
// (xenon/qemu/guion.txt) más una sección de benchmarks al final. Todo sale por stdout, que xsdk escribe en
// game:\smas_hito2.log; las líneas HASHALL, ACH y WAITLOAD se comparan con las de la run 004.
// Al terminar vuelve al dashboard.
#include <chrono>
#include <cstdio>

#include "xsdk/splash.h"

int smastool_main(int argc, char** argv);

int main()
{
  // Línea a línea: si el título se cuelga a mitad, lo escrito hasta ahí queda en el log (son ~40 líneas; en USB una
  // línea cuesta ~1,8 ms, run 001)
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  std::printf("smas_hito2: el guion de xenon/qemu en la consola (tarda unos minutos; al terminar vuelve al dashboard)\n");
  std::fflush(stdout);

  char name[] = "smas_hito2";
  char rom[] = "Super Mario All-Stars (USA).sfc";  // relativa: game:\ es la carpeta actual al arrancar
  char script[] = "guion.txt";
  char* args[] = {name, rom, script, nullptr};

  const auto t0 = std::chrono::steady_clock::now();
  const int status = smastool_main(3, args);
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("FIN: smastool_main devolvió %d en %.1f s\n", status, s);
  std::fflush(stdout);
  return status;
}
