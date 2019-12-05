#include <string>
#include <sstream>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <cstring>

#include "jni.h"
#include "jvmti.h"
#include "utils.h"

using namespace std;

jint throw_runtime_exception(JNIEnv *env, const char *message)
{
    if (runtime_exception_class == NULL)
    {
        cerr << "ERROR: Could not throw RuntimeException because such class could not be found" << endl;
        return JNI_ERR;
    }

    return env->ThrowNew(runtime_exception_class, message);
}

void throw_exception_if_error(JNIEnv *env, jvmtiError error)
{
    if (error != JVMTI_ERROR_NONE)
    {
        char message[30];
        sprintf(message, "JVMTI error %d", error);
        throw_runtime_exception(env, message);
    }
}

jvmtiHeapCallbacks *clean_callbacks(jvmtiHeapCallbacks &callbacks)
{
    (void) memset(&callbacks, 0, sizeof(jvmtiHeapCallbacks));
    return &callbacks;
}

void _release_jvmti_object(void **pointer) {
    if (*pointer != NULL)
    {
        jvmti_env->Deallocate((unsigned char *) *pointer);
        *pointer = NULL;
    }
}

void _release_jvmti_objects(jint count, ...)
{
    va_list pointers;
    va_start(pointers, count);

    for (int i=0; i<count; i++) {
        void **p = va_arg(pointers, void**);
        _release_jvmti_object(p);
    }

    va_end(pointers);
}
