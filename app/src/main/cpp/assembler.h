#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <sstream>

enum SymbolType {
    LABEL,
    CONSTANT
};

struct Symbol {
    std::string name;
    int32_t value;
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
    std::string assemble(const std::string& sourceCode);
    std::string getListing() const;
    void setIncludeFiles(const std::map<std::string, std::string>& includeFiles);

private:
    std::map<std::string, Symbol> symbolTable;
    std::vector<Instruction> instructions;
    uint16_t currentAddress;
    std::string listingOutput;
    std::map<std::string, std::string> includeFileContents;
    std::string expressionError;

    bool pass1(const std::vector<std::string>& lines, std::string& error);
    bool pass2(const std::vector<std::string>& lines, std::string& error);

    std::vector<std::string> preprocessIncludes(const std::vector<std::string>& lines, std::string& error,
                                                std::vector<std::string>& includeStack) const;
    std::string normalizeIncludeName(const std::string& includeToken) const;

    int parseRegister(const std::string& token);
    uint8_t getALUOpcode(const std::string& mnemonic, int ra, int rb);

    void encodeLoadStore(const std::string& mnemonic, const std::string& op1, const std::string& op2,
                         std::vector<uint8_t>& bytes, int lineNum, std::string& error);

    bool evaluateExpression(const std::string& expr, int32_t& result);
    int32_t parseExpression(const char*& p);
    int32_t parseTerm(const char*& p);
    int32_t parseFactor(const char*& p);

    std::map<std::string, uint8_t> opcodeMap;
    void initOpcodes();

    void generateListingLine(const Instruction& inst);
};

#endif // ASSEMBLER_H
