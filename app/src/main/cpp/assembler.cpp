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
    // Grupo 0xE0: Saltos
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

    // Grupo 0xC0: Stack & Misc
    opcodeMap["POP"] = 0xC0;
    opcodeMap["PUSH"] = 0xC8; 
    opcodeMap["RET"] = 0xC6;
    opcodeMap["RETI"] = 0xC7;
    opcodeMap["NOP"] = 0xFF;
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

// --- Expression Evaluator ---

bool Assembler::evaluateExpression(const std::string& expr, int32_t& result) {
    try {
        std::string cleanExpr = expr;
        // Simple trick: remove spaces to simplify parsing
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
    while (*p == '*' || *p == '/' || *p == '<') { // Added << support as '<'
        char op = *p++;
        if (op == '<' && *p == '<') { // Shift Left
            p++;
            int32_t y = parseFactor(p);
            x <<= y;
        } else if (op == '*') {
            int32_t y = parseFactor(p);
            x *= y;
        } else if (op == '/') {
            int32_t y = parseFactor(p);
            x /= y;
        } else {
             p--; // Backtrack if not <<
             break;
        }
    }
    return x;
}

int32_t Assembler::parseFactor(const char*& p) {
    if (*p == '(') {
        p++;
        int32_t x = parseExpression(p);
        p++; // closing ')'
        return x;
    } else if (*p == '+' || *p == '-') { // Unary operators
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
        // If Pass 1, return 0 (value might not be defined yet)
        // If Pass 2, this is an error if not defined, but we catch it later
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

    // Generate output format (HEX)
    std::stringstream hexOutput;
    for (const auto& inst : instructions) {
        if (inst.bytes.empty()) continue; // Skip directives without bytes (like ORG) or empty lines

        uint8_t count = inst.bytes.size();
        uint16_t addr = inst.address;
        uint8_t type = 0x00; // Data record
        
        int checksum = count + (addr >> 8) + (addr & 0xFF) + type;
        
        hexOutput << ":" 
                  << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << (int)count
                  << std::setw(4) << addr
                  << std::setw(2) << (int)type;
        
        for (uint8_t b : inst.bytes) {
            hexOutput << std::setw(2) << (int)b;
            checksum += b;
        }
        
        checksum = (~checksum + 1) & 0xFF;
        hexOutput << std::setw(2) << checksum << "\n";
    }
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
        for(size_t i=0; i<4; i++) {
            if (i < inst.bytes.size()) 
                ss << std::setw(2) << (int)inst.bytes[i] << " ";
            else 
                ss << "   ";
        }
    }
    
    ss << "    " << inst.originalLine << "\n";
    
    // If more bytes, print continuation lines
    if (inst.bytes.size() > 4) {
        for(size_t i=4; i< inst.bytes.size(); i+=4) {
             ss << "     " << "     ";
             for(size_t j=0; j<4; j++) {
                 if (i+j < inst.bytes.size())
                    ss << std::setw(2) << std::hex << std::setfill('0') << std::uppercase << (int)inst.bytes[i+j] << " ";
                 else
                    ss << "   ";
             }
             ss << "\n";
        }
    }
    
    const_cast<Assembler*>(this)->listingOutput += ss.str();
}

// --- Pass 1 & 2 Implementation ---

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

        // Label
        size_t labelPos = line.find(':');
        if (labelPos != std::string::npos) {
            std::string labelName = trim(line.substr(0, labelPos));
            symbolTable[labelName] = {labelName, (int32_t)currentAddress, LABEL, true};
            line = trim(line.substr(labelPos + 1));
        }
        
        if (line.empty()) continue;
        
        // Tokens
        std::replace(line.begin(), line.end(), ',', ' ');
        std::stringstream ss(line);
        std::string mnemonic;
        ss >> mnemonic;
        mnemonic = toUpper(mnemonic);
        
        if (mnemonic == "ORG") {
            std::string valStr;
            std::getline(ss, valStr);
            int32_t val;
            if (evaluateExpression(valStr, val)) {
                currentAddress = (uint16_t)val;
            }
        } else if (mnemonic == "INCLUDE") {
             // Ignore for now, assuming code is self-contained or user manually merges
             // In a full implementation, we would recursively parse the included file
             continue;
        } else if (mnemonic == "EQU") {
             // Handled better in pre-pass usually, but here we check syntax
             // Label EQU Value
             // In TicTacToe: M_TOP_LEFT EQU 1
             // My logic above handles "Label:" but EQU usually doesn't have colon?
             // Need to check previous label. 
             // Logic flaw: "Label:" definition sets value to currentAddress. 
             // If next word is EQU, update symbol value.
             // This simple parser assumes standard "Label:" syntax.
             // TicTacToe uses "M_TOP_LEFT EQU 1" NO COLON.
             // We need to re-parse the line to handle "Symbol EQU Value"
        } else if (mnemonic == "DB") {
             // Count arguments
             std::string args;
             std::getline(ss, args);
             std::vector<std::string> valStrs = split(args, ' '); // Split by space (commas replaced)
             for(auto& s: valStrs) if (!s.empty()) currentAddress += 1;
        } else if (mnemonic == "DW") {
             std::string args;
             std::getline(ss, args);
             std::vector<std::string> valStrs = split(args, ' ');
             for(auto& s: valStrs) if (!s.empty()) currentAddress += 2;
        } else {
             // Instruction sizing
             // Basic heuristic
             int size = 1;
             
             // Check if it's "Symbol EQU Value"
             if (lines[lineNum-1].find("EQU") != std::string::npos) { // Quick hack check in raw line
                  // "Please re-read line" - re-parsing
                  std::stringstream ss2(rawLine);
                  std::string sym, equ;
                  ss2 >> sym >> equ;
                  if (toUpper(equ) == "EQU") {
                       // Parse value
                       std::string valPart;
                       std::getline(ss2, valPart);
                       int32_t val;
                       if (evaluateExpression(valPart, val)) {
                           // Remove semicolon if present in value
                           symbolTable[sym] = {sym, val, CONSTANT, true};
                           continue; // Don't advance address
                       }
                  }
             }

             if (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.") {
                 // Check operand type roughly
                 std::string op1, op2;
                 ss >> op1 >> op2;
                 if (op2.find('(') != std::string::npos) {
                      if (op2.find("SP") != std::string::npos) size = 2; // (SP+m)
                      else size = 1; // (Rn)
                 } else {
                      size = 3; // Abs/Imm
                 }
             } else if (mnemonic[0] == 'B' && mnemonic.size() == 3) {
                 size = 2;
             } else if (mnemonic == "JMP" || mnemonic == "JSR") {
                 std::string op;
                 ss >> op;
                 if (op.find('(') != std::string::npos) size = 1;
                 else size = 3;
             } 
             // Default 1 for ALU/MOV R,R
             currentAddress += size;
        }
    }
    return true;
}

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
        
        std::string labelName;
        size_t labelPos = line.find(':');
        if (labelPos != std::string::npos) {
            labelName = trim(line.substr(0, labelPos));
            line = trim(line.substr(labelPos + 1));
        }

        if (line.empty()) {
            // Just a label or empty line
            inst.address = currentAddress;
            if (!labelName.empty()) instructions.push_back(inst);
            continue;
        }

        // Re-parse for EQU special case handling
         std::stringstream ss2(line); // clean line without label
         std::string firstTok, secondTok;
         ss2 >> firstTok >> secondTok;
         if (toUpper(secondTok) == "EQU") {
             // Handle EQU in pass 2 (already in symbol table, just output LST info)
             inst.address = symbolTable[firstTok].value;
             inst.isDirective = true;
             instructions.push_back(inst);
             continue;
         }
         
         // Standard parsing
         std::string cleanLine = line;
         std::replace(cleanLine.begin(), cleanLine.end(), ',', ' ');
         std::stringstream ss(cleanLine);
         std::string mnemonic;
         ss >> mnemonic;
         mnemonic = toUpper(mnemonic);
         
         inst.address = currentAddress; // Execution address

         if (mnemonic == "ORG") {
             std::string valStr;
             std::getline(ss, valStr);
             int32_t val;
             if (evaluateExpression(valStr, val)) currentAddress = (uint16_t)val;
             inst.isDirective = true;
             inst.address = currentAddress; // Update to new org
         } 
         else if (mnemonic == "INCLUDE") {
             inst.isDirective = true;
             // No bytes, just skip
         }
         else if (mnemonic == "DB") {
             std::string args;
             std::getline(ss, args);
             std::vector<std::string> vals = split(args, ' '); 
             for(auto& v : vals) {
                 if(v.empty()) continue;
                 int32_t val;
                 if(evaluateExpression(v, val)) inst.bytes.push_back((uint8_t)val);
             }
             currentAddress += inst.bytes.size();
         }
         else if (mnemonic == "DW") {
             std::string args;
             std::getline(ss, args);
             std::vector<std::string> vals = split(args, ' ');
             for(auto& v : vals) {
                 if(v.empty()) continue;
                 int32_t val;
                 if(evaluateExpression(v, val)) {
                     // Little Endian
                     inst.bytes.push_back((uint8_t)(val & 0xFF));
                     inst.bytes.push_back((uint8_t)((val >> 8) & 0xFF));
                 }
             }
             currentAddress += inst.bytes.size();
         }
         else {
             // Instruction encoding
             std::string op1, op2;
             ss >> op1 >> op2;
             
             std::vector<uint8_t>& b = inst.bytes;
             
             if (opcodeMap.count(mnemonic) && mnemonic[0] == 'B') {
                 // Branch
                 int32_t target;
                 if(!evaluateExpression(op1, target)) { error="Invalid branch target"; return false; }
                 int offset = target - (currentAddress + 2);
                 b.push_back(opcodeMap[mnemonic]);
                 b.push_back((uint8_t)offset);
             }
             else if (mnemonic == "JMP" || mnemonic == "JSR") {
                 // JMP (Rx) or JMP addr
                 if (op1[0] == '(') {
                     // Indirect
                     b.push_back(mnemonic == "JMP" ? 0xF2 : 0xCE); // Simp
                     // Check reg inside ()
                 } else {
                     // Absolute
                     int32_t target;
                     evaluateExpression(op1, target);
                     b.push_back(mnemonic == "JMP" ? 0xF3 : 0xCD); // Opcode (Check spec: F3 for JMP, CD for JSR)
                     b.push_back((uint8_t)(target & 0xFF));
                     b.push_back((uint8_t)((target >> 8) & 0xFF));
                 }
             }
             else if (mnemonic.substr(0, 3) == "LD." || mnemonic.substr(0, 3) == "ST.") {
                 // Complex Logic for loads
                 bool isStore = mnemonic[0] == 'S';
                 bool isByte = mnemonic[3] == 'B';
                 
                 int r = parseRegister(op1); // Dest/Src Register
                 
                 // Op2 parsing
                 if (op2[0] == '(') {
                      // Indirect variants
                      b.push_back(0x80); // Placeholder group 80, 90, A0
                 } else if (op2[0] == '#') {
                      // Immediate
                      int32_t val;
                      evaluateExpression(op2.substr(1), val);
                      b.push_back(0xE0); // Placeholder group E0/D0
                      b.push_back((uint8_t)(val & 0xFF));
                      b.push_back((uint8_t)((val >> 8) & 0xFF));
                 } else {
                      // Absolute
                      int32_t addr;
                      evaluateExpression(op2, addr);
                      b.push_back(0xB0); // Placeholder Group B0
                      b.push_back((uint8_t)(addr & 0xFF));
                      b.push_back((uint8_t)((addr >> 8) & 0xFF));
                 }
             }
             else if (mnemonic == "MOV" || mnemonic == "ADD") {
                  // ALU
                  int r1 = parseRegister(op1);
                  int r2 = parseRegister(op2);
                  b.push_back(0x00 | (r1 << 2) | r2); // Simplification from Pass 1 logic
             }
             else if (mnemonic == "RET") b.push_back(0xC6);
             else if (mnemonic == "RETI") b.push_back(0xC7);
             else if (mnemonic == "NOP") b.push_back(0xFF);
             else b.push_back(0xFF); // Unhandled default
             
             currentAddress += b.size();
         }
         instructions.push_back(inst);
    }
    return true;
}
