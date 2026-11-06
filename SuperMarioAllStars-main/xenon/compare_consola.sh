# Compara lo que devolvió smas_hito2.xex en la consola con la referencia x86 de la run 004 (hito 2).
# Uso, desde la raíz del repositorio, en WSL:
#   bash SuperMarioAllStars-main/xenon/compare_consola.sh <carpeta de la run de consola>
# La carpeta debe tener smas_hito2.log y, si se trajeron, los PNG y el WAV que escribió el título.
# Sale con 0 si todo lo que se pudo comparar coincide.
set -uo pipefail

run="${1:?uso: compare_consola.sh <carpeta de la run>}"
ref="runs/2026-11-06_004_qemu_hito1b/x86"
log="$run/smas_hito2.log"
[ -f "$log" ] || { echo "falta $log" >&2; exit 2; }
[ -f "$ref/hashes.txt" ] || { echo "falta la referencia $ref/hashes.txt (ejecutar desde la raíz)" >&2; exit 2; }

n=$(wc -l < "$ref/hashes.txt")
# El log de xsdk puede terminar las líneas en \r\n
tr -d '\r' < "$log" | grep -E '^(HASHALL|ACH|WAITLOAD) ' > "$run/hashes_consola.txt"
head -n "$n" "$run/hashes_consola.txt" > "$run/hashes_consola_guion.txt"
ok=1
got=$(wc -l < "$run/hashes_consola_guion.txt")
if [ "$got" -lt "$n" ]; then
  ok=0
  if head -n "$got" "$ref/hashes.txt" | diff -q - "$run/hashes_consola_guion.txt" > /dev/null; then
    echo "hashes: el log se corta en la línea $got de $n (¿se colgó o no terminó?); las $got que hay coinciden"
  else
    echo "hashes: el log se corta en la línea $got de $n, y además hay diferencias (ver diferencias.txt)"
    head -n "$got" "$ref/hashes.txt" | diff -u - "$run/hashes_consola_guion.txt" > "$run/diferencias.txt"
  fi
elif diff -u "$ref/hashes.txt" "$run/hashes_consola_guion.txt" > "$run/diferencias.txt"; then
  echo "hashes: IGUALES ($n líneas del guion)"
  rm -f "$run/diferencias.txt"
else
  ok=0
  echo "hashes: DISTINTOS (ver diferencias.txt)"
  paste -d'\n' "$ref/hashes.txt" "$run/hashes_consola_guion.txt" | awk 'NR%2{a=$0;next} a!=$0{
    n=split(a,x," "); split($0,y," "); printf "primera diferencia: %s %s ->", x[1], x[2];
    for(i=3;i<=n;i++) if(x[i]!=y[i]) printf " %s/%s", x[i], y[i]; print ""; exit}'
fi

# Archivos que escribe el guion, si se trajeron de la consola
checked=0
while read -r sum file; do
  file="${file#\*}"
  [ -f "$run/$file" ] || continue
  checked=$((checked + 1))
  if [ "$(sha1sum "$run/$file" | cut -d' ' -f1)" = "$sum" ]; then
    echo "archivo $file: igual"
  else
    ok=0
    echo "archivo $file: DISTINTO"
  fi
done < "$ref/archivos.sha1"
[ $checked -gt 0 ] || echo "archivos: no se trajo ninguno (PNG/WAV), solo se compararon los hashes"

echo "--- benchmarks (60 fps = tiempo real)"
tr -d '\r' < "$log" | grep -E 'BENCH|frames in|^FIN' | sed 's/^ *//'
[ $ok = 1 ] && echo "RESULTADO: IGUALES" || echo "RESULTADO: DISTINTOS"
[ $ok = 1 ]
