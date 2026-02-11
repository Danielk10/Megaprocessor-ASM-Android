; Direct and indirect JMP/JSR
ORG 0x0300
JMP target
JSR target
JMP (R2)
JSR (R3)
target:
NOP
RET
