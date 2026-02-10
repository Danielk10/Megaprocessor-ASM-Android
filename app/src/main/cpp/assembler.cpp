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
    // IMPORTANT: Complete opcode table based on Megaprocessor official specification
    // Reference: http://www.megaprocessor.com/instruction_set.pdf
    
    // ===== GROUP 0x00-0x7F: ALU Operations (Register-Register) =====
    // These are encoded based on destination and source registers
    // Formula varies per instruction type - see getALUOpcode() function
    
    // ===== GROUP 0x80-0x8F: Load/Store Indirect (Rn) =====
    // LD.W R0, (R2) = 0x80, LD.W R1, (R2) = 0x81
    // LD.W R0, (R3) = 0x82, LD.W R1, (R3) = 0x83
    // LD.B R0, (R2) = 0x84, LD.B R1, (R2) = 0x85
    // LD.B R0, (R3) = 0x86, LD.B R1, (R3) = 0x87
    // ST.W (R2), R0 = 0x88, ST.W (R2), R1 = 0x89
    // ST.W (R3), R0 = 0x8A, ST.W (R3), R1 = 0x8B
    // ST.B (R2), R0 = 0x8C, ST.B (R2), R1 = 0x8D
    // ST.B (R3), R0 = 0x8E, ST.B (R3), R1 = 0x8F
    
    // ===== GROUP 0x90-0x9F: Load/Store Post-Increment (Rn++) =====
    // LD.W R0, (R2++) = 0x90, LD.W R1, (R2++) = 0x91
    // LD.W R0, (R3++) = 0x92, LD.W R1, (R3++) = 0x93
    // LD.B R0, (R2++) = 0x94, LD.B R1, (R2++) = 0x95
    // LD.B R0, (R3++) = 0x96, LD.B R1, (R3++) = 0x97
    // ST.W (R2++), R0 = 0x98, ST.W (R2++), R1 = 0x99
    // ST.W (R3++), R0 = 0x9A, ST.W (R3++), R1 = 0x9B
    // ST.B (R2++), R0 = 0x9C, ST.B (R2++), R1 = 0x9D
    // ST.B (R3++), R0 = 0x9E, ST.B (R3++), R1 = 0x9F
    
    // ===== GROUP 0xA0-0xAF: Stack Relative (SP+m) =====
    // LD.W R0, (SP+m) = 0xA0, LD.W R1, (SP+m) = 0xA1
    // LD.W R2, (SP+m) = 0xA2, LD.W R3, (SP+m) = 0xA3
    // LD.B R0, (SP+m) = 0xA4, LD.B R1, (SP+m) = 0xA5
    // LD.B R2, (SP+m) = 0xA6, LD.B R3, (SP+m) = 0xA7
    // ST.W (SP+m), R0 = 0xA8, ST.W (SP+m), R1 = 0xA9
    // ST.W (SP+m), R2 = 0xAA, ST.W (SP+m), R3 = 0xAB
    // ST.B (SP+m), R0 = 0xAC, ST.B (SP+m), R1 = 0xAD
    // ST.B (SP+m), R2 = 0xAE, ST.B (SP+m), R3 = 0xAF
    
    // ===== GROUP 0xB0-0xBF: Absolute Address =====
    // LD.W R0, addr = 0xB0, LD.W R1, addr = 0xB1
    // LD.W R2, addr = 0xB2, LD.W R3, addr = 0xB3
    // LD.B R0, addr = 0xB4, LD.B R1, addr = 0xB5
    // LD.B R2, addr = 0xB6, LD.B R3, addr = 0xB7
    // ST.W addr, R0 = 0xB8, ST.W addr, R1 = 0xB9
    // ST.W addr, R2 = 0xBA, ST.W addr, R3 = 0xBB
    // ST.B addr, R0 = 0xBC, ST.B addr, R1 = 0xBD
    // ST.B addr, R2 = 0xBE, ST.B addr, R3 = 0xBF
    
    // ===== GROUP 0xC0-0xCF: Stack & Subroutine =====
    opcodeMap["POP"] = 0xC0;     // POP R0/R1/R2/R3 based on operand
    opcodeMap["PUSH"] = 0xC8;    // PUSH R0/R1/R2/R3 based on operand
    opcodeMap["RET"] = 0xC6;
    opcodeMap["RETI"] = 0xC7;
    opcodeMap["JSR"] = 0xCD;     // JSR addr (3 bytes)
    opcodeMap["TRAP"] = 0xCD;    // TRAP #n
    
    // ===== GROUP 0xD0-0xDF: Shift/Bit Operations =====
    // These use descriptors for bit position/shift amount
    
    // ===== GROUP 0xE0-0xEF: Conditional Branches =====
    opcodeMap["BCC"] = 0xE0;     // Branch if Carry Clear
    opcodeMap["BCS"] = 0xE1;     // Branch if Carry Set
    opcodeMap["BNE"] = 0xE2;     // Branch if Not Equal
    opcodeMap["BEQ"] = 0xE3;     // Branch if Equal
    opcodeMap["BVC"] = 0xE4;     // Branch if oVerflow Clear
    opcodeMap["BVS"] = 0xE5;     // Branch if oVerflow Set
    opcodeMap["BPL"] = 0xE6;     // Branch if PLus
    opcodeMap["BMI"] = 0xE7;     // Branch if MInus
    opcodeMap["BGE"] = 0xE8;     // Branch if Greater or Equal (signed)
    opcodeMap["BLT"] = 0xE9;     // Branch if Less Than (signed)
    opcodeMap["BGT"] = 0xEA;     // Branch if Greater Than (signed)
    opcodeMap["BLE"] = 0xEB;     // Branch if Less or Equal (signed)
    opcodeMap["BUC"] = 0xEC;     // Branch if U Clear
    opcodeMap["BUS"] = 0xED;     // Branch if U Set
    opcodeMap["BHI"] = 0xEE;     // Branch if Higher (unsigned)
    opcodeMap["BLS"] = 0xEF;     // Branch if Lower or Same (unsigned)
    
    // ===== GROUP 0xF0-0xFF: Miscellaneous =====
    opcodeMap["JMP"] = 0xF3;     // JMP addr (absolute, 3 bytes)
    opcodeMap["ANDI"] = 0xF4;    // ANDI PS, #data
    opcodeMap["ORI"] = 0xF5;     // ORI PS, #data
    opcodeMap["ADDI"] = 0xF6;    // ADDI SP, #data
    opcodeMap["SQRT"] = 0xF7;    // Square root
    opcodeMap["MULU"] = 0xF8;    // Multiply unsigned
    opcodeMap["MULS"] = 0xF9;    // Multiply signed
    opcodeMap["DIVU"] = 0xFA;    // Divide unsigned
    opcodeMap["DIVS"] = 0xFB;    // Divide signed
    opcodeMap["ADDX"] = 0xFC;    // Add with extended carry
    opcodeMap["SUBX"] = 0xFD;    // Subtract with extended carry
    opcodeMap["NEGX"] = 0xFE;    // Negate with extended carry
    opcodeMap["NOP"] = 0xFF;     // No operation
    
    // Pseudo-instructions (aliases)
    opcodeMap["CLR"] = 0xFF;     // CLR Rn = XOR Rn, Rn
    opcodeMap["INC"] = 0xFF;     // INC Rn = ADDQ Rn, #1
    opcodeMap["DEC"] = 0xFF;     // DEC Rn = ADDQ Rn, #-1
}

int Assembler::parseRegister(const std::string& token) {
    std::string t = toUpper(token);
    if (t == "R0") return 0;
    if (t == "R1") return 1;
    if (t == "R2") return 2;
    if (t == "R3") return 3;
    if (t == "SP") return 4; 
    if (t == "PS") return 5;
    return -1;
}

// Get correct opcode for ALU operations based on mnemonic and registers
uint8_t Assembler::getALUOpcode(const std::string& mnemonic, int ra, int rb) {
    // Based on Megaprocessor specification:
    // Encoding varies per instruction group
    
    if (mnemonic == "MOVE") {
        // MOVE RA, RB: opcode = (RA * 4 + RB) * 4
        // Examples: MOVE R1, R0 = 0x04
        return (ra * 4 + rb) * 4;
    }
    else if (mnemonic == "AND") {
        // AND RA, RB: base 0x10 + encoding
        return 0x10 + (ra * 4 + rb);
    }
    else if (mnemonic == "XOR") {
        // XOR RA, RB: base 0x20 + encoding
        return 0x20 + (ra * 4 + rb);
    }
    else if (mnemonic == "OR") {
        // OR RA, RB: base 0x30 + encoding
        return 0x30 + (ra * 4 + rb);
    }
    else if (mnemonic == "ADD") {
        // ADD RA, RB: base 0x40 + encoding
        return 0x40 + (ra * 4 + rb);
    }
    else if (mnemonic == "SUB") {
        // SUB RA, RB: base 0x60 + encoding
        return 0x60 + (ra * 4 + rb);
    }
    else if (mnemonic == "CMP") {
        // CMP RA, RB: base 0x70 + encoding
        return 0x70 + (ra * 4 + rb);
    }
    
    return 0xFF; // Invalid/NOP
}

// --- Expression Evaluator ---

bool Assembler::evaluateExpression(const std::string& expr, int32_t& result) {
    try {
        std::string cleanExpr = expr;
        // Remove spaces
        cleanExpr.erase(remove(cleanExpr.begin(), cleanExpr.end(), ' '), cleanExpr.end());
        const char* p = cleanExpr.c_str();
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
    while (*p == '*' || *p == '/' || *p == '<' || *p == '>') {
        char op = *p++;
        if (op == '<' && *p == '<') {  // Left shift
            p++;
            int32_t y = parseFactor(p);
            x <<= y;
        } else if (op == '>' && *p == '>') {  // Right shift
            p++;
            int32_t y = parseFactor(p);
            x >>= y;
        } else if (op == '*') {
            int32_t y = parseFactor(p);
            x *= y;
        } else if (op == '/') {
            int32_t y = parseFactor(p);
            if (y != 0) x /= y;
        } else {
            p--;  // Backtrack
            break;
        }
    }
    return x;
}

int32_t Assembler::parseFactor(const char*& p) {
    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;
    
    if (*p == '(') {
        p++;
        int32_t x = parseExpression(p);
        if (*p == ')') p++;  // closing ')'
        return x;
    } else if (*p == '+' || *p == '-') {  // Unary operators
        char op = *p++;
        int32_t x = parseFactor(p);
        return (op == '+') ? x : -x;
    } else if (isdigit(*p)) {
        // Handle hex 0x or plain decimal
        std::string numStr;
        if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X')) {
            p += 2;
            while (isxdigit(*p)) numStr += *p++;
            return std::stoi(numStr, nullptr, 16);
        } else {
            while (isdigit(*p)) numStr += *p++;
            return std::stoi(numStr);
        }
    } else if (isalpha(*p) || *p == '_') {
        // Label or Symbol
        std::string symName;
        while (isalnum(*p) || *p == '_') symName += *p++;
        
        // Check symbol table
        if (symbolTable.count(symName) && symbolTable[symName].isDefined) {
            return symbolTable[symName].value;
        }
        // If not defined yet, return 0 (will be resolved in pass 2)
        return 0;
    }
    return 0;
}

// --- Main Assembly Logic ---

std::string Assembler::assemble(const std::string& sourceCode) {
    symbolTable.clear();
    instructions.clear();
    currentAddress = 0;
    listingOutput = "";

    std::vector<std::string> lines = split(sourceCode, '\n');
    for(size_t i=0; i<lines.size(); ++i) lines[i] = trim(lines[i]);

    std::string error;
    if (!pass1(lines, error)) {
        return "ERROR: " + error;
    }

    if (!pass2(lines, error)) {
        return "ERROR: " + error;
    }

    // Generate Intel HEX output
    std::stringstream hexOutput;
    
    for (const auto& inst : instructions) {
        if (inst.bytes.empty()) continue;

        uint8_t count = inst.bytes.size();
        uint16_t addr = inst.address;
        uint8_t type = 0x00;  // Data record
        
        // Calculate checksum: two's complement of sum of all bytes
        int checksum = count + (addr >> 8) + (addr & 0xFF) + type;
        
        hexOutput << ":" 
                  << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)count
                  << std::setw(4) << addr
                  << std::setw(2) << (int)type;
        
        for (uint8_t b : inst.bytes) {
            hexOutput << std::setw(2) << (int)b;
            checksum += b;
        }
        
        // Two's complement
        checksum = (~checksum + 1) & 0xFF;
        hexOutput << std::setw(2) << checksum << "\n";
    }
    
    // End-of-file record
    hexOutput << ":00000001FF\n";
    
    // Generate Listing
    for (const auto& inst : instructions) {
        generateListingLine(inst);
    }

    return hexOutput.str();
}

std::string Assembler::getListing() const {
    return listingOutput;
}

void Assembler::generateListingLine(const Instruction& inst) {
    std::stringstream ss;
    ss << std::setw(4) << inst.lineNumber << ": ";
    
    if (inst.bytes.empty() && !inst.isDirective) {
        // Empty line or simple label
        ss << "               ";
    } else if (inst.isDirective && inst.bytes.empty()) {
        // ORG or EQU
        ss << std::setw(4) << std::hex << std::setfill('0') << std::uppercase << inst.address << "           ";
    } else {
        // Code/Data
        ss << std::setw(4) << std::hex << std::setfill('0') << std::uppercase << inst.address << " ";
        // Show up to 4 bytes
        for(size_t i=0; i<4 && i<inst.bytes.size(); i++) {
            ss << std::setw(2) << (int)inst.bytes[i] << " ";
        }
        for(size_t i=inst.bytes.size(); i<4; i++) {
            ss << "   ";
        }
    }
    
    ss << "    " << inst.originalLine << "\n";
    
    // If more bytes, print continuation lines
    if (inst.bytes.size() > 4) {
        for(size_t i=4; i<inst.bytes.size(); i+=4) {
            ss << "          ";
            for(size_t j=0; j<4 && (i+j)<inst.bytes.size(); j++) {
                ss << std::setw(2) << std::hex << std::setfill('0') << std::uppercase << (int)inst.bytes[i+j] << " ";
            }
            ss << "\n";
        }
    }
    
    const_cast<Assembler*>(this)->listingOutput += ss.str();
}

// --- Pass 1: Build Symbol Table & Calculate Addresses ---

bool Assembler::pass1(const std::vector<std::string>& lines, std::string& error) {
    currentAddress = 0;
    int lineNum = 0;
    
    for (const auto& rawLine : lines) {
        lineNum++;
        std::string line = rawLine;

        // Strip comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        
        line = trim(line);
        if (line.empty()) continue;

        // Check for "Symbol EQU value" format (no colon)
        std::stringstream testSS(line);
        std::string tok1, tok2, tok3;
        testSS >> tok1 >> tok2;
        if (toUpper(tok2) == "EQU") {
            // This is EQU directive: Symbol EQU expression
            std::string exprPart;
            std::getline(testSS, exprPart);
            int32_t val;
            if (evaluateExpression(exprPart, val)) {
                symbolTable[tok1] = {tok1, val, CONSTANT, true};
            }
            continue;  // EQU doesn't advance currentAddress
        }

        // Handle label (with colon)
        size_t labelPos = line.find(':');
        if (labelPos != std::string::npos) {
            std::string labelName = trim(line.substr(0, labelPos));
            symbolTable[labelName] = {labelName, (int32_t)currentAddress, LABEL, true};
            line = trim(line.substr(labelPos + 1));
        }
        
        if (line.empty()) continue;
        
        // Parse mnemonic
        std::string cleanLine = line;
        std::replace(cleanLine.begin(), cleanLine.end(), ',', ' ');
        std::stringstream ss(cleanLine);
        std::string mnemonic;
        ss >> mnemonic;
        mnemonic = toUpper(mnemonic);
        
        // Calculate instruction size
        int size = 1;  // Default
        
        if (mnemonic == "ORG") {
            std::string valStr;
            std::getline(ss, valStr);
            int32_t val;
            if (evaluateExpression(valStr, val)) {
                currentAddress = (uint16_t)val;
            }
            continue;  // ORG sets address but doesn't emit bytes
        }
        else if (mnemonic == "INCLUDE") {
            // Skip for now
            continue;
        }
        else if (mnemonic == "DB") {
            // Count byte arguments
            std::string args;
            std::getline(ss, args);
            std::vector<std::string> vals = split(args, ' ');
            for(auto& v : vals) {
                v = trim(v);
                if (!v.empty()) size++;
            }
            size--;  // Subtract initial 1
            currentAddress += size;
            continue;
        }
        else if (mnemonic == "DW") {
            // Count word arguments (each is 2 bytes)
            std::string args;
            std::getline(ss, args);
            std::vector<std::string> vals = split(args, ' ');
            int count = 0;
            for(auto& v : vals) {
                v = trim(v);
                if (!v.empty()) count++;
            }
            currentAddress += count * 2;
            continue;
        }
        
        // Instruction size calculation
        std::string op1, op2;
        ss >> op1 >> op2;
        
        // Branch instructions: 2 bytes
        if (mnemonic.length() == 3 && mnemonic[0] == 'B') {
            size = 2;
        }
        // JMP/JSR
        else if (mnemonic == "JMP" || mnemonic == "JSR") {
            if (op1.find('(') != std::string::npos) {
                size = 1;  // Indirect: JMP (R0)
            } else {
                size = 3;  // Absolute: JMP addr
            }
        }
        // Load/Store instructions
        else if (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.") {
            // Check addressing mode from second operand
            std::string addrOp = (mnemonic[0] == 'L') ? op2 : op1;
            
            if (addrOp.find("SP") != std::string::npos && addrOp.find('+') != std::string::npos) {
                size = 2;  // Stack relative: (SP+m)
            } else if (addrOp.find('(') != std::string::npos) {
                size = 1;  // Indirect: (R2) or (R2++)
            } else if (addrOp[0] == '#') {
                size = 3;  // Immediate: #data
            } else {
                size = 3;  // Absolute: addr
            }
        }
        // ALU operations (MOV, ADD, XOR, etc.): 1 byte
        // PUSH/POP/RET/RETI/NOP: 1 byte
        // Everything else defaults to 1 byte
        
        currentAddress += size;
    }
    
    return true;
}

// --- Pass 2: Generate Machine Code ---

bool Assembler::pass2(const std::vector<std::string>& lines, std::string& error) {
    currentAddress = 0;
    int lineNum = 0;
    
    for (const auto& rawLine : lines) {
        lineNum++;
        std::string line = rawLine;
        
        // Strip comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);

        Instruction inst;
        inst.lineNumber = lineNum;
        inst.originalLine = rawLine;
        inst.isDirective = false;
        inst.address = currentAddress;
        
        if (line.empty()) {
            instructions.push_back(inst);
            continue;
        }

        // Check for EQU (no colon)
        std::stringstream testSS(line);
        std::string tok1, tok2;
        testSS >> tok1 >> tok2;
        if (toUpper(tok2) == "EQU") {
            inst.isDirective = true;
            inst.address = symbolTable[tok1].value;
            instructions.push_back(inst);
            continue;
        }

        // Handle label
        size_t labelPos = line.find(':');
        if (labelPos != std::string::npos) {
            line = trim(line.substr(labelPos + 1));
        }
        
        if (line.empty()) {
            instructions.push_back(inst);
            continue;
        }
        
        // Parse instruction
        std::string cleanLine = line;
        std::replace(cleanLine.begin(), cleanLine.end(), ',', ' ');
        std::stringstream ss(cleanLine);
        std::string mnemonic;
        ss >> mnemonic;
        mnemonic = toUpper(mnemonic);
        
        std::vector<uint8_t>& bytes = inst.bytes;
        
        if (mnemonic == "ORG") {
            std::string valStr;
            std::getline(ss, valStr);
            int32_t val;
            if (evaluateExpression(valStr, val)) {
                currentAddress = (uint16_t)val;
                inst.address = currentAddress;
            }
            inst.isDirective = true;
            instructions.push_back(inst);
            continue;
        }
        else if (mnemonic == "INCLUDE") {
            inst.isDirective = true;
            instructions.push_back(inst);
            continue;
        }
        else if (mnemonic == "DB") {
            std::string args;
            std::getline(ss, args);
            std::vector<std::string> vals = split(args, ' ');
            for(auto& v : vals) {
                v = trim(v);
                if (v.empty()) continue;
                int32_t val;
                if (evaluateExpression(v, val)) {
                    bytes.push_back((uint8_t)(val & 0xFF));
                }
            }
        }
        else if (mnemonic == "DW") {
            std::string args;
            std::getline(ss, args);
            std::vector<std::string> vals = split(args, ' ');
            for(auto& v : vals) {
                v = trim(v);
                if (v.empty()) continue;
                int32_t val;
                if (evaluateExpression(v, val)) {
                    // Little-endian
                    bytes.push_back((uint8_t)(val & 0xFF));
                    bytes.push_back((uint8_t)((val >> 8) & 0xFF));
                }
            }
        }
        // === Instruction encoding ===
        else if (opcodeMap.count(mnemonic) && mnemonic[0] == 'B') {
            // Branch instruction
            std::string target;
            ss >> target;
            int32_t targetAddr;
            if (!evaluateExpression(target, targetAddr)) {
                error = "Invalid branch target at line " + std::to_string(lineNum);
                return false;
            }
            // Calculate PC-relative offset
            int offset = targetAddr - (currentAddress + 2);
            if (offset < -128 || offset > 127) {
                error = "Branch offset out of range at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(opcodeMap[mnemonic]);
            bytes.push_back((uint8_t)(offset & 0xFF));
        }
        else if (mnemonic == "JMP" || mnemonic == "JSR") {
            std::string target;
            ss >> target;
            if (target.find('(') != std::string::npos) {
                // Indirect
                bytes.push_back(mnemonic == "JMP" ? 0xF2 : 0xCE);
            } else {
                // Absolute
                int32_t targetAddr;
                if (!evaluateExpression(target, targetAddr)) {
                    error = "Invalid jump target at line " + std::to_string(lineNum);
                    return false;
                }
                bytes.push_back(mnemonic == "JMP" ? 0xF3 : 0xCD);
                // Little-endian
                bytes.push_back((uint8_t)(targetAddr & 0xFF));
                bytes.push_back((uint8_t)((targetAddr >> 8) & 0xFF));
            }
        }
        else if (mnemonic.length() >= 3 && (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.")) {
            // Load/Store instructions
            encodeLoadStore(mnemonic, ss, bytes, lineNum, error);
            if (!error.empty()) return false;
        }
        else if (mnemonic == "MOVE" || mnemonic == "ADD" || mnemonic == "SUB" ||
                 mnemonic == "XOR" || mnemonic == "OR" || mnemonic == "AND" || mnemonic == "CMP") {
            // ALU operations
            std::string r1Str, r2Str;
            ss >> r1Str >> r2Str;
            int r1 = parseRegister(r1Str);
            int r2 = parseRegister(r2Str);
            if (r1 < 0 || r2 < 0) {
                error = "Invalid register at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(getALUOpcode(mnemonic, r1, r2));
        }
        else if (mnemonic == "PUSH" || mnemonic == "POP") {
            std::string regStr;
            ss >> regStr;
            int reg = parseRegister(regStr);
            if (reg < 0 || reg > 5) {
                error = "Invalid register for PUSH/POP at line " + std::to_string(lineNum);
                return false;
            }
            // POP: 0xC0-0xC5, PUSH: 0xC8-0xCD
            bytes.push_back((mnemonic == "POP" ? 0xC0 : 0xC8) + reg);
        }
        else if (mnemonic == "RET") {
            bytes.push_back(0xC6);
        }
        else if (mnemonic == "RETI") {
            bytes.push_back(0xC7);
        }
        else if (mnemonic == "NOP") {
            bytes.push_back(0xFF);
        }
        // Pseudo-instructions
        else if (mnemonic == "CLR") {
            // CLR Rn = XOR Rn, Rn
            std::string regStr;
            ss >> regStr;
            int reg = parseRegister(regStr);
            if (reg < 0) {
                error = "Invalid register for CLR at line " + std::to_string(lineNum);
                return false;
            }
            bytes.push_back(getALUOpcode("XOR", reg, reg));
        }
        else if (mnemonic == "INC") {
            // INC Rn = ADDQ Rn, #1 (needs proper ADDQ encoding)
            std::string regStr;
            ss >> regStr;
            int reg = parseRegister(regStr);
            if (reg < 0) {
                error = "Invalid register for INC at line " + std::to_string(lineNum);
                return false;
            }
            // ADDQ encoding: 0x50 + reg*4 for +1
            bytes.push_back(0x50 + reg * 4);
        }
        else if (mnemonic == "DEC") {
            // DEC Rn = ADDQ Rn, #-1
            std::string regStr;
            ss >> regStr;
            int reg = parseRegister(regStr);
            if (reg < 0) {
                error = "Invalid register for DEC at line " + std::to_string(lineNum);
                return false;
            }
            // ADDQ encoding for -1: 0x58 + reg*4
            bytes.push_back(0x58 + reg * 4);
        }
        else if (opcodeMap.count(mnemonic)) {
            // Other single-byte instructions
            bytes.push_back(opcodeMap[mnemonic]);
        }
        else {
            // Unknown instruction - emit NOP
            bytes.push_back(0xFF);
        }
        
        currentAddress += bytes.size();
        instructions.push_back(inst);
    }
    
    return true;
}

// Helper function to encode LD/ST instructions
void Assembler::encodeLoadStore(const std::string& mnemonic, std::stringstream& ss,
                                std::vector<uint8_t>& bytes, int lineNum, std::string& error) {
    bool isLoad = (mnemonic[0] == 'L');
    bool isByte = (mnemonic[3] == 'B');
    
    std::string op1, op2;
    ss >> op1 >> op2;
    
    // Determine register and addressing mode
    int reg;
    std::string addrStr;
    
    if (isLoad) {
        // LD.x Rn, <addr>
        reg = parseRegister(op1);
        addrStr = op2;
    } else {
        // ST.x <addr>, Rn
        reg = parseRegister(op2);
        addrStr = op1;
    }
    
    if (reg < 0) {
        error = "Invalid register in LD/ST at line " + std::to_string(lineNum);
        return;
    }
    
    // Parse addressing mode
    if (addrStr.find("SP") != std::string::npos && addrStr.find('+') != std::string::npos) {
        // Stack relative: (SP+m)
        size_t plusPos = addrStr.find('+');
        std::string offsetStr = addrStr.substr(plusPos + 1);
        offsetStr.erase(remove(offsetStr.begin(), offsetStr.end(), ')'), offsetStr.end());
        int32_t offset;
        if (!evaluateExpression(offsetStr, offset)) {
            error = "Invalid SP offset at line " + std::to_string(lineNum);
            return;
        }
        // Opcode: 0xA0-0xAF
        uint8_t base = 0xA0;
        if (!isLoad) base += 8;  // ST
        if (isByte) base += 4;   // Byte
        bytes.push_back(base + reg);
        bytes.push_back((uint8_t)(offset & 0xFF));
    }
    else if (addrStr.find("++") != std::string::npos) {
        // Post-increment: (Rn++)
        int ptrReg = (addrStr.find("R2") != std::string::npos) ? 2 : 3;
        // Opcode: 0x90-0x9F
        uint8_t base = 0x90;
        if (!isLoad) base += 8;  // ST
        if (isByte) base += 4;   // Byte
        if (ptrReg == 3) base += 2;
        if (reg == 1) base += 1;
        bytes.push_back(base);
    }
    else if (addrStr.find('(') != std::string::npos) {
        // Indirect: (Rn)
        int ptrReg = (addrStr.find("R2") != std::string::npos) ? 2 : 3;
        // Opcode: 0x80-0x8F
        uint8_t base = 0x80;
        if (!isLoad) base += 8;  // ST
        if (isByte) base += 4;   // Byte
        if (ptrReg == 3) base += 2;
        if (reg == 1) base += 1;
        bytes.push_back(base);
    }
    else if (addrStr[0] == '#') {
        // Immediate (LD only)
        if (!isLoad) {
            error = "ST cannot use immediate addressing at line " + std::to_string(lineNum);
            return;
        }
        int32_t value;
        if (!evaluateExpression(addrStr.substr(1), value)) {
            error = "Invalid immediate value at line " + std::to_string(lineNum);
            return;
        }
        // Opcode: 0xE0-0xE3 (LD.W) or 0xE4-0xE7 (LD.B)
        uint8_t opcode = isByte ? (0xE4 + reg) : (0xE0 + reg);
        bytes.push_back(opcode);
        // Little-endian
        bytes.push_back((uint8_t)(value & 0xFF));
        if (!isByte) {
            bytes.push_back((uint8_t)((value >> 8) & 0xFF));
        }
    }
    else {
        // Absolute address
        int32_t addr;
        if (!evaluateExpression(addrStr, addr)) {
            error = "Invalid address at line " + std::to_string(lineNum);
            return;
        }
        // Opcode: 0xB0-0xBF
        uint8_t base = 0xB0;
        if (!isLoad) base += 8;  // ST
        if (isByte) base += 4;   // Byte
        bytes.push_back(base + reg);
        // Little-endian address
        bytes.push_back((uint8_t)(addr & 0xFF));
        bytes.push_back((uint8_t)((addr >> 8) & 0xFF));
    }
}
