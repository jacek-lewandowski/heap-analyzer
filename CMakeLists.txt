cmake_minimum_required(VERSION 3.15)

project(test-toolkit-native-agent)

set(LIB_MAJOR_VERSION "0")
set(LIB_MINOR_VERSION "1")
set(LIB_PATCH_VERSION "0")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

message("\nBuilding ${PROJECT_NAME} ${LIB_VERSION_STRING}\n")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(
        "$ENV{JAVA_HOME}/include"
        "build/generated/sources/native/headers/main"
)

if (WIN32)
    message("Win32")
    include_directories(
            "$ENV{JAVA_HOME}/include/win32"
            "$ENV{JAVA_HOME}/include"
            "build/generated/sources/native/headers/main"
            "src/main/h"
    )
elseif (APPLE)
    message("Apple")
    include_directories(
            "$ENV{JAVA_HOME}/include/darwin"
            "$ENV{JAVA_HOME}/include"
            "build/generated/sources/native/headers/main"
            "src/main/h"
    )
else ()
    message("Linux")
    include_directories(
            "$ENV{JAVA_HOME}/include/linux"
            "$ENV{JAVA_HOME}/include"
            "build/generated/sources/native/headers/main"
            "src/main/h"
    )
endif ()

set(SOURCE_FILES
        src/main/cpp/net_enigma_test_toolkit_TestToolkitAgent.cpp)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ../build/libs)

add_library(test-toolkit-native-agent SHARED ${SOURCE_FILES})