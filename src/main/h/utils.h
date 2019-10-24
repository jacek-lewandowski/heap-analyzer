#ifndef TEST_TOOLKIT_NATIVE_AGENT_UTILS_H
#define TEST_TOOLKIT_NATIVE_AGENT_UTILS_H

#include <string>
#include <sstream>
#include <iostream>

#include "jni.h"
#include "jvmti.h"

extern jvmtiEnv *jvmti_env;

extern jclass runtime_exception_class;
extern jclass heap_traversal_summary_class;

#define L_BITS 0xFFFFFFFFL
#define H_BITS 0xFFFF00000000L

/** Marking objects for analysis */
#define MARKER_BIT 0x100000000L
/** Marking visited objects while traversing references */
#define VISIT_BIT 0x200000000L
/** Marking Class instances */
#define CLASS_BIT 0x400000000L
/** Marking non-Class instances */
#define OBJECT_BIT 0x800000000L
/** Skip references from objects of class tagged with this flag */
#define SKIP_REFS_FROM_BIT 0x1000000000L

jint throw_runtime_exception(JNIEnv *env, const char *message);

void throw_exception_if_error(JNIEnv *env, jvmtiError error);

jvmtiHeapCallbacks *clean_callbacks(jvmtiHeapCallbacks &callbacks);

void _release_jvmti_objects(jint, ...);

#endif //TEST_TOOLKIT_NATIVE_AGENT_UTILS_H
