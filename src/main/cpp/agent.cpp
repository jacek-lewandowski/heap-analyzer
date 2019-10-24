#include <iostream>
#include "jvmti.h"
#include "net_enigma_test_toolkit_TestToolkitAgent.h"
#include <unordered_map>

using namespace std;

// ----------------------------------------------------
// --------======== Agent loading code ========--------
// ----------------------------------------------------

jvmtiEnv *jvmti_env = NULL;
jclass runtime_exception_class = NULL;
jclass heap_traversal_summary_class = NULL;

void _add_capabilities(jvmtiError *error, jvmtiEnv *jvmti)
{
    if (*error) return;

    jvmtiCapabilities capabilities;
    memset(&capabilities, 0, sizeof(jvmtiCapabilities));
    capabilities.can_tag_objects = 1;
    capabilities.can_get_line_numbers = 1;
    capabilities.can_get_source_file_name = 1;

    *error = jvmti->AddCapabilities(&capabilities);
    if (*error)
    {
        cerr << "ERROR: Failed to add capabilities, error is " << *error << endl;
        return;
    }
}

jint Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    jvmtiEnv *jvmti = NULL;
    jvmtiError error = JVMTI_ERROR_NONE;

    jint result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_2);
    if (result != JNI_OK)
    {
        cerr << "ERROR: Unable to access JVMTI!" << endl;
        return result;
    }

    _add_capabilities(&error, jvmti);

    if (error) return JNI_ERR;

    jvmti_env = jvmti;
    return JNI_OK;
}

jclass _find_class(JNIEnv *env, const char *name) {
    jclass ref = env->FindClass(name);
    if (ref) {
        jclass global_ref = (jclass) env->NewGlobalRef(ref);
        return global_ref;
    }
    return NULL;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_init(JNIEnv *env, jclass interface_class)
{
    if (!runtime_exception_class)
        runtime_exception_class = _find_class(env, "java/lang/RuntimeException");
    if (!heap_traversal_summary_class)
        heap_traversal_summary_class = _find_class(env, "net/enigma/test/toolkit/HeapTraversalSummary");
}
