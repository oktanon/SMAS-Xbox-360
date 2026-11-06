# Compila smastool (el emulador sin ventana) dos veces, para comparar el orden de bytes (docs/ENDIANNESS.md §2):
#   x86 : g++ nativo, little-endian (la referencia)
#   ppc : powerpc-linux-gnu-g++ -static, big-endian, se ejecuta con qemu-ppc
# Uso, desde la raíz del repositorio, en WSL:  bash SuperMarioAllStars-main/xenon/qemu/build.sh [x86|ppc|all]
# Salida en build-qemu/<x86|ppc>/smastool, más build-qemu/<x86|ppc>/stamp.txt con el commit compilado.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
game="$(cd "$here/../.." && pwd)"
root="$(cd "$game/.." && pwd)"
out="$root/build-qemu"
which_builds="${1:-all}"

sources=(
  src/core/cpu65816.cpp src/core/apu.cpp src/core/dsp.cpp src/core/ppu.cpp src/core/snes.cpp src/core/disasm.cpp
  src/core/png.cpp src/core/fileio.cpp src/core/rewind.cpp
  src/game/achievements.cpp src/game/smas.cpp src/game/smb1.cpp src/game/smb1coop.cpp src/game/palette.cpp
  src/game/smb3.cpp src/game/smb3coop.cpp src/game/smb2.cpp
  tools/smastool.cpp
)
# -ffp-contract=off: GCC en PowerPC funde a*b+c en fmadd y x86 no; sin esto la comparación se ensucia.
flags=(-std=c++17 -O2 -ffp-contract=off -fno-strict-aliasing -I"$game/src")

stamp="$(git -C "$root" rev-parse --short HEAD 2>/dev/null || echo sin-git)"
if ! git -C "$root" diff --quiet HEAD -- "$game/src" "$game/tools" 2>/dev/null; then stamp="$stamp+cambios"; fi

build() {
  local name="$1" cxx="$2"; shift 2
  mkdir -p "$out/$name/obj"
  local objs=()
  for s in "${sources[@]}"; do
    local o="$out/$name/obj/$(echo "$s" | tr '/' '_').o"
    objs+=("$o")
    (cd "$game" && "$cxx" "${flags[@]}" "$@" -c "$s" -o "$o") &
  done
  wait
  "$cxx" "$@" "${objs[@]}" -o "$out/$name/smastool"
  echo "$stamp" > "$out/$name/stamp.txt"
  echo "$name: build-qemu/$name/smastool ($stamp)"
}

case "$which_builds" in
  x86) build x86 g++ ;;
  ppc) build ppc powerpc-linux-gnu-g++ -static ;;
  all) build x86 g++; build ppc powerpc-linux-gnu-g++ -static ;;
  *) echo "uso: $0 [x86|ppc|all]" >&2; exit 2 ;;
esac
