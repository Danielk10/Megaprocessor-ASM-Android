#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <android/log.h>

#define LOG_TAG "MegaprocessorASM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::string trim(const std::string& str);
std::string toUpper(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);

#endif // UTILS_H
