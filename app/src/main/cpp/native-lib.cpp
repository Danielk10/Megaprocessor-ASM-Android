#include <jni.h>
#include <string>
#include "assembler.h"

// Global static for MVP
static std::string lastListing = "";

extern "C" JNIEXPORT jstring JNICALL
Java_com_diamon_megaprocessor_NativeAssembler_getListing(
        JNIEnv* env,
        jobject /* this */) {

    // Ideally we should have a persistent object or pass the context
    // But since `assemble` is stateless in previous implementaton, we can't easily get the listing 
    // from a separate call unless we store it globally or return a complex object.
    
    // BETTER APPROACH: Return a JSON string from 'assemble' containing both HEX and LST?
    // OR: Temporarily store the last listing in a static variable (simple but not thread safe)
    // Given the constraints and typical Android single-user usage, static is "okay" for MVP testing.
    
    return env->NewStringUTF(lastListing.c_str());
}

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
    
    lastListing = assembler.getListing();

    return env->NewStringUTF(result.c_str());
}