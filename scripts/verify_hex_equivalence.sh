#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXAMPLES=(life snail tetris tic_tac_toe_2 opcodes Megaprocessor_defs)

if ! command -v g++ >/dev/null 2>&1; then
  echo "FAIL"
  echo "Diagnóstico: no se encontró g++ en PATH." >&2
  exit 1
fi

for name in "${EXAMPLES[@]}"; do
  if [[ ! -f "${ROOT_DIR}/${name}.asm" || ! -f "${ROOT_DIR}/${name}.hex" ]]; then
    echo "FAIL"
    echo "Diagnóstico: faltan archivos de entrada para ${name}." >&2
    exit 1
  fi
done

workdir="$(mktemp -d)"
cleanup() { rm -rf "${workdir}"; }
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
#include <string>

#include "assembler.h"

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("No se pudo abrir: " + path);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Uso: " << argv[0] << " <source.asm> <include.asm> <out.hex> <name>\n";
        return 2;
    }

    try {
        Assembler assembler;
        std::map<std::string, std::string> includes;
        includes["MEGAPROCESSOR_DEFS.ASM"] = readFile(argv[2]);
        assembler.setIncludeFiles(includes);

        const std::string result = assembler.assemble(readFile(argv[1]));
        if (result.rfind("ERROR:", 0) == 0) {
            std::cerr << argv[4] << ": " << result << "\n";
            return 1;
        }

        std::ofstream out(argv[3], std::ios::binary);
        if (!out) {
            std::cerr << "No se pudo escribir salida HEX: " << argv[3] << "\n";
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
g++ -std=c++17 -O2 -I"${workdir}" -I"${ROOT_DIR}/app/src/main/cpp" \
  "${workdir}/assembler_cli.cpp" \
  "${ROOT_DIR}/app/src/main/cpp/assembler.cpp" \
  "${ROOT_DIR}/app/src/main/cpp/utils.cpp" \
  -o "${cli_bin}"

normalize_hex() {
  sed 's/\r$//' "$1" | awk '{ gsub(/[[:space:]]+$/, "", $0); print toupper($0); }' > "$2"
}

for name in "${EXAMPLES[@]}"; do
  generated="${workdir}/${name}.generated.hex"
  expected="${ROOT_DIR}/${name}.hex"
  normalized_expected="${workdir}/${name}.expected.norm"
  normalized_generated="${workdir}/${name}.generated.norm"

  "${cli_bin}" "${ROOT_DIR}/${name}.asm" "${ROOT_DIR}/Megaprocessor_defs.asm" "${generated}" "${name}"
  normalize_hex "${expected}" "${normalized_expected}"
  normalize_hex "${generated}" "${normalized_generated}"

  if ! cmp -s "${normalized_expected}" "${normalized_generated}"; then
    first_diff_line="$(awk 'NR==FNR{a[NR]=$0; max=NR; next} {b[NR]=$0; if (NR>max) max=NR} END {for (i=1;i<=max;i++) if (a[i] != b[i]) {print i; exit}}' "${normalized_expected}" "${normalized_generated}")"
    expected_line="$(sed -n "${first_diff_line}p" "${normalized_expected}")"
    generated_line="$(sed -n "${first_diff_line}p" "${normalized_generated}")"
    echo "FAIL"
    echo "Diagnóstico: '${name}' difiere de la referencia en línea ${first_diff_line}."
    echo "- Esperado: ${expected_line:-<EOF>}"
    echo "- Generado: ${generated_line:-<EOF>}"
    echo "- diff -u (normalizado):"
    diff -u "${normalized_expected}" "${normalized_generated}" || true
    exit 1
  fi

done

echo "PASS"
