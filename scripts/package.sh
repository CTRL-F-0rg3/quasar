#!/usr/bin/env bash
set -e
VERSION="${1:-v0.1.0}"
REPO="$(cd "$(dirname "$0")/.." && pwd)"
NAME="quasar-linux-${VERSION}"
DIR="${REPO}/dist/${NAME}"

if [ ! -f "${REPO}/build/quasar" ]; then
  echo "Error: build/quasar not found. Build first."
  exit 1
fi
if [ ! -f "${REPO}/assets/quasar.svg" ] || [ ! -f "${REPO}/assets/quasar.desktop" ]; then
  echo "Error: missing assets"
  exit 1
fi

rm -rf "${DIR}"
mkdir -p "${DIR}/bin" "${DIR}/share/applications" "${DIR}/share/icons/hicolor/scalable/apps" "${DIR}/src"
cp "${REPO}/build/quasar" "${DIR}/bin/"
chmod +x "${DIR}/bin/quasar"
cp "${REPO}/assets/quasar.desktop" "${DIR}/share/applications/"
cp "${REPO}/assets/quasar.svg" "${DIR}/share/icons/hicolor/scalable/apps/"
cp -r "${REPO}/src/"* "${DIR}/src/"
cp "${REPO}/CMakeLists.txt" "${DIR}/src/"
cp "${REPO}/scripts/install.sh" "${DIR}/" 2>/dev/null || true
chmod +x "${DIR}/install.sh" 2>/dev/null || true
cd "${REPO}/dist" && tar -czf "${NAME}.tar.gz" "${NAME}"
echo "=== Packaged: dist/${NAME}.tar.gz ==="
ls -lh "${REPO}/dist/${NAME}.tar.gz"
