# Verification results

| Case | Status | Notes |
|---|---|---|
| alu | ✅ PASS | Output matches expected |
| branches | ✅ PASS | Output matches expected |
| directives | ✅ PASS | Output matches expected |
| jumps | ✅ PASS | Output matches expected |
| ldst | ✅ PASS | Output matches expected |
| opcodes | ✅ PASS | Output matches expected |

## Implementation gaps detected

- `JMP (Rx)` y `JSR (Rx)` aceptan cualquier operando indirecto, pero la codificación no usa el registro indicado: siempre emiten opcode fijo (`0xF2`/`0xCE`).
- `encodeLoadStore` trata `ST. #imm, Rn` como válido (ruta de inmediato), aunque conceptualmente no es una forma típica de store.
- En modos `(...)` y `(...++)` de `encodeLoadStore`, solo se distinguen punteros `R2`/`R3` y datos `R0`/`R1`; otros registros pasan parsing pero no tienen mapeo explícito de encoding.
- Pass1 detecta branches por regla general (`mnemonic.length()==3 && mnemonic[0]=='B'`), lo que puede sobredimensionar instrucciones con `B??` no válidas.
