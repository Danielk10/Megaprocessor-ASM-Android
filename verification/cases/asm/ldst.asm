; LD./ST. coverage across all encodeLoadStore modes
ORG 0x0400
LD. R0, #0x1234
LD.B R1, #0x56
LD. R0, 0x2000
LD.B R1, 0x2001
LD. R0, (R2)
LD.B R1, (R3)
LD. R0, (R2++)
LD.B R1, (R3++)
LD. R0, (SP+4)
LD.B R1, (SP+5)
ST. #0x2222, R0
ST.B #0x77, R1
ST. 0x2100, R0
ST.B 0x2101, R1
ST. (R2), R0
ST.B (R3), R1
ST. (R2++), R0
ST.B (R3++), R1
ST. (SP+6), R0
ST.B (SP+7), R1
