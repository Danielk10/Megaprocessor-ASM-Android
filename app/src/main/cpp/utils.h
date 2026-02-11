#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

#define LOG_TAG "MegaprocessorASM"
#ifdef __ANDROID__
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do { (void)0; } while (0)
#define LOGE(...) do { std::fprintf(stderr, __VA_ARGS__); std::fprintf(stderr, "\n"); } while (0)
#endif

std::string trim(const std::string& str);
std::string toUpper(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);

#endif // UTILS_H
