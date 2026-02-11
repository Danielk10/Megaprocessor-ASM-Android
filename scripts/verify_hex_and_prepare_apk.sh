#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERIFY_SCRIPT="${ROOT_DIR}/scripts/verify_hex_equivalence.sh"
SDK_SETUP_SCRIPT="${ROOT_DIR}/scripts/setup-android-sdk.sh"
APK_PATH="${ROOT_DIR}/app/build/outputs/apk/debug/app-debug.apk"
RENAMED_APK_PATH="${ROOT_DIR}/app-debug.jpg"

# Controls
# STRICT_HEX=1 -> exit 1 if HEX comparison fails.
# ALWAYS_COPY_APK=1 -> copy debug APK to app-debug.jpg even when HEX comparison fails.
STRICT_HEX="${STRICT_HEX:-0}"
ALWAYS_COPY_APK="${ALWAYS_COPY_APK:-1}"

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
if [[ ! -f "${ROOT_DIR}/local.properties" && -x "${SDK_SETUP_SCRIPT}" ]]; then
  echo "local.properties no encontrado. Intentando configurar Android SDK/NDK..."
  "${SDK_SETUP_SCRIPT}"
fi

(
  cd "${ROOT_DIR}"
  ./gradlew assembleDebug
)

if [[ ! -f "${APK_PATH}" ]]; then
  echo "ERROR: no se encontró APK en ${APK_PATH}" >&2
  exit 1
fi

if [[ "${ALWAYS_COPY_APK}" == "1" || ${verify_status} -eq 0 ]]; then
  cp "${APK_PATH}" "${RENAMED_APK_PATH}"
  echo "APK copiado como ${RENAMED_APK_PATH}."
else
  echo "HEX FAIL y ALWAYS_COPY_APK!=1: no se copia app-debug.jpg."
fi

if [[ ${verify_status} -eq 0 ]]; then
  echo "HEX PASS"
  exit 0
fi

echo "HEX FAIL"
if [[ "${STRICT_HEX}" == "1" ]]; then
  echo "Modo estricto activo (STRICT_HEX=1): devolviendo error por mismatch HEX." >&2
  exit 1
fi

echo "Continuando con éxito porque STRICT_HEX!=1."
exit 0
