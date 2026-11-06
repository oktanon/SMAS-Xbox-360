// Hito 2 (docs/ROADMAP.md): el emulador sin ventana en la consola. Ejecuta tools/smastool.cpp (compilado con su main
// renombrado a smastool_main) con la ROM y el guion de game:\, el mismo guion que la comparación x86 / qemu-ppc
// (xenon/qemu/guion.txt) más una sección de benchmarks al final. Todo sale por stdout, que xsdk escribe en
// game:\smas_hito2.log; las líneas HASHALL, ACH y WAITLOAD se comparan con las de la run 004.
// Después, un diagnóstico del guardado seguro (run 007: capturas que quedaron como "menu.tmp" sin renombrar): prueba
// writeFileSafely y rename con varios nombres y lista game:\ tal como lo ve la consola.
// Al terminar vuelve al dashboard.
#include <dirent.h>
#include <sys/stat.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "core/fileio.h"
#include "xsdk/splash.h"

int smastool_main(int argc, char** argv);

namespace {

void diagnoseFiles()
{
  std::printf("DIAG guardado seguro\n");
  const char data[] = "smas_hito2 prueba de escritura\n";
  const char* names[] = {"diag_a.png", "diag_bbbbbbbb.png", "diag_c.state", "diag_d.png"};
  for (const char* n : names) {
    errno = 0;
    const bool ok = writeFileSafely(std::string(n), data, sizeof data - 1);
    struct stat st;
    const bool there = stat(n, &st) == 0;
    const std::string tmp = std::string(n) + ".tmp";
    const bool tmpThere = stat(tmp.c_str(), &st) == 0;
    std::printf("DIAG writeFileSafely %s: %s, errno %d (%s); queda %s: %s; queda %s: %s\n", n, ok ? "ok" : "FALLA", errno,
                std::strerror(errno), n, there ? "si" : "no", tmp.c_str(), tmpThere ? "si" : "no");
  }
  // rename a pelo: sobre un destino que no existe y sobre uno que existe
  FILE* f = std::fopen("diag_r.png.tmp", "wb");
  if (f) std::fputs("x", f), std::fclose(f);
  errno = 0;
  int r = std::rename("diag_r.png.tmp", "diag_r.png");
  std::printf("DIAG rename diag_r.png.tmp -> diag_r.png (no existe): %d, errno %d (%s)\n", r, errno, std::strerror(errno));
  f = std::fopen("diag_r.png.tmp", "wb");
  if (f) std::fputs("y", f), std::fclose(f);
  errno = 0;
  r = std::rename("diag_r.png.tmp", "diag_r.png");
  std::printf("DIAG rename diag_r.png.tmp -> diag_r.png (existe): %d, errno %d (%s)\n", r, errno, std::strerror(errno));

  std::printf("DIAG listado de game:\\\n");
  if (DIR* d = opendir(".")) {
    while (dirent* e = readdir(d)) {
      struct stat st;
      const long size = stat(e->d_name, &st) == 0 ? (long)st.st_size : -1;
      std::printf("DIAG   %s (%ld bytes)\n", e->d_name, size);
    }
    closedir(d);
  } else {
    std::printf("DIAG   opendir falla, errno %d\n", errno);
  }
}

}  // namespace

int main()
{
  // Línea a línea: si el título se cuelga a mitad, lo escrito hasta ahí queda en el log (son ~60 líneas; en USB una
  // línea cuesta ~1,8 ms, run 001)
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  std::printf("smas_hito2: el guion de xenon/qemu en la consola (tarda unos minutos; al terminar vuelve al dashboard)\n");

  char name[] = "smas_hito2";
  char rom[] = "Super Mario All-Stars (USA).sfc";  // relativa: game:\ es la carpeta actual al arrancar
  char script[] = "guion.txt";
  char* args[] = {name, rom, script, nullptr};

  const auto t0 = std::chrono::steady_clock::now();
  const int status = smastool_main(3, args);
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("FIN: smastool_main devolvió %d en %.1f s\n", status, s);
  diagnoseFiles();
  std::printf("FIN del título\n");
  std::fflush(stdout);
  return status;
}
