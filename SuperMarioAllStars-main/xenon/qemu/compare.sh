# Ejecuta el guion con smastool x86 y con smastool PowerPC (qemu-ppc) y compara (docs/ENDIANNESS.md §2).
# Uso, desde la raíz del repositorio, en WSL:
#   bash SuperMarioAllStars-main/xenon/qemu/compare.sh <carpeta de salida> [guion]
# Antes: build.sh all. La ROM es "Super Mario All-Stars (USA).sfc" en la raíz.
# Sale con 0 si todas las líneas HASHALL, ACH y WAITLOAD y los archivos que deja el guion (salvo los states)
# coinciden, con 1 si no. Control negativo: guion_control.txt, que debe dar DISTINTOS.
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
out="${1:?uso: compare.sh <carpeta de salida> [guion]}"
script="${2:-$here/guion.txt}"
rom="$root/Super Mario All-Stars (USA).sfc"
script="$(cd "$(dirname "$script")" && pwd)/$(basename "$script")"

[ -f "$rom" ] || { echo "falta la ROM en la raíz" >&2; exit 2; }
for b in x86 ppc; do
  [ -x "$root/build-qemu/$b/smastool" ] || { echo "falta build-qemu/$b/smastool: ejecutar build.sh" >&2; exit 2; }
done
stx="$(cat "$root/build-qemu/x86/stamp.txt")"
stp="$(cat "$root/build-qemu/ppc/stamp.txt")"
[ "$stx" = "$stp" ] || { echo "las builds son de commits distintos ($stx / $stp)" >&2; exit 2; }

# Salidas de una run anterior fuera, para no comparar resultados viejos (lección de bt3xbx).
rm -rf "$out/x86" "$out/ppc"
mkdir -p "$out/x86" "$out/ppc"

run() {
  local b="$1"; shift
  local t0=$(date +%s)
  (cd "$out/$b" && "$@" "$rom" "$script" > salida.txt 2>&1)
  echo "$b: código $? en $(( $(date +%s) - t0 )) s" | tee -a "$out/$b/salida.txt"
}
run x86 "$root/build-qemu/x86/smastool"
run ppc qemu-ppc "$root/build-qemu/ppc/smastool"

for b in x86 ppc; do grep -E '^(HASHALL|ACH|WAITLOAD) ' "$out/$b/salida.txt" > "$out/$b/hashes.txt"; done

{
  echo "build: $stx"
  echo "lineas x86: $(wc -l < "$out/x86/hashes.txt"), ppc: $(wc -l < "$out/ppc/hashes.txt")"
} > "$out/resumen.txt"

if [ ! -s "$out/x86/hashes.txt" ]; then
  echo "RESULTADO: sin hashes en x86 (ver x86/salida.txt)" | tee -a "$out/resumen.txt"; exit 1
fi
# Archivos que deja el guion (capturas PNG, WAV, dumps...): deben ser idénticos byte a byte. Los save states no
# entran: guardan los bytes en el orden del host y se espera que difieran (docs/ENDIANNESS.md §3.4).
for b in x86 ppc; do
  (cd "$out/$b" && find . -maxdepth 1 -type f ! -name salida.txt ! -name hashes.txt ! -name archivos.sha1 \
    ! -name '*.state' -printf '%f\n' | sort | xargs -r sha1sum) > "$out/$b/archivos.sha1"
done
files_ok=1
if ! diff -u "$out/x86/archivos.sha1" "$out/ppc/archivos.sha1" > "$out/diferencias_archivos.txt"; then
  files_ok=0
else
  rm -f "$out/diferencias_archivos.txt"
fi
echo "archivos: $(wc -l < "$out/x86/archivos.sha1") ($( [ $files_ok = 1 ] && echo iguales || echo DISTINTOS))" >> "$out/resumen.txt"

if diff -u "$out/x86/hashes.txt" "$out/ppc/hashes.txt" > "$out/diferencias.txt" && [ $files_ok = 1 ]; then
  echo "RESULTADO: IGUALES ($(wc -l < "$out/x86/hashes.txt") líneas, $(wc -l < "$out/x86/archivos.sha1") archivos)" | tee -a "$out/resumen.txt"
  rm -f "$out/diferencias.txt"
  exit 0
fi
[ -s "$out/diferencias.txt" ] || rm -f "$out/diferencias.txt"
echo "RESULTADO: DISTINTOS (ver diferencias*.txt)" | tee -a "$out/resumen.txt"
# La primera línea distinta y qué campos difieren, para empezar a buscar por ahí
paste -d'\n' "$out/x86/hashes.txt" "$out/ppc/hashes.txt" | awk 'NR%2{a=$0;next} a!=$0{
  n=split(a,x," "); split($0,y," "); printf "primera diferencia: %s %s ->", x[1], x[2];
  for(i=3;i<=n;i++) if(x[i]!=y[i]) printf " %s/%s", x[i], y[i]; print ""; exit}' | tee -a "$out/resumen.txt"
exit 1
