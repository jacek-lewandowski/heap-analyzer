#include <iostream>
#include "jvmti.h"
#include "utils.h"
#include "net_enigma_test_toolkit_TestToolkitAgent.h"

using namespace std;

static GlobalAgentData *global_agent_data;

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
        cerr << "ERROR: Failed to add capability to tag objects, error is " << error << endl;
    }

    global_agent_data = (GlobalAgentData *) malloc(sizeof(GlobalAgentData));
    global_agent_data->jvmti = jvmti;
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
    check_jvmti_error(env, global_agent_data->jvmti->IterateThroughHeap(0, cls, &callbacks, &count));

    return count;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_forceGC(JNIEnv *env, jclass thisClass)
{
    check_jvmti_error(env, global_agent_data->jvmti->ForceGarbageCollection());
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
    global_agent_data->jvmti->GetTag(object, &tag);
    global_agent_data->jvmti->SetTag(object, tag | 0x80000000);

    int count = 0;
    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_reference_callback = &count_live_references_callback;
    check_jvmti_error(env, global_agent_data->jvmti->FollowReferences(0, NULL, NULL, &callbacks, &count));
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
    check_jvmti_error(env, global_agent_data->jvmti->IterateThroughHeap(
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
    jboolean debug;
} ReferenceTraversalActionData;

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
    ReferenceTraversalActionData *action_data = (ReferenceTraversalActionData *) user_data;
    if (*tag_ptr == action_data->tag)
    {
        action_data->size += size;
        action_data->count += 1;
        *tag_ptr = *tag_ptr | 0x80000000;
        if (action_data->debug)
        {
            cerr << "Tag " << action_data->tag << " reference kind " << reference_kind;
            if (reference_kind == JVMTI_HEAP_REFERENCE_STACK_LOCAL)
            {
                cerr << " / stack local: ";
                cerr << " depth=" << reference_info->stack_local.depth;
                cerr << " threadId=" << reference_info->stack_local.thread_id;
            } else if (reference_kind == JVMTI_HEAP_REFERENCE_FIELD)
            {
                cerr << " / field: ";
                cerr << " idx=" << reference_info->field.index;
            }
            cerr << endl;
        }
    }
    return JVMTI_VISIT_OBJECTS;
}

void traverse_live_tagged_objects(
        JNIEnv *env,
        jlong tag,
        ReferenceTraversalActionData *data)
{
    data->size = 0;
    data->count = 0;
    data->tag = tag & 0x7FFFFFFFL;

    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_reference_callback = &count_size_of_live_tagged_objects_callback;

    check_jvmti_error(env, global_agent_data->jvmti->FollowReferences(
            JVMTI_HEAP_FILTER_UNTAGGED,
            NULL,
            NULL,
            &callbacks,
            data));

    clean_objects_tag(env);
}

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_countLiveTaggedObjects(
        JNIEnv *env,
        jclass interface_class,
        jlong tag,
        jboolean debug_references)
{
    ReferenceTraversalActionData data;
    data.debug = debug_references;
    traverse_live_tagged_objects(env, tag, &data);
    return data.count;
}

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_countSizeOfLiveTaggedObjects(
        JNIEnv *env,
        jclass interface_class,
        jlong tag,
        jboolean debug_references)
{
    ReferenceTraversalActionData data;
    data.debug = debug_references;
    traverse_live_tagged_objects(env, tag, &data);
    return data.size;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_setTag__Ljava_lang_Object_2J(
        JNIEnv *env,
        jclass interface_class,
        jobject object,
        jlong tag)
{
    check_jvmti_error(env, global_agent_data->jvmti->SetTag(object, tag & 0x7FFFFFFF));
}

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_getTag(JNIEnv *env, jclass interface_class, jobject object)
{
    jlong tag;
    check_jvmti_error(env, global_agent_data->jvmti->GetTag(object, &tag));
    return tag & 0x7FFFFFFF;
}

struct SetTagActionData
{
    long cur_tag;
    long new_tag;
};

jint tag_action_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    SetTagActionData *action_data = (SetTagActionData *) user_data;
    if (action_data->cur_tag == 0 || *tag_ptr == action_data->cur_tag)
        *tag_ptr = action_data->new_tag;
    return JVMTI_VISIT_OBJECTS;
}

void Java_net_enigma_test_toolkit_TestToolkitAgent_setTag__JJ(
        JNIEnv *env,
        jclass interface_class,
        jlong cur_tag,
        jlong new_tag)
{
    SetTagActionData tag_action_data;
    tag_action_data.cur_tag = cur_tag;
    tag_action_data.new_tag = new_tag;

    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_iteration_callback = &tag_action_callback;
    check_jvmti_error(env, global_agent_data->jvmti->IterateThroughHeap(0, NULL, &callbacks, &tag_action_data));
}
