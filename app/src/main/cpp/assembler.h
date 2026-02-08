#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Estructuras de datos
// Enum for symbol types
enum SymbolType {
    LABEL,
    CONSTANT // For EQU
};

struct Symbol {
    std::string name;
    int32_t value; // Changed to int32 for calculations
    SymbolType type;
    bool isDefined;
};

struct Instruction {
    uint16_t address;
    std::vector<uint8_t> bytes;
    std::string originalLine;
    int lineNumber;
    std::string errorMessage;
    bool isDirective; 
};

class Assembler {
public:
    Assembler();
    
    // Método principal que recibe el código fuente y devuelve el resultado HEX
    std::string assemble(const std::string& sourceCode);
    
    // Devuelve el listing generado
    std::string getListing() const;

private:
    std::map<std::string, Symbol> symbolTable;
    std::vector<Instruction> instructions;
    uint16_t currentAddress;
    std::string listingOutput;
    
    // Pasada 1: Definición de símbolos
    bool pass1(const std::vector<std::string>& lines, std::string& error);
    
    // Pasada 2: Generación de código
    bool pass2(const std::vector<std::string>& lines, std::string& error);
    
    // Utilitarios
    int parseRegister(const std::string& token);
    
    // Evaluador de expresiones
    bool evaluateExpression(const std::string& expr, int32_t& result);
    // Helpers recursivos para parsing
    int32_t parseExpression(const char*& p);
    int32_t parseTerm(const char*& p);
    int32_t parseFactor(const char*& p);
    
    // Mapa de opcodes
    std::map<std::string, uint8_t> opcodeMap;
    void initOpcodes();
    
    void generateListingLine(const Instruction& inst);
};

#endif // ASSEMBLER_H
