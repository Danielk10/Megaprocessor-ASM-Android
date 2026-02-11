# Verification cases

Este directorio contiene casos mínimos para validar el ensamblador.

## Estructura

- `cases/asm/`: fuentes `.asm` por categoría.
- `cases/asm/includes/`: includes usados por casos de directivas.
- `cases/expected/`: salida Intel HEX esperada por cada caso.
- `cases/actual/`: salida Intel HEX generada en la última ejecución.
- `run_cases.sh`: compila un runner host y ejecuta todos los casos.
- `results.md`: tabla consolidada de resultados y gaps detectados.

## Ejecución

```bash
verification/run_cases.sh
```

## Regenerar expected

Solo si hay un cambio intencional en el ensamblador:

```bash
verification/run_cases.sh
cp verification/cases/actual/*.hex verification/cases/expected/
verification/run_cases.sh
```
