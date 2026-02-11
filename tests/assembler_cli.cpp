#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "app/src/main/cpp/assembler.h"

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Unable to open file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: assembler_cli <source.asm> [include_name include_path]...\n";
        return 2;
    }

    try {
        Assembler assembler;
        if (argc > 2) {
            if (((argc - 2) % 2) != 0) {
                std::cerr << "Include arguments must be pairs: <include_name include_path>\n";
                return 2;
            }
            std::map<std::string, std::string> includeFiles;
            for (int i = 2; i < argc; i += 2) {
                includeFiles[argv[i]] = readFile(argv[i + 1]);
            }
            assembler.setIncludeFiles(includeFiles);
        }

        std::string source = readFile(argv[1]);
        std::string result = assembler.assemble(source);
        if (result.rfind("ERROR:", 0) == 0) {
            std::cerr << result << "\n";
            return 1;
        }

        std::cout << result;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }
}
