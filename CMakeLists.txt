cmake_minimum_required(VERSION 3.18.1)

#Android设置
set(NDK_PATH C:/Users/Administrator/AppData/Local/Android/Sdk/ndk/ndk_r25b_ollvm)
#set(NDK_PATH C:/Users/12981/AppData/Local/Android/Sdk/ndk/25.2.9519653)
set(CMAKE_SYSTEM_NAME ANDROID) # 设置目标编译平台参数 Android
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SYSTEM_VERSION 21) # 系统版本
set(ANDROID_PLATFORM 21) # 平台版本
set(ANDROID_SDK_TOOLS_VERSION 20) # 平台版本
set(ANDROID_ABI arm64-v8a) # 设置目标构架 armeabi-v7a arm64-v8a x86 x86_64
set(ANDROID_NDK ${NDK_PATH}) # 设置ndk路径
set(CMAKE_TOOLCHAIN_FILE ${NDK_PATH}/build/cmake/android.toolchain.cmake)
set(ANDROID_SDK_ROOT ${NDK_PATH})

project(NativeSurface)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ## 加载插件 -mllvm -irobf-cse
       "-fno-rtti -fno-exceptions -fvisibility=hidden -s -Wno-narrowing -fdeclspec -pthread -w -fexceptions -Wall -O3 -Wno-error=format-security -fpermissive")

#输出文件重定向
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/outputs/${CMAKE_ANDROID_ARCH_ABI}/) # 重定向输出产物(可执行文件)


#CMake头文件设置
FILE(GLOB_RECURSE FILE_INCLUDES # 遍历子目录下所有符合情况的头文件
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*)

include_directories( # 设置全局头文件目录 使其他源码文件可在任意目录<头文件.h>
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ImGui/fonts
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Android_touch
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Aosp_surface
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Android_draw
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Android_utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Android_data
        ${CMAKE_CURRENT_SOURCE_DIR}/include/icon
        )


#CMake源文件设置
FILE(GLOB_RECURSE FILE_SOURCES # 遍历子目录下所有符合情况的源文件
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*)

#添加产物
add_executable(NativeSurface # 生成可执行文件
        ${FILE_INCLUDES} # 头文件
        ${FILE_SOURCES} # 源文件
        )
#添加产物

#设置三方库文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
        )

#连接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE
        EGL GLESv3 log android GLESv2 m z dl
        )