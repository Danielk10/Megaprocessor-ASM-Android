#include "assembler.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cctype>

Assembler::Assembler() {
    initOpcodes();
}

void Assembler::initOpcodes() {
    // Reference: http://www.megaprocessor.com/instruction_set.pdf

    // Subroutine & Stack
    opcodeMap["POP"] = 0xC0;
    opcodeMap["PUSH"] = 0xC8;
    opcodeMap["RET"] = 0xC6;
    opcodeMap["RETI"] = 0xC7;
    opcodeMap["JSR"] = 0xCD;
    opcodeMap["TRAP"] = 0xCD;

    // Branches
    opcodeMap["BCC"] = 0xE0;
    opcodeMap["BCS"] = 0xE1;
    opcodeMap["BNE"] = 0xE2;
    opcodeMap["BEQ"] = 0xE3;
    opcodeMap["BVC"] = 0xE4;
    opcodeMap["BVS"] = 0xE5;
    opcodeMap["BPL"] = 0xE6;
    opcodeMap["BMI"] = 0xE7;
    opcodeMap["BGE"] = 0xE8;
    opcodeMap["BLT"] = 0xE9;
    opcodeMap["BGT"] = 0xEA;
    opcodeMap["BLE"] = 0xEB;
    opcodeMap["BUC"] = 0xEC;
    opcodeMap["BUS"] = 0xED;
    opcodeMap["BHI"] = 0xEE;
    opcodeMap["BLS"] = 0xEF;
    
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
    // Aggressively remove potential trailing punctuation/artifacts
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
    if (mnemonic == "MOVE") return (ra * 4 + rb) * 4;
    else if (mnemonic == "AND") return 0x10 + (ra * 4 + rb);
    else if (mnemonic == "XOR") return 0x20 + (ra * 4 + rb);
    else if (mnemonic == "OR")  return 0x30 + (ra * 4 + rb);
    else if (mnemonic == "ADD") return 0x40 + (ra * 4 + rb);
    else if (mnemonic == "SUB") return 0x60 + (ra * 4 + rb);
    else if (mnemonic == "CMP") return 0x70 + (ra * 4 + rb);
    return 0xFF;
}

bool Assembler::evaluateExpression(const std::string& expr, int32_t& result) {
    std::string cleanExpr = trim(expr);
    if (cleanExpr.empty()) {
        result = 0;
        return true;
    }
    // Remove trailing semicolon if any
    if (cleanExpr.back() == ';') cleanExpr.pop_back();
    cleanExpr = trim(cleanExpr);

    try {
        std::string noSpaces;
        for (char c : cleanExpr) if (!isspace(c)) noSpaces += c;
        const char* p = noSpaces.c_str();
        result = parseExpression(p);
        return true;
    } catch (...) {
        return false;
    }
}

int32_t Assembler::parseExpression(const char*& p) {
    int32_t x = parseTerm(p);
    while (*p == '+' || *p == '-') {
        char op = *p++;
        int32_t y = parseTerm(p);
        if (op == '+') x += y;
        else x -= y;
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
            if (y != 0) x /= y;
        }
    }
    return x;
}

int32_t Assembler::parseFactor(const char*& p) {
    while (isspace(*p)) p++;
    if (*p == '(') {
        p++;
        int32_t x = parseExpression(p);
        if (*p == ')') p++;
        return x;
    } else if (*p == '+' || *p == '-') {
        char op = *p++;
        int32_t x = parseFactor(p);
        return (op == '+') ? x : -x;
    } else if (isdigit(*p)) {
        std::string numStr;
        if (*p == '0' && (toupper(*(p+1)) == 'X')) {
            p += 2;
            while (isxdigit(*p)) numStr += *p++;
            return std::stoi(numStr, nullptr, 16);
        } else {
            while (isdigit(*p)) numStr += *p++;
            return std::stoi(numStr);
        }
    } else if (isalpha(*p) || *p == '_') {
        std::string symName;
        while (isalnum(*p) || *p == '_') symName += *p++;
        if (symbolTable.count(symName) && symbolTable[symName].isDefined) {
            return symbolTable[symName].value;
        }
        return 0; // Forward labels resolve to 0 in pass 1
    }
    return 0;
}

std::string Assembler::assemble(const std::string& sourceCode) {
    LOGI("Starting assembly process...");
    symbolTable.clear();
    instructions.clear();
    currentAddress = 0;
    listingOutput = "";

    // Pre-define required constants from Megaprocessor_defs.asm
    symbolTable["INT_RAM_START"] = {"INT_RAM_START", 0x0000, CONSTANT, true};
    symbolTable["INT_RAM_LEN"] = {"INT_RAM_LEN", 0x4000, CONSTANT, true};
    symbolTable["INT_RAM_BYTES_ACROSS"] = {"INT_RAM_BYTES_ACROSS", 4, CONSTANT, true};
    symbolTable["GEN_IO_INPUT"] = {"GEN_IO_INPUT", 0x8000, CONSTANT, true};
    symbolTable["IO_SWITCH_FLAG_SQUARE"] = {"IO_SWITCH_FLAG_SQUARE", 0x01, CONSTANT, true};
    symbolTable["IO_SWITCH_FLAG_R1"] = {"IO_SWITCH_FLAG_R1", 0x02, CONSTANT, true};
    symbolTable["IO_SWITCH_FLAG_LEFT"] = {"IO_SWITCH_FLAG_LEFT", 0x04, CONSTANT, true};
    symbolTable["IO_SWITCH_FLAG_RIGHT"] = {"IO_SWITCH_FLAG_RIGHT", 0x08, CONSTANT, true};

    std::vector<std::string> lines = split(sourceCode, '\n');
    std::string error;
    if (!pass1(lines, error)) return "ERROR: " + error;
    if (!pass2(lines, error)) return "ERROR: " + error;

    std::stringstream hexOutput;
    for (const auto& inst : instructions) {
        if (inst.bytes.empty()) continue;
        uint8_t count = inst.bytes.size();
        uint16_t addr = inst.address;
        int checksum = count + (addr >> 8) + (addr & 0xFF);
        hexOutput << ":" << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)count
                  << std::setw(4) << addr << "00";
        for (uint8_t b : inst.bytes) {
            hexOutput << std::setw(2) << (int)b;
            checksum += b;
        }
        hexOutput << std::setw(2) << ((~checksum + 1) & 0xFF) << "\n";
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

        // Check for EQU
        std::stringstream ssEqu(line);
        std::string label, equMnemonic;
        ssEqu >> label >> equMnemonic;
        if (toUpper(equMnemonic) == "EQU") {
            std::string expr; std::getline(ssEqu, expr);
            int32_t val; if (evaluateExpression(expr, val)) symbolTable[label] = {label, val, CONSTANT, true};
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string labelName = trim(line.substr(0, colonPos));
            if (!labelName.empty()) symbolTable[labelName] = {labelName, (int32_t)currentAddress, LABEL, true};
            line = trim(line.substr(colonPos + 1));
        }
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string mnemonic; ss >> mnemonic; mnemonic = toUpper(mnemonic);
        int size = 1;
        if (mnemonic == "ORG") {
            std::string valStr; std::getline(ss, valStr);
            int32_t val; if (evaluateExpression(valStr, val)) currentAddress = (uint16_t)val;
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
        } else if (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.") {
            std::string rest; std::getline(ss, rest);
            std::vector<std::string> opList = split(rest, ',');
            std::string addrOp = (mnemonic[0] == 'L') ? (opList.size() > 1 ? opList[1] : "") : (opList.size() > 0 ? opList[0] : "");
            if (addrOp.find("SP") != std::string::npos && addrOp.find('+') != std::string::npos) size = 2;
            else if (addrOp.find('(') != std::string::npos) size = 1;
            else if (!addrOp.empty() && addrOp[0] == '#') size = (mnemonic[2] == 'B' ? 2 : 3);
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
            inst.isDirective = true; inst.address = symbolTable[label].value;
            instructions.push_back(inst); continue;
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
            int32_t val; if (evaluateExpression(op1, val)) { currentAddress = (uint16_t)val; inst.address = currentAddress; }
            inst.isDirective = true;
        } else if (mnemonic == "DB" || mnemonic == "DW") {
            for (auto& v : opList) {
                int32_t val; if (evaluateExpression(v, val)) {
                    bytes.push_back((uint8_t)(val & 0xFF));
                    if (mnemonic == "DW") bytes.push_back((uint8_t)((val >> 8) & 0xFF));
                }
            }
        } else if (opcodeMap.count(mnemonic) && mnemonic[0] == 'B') {
            int32_t target; evaluateExpression(op1, target);
            int offset = target - (currentAddress + 2);
            bytes.push_back(opcodeMap[mnemonic]); bytes.push_back((uint8_t)(offset & 0xFF));
        } else if (mnemonic == "JMP" || mnemonic == "JSR") {
            if (op1.find('(') != std::string::npos) {
                bytes.push_back(mnemonic == "JMP" ? 0xF2 : 0xCE);
            } else {
                int32_t target; evaluateExpression(op1, target);
                bytes.push_back(mnemonic == "JMP" ? 0xF3 : 0xCD);
                bytes.push_back((uint8_t)(target & 0xFF)); bytes.push_back((uint8_t)((target >> 8) & 0xFF));
            }
        } else if (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.") {
            encodeLoadStore(mnemonic, op1, op2, bytes, lineNum, error);
            if (!error.empty()) return false;
        } else if (mnemonic == "MOVE" || mnemonic == "AND" || mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "ADD" || mnemonic == "SUB" || mnemonic == "CMP") {
            int r1 = parseRegister(op1), r2 = parseRegister(op2);
            if (r1 >= 0 && r2 >= 0) bytes.push_back(getALUOpcode(mnemonic, r1, r2));
            else { error = "Invalid register at line " + std::to_string(lineNum); return false; }
        } else if (mnemonic == "PUSH" || mnemonic == "POP") {
            int r = parseRegister(op1); if (r >= 0) bytes.push_back((mnemonic == "POP" ? 0xC0 : 0xC8) + r);
        } else if (mnemonic == "CLR") {
            int r = parseRegister(op1); if (r >= 0) bytes.push_back(getALUOpcode("XOR", r, r));
        } else if (mnemonic == "INC" || mnemonic == "DEC") {
            int r = parseRegister(op1); if (r >= 0) bytes.push_back((mnemonic == "INC" ? 0x50 : 0x58) + r * 4);
        } else if (mnemonic == "ADDQ") {
            int r = parseRegister(op1); int32_t val;
            std::string valStr = op2; if (!valStr.empty() && valStr[0] == '#') valStr = valStr.substr(1);
            evaluateExpression(valStr, val);
            bytes.push_back((val >= 0 ? 0x50 : 0x58) + r * 4);
        } else if (mnemonic == "TEST") {
            int r = parseRegister(op1); if (r >= 0) bytes.push_back(0x70 + (r * 4 + r));
        } else if (opcodeMap.count(mnemonic)) {
            bytes.push_back(opcodeMap[mnemonic]);
        } else if (mnemonic != "INCLUDE") {
            LOGE("Unknown instruction '%s' at line %d", mnemonic.c_str(), lineNum);
            bytes.push_back(0xFF);
        }
        currentAddress += bytes.size();
        instructions.push_back(inst);
    }
    LOGI("Finished Pass 2.");
    return true;
}

void Assembler::encodeLoadStore(const std::string& mnemonic, const std::string& op1, const std::string& op2,
                                std::vector<uint8_t>& bytes, int lineNum, std::string& error) {
    bool isLoad = (mnemonic[0] == 'L'), isByte = (mnemonic[3] == 'B');
    int reg = parseRegister(isLoad ? op1 : op2);
    std::string addrStr = isLoad ? op2 : op1;

    if (reg < 0) {
        LOGE("LD/ST Error line %d: reg parse fail. op1='%s' op2='%s'", lineNum, op1.c_str(), op2.c_str());
        error = "Invalid register in LD/ST at line " + std::to_string(lineNum);
        return;
    }

    if (addrStr.find("SP") != std::string::npos && addrStr.find('+') != std::string::npos) {
        size_t plusPos = addrStr.find('+');
        std::string offsetStr = addrStr.substr(plusPos + 1);
        while (!offsetStr.empty() && (offsetStr.back() == ')' || offsetStr.back() == ';')) offsetStr.pop_back();
        int32_t offset; evaluateExpression(offsetStr, offset);
        uint8_t base = isLoad ? (isByte ? 0xA4 : 0xA0) : (isByte ? 0xAC : 0xA8);
        bytes.push_back(base + reg); bytes.push_back((uint8_t)(offset & 0xFF));
    } else if (addrStr.find("++") != std::string::npos) {
        int ptrReg = (addrStr.find("R2") != std::string::npos || addrStr.find("r2") != std::string::npos) ? 2 : 3;
        uint8_t base = 0x90; if (!isLoad) base += 8; if (isByte) base += 4;
        if (ptrReg == 3) base += 2; if (reg == 1) base += 1;
        bytes.push_back(base);
    } else if (addrStr.find('(') != std::string::npos) {
        int ptrReg = (addrStr.find("R2") != std::string::npos || addrStr.find("r2") != std::string::npos) ? 2 : 3;
        uint8_t base = 0x80; if (!isLoad) base += 8; if (isByte) base += 4;
        if (ptrReg == 3) base += 2; if (reg == 1) base += 1;
        bytes.push_back(base);
    } else if (!addrStr.empty() && addrStr[0] == '#') {
        int32_t value; evaluateExpression(addrStr.substr(1), value);
        bytes.push_back(isByte ? (0xE4 + reg) : (0xE0 + reg));
        bytes.push_back((uint8_t)(value & 0xFF)); if (!isByte) bytes.push_back((uint8_t)((value >> 8) & 0xFF));
    } else {
        int32_t addr; evaluateExpression(addrStr, addr);
        uint8_t base = isLoad ? (isByte ? 0xB4 : 0xB0) : (isByte ? 0xBC : 0xB8);
        bytes.push_back(base + reg); bytes.push_back((uint8_t)(addr & 0xFF)); bytes.push_back((uint8_t)((addr >> 8) & 0xFF));
    }
}
