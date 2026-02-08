#include <jni.h>
#include <string>
#include "assembler.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_diamon_megaprocessor_NativeAssembler_assemble(
        JNIEnv* env,
        jobject /* this */,
        jstring sourceCode) {
    
    const char* nativeString = env->GetStringUTFChars(sourceCode, 0);
    std::string source(nativeString);
    env->ReleaseStringUTFChars(sourceCode, nativeString);

    Assembler assembler;
    std::string result = assembler.assemble(source);

    return env->NewStringUTF(result.c_str());
}