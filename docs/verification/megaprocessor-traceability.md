# Trazabilidad Megaprocessor (documentación oficial ↔ ensamblador Android)

## Alcance y método

- **Fuentes oficiales revisadas**:
  - http://www.megaprocessor.com/architecture.html
  - http://www.megaprocessor.com/arch_fsm.html
  - http://www.megaprocessor.com/arch_status.html
  - http://www.megaprocessor.com/arch_alu.html
  - http://www.megaprocessor.com/arch_iterative.html
  - http://www.megaprocessor.com/index.html
- **Código contrastado**: `app/src/main/cpp/assembler.cpp` (funciones clave: `initOpcodes`, `getALUOpcode`, `encodeLoadStore`, `pass1`, `pass2`).
- **Criterio de estado**:
  - **Implementado**: existe soporte de ensamblado y codificación coherente para el punto.
  - **Parcial**: existe soporte incompleto, simplificado o con diferencias semánticas.
  - **No implementado**: no hay soporte observable en el ensamblador para el punto documentado.

> Nota: `architecture.html` delega el detalle completo de instrucciones al PDF de instruction set. Esta trazabilidad cubre las reglas **explícitas en las páginas HTML** y los mnemónicos/opcodes observables en `initOpcodes`.

---

## `architecture.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| Arquitectura load/store (ALU solo entre registros) | Operaciones ALU registro-registro | `getALUOpcode` solo codifica pares de registros para `MOVE/AND/XOR/OR/ADD/SUB/CMP`; `pass2` valida registros con `parseRegister` | Implementado | `ADD R0,R1` | La regla se cumple para ALU principal; existen formas inmediatas (`ADDI/ANDI/ORI`) como opcodes separados. |
| Instrucciones con opcode de 8 bits + 0..2 bytes inmediatos | Tamaño variable por instrucción | `pass1` calcula tamaños (ramas 2 bytes, `JMP/JSR` directo 3 bytes, indirecto 1 byte, LD/ST según modo); `pass2` emite bytes reales | Implementado | `JMP 0x1234` | El cálculo de tamaño en `pass1` no modela ciclos, solo longitud de código. |
| Saltos condicionales (familia Bxx) | Branch condicional/relativo | `initOpcodes` define `BCC..BLS`; `pass2` calcula offset relativo (`target-(PC+2)`) y valida rango [-128,127] | Implementado | `BEQ etiqueta` | Semántica de condición depende de flags en CPU real; el ensamblador solo codifica. |
| Salto/subrutina directo e indirecto | `JMP/JSR` modos directo e indirecto | `pass2` distingue operando con `(` para usar opcodes cortos (`JMP` indirecto `0xF2`, `JSR` indirecto `0xCE`) o directo 16 bits (`0xF3/0xCD`) | Implementado | `JSR rutina` y `JMP (R2)` | Detección de indirecto por presencia de `(`; no valida profundamente sintaxis de direccionamiento. |
| Stack/subrutina | `PUSH/POP/RET/RETI` | `initOpcodes` contiene `RET/RETI`; `pass2` codifica `PUSH/POP` por registro (`0xC8/0xC0 + reg`) | Implementado | `PUSH R0` / `POP R0` / `RET` | `TRAP` comparte opcode con `JSR` en `initOpcodes` (`0xCD`), ambigüedad semántica relevante. |
| Excepciones e interrupciones | Vectores/flujo de excepción | Solo presencia de `RETI`/`TRAP` en mapa de opcodes; no hay modelado de vectores, tabla de excepciones ni validaciones específicas | Parcial | `RETI` | El ensamblador no simula arquitectura de excepción; solo emite opcodes. |
| Pipeline y tiempos por byte/ciclo | Modelo temporal | No hay lógica temporal/ciclos en ensamblador (solo parseo y codificación) | No implementado | `NOP` | Fuera de alcance natural de un ensamblador estático. |
| Opcode no usado mapeado a NOP (0xC5 según doc) | Compatibilidad opcode reservado | `initOpcodes` define `NOP=0xFF`; no se expone mnemónico para `0xC5` | Parcial | `NOP` | Posible diferencia entre documentación histórica y mapa efectivo en este proyecto. |

### Diferencias semánticas y ambigüedades detectadas (`architecture.html`)

- La página indica que “all 256 opcodes are used”, pero en el ensamblador no están todos los mnemónicos documentados por texto; hay cobertura focalizada en subconjunto operativo.
- `TRAP` y `JSR` comparten opcode (`0xCD`) en `initOpcodes`; sin metadato adicional, la intención semántica de `TRAP` queda ambigua.
- La documentación general menciona comportamiento microarquitectural (pipeline/latencias) que el ensamblador no pretende validar.

---

## `arch_fsm.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| Máquina de estados de 16 estados | Control secuencial de ejecución | No existe representación explícita de estados en ensamblador | No implementado | `ADD R0,R1` | El comportamiento FSM es responsabilidad de CPU/simulador, no del ensamblador. |
| Estado 0: ejecución simple + fetch concurrente | Ciclo base instrucción simple | Sin modelado temporal; `pass2` solo emite bytes | No implementado | `NOP` | No se puede verificar fetch/solapamiento desde ensamblado. |
| Estado 1: carga de MSB en inmediatos de 16 bits | Inmediato de 16 bits en segundo byte de dato | `pass1`/`pass2` emiten instrucciones de 3 bytes cuando aplica (`JMP`, `LD.W #imm`) | Parcial | `LD.W R0,#0x1234` | Se valida longitud/bytes, no transición FSM. |
| Estados de load/store (lectura/escritura por bytes) | Acceso memoria secuencial en bus de 8 bits | `encodeLoadStore` distingue modos (`(R2)`, `(R2++)`, `(SP+n)`, absoluto, inmediato) y tamaños byte/word | Parcial | `ST.W 0x2000,R0` | Se codifica direccionamiento, no secuencia de estados/ciclos. |
| Estados iterativos (SHIFT/MUL/DIV/SQRT) | Iteración multi-ciclo | `MULU/MULS/DIVU/DIVS/SQRT` existen en `initOpcodes`; no hay soporte explícito para mnemónico `SHIFT` | Parcial | `MULU` | Documentación FSM menciona `SHIFT` multiestado, pero no se observa codificación dedicada de `SHIFT`. |

### Diferencias semánticas y ambigüedades detectadas (`arch_fsm.html`)

- La documentación describe **cómo** ejecuta el hardware cada instrucción; el ensamblador sólo define **qué bytes** se emiten.
- Hay una brecha explícita en `SHIFT`: aparece en FSM/arquitectura, pero no en las ramas de parsing de `pass2`.

---

## `arch_status.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| Registro PS con bits I,N,Z,V,X,C,... | Estado de flags del procesador | `parseRegister` reconoce `PS`; branches de `initOpcodes` (`BCC/BCS/BEQ/BNE/...`) dependen de flags | Parcial | `BNE destino` | El ensamblador no evalúa flags; solo codifica instrucciones que los consumen/producen en CPU. |
| Condiciones de branch por flags | Branch condicional por estado PS | `initOpcodes` mapea familia completa de Bxx; `pass2` codifica salto relativo | Implementado | `BCS destino` | Semántica exacta de cada condición (ej. firmado/no firmado) no se valida en ensamblado. |
| Bit X para aritmética extendida | Aritmética con carry extendido | `initOpcodes` incluye `ADDX/SUBX/NEGX` | Implementado | `ADDX` | No hay verificación contextual del uso de X; emite opcode directo. |
| Habilitación de interrupciones (bit I) | Control de interrupciones | No se observan mnemónicos explícitos de set/clear de I en parser principal | No implementado | `; sin caso directo` | Podría existir en ISA completa (PDF), pero no está expuesto aquí. |

### Diferencias semánticas y ambigüedades detectadas (`arch_status.html`)

- El doc de PS describe efectos de ejecución (set/clear), pero el ensamblador no modela ni verifica transiciones de bits.
- La presencia de `PS` como registro parseable no implica cobertura completa de instrucciones de manipulación de estado.

---

## `arch_alu.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| ALU lógica: AND/OR/XOR | Operaciones lógicas RR | `getALUOpcode` genera base `0x10/0x30/0x20` + par de registros; `pass2` enruta mnemónicos | Implementado | `XOR R1,R1` | Cobertura explícita para modo registro-registro. |
| ALU aritmética: ADD/SUB/CMP | Operaciones aritméticas RR | `getALUOpcode` usa bases `0x40/0x60/0x70` | Implementado | `CMP R0,R1` | `CMP` codifica opcode; semántica de flags la resuelve CPU. |
| MOVE entre registros | Transferencia RR | `getALUOpcode("MOVE",...)` base `0x00` | Implementado | `MOVE R2,R0` | N/A |
| Constantes pequeñas locales (0, ±1, ±2) | Incremento/decremento/quick-add | `pass2` implementa `INC`, `DEC`, `ADDQ` (solo `#1/#2/#-1/#-2`), `CLR` como `XOR r,r` | Implementado | `ADDQ R0,#2` | Restricción fuerte en `ADDQ` (solo cuatro valores). |
| Shift izquierdo/derecho en ALU1 | Operaciones SHIFT | No aparece rama de parsing para `SHIFT` en `pass2` | No implementado | `SHIFT R0,#1` | Gap explícito con lo descrito en página ALU/FSM. |
| ALU en cálculos de dirección load/store | Effective address | `encodeLoadStore` calcula modos directos, indirectos, post-incremento y `SP+offset` | Implementado | `LD.W R0,(SP+4)` | Cálculo semántico exacto de EA en runtime no se verifica. |

### Diferencias semánticas y ambigüedades detectadas (`arch_alu.html`)

- La página ALU mezcla bloques hardware (ALU1/ADDER2/bit-test); el ensamblador sólo refleja mnemónicos visibles de ISA.
- No hay trazabilidad directa para funciones internas de hardware (bit-test dedicado) salvo instrucciones consumidoras.

---

## `arch_iterative.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| Multiplicación no signada iterativa | `MULU` | `initOpcodes["MULU"]=0xF8`; `pass2` emite opcode vía `opcodeMap` | Implementado | `MULU` | No se valida precondición de registros implícitos (R0/R1, etc.). |
| Multiplicación signada iterativa | `MULS` | `initOpcodes["MULS"]=0xF9` | Implementado | `MULS` | Semántica de signo se delega al hardware. |
| División no signada iterativa | `DIVU` | `initOpcodes["DIVU"]=0xFA` | Implementado | `DIVU` | No se detecta división por cero en ensamblado (sólo en evaluación de expresiones host). |
| División signada iterativa | `DIVS` | `initOpcodes["DIVS"]=0xFB` | Implementado | `DIVS` | Ajustes de signo/estado descritos por hardware no se modelan. |
| Raíz cuadrada iterativa | `SQRT` | `initOpcodes["SQRT"]=0xF7` | Implementado | `SQRT` | No se valida dominio de entrada en ensamblador. |

### Diferencias semánticas y ambigüedades detectadas (`arch_iterative.html`)

- La documentación describe algoritmo interno por iteraciones/estados; el ensamblador sólo necesita emitir el opcode de la instrucción macro.
- Posibles convenciones de registros implícitos no están documentadas/validadas en `pass2`.

---

## `index.html`

| Regla/instrucción documentada | Nombre funcional | Evidencia en código (`assembler.cpp`) | Estado | Caso de prueba mínimo ASM | Diferencias semánticas / ambigüedades |
|---|---|---|---|---|---|
| Presentación general del Megaprocessor (16-bit, proyecto educativo) | Contexto arquitectónico | No define reglas de codificación concretas; referencia a secciones técnicas externas | Parcial | `NOP` | `index.html` no es una especificación formal de ISA, solo contexto de alto nivel. |
| Navegación hacia páginas de arquitectura/programación | Trazabilidad documental | Este documento enlaza secciones técnicas contra funciones del ensamblador | Implementado | `; N/A` | Ambigüedad inevitable por falta de detalle técnico en portada. |

### Diferencias semánticas y ambigüedades detectadas (`index.html`)

- La portada no introduce reglas ensamblables detalladas; toda validación concreta debe venir de páginas técnicas y/o instruction set PDF.

---

## Resumen de brechas principales

1. **Brecha de modelado temporal/FSM**: No hay simulación de estados/ciclos (esperable para ensamblador).
2. **Brecha de ISA visible**: Algunos elementos mencionados por arquitectura/FSM (por ejemplo `SHIFT`) no aparecen en `pass2`.
3. **Ambigüedad `TRAP` vs `JSR`**: ambos mapeados a `0xCD` en `initOpcodes`.
4. **Flags/PS**: cobertura de codificación amplia (branches y extended arithmetic), pero sin validación semántica dinámica.

## Backlog sugerido para cerrar trazabilidad

- Añadir soporte explícito de mnemónicos faltantes de la ISA documentada (`SHIFT`, y otros del instruction set PDF si aplican).
- Separar semánticamente `TRAP` y `JSR` si la ISA oficial define comportamiento distinto con mismo opcode/contexto.
- Incorporar en `docs/verification` una matriz adicional contra `instruction_set.pdf` para cobertura completa de los 256 opcodes.
