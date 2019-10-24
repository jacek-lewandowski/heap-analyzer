#include <iostream>
#include "jvmti.h"
#include "utils.h"
#include "net_enigma_test_toolkit_TestToolkitAgent.h"
#include <unordered_map>

using namespace std;

struct SetTagActionData
{
    long cur_tag = 0;
    long new_tag = 0;
};

typedef struct
{
    jlong size = 0;
    jlong count = 0;
    jlong marked_size = 0;
    jlong marked_count = 0;
} FollowRefsData;

jint _reset_tags_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    *tag_ptr &= MARKER_BIT | CLASS_BIT;
    return JVMTI_VISIT_OBJECTS;
}

void _reset_tags()
{
    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_iteration_callback = &_reset_tags_callback;
    jvmti_env->IterateThroughHeap(JVMTI_HEAP_FILTER_UNTAGGED, NULL, &callbacks, NULL);
}


// -------- forceGC --------

void Java_net_enigma_test_toolkit_TestToolkitAgent_forceGC(JNIEnv *env, jclass thisClass)
{
    throw_exception_if_error(env, jvmti_env->ForceGarbageCollection());
}

// -------- traverseHeap --------

jint _follow_refs_callback(
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
    FollowRefsData *data = (FollowRefsData *) user_data;

    // this object is referred from the object of class which is flagged
    if (referrer_class_tag & SKIP_REFS_FROM_BIT)
        return JVMTI_VISIT_ABORT;

    // this object was already visited or it is a class which do not care much about here
    if ((*tag_ptr) & VISIT_BIT || (*tag_ptr) & CLASS_BIT) return JVMTI_VISIT_OBJECTS;

    data->size += size;
    data->count += 1;

    if ((*tag_ptr) & MARKER_BIT)
    {
        data->marked_size += size;
        data->marked_count += 1;
    }

    *tag_ptr |= VISIT_BIT;
    return JVMTI_VISIT_OBJECTS;
}

jobject Java_net_enigma_test_toolkit_TestToolkitAgent_traverseHeap(JNIEnv *env, jclass interface_class)
{
    _reset_tags();

    FollowRefsData data;

    jvmtiHeapCallbacks callbacks;
    clean_callbacks(callbacks)->heap_reference_callback = &_follow_refs_callback;

    jvmtiError error = jvmti_env->FollowReferences(0, NULL, NULL, &callbacks, &data);
    throw_exception_if_error(env, error);

    if (heap_traversal_summary_class == NULL)
    {
        throw_runtime_exception(env, "Failed to get HeapTraversalSummary class");
        return NULL;
    }
    jmethodID heap_traversal_summary_ctor = env->GetMethodID(heap_traversal_summary_class, "<init>", "(JJJJ)V");
    if (heap_traversal_summary_ctor == NULL)
    {
        throw_runtime_exception(env, "Failed to get HeapTraversalSummary constructor");
        return NULL;
    }
    jobject summary = env->NewObject(heap_traversal_summary_class, heap_traversal_summary_ctor, data.size, data.count,
                                     data.marked_size,
                                     data.marked_count);
    return summary;
}

// -------- setTag(Object, long) --------

void Java_net_enigma_test_toolkit_TestToolkitAgent_setTag__Ljava_lang_Object_2J(
        JNIEnv *env,
        jclass interface_class,
        jobject object,
        jlong tag)
{
    throw_exception_if_error(env, jvmti_env->SetTag(object, tag));
}

// -------- getTag(Object) --------

jlong Java_net_enigma_test_toolkit_TestToolkitAgent_getTag(JNIEnv *env, jclass interface_class, jobject object)
{
    jlong tag = 0;
    throw_exception_if_error(env, jvmti_env->GetTag(object, &tag));
    return tag;
}

// -------- setTag(long, long) --------

jint _tagging_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    SetTagActionData *data = (SetTagActionData *) user_data;
    if (data->cur_tag == 0 || *tag_ptr == data->cur_tag) *tag_ptr = data->new_tag;
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
    clean_callbacks(callbacks)->heap_iteration_callback = &_tagging_callback;
    throw_exception_if_error(env, jvmti_env->IterateThroughHeap(0, NULL, &callbacks, &tag_action_data));
}

// -------- markObject(Object) --------

void Java_net_enigma_test_toolkit_TestToolkitAgent_markObject(JNIEnv *env, jclass interface_class, jobject obj)
{
    jlong tag = 0;
    throw_exception_if_error(env, jvmti_env->GetTag(obj, &tag));
    throw_exception_if_error(env, jvmti_env->SetTag(obj, tag | MARKER_BIT));
}

// -------- skipRefsFromClassesBySubstring(String) --------

void Java_net_enigma_test_toolkit_TestToolkitAgent_skipRefsFromClassesBySubstring(JNIEnv *env, jclass interface_class,
                                                                                  jstring pattern_string)
{
    const char *pattern = env->GetStringUTFChars(pattern_string, 0);

    jint tagged = 0;
    jint cnt = 0;
    jclass *classes = NULL;
    jvmti_env->GetLoadedClasses(&cnt, &classes);
    if (classes != NULL)
    {
        char *signature = NULL;
        for (jint i = 0; i < cnt; i++)
        {
            jvmti_env->GetClassSignature(classes[i], &signature, NULL);
            if (signature != NULL && strstr(signature, pattern))
            {
                long tag = 0;
                jvmti_env->GetTag(classes[i], &tag);
                jvmti_env->SetTag(classes[i], tag | SKIP_REFS_FROM_BIT);
                tagged++;
            }
            jvmti_env->Deallocate((unsigned char *) signature);
        }
        jvmti_env->Deallocate((unsigned char *) classes);
    }

    env->ReleaseStringUTFChars(pattern_string, pattern);
}
