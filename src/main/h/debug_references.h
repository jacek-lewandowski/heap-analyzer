#ifndef TEST_TOOLKIT_NATIVE_AGENT_DEBUG_REFERENCES_H
#define TEST_TOOLKIT_NATIVE_AGENT_DEBUG_REFERENCES_H

#include "jvmti.h"
#include <vector>

struct ObjectInfo
{
    jlong tag;
    jlong size;
    jlong class_tag;
};

struct RefInfo
{
    jlong referrer_tag = 0;
    jlong referrer_class = 0;
    jvmtiHeapReferenceKind kind;
    jvmtiHeapReferenceInfo *info = NULL;
};

struct DebugRefsData
{
    /** only objects with size equal or greater will be reported with details */
    jlong size_threshold = 0;

    /** vector of objects selected to report with details */
    std::vector<ObjectInfo *> selected_objects;

    /** array of pointers to vectors; each array item corresponds to object with tag being the same as its index in
     * this array; each vector contains information about direct references to that object */
    std::vector<RefInfo *> **referrerss = NULL;
};

#endif //TEST_TOOLKIT_NATIVE_AGENT_DEBUG_REFERENCES_H
