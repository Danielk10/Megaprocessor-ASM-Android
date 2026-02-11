#include "assembler.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string readFile(const fs::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("No se pudo abrir: " + filePath.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeFile(const fs::path& filePath, const std::string& content) {
    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("No se pudo escribir: " + filePath.string());
    }
    output << content;
}

std::string normalizeIncludeToken(std::string token) {
    token.erase(std::remove(token.begin(), token.end(), '"'), token.end());
    token.erase(std::remove(token.begin(), token.end(), '\''), token.end());
    token.erase(std::remove(token.begin(), token.end(), ';'), token.end());

    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
        token.pop_back();
    }
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
        token.erase(token.begin());
    }

    return token;
}

fs::path resolveIncludePath(const std::string& includeName, const fs::path& asmDir, const fs::path& projectRoot) {
    const fs::path includePath(includeName);

    std::vector<fs::path> candidates = {
        asmDir / includePath,
        projectRoot / includePath,
        projectRoot / "app" / "src" / "main" / "assets" / includePath,
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            return fs::canonical(candidate);
        }
    }

    throw std::runtime_error("Include no encontrado: " + includeName);
}

void loadIncludeRecursive(const fs::path& includeFile,
                         const fs::path& projectRoot,
                         std::map<std::string, std::string>& includeFiles,
                         std::set<std::string>& visited) {
    const std::string canonical = fs::canonical(includeFile).string();
    if (!visited.insert(canonical).second) {
        return;
    }

    const std::string content = readFile(includeFile);
    includeFiles[includeFile.filename().string()] = content;

    static const std::regex includeRegex(R"(^\s*include\s+([^\s]+))", std::regex::icase);
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        std::smatch match;
        if (!std::regex_search(line, match, includeRegex)) {
            continue;
        }

        const std::string includeName = normalizeIncludeToken(match[1].str());
        if (includeName.empty()) {
            continue;
        }

        const fs::path nestedInclude = resolveIncludePath(includeName, includeFile.parent_path(), projectRoot);
        loadIncludeRecursive(nestedInclude, projectRoot, includeFiles, visited);
    }
}

void printUsage(const char* programName) {
    std::cerr << "Uso: " << programName << " <archivo.asm> [--lst] [--out <archivo.hex>] [--lst-out <archivo.lst>]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        fs::path asmPath;
        fs::path hexOutputPath;
        fs::path lstOutputPath;
        bool writeListing = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--lst") {
                writeListing = true;
                continue;
            }
            if (arg == "--out" && i + 1 < argc) {
                hexOutputPath = argv[++i];
                continue;
            }
            if (arg == "--lst-out" && i + 1 < argc) {
                lstOutputPath = argv[++i];
                writeListing = true;
                continue;
            }
            if (!arg.empty() && arg[0] == '-') {
                std::cerr << "Argumento no reconocido: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
            if (!asmPath.empty()) {
                std::cerr << "Solo se permite un archivo .asm de entrada\n";
                return 1;
            }
            asmPath = arg;
        }

        if (asmPath.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        asmPath = fs::absolute(asmPath);
        if (!fs::exists(asmPath) || !fs::is_regular_file(asmPath)) {
            std::cerr << "Archivo de entrada no existe: " << asmPath << "\n";
            return 1;
        }

        if (hexOutputPath.empty()) {
            hexOutputPath = asmPath;
            hexOutputPath.replace_extension(".hex");
        }

        if (writeListing && lstOutputPath.empty()) {
            lstOutputPath = asmPath;
            lstOutputPath.replace_extension(".lst");
        }

        const fs::path asmDir = asmPath.parent_path();
        const fs::path projectRoot = fs::canonical(fs::path(PROJECT_ROOT_PATH));

        std::map<std::string, std::string> includeFiles;
        std::set<std::string> visited;

        // Cargar include por defecto si existe.
        try {
            const fs::path defaultDefs = resolveIncludePath("Megaprocessor_defs.asm", asmDir, projectRoot);
            loadIncludeRecursive(defaultDefs, projectRoot, includeFiles, visited);
        } catch (const std::exception&) {
            // Opcional: algunos programas no lo usan.
        }

        // Cargar includes declarados por el asm principal.
        const std::string source = readFile(asmPath);
        static const std::regex includeRegex(R"(^\s*include\s+([^\s]+))", std::regex::icase);
        std::istringstream stream(source);
        std::string line;
        while (std::getline(stream, line)) {
            std::smatch match;
            if (!std::regex_search(line, match, includeRegex)) {
                continue;
            }
            const std::string includeName = normalizeIncludeToken(match[1].str());
            if (includeName.empty()) {
                continue;
            }
            const fs::path includePath = resolveIncludePath(includeName, asmDir, projectRoot);
            loadIncludeRecursive(includePath, projectRoot, includeFiles, visited);
        }

        Assembler assembler;
        assembler.setIncludeFiles(includeFiles);
        const std::string hex = assembler.assemble(source);
        if (hex.rfind("ERROR:", 0) == 0) {
            std::cerr << hex << "\n";
            return 2;
        }

        writeFile(hexOutputPath, hex);
        if (writeListing) {
            writeFile(lstOutputPath, assembler.getListing());
        }

        std::cout << "HEX generado: " << fs::absolute(hexOutputPath) << "\n";
        if (writeListing) {
            std::cout << "LST generado: " << fs::absolute(lstOutputPath) << "\n";
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fallo: " << ex.what() << "\n";
        return 1;
    }
}
