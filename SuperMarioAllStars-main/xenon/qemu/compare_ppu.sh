# Comparación frame a frame de la PPU rápida (SMAS_PPU_FAST) con la original (docs/PERFORMANCE.md §3.1).
# Compila smastool x86 dos veces (sin y con SMAS_PPU_FAST), ejecuta guion_ppu.txt en las dos y compara cada línea
# PIC (el hash de la imagen de cada frame). Uso, desde la raíz del repositorio, en WSL:
#   bash SuperMarioAllStars-main/xenon/qemu/compare_ppu.sh <carpeta de salida> [control]
# Con HILOS=N en el entorno, la rápida dibuja además con N hilos (threads N, docs/PERFORMANCE.md §3.3).
# Sale con 0 si todos los frames coinciden. Con "control", la rápida se compila rota a propósito
# (SMAS_PPU_FAST_CONTROL: un bit de color en un píxel de BG1) y debe dar DISTINTOS.
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
game="$(cd "$here/../.." && pwd)"
root="$(cd "$game/.." && pwd)"
out="${1:?uso: compare_ppu.sh <carpeta de salida> [control]}"
mkdir -p "$out" && out="$(cd "$out" && pwd)"
extra=()
[ "${2:-}" = control ] && extra=(-DSMAS_PPU_FAST_CONTROL)
rom="$root/Super Mario All-Stars (USA).sfc"
[ -f "$rom" ] || { echo "falta la ROM en la raíz" >&2; exit 2; }

bin="$root/build-qemu/ppu"
mkdir -p "$bin/ref" "$bin/fast"
flags=(-pthread -std=c++17 -O2 -DSMAS_PORT_FIXES -DSMAS_CPU_FAST -ffp-contract=off -fno-strict-aliasing -I"$game/src")
srcs=($(cd "$game" && echo src/core/*.cpp src/game/*.cpp tools/smastool.cpp))
(cd "$game" && g++ "${flags[@]}" "${srcs[@]}" -o "$bin/ref/smastool") &
(cd "$game" && g++ "${flags[@]}" -DSMAS_PPU_FAST "${extra[@]}" "${srcs[@]}" -o "$bin/fast/smastool") &
wait
[ -x "$bin/ref/smastool" ] && [ -x "$bin/fast/smastool" ] || { echo "no compiló" >&2; exit 2; }

rm -rf "$out/ref" "$out/fast" "$out/diferencias.txt" "$out/resumen.txt"
mkdir -p "$out/ref" "$out/fast"
guion_fast="$here/guion_ppu.txt"
if [ -n "${HILOS:-}" ]; then
  guion_fast="$out/guion_hilos.txt"
  { echo "threads $HILOS"; cat "$here/guion_ppu.txt"; } > "$guion_fast"
fi
for b in ref fast; do
  t0=$(date +%s)
  guion="$here/guion_ppu.txt"
  [ $b = fast ] && guion="$guion_fast"
  (cd "$out/$b" && "$bin/$b/smastool" "$rom" "$guion" > salida.txt 2>&1)
  echo "$b: código $? en $(( $(date +%s) - t0 )) s"
  grep -E '^(PIC|WAITLOAD) ' "$out/$b/salida.txt" > "$out/$b/pic.txt"
done
n=$(grep -c '^PIC ' "$out/ref/pic.txt")
if diff -q "$out/ref/pic.txt" "$out/fast/pic.txt" > /dev/null && [ "$n" -gt 0 ]; then
  echo "RESULTADO: IGUALES ($n frames)" | tee "$out/resumen.txt"
  exit 0
fi
diff "$out/ref/pic.txt" "$out/fast/pic.txt" | head -20 > "$out/diferencias.txt"
echo "RESULTADO: DISTINTOS ($n frames en ref; primeras diferencias en diferencias.txt)" | tee "$out/resumen.txt"
exit 1
