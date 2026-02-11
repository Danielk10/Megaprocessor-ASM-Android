#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "../../app/src/main/cpp/assembler.h"
#include "../../app/src/main/cpp/utils.h"

namespace fs = std::filesystem;

std::string readFile(const fs::path& path) {
    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void loadIncludeFiles(const fs::path& includeRoot, std::map<std::string, std::string>& includes) {
    if (!fs::exists(includeRoot)) return;

    for (const auto& entry : fs::recursive_directory_iterator(includeRoot)) {
        if (!entry.is_regular_file()) continue;
        std::string content = readFile(entry.path());

        std::string baseName = toUpper(trim(entry.path().filename().string()));
        includes[baseName] = content;

        std::string relName = toUpper(trim(fs::relative(entry.path(), includeRoot).generic_string()));
        includes[relName] = content;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: assemble_cli <source.asm> [include_dir]" << std::endl;
        return 2;
    }

    fs::path sourcePath = argv[1];
    fs::path includeDir = argc == 3 ? fs::path(argv[2]) : sourcePath.parent_path() / "includes";

    std::string source = readFile(sourcePath);
    std::map<std::string, std::string> includes;
    loadIncludeFiles(includeDir, includes);

    Assembler assembler;
    assembler.setIncludeFiles(includes);

    std::string output = assembler.assemble(source);
    if (output.rfind("ERROR:", 0) == 0) {
        std::cerr << output << std::endl;
        return 1;
    }

    std::cout << output;
    return 0;
}
