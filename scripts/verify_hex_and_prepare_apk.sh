#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERIFY_SCRIPT="${ROOT_DIR}/scripts/verify_hex_equivalence.sh"
APK_PATH="${ROOT_DIR}/app/build/outputs/apk/debug/app-debug.apk"
RENAMED_APK_PATH="${ROOT_DIR}/app-debug.jpg"

if [[ ! -x "${VERIFY_SCRIPT}" ]]; then
  echo "ERROR: no se encontró script ejecutable en ${VERIFY_SCRIPT}" >&2
  exit 1
fi

echo "==> Paso 1/2: verificando equivalencia HEX"
set +e
"${VERIFY_SCRIPT}"
verify_status=$?
set -e

echo "==> Paso 2/2: compilando APK debug"
(
  cd "${ROOT_DIR}"
  ./gradlew assembleDebug
)

if [[ ! -f "${APK_PATH}" ]]; then
  echo "ERROR: no se encontró APK en ${APK_PATH}" >&2
  exit 1
fi

cp "${APK_PATH}" "${RENAMED_APK_PATH}"
echo "APK copiado como ${RENAMED_APK_PATH} (independiente del resultado HEX)."

if [[ ${verify_status} -eq 0 ]]; then
  echo "HEX PASS"
else
  echo "HEX FAIL"
fi

exit ${verify_status}
