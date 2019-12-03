#include <iostream>
#include "jvmti.h"
#include "utils.h"
#include "net_enigma_test_toolkit_TestToolkitAgent.h"
#include <unordered_map>
#include <vector>
#include "debug_references.h"
#include <string.h>

using namespace std;

RefInfo *_create_ref_info(
        const jlong referrer_class_tag,
        const jlong referrer_tag,
        const jvmtiHeapReferenceKind reference_kind,
        const jvmtiHeapReferenceInfo *reference_info)
{
    RefInfo *ref_info = (RefInfo *) calloc(1, sizeof(RefInfo));
    ref_info->referrer_class = referrer_class_tag & L_BITS;
    ref_info->referrer_tag = referrer_tag;
    ref_info->kind = reference_kind;
    switch (reference_kind)
    {
        case JVMTI_HEAP_REFERENCE_FIELD:
        case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
            ref_info->info = (jvmtiHeapReferenceInfo *) calloc(1, sizeof(jvmtiHeapReferenceInfo));
            ref_info->info->field = reference_info->field;
            break;
        case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
            ref_info->info = (jvmtiHeapReferenceInfo *) calloc(1, sizeof(jvmtiHeapReferenceInfo));
            ref_info->info->array = reference_info->array;
            break;
        case JVMTI_HEAP_REFERENCE_CONSTANT_POOL:
            ref_info->info = (jvmtiHeapReferenceInfo *) calloc(1, sizeof(jvmtiHeapReferenceInfo));
            ref_info->info->constant_pool = reference_info->constant_pool;
            break;
        case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
            ref_info->info = (jvmtiHeapReferenceInfo *) calloc(1, sizeof(jvmtiHeapReferenceInfo));
            ref_info->info->jni_local = reference_info->jni_local;
            break;
        case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
            ref_info->info = (jvmtiHeapReferenceInfo *) calloc(1, sizeof(jvmtiHeapReferenceInfo));
            ref_info->info->stack_local = reference_info->stack_local;
            break;
        default:
            break;
    }
    return ref_info;
}

void _release_ref_info(RefInfo *ref_info)
{
    if (ref_info == NULL) return;
    if (ref_info->info != NULL)
    {
        free(ref_info->info);
        ref_info->info = NULL;
    }
    free(ref_info);
}

vector<RefInfo *> *_get_or_create_refs_vector(vector<RefInfo *> **referrerss, jlong idx)
{
    vector<RefInfo *> *referrers = referrerss[idx];
    if (referrers == NULL)
    {
        referrers = (vector<RefInfo *> *) calloc(1, sizeof(vector<RefInfo *>));
        referrerss[idx] = referrers;
    }
    return referrers;
}

void _release_refs_vector(vector<RefInfo *> **referrerss, jint size)
{
    if (referrerss == NULL) return;
    for (int i = 0; i < size; i++)
    {
        vector<RefInfo *> *referrers = referrerss[i];
        if (referrers != NULL)
        {
            for (auto ref_info = referrers->begin(); ref_info != referrers->end(); ++ref_info)
                _release_ref_info(*ref_info);
            referrers->clear();
            free(referrers);
        }
    }
    free(referrerss);
}

ObjectInfo *_create_object_info(jlong class_tag, jlong tag, jlong size)
{
    ObjectInfo *object_info = (ObjectInfo *) calloc(1, sizeof(ObjectInfo));
    object_info->size = size;
    object_info->tag = tag;
    object_info->class_tag = (class_tag & CLASS_BIT) ? (class_tag & L_BITS) : -1;
    return object_info;
}

jint _debug_refs_callback(
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
    DebugRefsData *data = (DebugRefsData *) user_data;

    if (referrer_class_tag & SKIP_REFS_FROM_BIT)
        return 0; // prune this branch

    jlong tag_ptr_lb;
    if (*tag_ptr & OBJECT_BIT)
        // if this is object, we get LSB of it
        tag_ptr_lb = *tag_ptr & L_BITS;
    else
        // if this is a class or not tagged object, we skip it
        return JVMTI_VISIT_OBJECTS;

    jlong referrer_tag_ptr_lb = -1L;
    if (referrer_tag_ptr && (*referrer_tag_ptr & OBJECT_BIT))
        // if referrer tag is defined and is an object, we get LSB of it
        referrer_tag_ptr_lb = *referrer_tag_ptr & L_BITS;

    vector<RefInfo *> *referrers = _get_or_create_refs_vector(data->referrerss, tag_ptr_lb);
    RefInfo *ref_info = _create_ref_info(referrer_class_tag, referrer_tag_ptr_lb, reference_kind, reference_info);
    referrers->push_back(ref_info);

    if (!(*tag_ptr & VISIT_BIT) && (*tag_ptr & MARKER_BIT))
    {
        *tag_ptr |= VISIT_BIT;
        if (size >= data->size_threshold)
        {
            ObjectInfo *object_info = _create_object_info(class_tag, tag_ptr_lb, size);
            data->selected_objects.push_back(object_info);
        }
    }

    return JVMTI_VISIT_OBJECTS;
}

jint tag_all_objects_callback(jlong class_tag, jlong size, jlong *tag_ptr, jint length, void *user_data)
{
    jlong *objects_cnt = (jlong *) user_data;

    if (!(*tag_ptr & CLASS_BIT))
    {
        *tag_ptr = (*objects_cnt & L_BITS) | (*tag_ptr & MARKER_BIT) | // keep the marker if it is set
                   OBJECT_BIT; // set object bit
        *objects_cnt += 1;
    }

    return JVMTI_VISIT_OBJECTS;
}

jint _get_fields_count(jclass cls)
{
    jint fields_cnt = 0;
    jfieldID *fields = NULL;
    jvmti_env->GetClassFields(cls, &fields_cnt, &fields);
    _release_jvmti_objects(1, &fields);
    return fields_cnt;
}

jint _get_all_fields_cnt(JNIEnv *env, jclass start_class)
{
    jboolean is_interface = false;
    jvmti_env->IsInterface(start_class, &is_interface);

    jint fields_cnt = _get_fields_count(start_class);

    jint ifaces_cnt = 0;
    jclass *ifaces = NULL;
    jvmti_env->GetImplementedInterfaces(start_class, &ifaces_cnt, &ifaces);
    for (jint i = 0; i < ifaces_cnt; i++)
    {
        fields_cnt += _get_all_fields_cnt(env, ifaces[i]);
    }
    _release_jvmti_objects(1, &ifaces);

    if (!is_interface)
    {
        jclass super_class = env->GetSuperclass(start_class);
        if (super_class != NULL)
        {
            fields_cnt += _get_all_fields_cnt(env, super_class);
        }
    }

    return fields_cnt;
}

void _print_method_location(jclass method_class, jmethodID method, jlocation location)
{
    char *source_file_name = NULL;
    jvmti_env->GetSourceFileName(method_class, &source_file_name);

    cerr << "(";
    if (source_file_name != NULL)
    {
        cerr << source_file_name;
        _release_jvmti_objects(1, &source_file_name);
    }
    if (location >= 0L)
    {
        jint mappings_cnt = 0;
        jvmtiLineNumberEntry *mappings = NULL;
        jvmti_env->GetLineNumberTable(method, &mappings_cnt, &mappings);

        jint line_number = -1;
        for (int i = 0; i < mappings_cnt && mappings[i].start_location <= location; i++)
        {
            line_number = mappings[i].line_number;
        }
        if (line_number >= 0)
        {
            cerr << ":" << line_number;
        }

        _release_jvmti_objects(1, &mappings);
    }

    cerr << ")";

}

void _print_constructor_location(jclass cls)
{
    jint methods_cnt = 0;
    jmethodID *methods = NULL;
    jvmti_env->GetClassMethods(cls, &methods_cnt, &methods);
    for (int i = 0; i < methods_cnt; i++)
    {
        char *method_name = NULL;
        jvmti_env->GetMethodName(methods[i], &method_name, NULL, NULL);
        if (method_name != NULL && strcmp(method_name, "<init>") == 0)
        {
            cerr << " ";
            _print_method_location(cls, methods[i], 0);
        }
        _release_jvmti_objects(1, &method_name);
    }

    _release_jvmti_objects(1, &methods);
}


void _print_field_details(JNIEnv *env, jvmtiHeapReferenceInfoField *field, jclass referrer_class, jboolean is_inner)
{
    jint all_fields_cnt = _get_all_fields_cnt(env, referrer_class);

    jint fields_cnt = 0;
    jfieldID *fields = NULL;
    jvmti_env->GetClassFields(referrer_class, &fields_cnt, &fields);

    if (field->index < all_fields_cnt && field->index >= (all_fields_cnt - fields_cnt))
    {
        char *field_name = NULL;
        char *field_signature = NULL;
        jvmti_env->GetFieldName(referrer_class, fields[field->index - (all_fields_cnt - fields_cnt)], &field_name,
                                &field_signature, NULL);
        cerr << field_name << " ";
        if (is_inner) _print_constructor_location(referrer_class);
        _release_jvmti_objects(2, &field_name, &field_signature);
    }
    else
    {
        cerr << "[" << field->index << "]";
    }

    _release_jvmti_objects(1, &fields);
}

void _print_array_element_detail(jvmtiHeapReferenceInfoArray *array)
{
    cerr << "[" << array->index << "]";
}

void _print_constant_pool_details(jvmtiHeapReferenceInfoConstantPool *pool)
{
    cerr << "[" << pool->index << "]";
}

void _print_frame_details(jmethodID method, jint thread_id, jint depth, jlocation location)
{
    char *method_name = NULL;
    char *method_signature = NULL;
    jclass method_class = NULL;
    char *class_signature = NULL;

    jvmti_env->GetMethodName(method, &method_name, &method_signature, NULL);
    jvmti_env->GetMethodDeclaringClass(method, &method_class);
    jvmti_env->GetClassSignature(method_class, &class_signature, NULL);

    cerr << "thread: " << thread_id << "/" << depth << ", " << class_signature << "." << method_name;
    _print_method_location(method_class, method, location);

    _release_jvmti_objects(3, &class_signature, &method_signature, &method_name);
}

void _print_jni_local_details(jvmtiHeapReferenceInfoJniLocal *jni_local)
{
    _print_frame_details(jni_local->method, jni_local->thread_id, jni_local->depth, -1L);
    cerr << " ";
}

void _print_stack_local_details(jvmtiHeapReferenceInfoStackLocal *stack_local)
{
    _print_frame_details(stack_local->method, stack_local->thread_id, stack_local->depth, stack_local->location);
}

void
_print_references(JNIEnv *env, jint max_depth, jint level, jlong tag, vector<RefInfo *> **referrers, jclass *classes)
{
    if (level >= max_depth || tag < 0) return;
    vector<RefInfo *> *obj_referrers = referrers[tag & L_BITS];
    for (auto referrer = obj_referrers->begin(); referrer != obj_referrers->end(); ++referrer)
    {
        for (jint i = 0; i < level; i++) cerr << " │  ";
        char *signature = NULL;

        jvmti_env->GetClassSignature(classes[(*referrer)->referrer_class], &signature, NULL);
        cerr << " ├── " << signature << ", ";

        jboolean is_inner = signature != NULL ? (strstr(signature, "$") != NULL) : false;

        switch ((*referrer)->kind)
        {
            case JVMTI_HEAP_REFERENCE_FIELD:
                cerr << "field: ";
                _print_field_details(env, &(*referrer)->info->field, classes[(*referrer)->referrer_class], is_inner);
                break;
            case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
                cerr << "static field: ";
                _print_field_details(env, &(*referrer)->info->field, classes[(*referrer)->referrer_class], is_inner);
                break;
            case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
                cerr << "array element: ";
                _print_array_element_detail(&(*referrer)->info->array);
                break;
            case JVMTI_HEAP_REFERENCE_CONSTANT_POOL:
                cerr << "constant pool: ";
                _print_constant_pool_details(&(*referrer)->info->constant_pool);
                break;
            case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
                cerr << "jni local: ";
                _print_jni_local_details(&(*referrer)->info->jni_local);
                break;
            case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
                cerr << "stack local: ";
                _print_stack_local_details(&(*referrer)->info->stack_local);
                break;
            case JVMTI_HEAP_REFERENCE_CLASS:
                cerr << "class";
                break;
            case JVMTI_HEAP_REFERENCE_CLASS_LOADER:
                cerr << "class loader";
                break;
            case JVMTI_HEAP_REFERENCE_INTERFACE:
                cerr << "interface";
                break;
            case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
                cerr << "JNI global";
                break;
            case JVMTI_HEAP_REFERENCE_MONITOR:
                cerr << "monitor";
                break;
            case JVMTI_HEAP_REFERENCE_OTHER:
                cerr << "other";
                break;
            case JVMTI_HEAP_REFERENCE_PROTECTION_DOMAIN:
                cerr << "protection domain";
                break;
            case JVMTI_HEAP_REFERENCE_SIGNERS:
                cerr << "signers";
                break;
            case JVMTI_HEAP_REFERENCE_SUPERCLASS:
                cerr << "super class";
                break;
            case JVMTI_HEAP_REFERENCE_SYSTEM_CLASS:
                cerr << "system class";
                break;
            case JVMTI_HEAP_REFERENCE_THREAD:
                cerr << "thread";
                break;
            default:
                cerr << "unknown";
                break;
        }
        cerr << "\n";
        _release_jvmti_objects(1, &signature);
        _print_references(env, max_depth, level + 1, (*referrer)->referrer_tag, referrers, classes);
    }
}

void debug_refs(JNIEnv *env, jlong size_threshold, jint depth)
{
    jvmtiHeapCallbacks callbacks;

    // we start with tagging all classes with consecutive numbers starting with 0, with CLASS_BIT set
    jint classes_cnt = 0;
    jclass *classes = NULL;
    jvmti_env->GetLoadedClasses(&classes_cnt, &classes);
    for (int i = 0; i < classes_cnt; i++)
    {
        long tag = 0;
        jvmti_env->GetTag(classes[i], &tag);
        jvmti_env->SetTag(classes[i], i | CLASS_BIT | (tag & SKIP_REFS_FROM_BIT));
    }

    // before traversing references, we tag all objects (except classes) on heap with consecutive numbers
    // and OBJECT_BIT flag, leaving MARKER_FLAG if it was set; after this operation objects_cnt contains
    // the number of objects
    jlong objects_cnt = 0;
    clean_callbacks(callbacks)->heap_iteration_callback = &tag_all_objects_callback;
    jvmti_env->IterateThroughHeap(0, NULL, &callbacks, &objects_cnt);

    // here we prepare data for reference traversal; we are going to collect objects which needs to be reported
    // in details, as well as whole map of references
    DebugRefsData debug_refs_data;
    debug_refs_data.referrerss = (vector<RefInfo *> **) calloc(objects_cnt, sizeof(vector<RefInfo *> **));
    debug_refs_data.selected_objects.clear();
    debug_refs_data.size_threshold = size_threshold;

    clean_callbacks(callbacks)->heap_reference_callback = &_debug_refs_callback;
    jvmti_env->FollowReferences(0, NULL, NULL, &callbacks, &debug_refs_data);

    // at this point, we have objects to print in detail and full reference map; we can print them out to stderr
    char *class_signature = NULL;
    for (auto sel_object = debug_refs_data.selected_objects.begin();
         sel_object != debug_refs_data.selected_objects.end(); ++sel_object)
    {
        jvmti_env->GetClassSignature(classes[(*sel_object)->class_tag], &class_signature, NULL);
        cerr << "Object (" << (*sel_object)->size << " bytes) of " << class_signature << ", referenced from:" << endl;
        _release_jvmti_objects(1, &class_signature);
        _print_references(env, depth, 0, (*sel_object)->tag, debug_refs_data.referrerss, classes);
        cerr.flush();
    }

    debug_refs_data.selected_objects.clear();
    _release_refs_vector(debug_refs_data.referrerss, objects_cnt);
    _release_jvmti_objects(1, &classes);
}

void
Java_net_enigma_test_toolkit_TestToolkitAgent_debugReferences(JNIEnv *env, jclass interface_class, jlong size_threshold,
                                                              jint depth)
{
    debug_refs(env, size_threshold, depth);
}
