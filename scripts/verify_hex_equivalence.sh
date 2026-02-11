#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ASM_FILE="${ROOT_DIR}/tic_tac_toe_2.asm"
INCLUDE_FILE="${ROOT_DIR}/Megaprocessor_defs.asm"
REFERENCE_HEX="${ROOT_DIR}/tic_tac_toe_2.hex"

if ! command -v g++ >/dev/null 2>&1; then
  echo "FAIL"
  echo "Diagnóstico: no se encontró g++ en PATH." >&2
  exit 1
fi

if [[ ! -f "${ASM_FILE}" || ! -f "${INCLUDE_FILE}" || ! -f "${REFERENCE_HEX}" ]]; then
  echo "FAIL"
  echo "Diagnóstico: faltan archivos de entrada requeridos (.asm/.hex/.asm include)." >&2
  exit 1
fi

workdir="$(mktemp -d)"
cleanup() {
  rm -rf "${workdir}"
}
trap cleanup EXIT

mkdir -p "${workdir}/android"
cat > "${workdir}/android/log.h" <<'STUB'
#pragma once
#include <cstdarg>
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_ERROR 6
inline int __android_log_print(int, const char*, const char*, ...) { return 0; }
STUB

cat > "${workdir}/assembler_cli.cpp" <<'CPP'
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

#include "assembler.h"

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("No se pudo abrir: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <source.asm> <include.asm> <out.hex>\n";
        return 2;
    }

    try {
        const std::string asmPath = argv[1];
        const std::string includePath = argv[2];
        const std::string outPath = argv[3];

        Assembler assembler;
        std::map<std::string, std::string> includes;
        includes["MEGAPROCESSOR_DEFS.ASM"] = readFile(includePath);
        assembler.setIncludeFiles(includes);

        const std::string result = assembler.assemble(readFile(asmPath));
        if (result.rfind("ERROR:", 0) == 0) {
            std::cerr << result << "\n";
            return 1;
        }

        std::ofstream out(outPath, std::ios::binary);
        if (!out) {
            std::cerr << "No se pudo escribir salida HEX: " << outPath << "\n";
            return 1;
        }
        out << result;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 1;
    }
}
CPP

cli_bin="${workdir}/assembler_cli"
assembled_hex="${workdir}/tic_tac_toe_2.generated.hex"
normalized_expected="${workdir}/expected.normalized.hex"
normalized_generated="${workdir}/generated.normalized.hex"

g++ -std=c++17 -O2 -I"${workdir}" -I"${ROOT_DIR}/app/src/main/cpp" \
  "${workdir}/assembler_cli.cpp" \
  "${ROOT_DIR}/app/src/main/cpp/assembler.cpp" \
  "${ROOT_DIR}/app/src/main/cpp/utils.cpp" \
  -o "${cli_bin}"

"${cli_bin}" "${ASM_FILE}" "${INCLUDE_FILE}" "${assembled_hex}"

normalize_hex() {
  local input_file="$1"
  local output_file="$2"
  sed 's/\r$//' "${input_file}" | awk '{ gsub(/[[:space:]]+$/, "", $0); print toupper($0); }' > "${output_file}"
}

normalize_hex "${REFERENCE_HEX}" "${normalized_expected}"
normalize_hex "${assembled_hex}" "${normalized_generated}"

if cmp -s "${normalized_expected}" "${normalized_generated}"; then
  echo "PASS"
  exit 0
fi

first_diff_line="$(awk 'NR==FNR{a[NR]=$0; max=NR; next} {b[NR]=$0; if (NR>max) max=NR} END {for (i=1;i<=max;i++) if (a[i] != b[i]) {print i; exit}}' "${normalized_expected}" "${normalized_generated}")"
expected_line="$(sed -n "${first_diff_line}p" "${normalized_expected}")"
generated_line="$(sed -n "${first_diff_line}p" "${normalized_generated}")"

extract_intel_hex_address() {
  local line="$1"
  if [[ "${line}" =~ ^:[0-9A-F]{8} ]]; then
    printf '0x%s' "${line:3:4}"
  else
    printf 'N/A'
  fi
}

record_address="$(extract_intel_hex_address "${generated_line}")"
if [[ "${record_address}" == "N/A" ]]; then
  record_address="$(extract_intel_hex_address "${expected_line}")"
fi

echo "FAIL"
echo "Diagnóstico: diferencia detectada en la primera línea distinta."
echo "- Línea: ${first_diff_line}"
echo "- Dirección Intel HEX (registro): ${record_address}"
echo "- Esperado: ${expected_line:-<EOF>}"
echo "- Generado: ${generated_line:-<EOF>}"
echo "- diff -u (normalizado):"
diff -u "${normalized_expected}" "${normalized_generated}" || true

exit 1
