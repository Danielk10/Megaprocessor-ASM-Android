#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CASES_DIR="$ROOT_DIR/verification/cases/asm"
INCLUDES_DIR="$CASES_DIR/includes"
EXPECTED_DIR="$ROOT_DIR/verification/cases/expected"
ACTUAL_DIR="$ROOT_DIR/verification/cases/actual"
RESULTS_FILE="$ROOT_DIR/verification/results.md"
RUNNER="/tmp/assemble_cli_megaprocessor"

mkdir -p "$ACTUAL_DIR"

c++ -std=c++17 -I"$ROOT_DIR/verification/tools" \
  "$ROOT_DIR/verification/tools/assemble_cli.cpp" \
  "$ROOT_DIR/app/src/main/cpp/assembler.cpp" \
  "$ROOT_DIR/app/src/main/cpp/utils.cpp" \
  -o "$RUNNER"

{
  echo "# Verification results"
  echo
  echo "| Case | Status | Notes |"
  echo "|---|---|---|"
} > "$RESULTS_FILE"

status_code=0
for asm_file in "$CASES_DIR"/*.asm; do
  case_name="$(basename "$asm_file" .asm)"
  expected_file="$EXPECTED_DIR/$case_name.hex"
  actual_file="$ACTUAL_DIR/$case_name.hex"

  if "$RUNNER" "$asm_file" "$INCLUDES_DIR" > "$actual_file"; then
    if [[ -f "$expected_file" ]]; then
      expected_norm="/tmp/${case_name}_expected.norm.hex"
      actual_norm="/tmp/${case_name}_actual.norm.hex"
      sed 's/\r$//' "$expected_file" > "$expected_norm"
      sed 's/\r$//' "$actual_file" > "$actual_norm"

      if diff -u "$expected_norm" "$actual_norm" > /tmp/${case_name}_hex.diff; then
        echo "| $case_name | ✅ PASS | Output matches expected |" >> "$RESULTS_FILE"
      else
        echo "| $case_name | ❌ FAIL | Output differs from expected (`diff -u verification/cases/expected/$case_name.hex verification/cases/actual/$case_name.hex`) |" >> "$RESULTS_FILE"
        status_code=1
      fi
    else
      echo "| $case_name | ⚠️ WARN | Missing expected file: verification/cases/expected/$case_name.hex |" >> "$RESULTS_FILE"
      status_code=1
    fi
  else
    echo "| $case_name | ❌ FAIL | Assembler returned error |" >> "$RESULTS_FILE"
    status_code=1
  fi
done

cat >> "$RESULTS_FILE" <<'GAPS'

## Implementation gaps detected

- `JMP (Rx)` y `JSR (Rx)` aceptan cualquier operando indirecto, pero la codificación no usa el registro indicado: siempre emiten opcode fijo (`0xF2`/`0xCE`).
- `encodeLoadStore` trata `ST. #imm, Rn` como válido (ruta de inmediato), aunque conceptualmente no es una forma típica de store.
- En modos `(...)` y `(...++)` de `encodeLoadStore`, solo se distinguen punteros `R2`/`R3` y datos `R0`/`R1`; otros registros pasan parsing pero no tienen mapeo explícito de encoding.
- Pass1 detecta branches por regla general (`mnemonic.length()==3 && mnemonic[0]=='B'`), lo que puede sobredimensionar instrucciones con `B??` no válidas.
GAPS

exit "$status_code"
