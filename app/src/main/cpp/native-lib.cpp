#include <jni.h>
#include <string>
#include "assembler.h"
#include "utils.h"

// Global static for MVP
static std::string lastListing = "";

extern "C" JNIEXPORT jstring JNICALL
Java_com_diamon_megaprocessor_NativeAssembler_getListing(
        JNIEnv* env,
        jobject /* this */) {

    LOGI("JNI: getListing called");
    return env->NewStringUTF(lastListing.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_diamon_megaprocessor_NativeAssembler_assemble(
        JNIEnv* env,
        jobject /* this */,
        jstring sourceCode) {

    LOGI("JNI: assemble called");

    if (sourceCode == NULL) {
        LOGE("JNI: sourceCode is NULL");
        return env->NewStringUTF("ERROR: Source code is null");
    }

    const char* nativeString = env->GetStringUTFChars(sourceCode, 0);
    std::string source(nativeString);
    env->ReleaseStringUTFChars(sourceCode, nativeString);

    LOGI("JNI: Source length: %zu characters", source.length());

    Assembler assembler;
    std::string result = assembler.assemble(source);
    
    lastListing = assembler.getListing();

    if (result.find("ERROR") == 0) {
        LOGE("JNI: Assembly failed with result: %s", result.c_str());
    } else {
        LOGI("JNI: Assembly successful. Result length: %zu", result.length());
    }

    return env->NewStringUTF(result.c_str());
}