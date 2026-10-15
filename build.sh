#!/usr/bin/env bash
# Build official WLED with this usermod for the QuinLED boards.
# Usage: ./build.sh <wled-tag> [--local]
#   --local  use this checkout instead of the pinned git tag of the usermod
set -euo pipefail
TAG=${1:?usage: build.sh <wled-tag, e.g. v16.0.1> [--local]}
HERE=$(cd "$(dirname "$0")" && pwd)
WORK=${WORK:-$HERE/.build}
SRC=$WORK/wled-$TAG
OUT=$WORK/out-$TAG

[ -d "$SRC" ] || git clone --quiet --depth 1 --branch "$TAG" https://github.com/wled/WLED.git "$SRC"
cp "$HERE/platformio_override.ini" "$SRC/"
if [ "${2:-}" = "--local" ]; then
  sed -i "s|^usermod = .*|usermod = symlink://$HERE|" "$SRC/platformio_override.ini"
fi

cd "$SRC"
npm ci --silent
pio run

mkdir -p "$OUT"
for env in $(sed -n '/^default_envs/,/^$/p' platformio_override.ini | tail -n +2); do
  cp ".pio/build/$env/firmware.bin" "$OUT/WLED_${TAG#v}_${env}.bin"
done
ls -l "$OUT"
