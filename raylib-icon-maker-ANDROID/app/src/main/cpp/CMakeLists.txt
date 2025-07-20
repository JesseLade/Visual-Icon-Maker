cmake_minimum_required(VERSION 3.10...3.27)
project(visualiconmaker)

set(RAYLIB_PATH "/Users/demi/raylib_editor_game/raylib-icon-maker/raylib-icon-maker-ANDROID/app/src/main/cpp/raylib")
set(RAYLIB_INCLUDE_PATH "${RAYLIB_PATH}/include")
set(RAYLIB_LIB_PATH "${RAYLIB_PATH}/lib")

# Define the path to android_native_app_glue.h (comes with NDK)
set(NDK_APP_GLUE_PATH "/Users/demi/raylib_editor_game/raylib-icon-maker/raylib-icon-maker-ANDROID/app/src/main/cpp/android_native_app_glue")

# Build native shared library
add_library(iconmaker-lib SHARED
    main.cpp
)

# Raylib import
add_library(raylib STATIC IMPORTED)
if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(RAYLIB_LIB_FILE ${RAYLIB_LIB_PATH}/armeabi-v7a/libraylib.a)
elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(RAYLIB_LIB_FILE ${RAYLIB_LIB_PATH}/arm64-v8a/libraylib.a)
else()
    message(FATAL_ERROR "Unsupported ABI: ${ANDROID_ABI}")
endif()
set_target_properties(raylib PROPERTIES IMPORTED_LOCATION ${RAYLIB_LIB_FILE})

# Android native headers
target_include_directories(iconmaker-lib PRIVATE
    ${RAYLIB_INCLUDE_PATH}
    ${NDK_APP_GLUE_PATH}
)

# Link Android system libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(GLESv2-lib GLESv2)
find_library(EGL-lib EGL)
find_library(OpenSLES-lib OpenSLES)

target_link_libraries(iconmaker-lib
    -Wl,--whole-archive
    ${RAYLIB_LIB_FILE}
    -Wl,--no-whole-archive
    atomic
    m
    ${android-lib}
    ${GLESv2-lib}
    ${EGL-lib}
    ${OpenSLES-lib}
    ${log-lib}
)
