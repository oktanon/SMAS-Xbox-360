// Hito 2 (docs/ROADMAP.md): el emulador sin ventana en la consola. Ejecuta tools/smastool.cpp (compilado con su main
// renombrado a smastool_main) con la ROM y el guion de game:\, el mismo guion que la comparación x86 / qemu-ppc
// (xenon/qemu/guion.txt) más una sección de benchmarks al final. Todo sale por stdout, que xsdk escribe en
// game:\smas_hito2.log; las líneas HASHALL, ACH y WAITLOAD se comparan con las de la run 004.
// Los hilos de dibujo de la PPU (threads N, docs/PERFORMANCE.md §3.3) se colocan en los hilos hardware 2, 4, 3...
// (los otros dos núcleos primero; la emulación va en el 0).
// Después, un diagnóstico del guardado seguro (fileio.cpp: los nombres que el USB recorta).
// Al terminar vuelve al dashboard.
#include <dirent.h>
#include <sys/stat.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "core/fileio.h"
#include "core/ppu.h"
#include "xsdk/splash.h"
#include "xsdk/thread.h"

int smastool_main(int argc, char** argv);

namespace {

constexpr uint32_t kRenderHw[] = {2, 4, 3, 5, 1};
// (where each render thread ended up, and the move's status, for the log)
std::atomic<int> placedHw[5];
std::atomic<long> placedStatus[5];

void placeRenderThread(int index)
{
  const uint32_t hw = kRenderHw[index % 5];
  const NTSTATUS st = xsdk_thread_move(hw);
  placedStatus[index % 5].store((long)st);
  placedHw[index % 5].store((int)xsdk_thread_hardware_thread());
}

void diagnoseFiles()
{
  std::printf("DIAG guardado seguro\n");
  const char data[] = "smas_hito2 prueba de escritura\n";
  // writeFileSafely (en Xbox, el temporal con los puntos como _: fileio.cpp) con nombres que el USB recortaba
  const char* names[] = {"diag_a.png", "menu_diag.png", "abcd.png", "abc.defg", "smb9.png", "diag_c.state"};
  for (const char* n : names) {
    errno = 0;
    const bool ok = writeFileSafely(std::string(n), data, sizeof data - 1);
    struct stat st;
    const bool there = stat(n, &st) == 0 && st.st_size == (off_t)(sizeof data - 1);
    std::printf("DIAG writeFileSafely %s: %s, errno %d (%s); queda %s con su contenido: %s\n", n, ok ? "ok" : "FALLA",
                errno, std::strerror(errno), n, there ? "si" : "NO");
  }
}

}  // namespace

int main()
{
  // Línea a línea: si el título se cuelga a mitad, lo escrito hasta ahí queda en el log (en USB una línea cuesta
  // ~1,8 ms, run 001)
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  xsdk_splash_show(1500);
  for (auto& h : placedHw) h.store(-1);
  Ppu::renderThreadStart = placeRenderThread;
  std::printf("smas_hito2: el guion de xenon/qemu en la consola (tarda unos minutos; al terminar vuelve al dashboard)\n");
  std::printf("smas_hito2: la emulación en el hilo hardware %u\n", (unsigned)xsdk_thread_hardware_thread());

  char name[] = "smas_hito2";
  char rom[] = "Super Mario All-Stars (USA).sfc";  // relativa: game:\ es la carpeta actual al arrancar
  char script[] = "guion.txt";
  char* args[] = {name, rom, script, nullptr};

  const auto t0 = std::chrono::steady_clock::now();
  const int status = smastool_main(3, args);
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("FIN: smastool_main devolvió %d en %.1f s\n", status, s);
  for (int i = 0; i < 5; i++)
    if (placedHw[i].load() >= 0)
      std::printf("hilo de dibujo %d: pedido el hilo hardware %u, quedó en el %d (estado 0x%08lX)\n", i,
                  (unsigned)kRenderHw[i], placedHw[i].load(), (unsigned long)placedStatus[i].load());
  diagnoseFiles();
  std::printf("FIN del título\n");
  std::fflush(stdout);
  return status;
}
