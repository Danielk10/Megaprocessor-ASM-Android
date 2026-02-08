#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Estructuras de datos
struct Symbol {
    std::string name;
    uint16_t address;
    bool isDefined;
};

struct Instruction {
    uint16_t address;
    std::vector<uint8_t> bytes;
    std::string originalLine;
    int lineNumber;
    std::string errorMessage;
};

class Assembler {
public:
    Assembler();
    
    // Método principal que recibe el código fuente y devuelve el resultado (hex o error)
    std::string assemble(const std::string& sourceCode);

private:
    std::map<std::string, Symbol> symbolTable;
    std::vector<Instruction> instructions;
    uint16_t currentAddress;
    
    // Pasada 1: Análisis de etiquetas y cálculo de direcciones
    bool pass1(const std::vector<std::string>& lines, std::string& error);
    
    // Pasada 2: Generación de código máquina
    bool pass2(const std::vector<std::string>& lines, std::string& error);
    
    // Utilitarios de ensamblaje
    int parseRegister(const std::string& token);
    bool parseOperand(const std::string& operand, int& value, int& type);
    
    // Mapa de opcodes (nombre -> opcode base)
    std::map<std::string, uint8_t> opcodeMap;
    
    void initOpcodes();
};

#endif // ASSEMBLER_H
