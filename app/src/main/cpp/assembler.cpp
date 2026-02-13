#include "assembler.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

Assembler::Assembler() {
    initOpcodes();
}

void Assembler::setIncludeFiles(const std::map<std::string, std::string>& includeFiles) {
    includeFileContents.clear();
    for (const auto& entry : includeFiles) {
        includeFileContents[toUpper(trim(entry.first))] = entry.second;
    }
}

void Assembler::initOpcodes() {
    // Reference: http://www.megaprocessor.com/instruction_set.pdf

    // Subroutine & Stack
    opcodeMap["POP"] = 0xC0;
    opcodeMap["PUSH"] = 0xC8;
    opcodeMap["RET"] = 0xC6;
    opcodeMap["RETI"] = 0xC7;
    opcodeMap["JSR"] = 0xCF;
    opcodeMap["TRAP"] = 0xCD;

    // Branches
    opcodeMap["BUC"] = 0xE0;
    opcodeMap["BUS"] = 0xE1;
    opcodeMap["BHI"] = 0xE2;
    opcodeMap["BLS"] = 0xE3;
    opcodeMap["BCC"] = 0xE4;
    opcodeMap["BCS"] = 0xE5;
    opcodeMap["BNE"] = 0xE6;
    opcodeMap["BEQ"] = 0xE7;
    opcodeMap["BVC"] = 0xE8;
    opcodeMap["BVS"] = 0xE9;
    opcodeMap["BPL"] = 0xEA;
    opcodeMap["BMI"] = 0xEB;
    opcodeMap["BGE"] = 0xEC;
    opcodeMap["BLT"] = 0xED;
    opcodeMap["BGT"] = 0xEE;
    opcodeMap["BLE"] = 0xEF;

    // Misc
    opcodeMap["JMP"] = 0xF3;
    opcodeMap["ANDI"] = 0xF4;
    opcodeMap["ORI"] = 0xF5;
    opcodeMap["ADDI"] = 0xF6;
    opcodeMap["SQRT"] = 0xF7;
    opcodeMap["MULU"] = 0xF8;
    opcodeMap["MULS"] = 0xF9;
    opcodeMap["DIVU"] = 0xFA;
    opcodeMap["DIVS"] = 0xFB;
    opcodeMap["ADDX"] = 0xFC;
    opcodeMap["SUBX"] = 0xFD;
    opcodeMap["NEGX"] = 0xFE;
    opcodeMap["NOP"] = 0xFF;
}

int Assembler::parseRegister(const std::string& token) {
    std::string t = toUpper(trim(token));
    while (!t.empty() && (t.back() == ',' || t.back() == ';' || t.back() == ')' || t.back() == ' ')) t.pop_back();
    while (!t.empty() && (t.front() == '(' || t.front() == ' ')) t.erase(0, 1);
    t = trim(t);

    if (t == "R0") return 0;
    if (t == "R1") return 1;
    if (t == "R2") return 2;
    if (t == "R3") return 3;
    if (t == "SP") return 4;
    if (t == "PS") return 5;
    return -1;
}

uint8_t Assembler::getALUOpcode(const std::string& mnemonic, int ra, int rb) {
    if (ra < 0 || rb < 0 || ra > 3 || rb > 3) return 0xFF;

    const uint8_t regCode = static_cast<uint8_t>(rb * 4 + ra);

    if (mnemonic == "MOVE") return regCode;
    if (mnemonic == "AND" || mnemonic == "TEST") return 0x10 + regCode;
    if (mnemonic == "XOR" || mnemonic == "CLR")  return 0x20 + regCode;
    if (mnemonic == "OR"  || mnemonic == "INV")  return 0x30 + regCode;
    if (mnemonic == "ADD") return 0x40 + regCode;
    // Note: SUB and CMP use the same base for registers? 
    // opcodes.lst: sub r1,r0 -> 61. cmp r1,r0 -> 71.
    if (mnemonic == "SUB" || mnemonic == "NEG") return 0x60 + regCode;
    if (mnemonic == "CMP" || mnemonic == "ABS") return 0x70 + regCode;
    if (mnemonic == "SXT") return 0x00 + regCode; // SXT uses ra=rb in opcodes.lst (MOVE rx, rx)
    
    return 0xFF;
}

void Assembler::encodeALU(const std::string& mnemonic, const std::string& op1, const std::string& op2,
                           std::vector<uint8_t>& bytes, int lineNum, std::string& error) {
    int r1 = parseRegister(op1);
    
    if (mnemonic == "INC" || mnemonic == "DEC") {
        if (r1 < 0 || r1 > 3) { error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum); return; }
        bytes.push_back((mnemonic == "INC" ? 0x54 : 0x5C) + r1);
        return;
    }
    
    if (mnemonic == "ADDQ") {
        int r = r1;
        int32_t val;
        std::string valStr = op2;
        if (!valStr.empty() && valStr[0] == '#') valStr = valStr.substr(1);
        if (!evaluateExpression(valStr, val)) {
            error = "Invalid ADDQ value at line " + std::to_string(lineNum) + ": " + expressionError;
            return;
        }
        if (r < 0 || r > 3 || (val != 1 && val != 2 && val != -1 && val != -2)) {
            error = "ADDQ supports only #1/#2/#-1/#-2 for registers R0-R3 at line " + std::to_string(lineNum);
            return;
        }
        if (val ==  1) bytes.push_back(0x54 + r);
        else if (val ==  2) bytes.push_back(0x50 + r);
        else if (val == -1) bytes.push_back(0x5C + r);
        else if (val == -2) bytes.push_back(0x58 + r);
        return;
    }

    int r2 = parseRegister(op2);
    
    // Single operand ALU ops
    if (mnemonic == "SXT" || mnemonic == "ABS" || mnemonic == "INV" || mnemonic == "NEG" || mnemonic == "CLR" || mnemonic == "TEST") {
        if (r1 < 0 || r1 > 3) { error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum); return; }
        bytes.push_back(getALUOpcode(mnemonic, r1, r1));
        return;
    }

    // Two operand ALU ops
    if (r1 >= 0 && r2 >= 0) {
        if (mnemonic == "MOVE" && r1 == 4 && r2 == 0) { // move sp,r0
            bytes.push_back(0xF1);
            return;
        }
        if (mnemonic == "MOVE" && r1 == 0 && r2 == 4) { // move r0,sp
            bytes.push_back(0xF0);
            return;
        }
        
        uint8_t code = getALUOpcode(mnemonic, r1, r2);
        if (code != 0xFF) {
            bytes.push_back(code);
        } else {
            error = "Invalid operands or mnemonic " + mnemonic + " at line " + std::to_string(lineNum);
        }
    } else {
        error = "Invalid register(s) for " + mnemonic + " at line " + std::to_string(lineNum);
    }
}

void Assembler::encodeBitOp(const std::string& mnemonic, const std::string& op1, const std::string& op2,
                             std::vector<uint8_t>& bytes, int lineNum, std::string& error) {
    int r1 = parseRegister(op1);
    if (r1 < 0 || r1 > 3) {
        error = "Invalid destination register for bit operation at line " + std::to_string(lineNum);
        return;
    }

    uint8_t opType = 0; // BTST
    if (mnemonic == "BCHG") opType = 1;
    else if (mnemonic == "BCLR") opType = 2;
    else if (mnemonic == "BSET") opType = 3;

    int r2 = parseRegister(op2);
    uint8_t opByte = (opType << 6);
    
    if (r2 >= 0) {
        // Register mode
        opByte |= 0x20; // Bit 5
        opByte |= (r2 & 0x1F); // Actually only 0-5 likely valid
    } else {
        // Immediate mode
        std::string valStr = op2;
        if (!valStr.empty() && valStr[0] == '#') valStr = valStr.substr(1);
        int32_t bitNum;
        if (!evaluateExpression(valStr, bitNum)) {
            error = "Invalid bit number at line " + std::to_string(lineNum) + ": " + expressionError;
            return;
        }
        opByte |= (bitNum & 0x1F);
    }

    bytes.push_back(0xDC + r1);
    bytes.push_back(opByte);
}

bool Assembler::evaluateExpression(const std::string& expr, int32_t& result) {
    std::string cleanExpr = trim(expr);
    expressionError.clear();
    if (cleanExpr.empty()) {
        result = 0;
        return true;
    }
    if (cleanExpr.back() == ';') cleanExpr.pop_back();
    cleanExpr = trim(cleanExpr);

    try {
        std::string noSpaces;
        for (char c : cleanExpr) if (!isspace(c)) noSpaces += c;
        const char* p = noSpaces.c_str();
        result = parseExpression(p);
        if (*p != '\0') {
            expressionError = "Unexpected token in expression: " + std::string(p);
            return false;
        }
        return true;
    } catch (const std::exception& ex) {
        expressionError = ex.what();
        return false;
    } catch (...) {
        expressionError = "Unknown expression evaluation error";
        return false;
    }
}

int32_t Assembler::parseExpression(const char*& p) {
    int32_t x = parseTerm(p);
    while (*p == '+' || *p == '-') {
        char op = *p++;
        int32_t y = parseTerm(p);
        x = (op == '+') ? (x + y) : (x - y);
    }
    return x;
}

int32_t Assembler::parseTerm(const char*& p) {
    int32_t x = parseFactor(p);
    while (*p == '*' || *p == '/' || (*p == '<' && *(p+1) == '<') || (*p == '>' && *(p+1) == '>')) {
        if (*p == '<') {
            p += 2;
            x <<= parseFactor(p);
        } else if (*p == '>') {
            p += 2;
            x >>= parseFactor(p);
        } else if (*p == '*') {
            p++;
            x *= parseFactor(p);
        } else if (*p == '/') {
            p++;
            int32_t y = parseFactor(p);
            if (y == 0) throw std::runtime_error("Division by zero in expression");
            x /= y;
        }
    }
    return x;
}

int32_t Assembler::parseFactor(const char*& p) {
    while (isspace(*p)) p++;
    if (*p == '(') {
        p++;
        int32_t x = parseExpression(p);
        if (*p != ')') throw std::runtime_error("Missing ')' in expression");
        p++;
        return x;
    }
    if (*p == '+' || *p == '-') {
        char op = *p++;
        int32_t x = parseFactor(p);
        return (op == '+') ? x : -x;
    }
    if (*p == '$') {
        p++;
        return static_cast<int32_t>(currentAddress);
    }
    if (isdigit(*p)) {
        std::string numStr;
        if (*p == '0' && (toupper(*(p+1)) == 'X')) {
            p += 2;
            while (isxdigit(*p)) numStr += *p++;
            if (numStr.empty()) throw std::runtime_error("Invalid hex literal");
            // Use stoul to avoid overflow for values like 0xDEADBEEF
            return static_cast<int32_t>(std::stoul(numStr, nullptr, 16));
        }
        if (*p == '0' && (toupper(*(p+1)) == 'B')) {
            p += 2;
            while (*p == '0' || *p == '1') numStr += *p++;
            if (numStr.empty()) throw std::runtime_error("Invalid binary literal");
            return static_cast<int32_t>(std::stoul(numStr, nullptr, 2));
        }
        while (isdigit(*p)) numStr += *p++;
        return static_cast<int32_t>(std::stoul(numStr));
    }
    if (isalpha(*p) || *p == '_') {
        std::string symName;
        while (isalnum(*p) || *p == '_') symName += *p++;
        std::string key = toUpper(symName);
        auto it = symbolTable.find(key);
        if (it != symbolTable.end() && it->second.isDefined) {
            return it->second.value;
        }
        std::string msg = "Undefined symbol: " + symName;
        if (symbolTable.size() < 20) { // Small table, list symbols
             msg += " (Defined: ";
             for (auto const& [k, v] : symbolTable) msg += k + " ";
             msg += ")";
        }
        throw std::runtime_error(msg);
    }
    throw std::runtime_error("Unexpected character in expression");
}

std::string Assembler::normalizeIncludeName(const std::string& includeToken) const {
    std::string token = trim(includeToken);
    if (!token.empty() && token.back() == ';') token.pop_back();
    token = trim(token);
    if (token.size() >= 2 && ((token.front() == '"' && token.back() == '"') || (token.front() == '\'' && token.back() == '\''))) {
        token = token.substr(1, token.size() - 2);
    }
    return toUpper(trim(token));
}

std::vector<std::string> Assembler::preprocessIncludes(const std::vector<std::string>& rawLines, std::string& error,
                                                       std::vector<std::string>& includeStack) const {
    std::vector<std::string> lines = rawLines;
    
    // First, handle block comments /* ... */ across all lines in this file/buffer.
    // We MUST preserve newlines to keep line numbers accurate.
    std::string fullSource;
    for (const auto& line : lines) fullSource += line + "\n";
    
    std::string cleaned;
    bool inBlockComment = false;
    for (size_t i = 0; i < fullSource.size(); ++i) {
        if (!inBlockComment && i + 1 < fullSource.size() && fullSource[i] == '/' && fullSource[i+1] == '*') {
            inBlockComment = true;
            i++;
        } else if (inBlockComment && i + 1 < fullSource.size() && fullSource[i] == '*' && fullSource[i+1] == '/') {
            inBlockComment = false;
            i++;
        } else if (!inBlockComment) {
            cleaned += fullSource[i];
        } else if (fullSource[i] == '\n') {
            cleaned += '\n'; // Preserve newline character even inside block comments
        }
    }
    
    std::vector<std::string> expanded;
    std::vector<std::string> linesToProcess = split(cleaned, '\n');
    
    for (const auto& rawLine : linesToProcess) {
        std::string line = rawLine;
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);

        if (line.empty()) {
            expanded.push_back(rawLine); // Preserve empty lines for accurate line numbering
            continue;
        }

        std::stringstream ss(line);
        std::string mnemonic;
        ss >> mnemonic;
        if (toUpper(mnemonic) == "INCLUDE") {
            std::string includeToken;
            std::getline(ss, includeToken);
            std::string includeName = normalizeIncludeName(includeToken);
            auto it = includeFileContents.find(includeName);
            if (it == includeFileContents.end()) {
                error = "Include file not found: " + includeName;
                return {};
            }

            if (std::find(includeStack.begin(), includeStack.end(), includeName) != includeStack.end()) {
                error = "Recursive include detected: " + includeName;
                return {};
            }

            includeStack.push_back(includeName);
            std::vector<std::string> includeLines = split(it->second, '\n');
            std::string includeError;
            std::vector<std::string> nested = preprocessIncludes(includeLines, includeError, includeStack);
            includeStack.pop_back();
            if (!includeError.empty()) {
                error = includeError;
                return {};
            }
            expanded.insert(expanded.end(), nested.begin(), nested.end());
            continue;
        }
        expanded.push_back(rawLine);
    }
    return expanded;
}

std::string Assembler::assemble(const std::string& sourceCode) {
    LOGI("Starting assembly process...");
    symbolTable.clear();
    instructions.clear();
    currentAddress = 0;
    listingOutput = "";

    std::vector<std::string> lines = split(sourceCode, '\n');
    std::string error;
    std::vector<std::string> includeStack;
    std::vector<std::string> expandedLines = preprocessIncludes(lines, error, includeStack);
    if (!error.empty()) return "ERROR: " + error;

    if (!pass1(expandedLines, error)) return "ERROR: " + error;
    if (!pass2(expandedLines, error)) return "ERROR: " + error;

    std::vector<std::pair<uint16_t, uint8_t>> image;
    for (const auto& inst : instructions) {
        for (size_t i = 0; i < inst.bytes.size(); ++i) {
            image.push_back({static_cast<uint16_t>(inst.address + i), inst.bytes[i]});
        }
    }

    std::sort(image.begin(), image.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::stringstream hexOutput;
    size_t idx = 0;
    const size_t recordSize = 0x20;
    while (idx < image.size()) {
        uint16_t startAddr = image[idx].first;
        std::vector<uint8_t> record;
        record.push_back(image[idx].second);
        size_t j = idx + 1;
        while (j < image.size() && record.size() < recordSize && image[j].first == static_cast<uint16_t>(startAddr + record.size())) {
            record.push_back(image[j].second);
            ++j;
        }

        uint8_t count = static_cast<uint8_t>(record.size());
        int checksum = count + (startAddr >> 8) + (startAddr & 0xFF);
        hexOutput << ":" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)count
                  << std::setw(4) << startAddr << "00";
        for (uint8_t b : record) {
            hexOutput << std::setw(2) << (int)b;
            checksum += b;
        }
        hexOutput << std::setw(2) << ((~checksum + 1) & 0xFF) << "\n";
        idx = j;
    }
    hexOutput << ":00000001FF\n";

    for (const auto& inst : instructions) generateListingLine(inst);
    LOGI("Assembly finished successfully.");
    return hexOutput.str();
}

std::string Assembler::getListing() const { return listingOutput; }

void Assembler::generateListingLine(const Instruction& inst) {
    std::stringstream ss;
    ss << std::setw(4) << std::setfill(' ') << std::dec << inst.lineNumber << ": ";
    if (inst.bytes.empty() && !inst.isDirective) ss << "               ";
    else if (inst.isDirective && inst.bytes.empty()) ss << std::setw(4) << std::hex << std::setfill('0') << std::uppercase << inst.address << "           ";
    else {
        ss << std::setw(4) << std::hex << std::setfill('0') << std::uppercase << inst.address << " ";
        for(size_t i=0; i<4 && i<inst.bytes.size(); i++) ss << std::setw(2) << std::setfill('0') << (int)inst.bytes[i] << " ";
        for(size_t i=inst.bytes.size(); i<4; i++) ss << "   ";
    }
    ss << "    " << inst.originalLine << "\n";
    if (inst.bytes.size() > 4) {
        for(size_t i=4; i<inst.bytes.size(); i+=4) {
            ss << "          ";
            for(size_t j=0; j<4 && (i+j)<inst.bytes.size(); j++) ss << std::setw(2) << std::hex << std::setfill('0') << std::uppercase << (int)inst.bytes[i+j] << " ";
            ss << "\n";
        }
    }
    const_cast<Assembler*>(this)->listingOutput += ss.str();
}

bool Assembler::pass1(const std::vector<std::string>& lines, std::string& error) {
    LOGI("Starting Pass 1...");
    currentAddress = 0;
    int lineNum = 0;
    std::vector<std::pair<std::string, std::pair<std::string, int>>> pendingEQUs;

    for (const auto& rawLine : lines) {
        lineNum++;
        std::string line = rawLine;
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);

        if (line.empty()) continue;

        std::stringstream ssEqu(line);
        std::string label, equMnemonic;
        ssEqu >> label >> equMnemonic;
        if (toUpper(equMnemonic) == "EQU") {
            std::string valStr; std::getline(ssEqu, valStr);
            valStr = trim(valStr);
            int32_t val;
            if (evaluateExpression(valStr, val)) {
                symbolTable[toUpper(label)] = {label, val, CONSTANT, true};
            } else {
                // Defer resolution
                symbolTable[toUpper(label)] = {label, 0, CONSTANT, false}; // Mark as undefined for now
                pendingEQUs.push_back({toUpper(label), {valStr, lineNum}});
            }
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string labelName = trim(line.substr(0, colonPos));
            if (!labelName.empty()) {
                symbolTable[toUpper(labelName)] = {labelName, (int32_t)currentAddress, LABEL, true};
            }
            line = trim(line.substr(colonPos + 1));
        }
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string mnemonic; ss >> mnemonic; mnemonic = toUpper(mnemonic);
        
        // Handle .WT suffix for shifts
        if (mnemonic.size() > 3 && mnemonic.substr(mnemonic.size()-3) == ".WT") {
            mnemonic = mnemonic.substr(0, mnemonic.size()-3);
        }

        if (mnemonic.empty()) continue;
        int size = 1;
        if (mnemonic == "ORG") {
            std::string valStr; std::getline(ss, valStr);
            int32_t val;
            if (!evaluateExpression(valStr, val)) {
                error = "Invalid ORG expression at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            currentAddress = (uint16_t)val;
            continue;
        } else if (mnemonic == "DB") {
            std::string args; std::getline(ss, args);
            std::vector<std::string> vals = split(args, ',');
            size = std::max(1, (int)vals.size());
        } else if (mnemonic == "DW") {
            std::string args; std::getline(ss, args);
            std::vector<std::string> vals = split(args, ',');
            size = std::max(1, (int)vals.size()) * 2;
        } else if (mnemonic == "DL") {
            std::string args; std::getline(ss, args);
            std::vector<std::string> vals = split(args, ',');
            size = std::max(1, (int)vals.size()) * 4;
        } else if (mnemonic == "DM") {
            std::string args; std::getline(ss, args);
            std::string t = trim(args);
            if (t.size() >= 2 && ((t.front() == '"' && t.back() == '"') || (t.front() == '\'' && t.back() == '\''))) {
                size = std::max(1, (int)t.substr(1, t.size() - 2).size());
            } else {
                size = 1;
            }
        } else if (mnemonic == "DS") {
            std::string args; std::getline(ss, args);
            std::vector<std::string> vals = split(args, ',');
            int32_t count = 1;
            if (!vals.empty() && !trim(vals[0]).empty()) {
                if (!evaluateExpression(vals[0], count)) {
                    error = "Invalid DS count at line " + std::to_string(lineNum) + ": " + expressionError;
                    return false;
                }
            }
            if (count < 0) {
                error = "Negative DS count at line " + std::to_string(lineNum);
                return false;
            }
            size = static_cast<int>(count);
        } else if (mnemonic.length() == 3 && mnemonic[0] == 'B') {
            size = 2;
        } else if (mnemonic == "JMP" || mnemonic == "JSR") {
            std::string rest; std::getline(ss, rest);
            size = (rest.find('(') != std::string::npos) ? 1 : 3;
        } else if (mnemonic == "LSR" || mnemonic == "LSL" || mnemonic == "ASL" || mnemonic == "ASR" ||
                   mnemonic == "ROL" || mnemonic == "ROR" || mnemonic == "ROXL" || mnemonic == "ROXR" ||
                   mnemonic == "BTST" || mnemonic == "BCHG" || mnemonic == "BCLR" || mnemonic == "BSET" ||
                   mnemonic == "ANDI" || mnemonic == "ORI" || mnemonic == "ADDI") {
            size = 2;
        } else if (mnemonic.rfind("LD.", 0) == 0 || mnemonic.rfind("ST.", 0) == 0) {
            std::string rest; std::getline(ss, rest);
            std::vector<std::string> opList = split(rest, ',');
            std::string addrOp = trim((mnemonic[0] == 'L') ? (opList.size() > 1 ? opList[1] : "") : (opList.size() > 0 ? opList[0] : ""));

            bool wrapped = !addrOp.empty() && addrOp.front() == '(' && addrOp.find(')') != std::string::npos;
            std::string inside = wrapped ? addrOp.substr(1, addrOp.find(')') - 1) : "";
            std::string insideUpper = toUpper(trim(inside));

            if (wrapped && insideUpper.find("SP") != std::string::npos && inside.find('+') != std::string::npos) size = 2;
            else if (wrapped && inside.find("++") != std::string::npos) size = 1;
            else if (wrapped) size = 1;
            else if (!addrOp.empty() && addrOp[0] == '#') size = (mnemonic[3] == 'B' ? 2 : 3);
            else size = 3;
        }
        currentAddress += size;
    }
    
    // Attempt to resolve pending EQUs
    bool progress = true;
    while (progress && !pendingEQUs.empty()) {
        progress = false;
        for (auto it = pendingEQUs.begin(); it != pendingEQUs.end(); ) {
            int32_t val;
            if (evaluateExpression(it->second.first, val)) {
                Symbol& sym = symbolTable[it->first];
                sym.value = val;
                sym.isDefined = true;
                it = pendingEQUs.erase(it);
                progress = true;
            } else {
                ++it;
            }
        }
    }
    if (!pendingEQUs.empty()) {
        error = "Undefined Symbol: " + pendingEQUs.front().second.first + " at line " + std::to_string(pendingEQUs.front().second.second);
        // We report the expression mostly, but ideally the missing symbol is inside evaluation
        // Let's just default to the original error message style
         error = "Invalid EQU expression at line " + std::to_string(pendingEQUs.front().second.second) + " (Unresolved forward reference?)";
        return false;
    }

    LOGI("Finished Pass 1.");
    return true;
}

bool Assembler::pass2(const std::vector<std::string>& lines, std::string& error) {
    LOGI("Starting Pass 2...");
    currentAddress = 0;
    int lineNum = 0;
    for (const auto& rawLine : lines) {
        lineNum++;
        std::string line = rawLine;
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);

        Instruction inst; inst.lineNumber = lineNum; inst.originalLine = rawLine;
        inst.isDirective = false; inst.address = currentAddress;
        if (line.empty()) { instructions.push_back(inst); continue; }

        std::stringstream ssEqu(line);
        std::string label, equMnemonic;
        ssEqu >> label >> equMnemonic;
        if (toUpper(equMnemonic) == "EQU") {
            inst.isDirective = true;
            auto it = symbolTable.find(toUpper(label));
            inst.address = (it == symbolTable.end()) ? currentAddress : static_cast<uint16_t>(it->second.value);
            instructions.push_back(inst);
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) line = trim(line.substr(colonPos + 1));
        if (line.empty()) { instructions.push_back(inst); continue; }

        std::stringstream ss(line);
        std::string mnemonic; ss >> mnemonic; mnemonic = toUpper(mnemonic);
        
        bool isWT = false;
        if (mnemonic.size() > 3 && mnemonic.substr(mnemonic.size()-3) == ".WT") {
            mnemonic = mnemonic.substr(0, mnemonic.size()-3);
            isWT = true;
        }

        if (mnemonic.empty()) { instructions.push_back(inst); continue; }

        std::vector<uint8_t>& bytes = inst.bytes;
        std::string rest; std::getline(ss, rest);

        std::vector<std::string> opList = split(rest, ',');
        std::string op1 = opList.size() > 0 ? opList[0] : "";
        std::string op2 = opList.size() > 1 ? opList[1] : "";

        if (mnemonic == "ORG") {
            int32_t val;
            if (!evaluateExpression(op1, val)) {
                error = "Invalid ORG expression at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            currentAddress = (uint16_t)val;
            inst.address = currentAddress;
            inst.isDirective = true;
        } else if (mnemonic == "DB" || mnemonic == "DW" || mnemonic == "DL") {
            if (opList.empty()) {
                bytes.push_back(0x00);
                if (mnemonic == "DW" || mnemonic == "DL") bytes.push_back(0x00);
                if (mnemonic == "DL") { bytes.push_back(0x00); bytes.push_back(0x00); }
            } else {
                for (auto& v : opList) {
                    int32_t val;
                    if (!evaluateExpression(v, val)) {
                        error = "Invalid " + mnemonic + " value at line " + std::to_string(lineNum) + ": " + expressionError;
                        return false;
                    }
                    bytes.push_back((uint8_t)(val & 0xFF));
                    if (mnemonic == "DW" || mnemonic == "DL") bytes.push_back((uint8_t)((val >> 8) & 0xFF));
                    if (mnemonic == "DL") {
                        bytes.push_back((uint8_t)((val >> 16) & 0xFF));
                        bytes.push_back((uint8_t)((val >> 24) & 0xFF));
                    }
                }
            }
        } else if (mnemonic == "DM") {
            std::string t = trim(rest);
            if (t.size() >= 2 && ((t.front() == '"' && t.back() == '"') || (t.front() == '\'' && t.back() == '\''))) {
                std::string content = t.substr(1, t.size() - 2);
                for (char c : content) bytes.push_back((uint8_t)c);
                if (content.empty()) bytes.push_back(0x00);
            } else {
                bytes.push_back(0x00);
            }
        } else if (mnemonic == "DS") {
            int32_t count = 0;
            int32_t fill = 0;
            if (!opList.empty() && !trim(opList[0]).empty()) {
                if (!evaluateExpression(opList[0], count)) {
                    error = "Invalid DS count at line " + std::to_string(lineNum) + ": " + expressionError;
                    return false;
                }
            }
            if (opList.size() > 1 && !trim(opList[1]).empty()) {
                if (!evaluateExpression(opList[1], fill)) {
                    error = "Invalid DS fill at line " + std::to_string(lineNum) + ": " + expressionError;
                    return false;
                }
            }
            if (count < 0) {
                error = "Negative DS count at line " + std::to_string(lineNum);
                return false;
            }
            for (int32_t i = 0; i < count; ++i) bytes.push_back((uint8_t)(fill & 0xFF));
        } else if (opcodeMap.count(mnemonic) && mnemonic[0] == 'B') {
            int32_t target;
            if (!evaluateExpression(op1, target)) {
                error = "Invalid branch target at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            int offset = target - (currentAddress + 2);
            if (offset < -128 || offset > 127) {
                error = "Branch out of range at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(opcodeMap[mnemonic]);
            bytes.push_back((uint8_t)(offset & 0xFF));
        } else if (mnemonic == "JMP" || mnemonic == "JSR") {
            if (op1.find('(') != std::string::npos) {
                bytes.push_back(mnemonic == "JMP" ? 0xF2 : 0xCE);
            } else {
                int32_t target;
                if (!evaluateExpression(op1, target)) {
                    error = "Invalid jump target at line " + std::to_string(lineNum) + ": " + expressionError;
                    return false;
                }
                bytes.push_back(mnemonic == "JMP" ? 0xF3 : 0xCF);
                bytes.push_back((uint8_t)(target & 0xFF));
                bytes.push_back((uint8_t)((target >> 8) & 0xFF));
            }
        } else if (mnemonic.rfind("LD.", 0) == 0 || mnemonic.rfind("ST.", 0) == 0) {
            encodeLoadStore(mnemonic, op1, op2, bytes, lineNum, error);
            if (!error.empty()) return false;
        } else if (mnemonic == "MOVE" || mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || 
                   mnemonic == "ADD" || mnemonic == "SUB" || mnemonic == "CMP" || mnemonic == "TEST" ||
                   mnemonic == "SXT" || mnemonic == "ABS" || mnemonic == "INV" || mnemonic == "NEG" ||
                   mnemonic == "CLR" || mnemonic == "INC" || mnemonic == "DEC" || mnemonic == "ADDQ") {
             encodeALU(mnemonic, op1, op2, bytes, lineNum, error);
             if (!error.empty()) return false;

        } else if (mnemonic == "PUSH" || mnemonic == "POP") {
            int r = parseRegister(op1);
            if (r < 0) {
                // Check if it's PS or another special register
                std::string rStr = toUpper(trim(op1));
                if (rStr == "PS") r = 5;
                else {
                    error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum);
                    return false;
                }
            }
            bytes.push_back((mnemonic == "POP" ? 0xC0 : 0xC8) + r);
        } else if (mnemonic == "RET") {
            bytes.push_back(0xCB);
        } else if (mnemonic == "RETI") {
            bytes.push_back(0xCC);
        } else if (mnemonic == "TRAP") {
            bytes.push_back(0xCD);
        } else if (mnemonic == "ASL" || mnemonic == "ASR" || mnemonic == "LSL" || mnemonic == "LSR" ||
                   mnemonic == "ROL" || mnemonic == "ROR" || mnemonic == "ROXL" || mnemonic == "ROXR") {
            int r = parseRegister(op1);
            if (r < 0) {
                error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum);
                return false;
            }

            int type = 0; // LSL/LSR
            if (mnemonic == "ASL" || mnemonic == "ASR") type = 1;
            else if (mnemonic == "ROL" || mnemonic == "ROR") type = 2;
            else if (mnemonic == "ROXL" || mnemonic == "ROXR") type = 3;

            bool isRight = (mnemonic.back() == 'R');
            bool isRegShift = false;
            int shiftVal = 0;
            int srcReg = -1;

            if (op2.empty()) {
                 error = "Missing operand for " + mnemonic + " at line " + std::to_string(lineNum);
                 return false;
            }

            // Check if second operand is a register
            srcReg = parseRegister(op2);
            if (srcReg >= 0) {
                isRegShift = true;
                if (isRight) {
                     // In register mode, direction is determined by the register value (sign),
                     // so the mnemonic should typically be the 'Left' version (LSL, ASL, etc.).
                     // However, if user writes LSR R0, R1, we could map it to LSL R0, R1 assuming
                     // they know R1 should be negative? Or block it?
                     // opcodes.asm comments out lsr r0,r1. We will allow it but map to 'Left' opcode structure.
                     // The hardware likely creates the shift amount from the register directly.
                }
            } else {
                // Immediate
                std::string valStr = op2;
                if (valStr[0] == '#') valStr = valStr.substr(1);
                if (!evaluateExpression(valStr, shiftVal)) {
                    error = "Invalid shift value at line " + std::to_string(lineNum) + ": " + expressionError;
                    return false;
                }
                if (isRight) shiftVal = -shiftVal;
            }

            uint8_t opByte = (type << 6);
            if (isRegShift) {
                opByte |= 0x20; // Bit 5 set for register shift
                opByte |= (srcReg & 0x03);
            } else {
                opByte |= (shiftVal & 0x1F);
            }
            
            if (isWT) opByte |= 0x08; // Set write-through/flag bit if .WT used

            bytes.push_back(0xD8 + r);
            bytes.push_back(opByte);

        } else if (mnemonic == "BTST" || mnemonic == "BCHG" || mnemonic == "BCLR" || mnemonic == "BSET") {
            encodeBitOp(mnemonic, op1, op2, bytes, lineNum, error);
            if (!error.empty()) return false;
        } else if (mnemonic == "ANDI" || mnemonic == "ORI" || mnemonic == "ADDI") {
            bytes.push_back(opcodeMap[mnemonic]);
            std::string valStr = op2;
            if (valStr.empty()) valStr = op1; // Fallback if only one operand? No, normally ps,#val
            if (!valStr.empty() && valStr[0] == '#') valStr = valStr.substr(1);
            int32_t val;
            if (!evaluateExpression(valStr, val)) {
                error = "Invalid immediate for " + mnemonic + " at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            bytes.push_back((uint8_t)(val & 0xFF));
        } else if (opcodeMap.count(mnemonic)) {
            bytes.push_back(opcodeMap[mnemonic]);
        } else {
            error = "Unknown instruction '" + mnemonic + "' at line " + std::to_string(lineNum);
            return false;
        }
        currentAddress += bytes.size();
        instructions.push_back(inst);
    }
    LOGI("Finished Pass 2.");
    return true;
}

void Assembler::encodeLoadStore(const std::string& mnemonic, const std::string& op1, const std::string& op2,
                                std::vector<uint8_t>& bytes, int lineNum, std::string& error) {
    bool isLoad = (mnemonic[0] == 'L');
    bool isByte = (mnemonic.size() > 3 && mnemonic[3] == 'B');
    int reg = parseRegister(isLoad ? op1 : op2);
    std::string addrStr = isLoad ? op2 : op1;

    if (reg < 0) {
        error = "Invalid register in LD/ST at line " + std::to_string(lineNum);
        return;
    }

    std::string addr = trim(addrStr);
    bool wrapped = !addr.empty() && addr.front() == '(' && addr.find(')') != std::string::npos;
    std::string inside = wrapped ? addr.substr(1, addr.find(')') - 1) : "";
    std::string insideUpper = toUpper(trim(inside));

    if (wrapped && insideUpper.find("SP") != std::string::npos && inside.find('+') != std::string::npos) {
        size_t plusPos = inside.find('+');
        std::string offsetStr = inside.substr(plusPos + 1);
        int32_t offset;
        if (!evaluateExpression(offsetStr, offset)) {
            error = "Invalid SP offset at line " + std::to_string(lineNum) + ": " + expressionError;
            return;
        }
        uint8_t base = isLoad ? (isByte ? 0xA4 : 0xA0) : (isByte ? 0xAC : 0xA8);
        bytes.push_back(base + reg);
        bytes.push_back((uint8_t)(offset & 0xFF));
    } else if (wrapped && inside.find("++") != std::string::npos) {
        int ptrReg = (insideUpper.find("R2") != std::string::npos) ? 2 : 3;
        uint8_t base = 0x90;
        if (!isLoad) base += 8;
        if (isByte) base += 4;
        if (ptrReg == 3) base += 2;
        if (reg == 1) base += 1;
        bytes.push_back(base);
    } else if (wrapped) {
        int ptrReg = (insideUpper.find("R2") != std::string::npos) ? 2 : 3;
        uint8_t base = 0x80;
        if (!isLoad) base += 8;
        if (isByte) base += 4;
        if (ptrReg == 3) base += 2;
        if (reg == 1) base += 1;
        bytes.push_back(base);
    } else if (!addr.empty() && addr[0] == '#') {
        int32_t value;
        if (!evaluateExpression(addr.substr(1), value)) {
            error = "Invalid immediate at line " + std::to_string(lineNum) + ": " + expressionError;
            return;
        }
        bytes.push_back(isByte ? (0xD4 + reg) : (0xD0 + reg));
        bytes.push_back((uint8_t)(value & 0xFF));
        if (!isByte) bytes.push_back((uint8_t)((value >> 8) & 0xFF));
    } else {
        int32_t addressValue;
        if (!evaluateExpression(addr, addressValue)) {
            error = "Invalid address expression at line " + std::to_string(lineNum) + ": " + expressionError;
            return;
        }
        uint8_t base = isLoad ? (isByte ? 0xB4 : 0xB0) : (isByte ? 0xBC : 0xB8);
        bytes.push_back(base + reg);
        bytes.push_back((uint8_t)(addressValue & 0xFF));
        bytes.push_back((uint8_t)((addressValue >> 8) & 0xFF));
    }
}
