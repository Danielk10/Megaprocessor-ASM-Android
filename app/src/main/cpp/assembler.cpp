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
    opcodeMap["BUC"] = 0xF0;
    opcodeMap["BUS"] = 0xF1;
    opcodeMap["BHI"] = 0xF2;
    opcodeMap["BLS"] = 0xF3;

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
    if (mnemonic == "MOVE" && ra == 4 && rb == 0) return 0xF1; // MOVE SP,R0

    if (ra < 0 || rb < 0 || ra > 3 || rb > 3) {
        return 0xFF;
    }

    const uint8_t regCode = static_cast<uint8_t>(rb * 4 + ra);

    if (mnemonic == "MOVE") return regCode;
    if (mnemonic == "AND") return static_cast<uint8_t>(0x10 + regCode);
    if (mnemonic == "XOR") return static_cast<uint8_t>(0x20 + regCode);
    if (mnemonic == "OR")  return static_cast<uint8_t>(0x30 + regCode);
    if (mnemonic == "ADD") return static_cast<uint8_t>(0x40 + regCode);
    if (mnemonic == "SUB") return static_cast<uint8_t>(0x60 + regCode);
    if (mnemonic == "CMP") return static_cast<uint8_t>(0x70 + regCode);
    return 0xFF;
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
    if (isdigit(*p)) {
        std::string numStr;
        if (*p == '0' && (toupper(*(p+1)) == 'X')) {
            p += 2;
            while (isxdigit(*p)) numStr += *p++;
            if (numStr.empty()) throw std::runtime_error("Invalid hex literal");
            return std::stoi(numStr, nullptr, 16);
        }
        while (isdigit(*p)) numStr += *p++;
        return std::stoi(numStr);
    }
    if (isalpha(*p) || *p == '_') {
        std::string symName;
        while (isalnum(*p) || *p == '_') symName += *p++;
        std::string key = toUpper(symName);
        auto it = symbolTable.find(key);
        if (it != symbolTable.end() && it->second.isDefined) {
            return it->second.value;
        }
        throw std::runtime_error("Undefined symbol: " + symName);
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

std::vector<std::string> Assembler::preprocessIncludes(const std::vector<std::string>& lines, std::string& error,
                                                       std::vector<std::string>& includeStack) const {
    std::vector<std::string> expanded;
    for (const auto& rawLine : lines) {
        std::string line = rawLine;
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);

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
            std::string expr; std::getline(ssEqu, expr);
            int32_t val;
            if (!evaluateExpression(expr, val)) {
                error = "Invalid EQU expression at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            std::string labelKey = toUpper(label);
            symbolTable[labelKey] = {label, val, CONSTANT, true};
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string labelName = trim(line.substr(0, colonPos));
            if (!labelName.empty()) {
                std::string labelKey = toUpper(labelName);
                symbolTable[labelKey] = {labelName, (int32_t)currentAddress, LABEL, true};
            }
            line = trim(line.substr(colonPos + 1));
        }
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string mnemonic; ss >> mnemonic; mnemonic = toUpper(mnemonic);
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
        } else if (mnemonic.length() == 3 && mnemonic[0] == 'B') {
            size = 2;
        } else if (mnemonic == "JMP" || mnemonic == "JSR") {
            std::string rest; std::getline(ss, rest);
            size = (rest.find('(') != std::string::npos) ? 1 : 3;
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
        } else if (mnemonic == "DB" || mnemonic == "DW") {
            if (opList.empty()) {
                bytes.push_back(0x00);
                if (mnemonic == "DW") bytes.push_back(0x00);
            } else {
                for (auto& v : opList) {
                    int32_t val;
                    if (!evaluateExpression(v, val)) {
                        error = "Invalid " + mnemonic + " value at line " + std::to_string(lineNum) + ": " + expressionError;
                        return false;
                    }
                    bytes.push_back((uint8_t)(val & 0xFF));
                    if (mnemonic == "DW") bytes.push_back((uint8_t)((val >> 8) & 0xFF));
                }
            }
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
        } else if (mnemonic == "MOVE" || mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "ADD" || mnemonic == "SUB" || mnemonic == "CMP") {
            int r1 = parseRegister(op1), r2 = parseRegister(op2);
            if (r1 >= 0 && r2 >= 0) bytes.push_back(getALUOpcode(mnemonic, r1, r2));
            else {
                error = "Invalid register at line " + std::to_string(lineNum);
                return false;
            }
        } else if (mnemonic == "PUSH" || mnemonic == "POP") {
            int r = parseRegister(op1);
            if (r < 0) {
                error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back((mnemonic == "POP" ? 0xC0 : 0xC8) + r);
        } else if (mnemonic == "CLR") {
            int r = parseRegister(op1);
            if (r < 0) {
                error = "Invalid register in CLR at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(getALUOpcode("XOR", r, r));
        } else if (mnemonic == "INC" || mnemonic == "DEC") {
            int r = parseRegister(op1);
            if (r < 0) {
                error = "Invalid register in " + mnemonic + " at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back((mnemonic == "INC" ? 0x54 : 0x5C) + r);
        } else if (mnemonic == "ADDQ") {
            int r = parseRegister(op1);
            int32_t val;
            std::string valStr = op2;
            if (!valStr.empty() && valStr[0] == '#') valStr = valStr.substr(1);
            if (!evaluateExpression(valStr, val)) {
                error = "Invalid ADDQ value at line " + std::to_string(lineNum) + ": " + expressionError;
                return false;
            }
            if (r < 0 || (val != 1 && val != 2 && val != -1 && val != -2)) {
                error = "ADDQ supports only #1/#2/#-1/#-2 at line " + std::to_string(lineNum);
                return false;
            }
            if (val == 1) bytes.push_back(0x54 + r);
            else if (val == 2) bytes.push_back(0x50 + r);
            else if (val == -1) bytes.push_back(0x5C + r);
            else bytes.push_back(0x58 + r);
        } else if (mnemonic == "TEST") {
            int r = parseRegister(op1);
            if (r < 0) {
                error = "Invalid register in TEST at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(getALUOpcode("AND", r, r));
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
