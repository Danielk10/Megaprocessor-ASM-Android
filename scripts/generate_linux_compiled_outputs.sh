#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/linux_cpp_compiled_outputs"
GEN_DIR="${OUT_DIR}/generated"
REF_DIR="${OUT_DIR}/reference"
REPORT_FILE="${OUT_DIR}/REPORT.md"
VERSION_FILE="${OUT_DIR}/VERSION.txt"

EXAMPLES=(Megaprocessor_defs life snail tetris tic_tac_toe_2 opcodes)

cmake -S "${ROOT_DIR}/tools/assembler-cli" -B "${ROOT_DIR}/build/assembler-cli" >/dev/null
cmake --build "${ROOT_DIR}/build/assembler-cli" -j4 >/dev/null
CLI="${ROOT_DIR}/build/assembler-cli/assembler-cli"

rm -rf "${OUT_DIR}"
mkdir -p "${GEN_DIR}" "${REF_DIR}"

git_rev="$(git -C "${ROOT_DIR}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
now_utc="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
cat > "${VERSION_FILE}" <<TXT
Linux C++ assembler output bundle
Generated at (UTC): ${now_utc}
Repository revision: ${git_rev}
Assembler CLI: tools/assembler-cli
TXT

{
  echo "# Linux C++ compiled outputs vs reference"
  echo
  echo "| ASM | HEX byte-identical | LST byte-identical | Generated HEX bytes | Reference HEX bytes | Generated LST bytes | Reference LST bytes |"
  echo "|---|---:|---:|---:|---:|---:|---:|"
} > "${REPORT_FILE}"

for name in "${EXAMPLES[@]}"; do
  asm="${ROOT_DIR}/${name}.asm"
  ref_hex="${ROOT_DIR}/${name}.hex"
  ref_lst="${ROOT_DIR}/${name}.lst"

  gen_hex="${GEN_DIR}/${name}.hex"
  gen_lst="${GEN_DIR}/${name}.lst"

  cp "${ref_hex}" "${REF_DIR}/${name}.hex"
  cp "${ref_lst}" "${REF_DIR}/${name}.lst"

  "${CLI}" "${asm}" --out "${gen_hex}" --lst --lst-out "${gen_lst}" >/dev/null

  gen_hex_bytes=$(wc -c < "${gen_hex}")
  ref_hex_bytes=$(wc -c < "${ref_hex}")
  gen_lst_bytes=$(wc -c < "${gen_lst}")
  ref_lst_bytes=$(wc -c < "${ref_lst}")

  hex_equal="NO"
  lst_equal="NO"
  if cmp -s "${gen_hex}" "${ref_hex}"; then hex_equal="YES"; fi
  if cmp -s "${gen_lst}" "${ref_lst}"; then lst_equal="YES"; fi

  echo "| ${name}.asm | ${hex_equal} | ${lst_equal} | ${gen_hex_bytes} | ${ref_hex_bytes} | ${gen_lst_bytes} | ${ref_lst_bytes} |" >> "${REPORT_FILE}"

  # Keep first diff snippets for review.
  if [[ "${hex_equal}" == "NO" ]]; then
    diff -u "${ref_hex}" "${gen_hex}" > "${OUT_DIR}/${name}.hex.diff" || true
  fi
  if [[ "${lst_equal}" == "NO" ]]; then
    diff -u "${ref_lst}" "${gen_lst}" > "${OUT_DIR}/${name}.lst.diff" || true
  fi

done

echo "Generated: ${OUT_DIR}"
