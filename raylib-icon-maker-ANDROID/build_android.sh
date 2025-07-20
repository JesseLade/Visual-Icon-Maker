#!/bin/bash

# armeabi-v7a
echo "Building libraylib for armeabi-v7a..."
cd app/src/main/cpp/raylib
mkdir -p build-android/armeabi-v7a
cd build-android/armeabi-v7a
cmake ../.. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=armeabi-v7a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target raylib
cd ../../../../../../..

# arm64-v8a
echo "Building libraylib for arm64-v8a..."
cd app/src/main/cpp/raylib
mkdir -p build-android/arm64-v8a
cd build-android/arm64-v8a
cmake ../.. \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --target raylib
cd ../../../../../../..
