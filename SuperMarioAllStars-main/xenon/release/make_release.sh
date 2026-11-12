# Paquete de release (hito 10): uso, desde la raíz del repositorio, en WSL:
#   bash SuperMarioAllStars-main/xenon/release/make_release.sh [versión]
# Deja en build-xenon/release/ la carpeta SuperMarioAllStars-x360-<versión>/ y su .zip, con:
#   SuperMarioAllStars.xex (el de build-xenon/), README.txt y THIRD_PARTY_NOTICES.txt.
# Los avisos se arman con los archivos de licencia de cada proyecto, tal cual: todo lo que entra en el .xex.
set -eu
ver="${1:-1.0}"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
game="$root/SuperMarioAllStars-main"
xex="$root/build-xenon/SuperMarioAllStars.xex"
out="$root/build-xenon/release/SuperMarioAllStars-x360-$ver"
[ -f "$xex" ] || { echo "falta build-xenon/SuperMarioAllStars.xex: compilarlo antes" >&2; exit 1; }
for f in "$game/LICENSE" "$root/xsdk-main/LICENSE" "$root/xsdk-main/THIRD_PARTY_NOTICES.md" \
         "$root/vulkan4xenon-main/LICENSE" "$root/SDL-xenon-xenon/LICENSE.txt" \
         "$game/third_party/fonts/DejaVu-LICENSE.txt" "$game/third_party/fonts/LICENSE.txt"; do
  [ -f "$f" ] || { echo "falta ${f#$root/}" >&2; exit 1; }
done
rm -rf "$out" "$out.zip"
mkdir -p "$out"
cp "$xex" "$out/"
sed 's/$/\r/' "$here/README.txt" > "$out/README.txt"

section() { printf '\n\n%s\n%s\n\n' "$1" "$(printf '%*s' ${#1} '' | tr ' ' '=')"; }
{
  echo "THIRD-PARTY NOTICES - Super Mario All-Stars for Xbox 360 $ver"
  echo
  echo "SuperMarioAllStars.xex contains code and data from the projects below. Each"
  echo "notice is reproduced as its project ships it."
  section "Super Mario All-Stars PC port (Darhkfox), MIT License"
  cat "$game/LICENSE"
  section "xsdk, MIT License"
  cat "$root/xsdk-main/LICENSE"
  section "libxenon and Xenia code inside xsdk (3-clause BSD License)"
  sed -n '/^## In every title built with xsdk/,/^## In titles that draw text/p' "$root/xsdk-main/THIRD_PARTY_NOTICES.md" |
    sed '1d;$d' | grep -v '^```$'
  section "vulkan4xenon, MIT License"
  cat "$root/vulkan4xenon-main/LICENSE"
  echo
  echo "vulkan4xenon is built with the Khronos Group's Vulkan headers (Apache-2.0 or MIT)."
  section "SDL 3 (SDL-xenon), zlib License"
  cat "$root/SDL-xenon-xenon/LICENSE.txt"
  section "DejaVu Sans fonts (Bitstream Vera license)"
  cat "$game/third_party/fonts/DejaVu-LICENSE.txt"
  section "Luckiest Guy font, Apache License 2.0"
  cat "$game/third_party/fonts/LICENSE.txt"
  section "stb_truetype and stb_image (Sean Barrett)"
  echo "Public domain (or MIT, at your choice): http://nothings.org/stb"
  section "Compiler runtime and C library"
  echo "libgcc and libstdc++ (GCC) are used under the GCC Runtime Library Exception."
  echo "The C library is newlib, under the permissive licenses listed in its COPYING.NEWLIB."
} | sed 's/\r$//; s/$/\r/' > "$out/THIRD_PARTY_NOTICES.txt"

(cd "$out" && sha1sum SuperMarioAllStars.xex > SHA1SUMS.txt)
(cd "$out/.." && python3 -c "import shutil,sys; shutil.make_archive(sys.argv[1], 'zip', '.', sys.argv[1])" "$(basename "$out")")
ls -la "$out" "$out.zip"
cat "$out/SHA1SUMS.txt"
