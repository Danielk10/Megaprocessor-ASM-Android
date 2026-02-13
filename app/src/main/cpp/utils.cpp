#include "utils.h"
#include <sstream>
#include <algorithm>

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string toUpper(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (unsigned char c : str) {
        if (c >= 'a' && c <= 'z') result += (char)(c - 'a' + 'A');
        else if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.' || c == '#' || c == '(' || c == ')' || c == '+' || c == '-') 
            result += (char)c;
        // Skip non-printable or suspicious chars for symbols
    }
    return result;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    // Handle case where string ends with delimiter
    if (!str.empty() && str.back() == delimiter) {
        tokens.push_back("");
    }
    return tokens;
}
