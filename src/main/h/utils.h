#ifndef TEST_TOOLKIT_NATIVE_AGENT_UTILS_H
#define TEST_TOOLKIT_NATIVE_AGENT_UTILS_H

#include <string>
#include <sstream>
#include <iostream>

#include "jni.h"
#include "jvmti.h"

using namespace std;

typedef struct
{
    jvmtiEnv *jvmti;
} GlobalAgentData;

const char *NATIVE_METHOD_EXCEPTION = "com/sun/jdi/NativeMethodException";

jint throw_native_method_exception(JNIEnv *env, const char *message)
{
    jclass exception_class;

    exception_class = env->FindClass(NATIVE_METHOD_EXCEPTION);
    if (exception_class == NULL)
    {
        cerr << "ERROR: Could not throw NativeMethodException because such class could not be found" << endl;
        return JNI_ERR;
    }

    return env->ThrowNew(exception_class, message);
}

void check_jvmti_error(JNIEnv *env, jvmtiError error)
{
    if (error != JVMTI_ERROR_NONE)
    {
        char message[30];
        sprintf(message, "JVMTI error %d", error);
        throw_native_method_exception(env, message);
    }
}

jvmtiHeapCallbacks* clean_callbacks(jvmtiHeapCallbacks& callbacks)
{
    (void) memset(&callbacks, 0, sizeof(jvmtiHeapCallbacks));
    return &callbacks;
}

#endif //TEST_TOOLKIT_NATIVE_AGENT_UTILS_H
