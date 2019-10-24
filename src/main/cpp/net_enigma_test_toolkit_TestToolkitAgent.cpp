// In order to compile on MacOS:
// clang -shared -undefined dynamic_lookup -o agent.so -I $JAVA_HOME/include -I $JAVA_HOME/include/darwin agent.cpp

#include <iostream>
#include "jvmti.h"
#include "utils.h"
#include "net_enigma_test_toolkit_TestToolkitAgent.h"

using namespace std;

static GlobalAgentData *globalAgentData;

jint Agent_OnLoad(JavaVM *jvm, char *options, void *reserved)
{
    jvmtiEnv *jvmti = NULL;
    jvmtiCapabilities capabilities;
    jvmtiError error;

    jint result = jvm->GetEnv((void **) &jvmti, JVMTI_VERSION_1_1);
    if (result != JNI_OK)
    {
        cerr << "ERROR: Unable to access JVMTI!" << endl;
        return result;
    }

    (void) memset((void *) &capabilities, 0, sizeof(jvmtiCapabilities));
    capabilities.can_tag_objects = 1;
    error = (jvmti)->AddCapabilities(&capabilities);
    if (error)
    {
        cout << "ERROR: Failed to add capability to tag objects, error is " << error << endl;
    }

    globalAgentData = (GlobalAgentData *) malloc(sizeof(GlobalAgentData));
    globalAgentData->jvmti = jvmti;
    return JNI_OK;
}

jint count_instances_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    int *count = (int *) user_data;
    *count += 1;
    return JVMTI_VISIT_OBJECTS;
}

jint Java_net_enigma_test_toolkit_TestToolkitAgent_countInstances(JNIEnv *env, jclass thisClass, jclass cls)
{
    int count = 0;
    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_iteration_callback = &count_instances_callback;
    check_jvmti_error(env, globalAgentData->jvmti->IterateThroughHeap(0, cls, &callbacks, &count));

    return count;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_forceGC(JNIEnv *env, jclass thisClass)
{
    check_jvmti_error(env, globalAgentData->jvmti->ForceGarbageCollection());
}

jint count_live_references_callback(
        jvmtiHeapReferenceKind reference_kind,
        const jvmtiHeapReferenceInfo *reference_info,
        jlong class_tag,
        jlong referrer_class_tag,
        jlong size,
        jlong *tag_ptr,
        jlong *referrer_tag_ptr,
        jint length,
        void *user_data)
{
    int *count = (int *) user_data;
    if (*tag_ptr & 0x80000000) *count += 1;
    return JVMTI_VISIT_OBJECTS;
}

jint Java_net_enigma_test_toolkit_TestToolkitAgent_countLiveReferences(JNIEnv *env, jclass thisClass, jobject object)
{
    jlong tag;
    globalAgentData->jvmti->GetTag(object, &tag);
    globalAgentData->jvmti->SetTag(object, tag | 0x80000000);

    int count = 0;
    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_reference_callback = &count_live_references_callback;
    check_jvmti_error(env, globalAgentData->jvmti->FollowReferences(0, NULL, NULL, &callbacks, &count));
    return count;
}

jint clean_objects_tag_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    *tag_ptr &= 0x7FFFFFFF;
    return JVMTI_VISIT_OBJECTS;
}

void clean_objects_tag(JNIEnv *env)
{
    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_iteration_callback = &clean_objects_tag_callback;
    check_jvmti_error(env, globalAgentData->jvmti->IterateThroughHeap(
            JVMTI_HEAP_FILTER_UNTAGGED,
            NULL,
            &callbacks,
            NULL));
}

typedef struct
{
    jlong size;
    jlong count;
    jlong tag;
} SizeCountData;

jint count_size_of_live_tagged_objects_callback(
        jvmtiHeapReferenceKind reference_kind,
        const jvmtiHeapReferenceInfo *reference_info,
        jlong class_tag,
        jlong referrer_class_tag,
        jlong size,
        jlong *tag_ptr,
        jlong *referrer_tag_ptr,
        jint length,
        void *user_data)
{
    SizeCountData *data = (SizeCountData *) user_data;
    if (*tag_ptr == data->tag)
    {
        data->size += size;
        data->count += 1;
        *tag_ptr = *tag_ptr | 0x80000000;
    }
    return JVMTI_VISIT_OBJECTS;
}

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_countSizeOfLiveTaggedObjects(
        JNIEnv *env,
        jclass interface_class,
        jlong tag)
{
    SizeCountData data;
    data.size = 0;
    data.count = 0;
    data.tag = tag & 0x7FFFFFFFL;

    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_reference_callback = &count_size_of_live_tagged_objects_callback;

    check_jvmti_error(env, globalAgentData->jvmti->FollowReferences(
            JVMTI_HEAP_FILTER_UNTAGGED,
            NULL,
            NULL,
            &callbacks,
            &data));

    clean_objects_tag(env);

    return data.size;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_setTag(
        JNIEnv *env,
        jclass interface_class,
        jobject object,
        jlong tag)
{
    check_jvmti_error(env, globalAgentData->jvmti->SetTag(object, tag & 0x7FFFFFFF));
}

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_getTag(JNIEnv *env, jclass interface_class, jobject object)
{
    jlong tag;
    check_jvmti_error(env, globalAgentData->jvmti->GetTag(object, &tag));
    return tag & 0x7FFFFFFF;
}
