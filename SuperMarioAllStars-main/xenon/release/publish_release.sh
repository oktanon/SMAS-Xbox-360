# Publica un release en GitHub con el .xex solo (hito 10). Uso, desde la raíz del repositorio, en Git Bash (necesita
# gh con la sesión iniciada):
#   bash SuperMarioAllStars-main/xenon/release/publish_release.sh <versión> [--publish]
# Antes: make_release.sh <versión> (el .xex en build-xenon/release/SuperMarioAllStars-x360-<versión>/) y las notas en
# inglés en xenon/release/notes/<versión>.md. Sin --publish queda como borrador, para revisarlo en GitHub y publicarlo
# desde ahí. La etiqueta v<versión> se crea en GitHub sobre el commit actual, que tiene que estar subido a origin/main.
# Un workflow de GitHub no puede compilar el .xex: el GCC de xsdk, xsdk, vulkan4xenon y SDL-xenon no están en el
# repositorio. Por eso el release se hace desde la máquina que compila.
set -eu
ver="${1:?uso: publish_release.sh <versión> [--publish]}"
draft="--draft"
[ "${2:-}" = "--publish" ] && draft=""
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
xex="$root/build-xenon/release/SuperMarioAllStars-x360-$ver/SuperMarioAllStars.xex"
notes="$here/notes/$ver.md"
[ -f "$xex" ] || { echo "falta el .xex del paquete: make_release.sh $ver" >&2; exit 1; }
[ -f "$notes" ] || { echo "faltan las notas: xenon/release/notes/$ver.md" >&2; exit 1; }
cd "$root"
git fetch -q origin
head="$(git rev-parse HEAD)"
[ "$head" = "$(git rev-parse origin/main)" ] || { echo "el commit actual no es origin/main: subirlo antes" >&2; exit 1; }
git diff --quiet HEAD -- || { echo "hay cambios sin commit" >&2; exit 1; }
if gh release view "v$ver" > /dev/null 2>&1; then echo "el release v$ver ya existe" >&2; exit 1; fi
sha="$(sha1sum "$xex" | cut -d' ' -f1)"
body="$(mktemp)"
{ cat "$notes"; printf '\n---\n\n`SuperMarioAllStars.xex` SHA-1: `%s`\n' "$sha"; } > "$body"
gh release create "v$ver" "$xex" --target "$head" --title "Super Mario All-Stars for Xbox 360 v$ver" \
  --notes-file "$body" $draft
rm -f "$body"
gh release view "v$ver" --json url,isDraft,assets --jq '.url, .isDraft, (.assets[].name)'
