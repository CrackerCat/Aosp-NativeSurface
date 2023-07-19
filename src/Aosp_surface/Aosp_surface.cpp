//
// Created by 泓清 on 2022/8/26.
//
// Self lib
#include <Aosp_surface.h>
// System libs
#include <dlfcn.h>
#include <android/api-level.h>
// User libs
#include <Android_utils.h>
#include <aosp_11/native_surface_11.h>
#include <aosp_10/native_surface_10.h>
#include <aosp_12/native_surface_12.h>
#include <aosp_13/native_surface_13.h>

FuncPointer funcPointer;

bool init_aosp_function(){
    void *handle;// 动态库方案
    std::string out = "";
    // https://raw.githubusercontent.com/SsageParuders/RES/master/img/202209062018266.png
    // 也不知道为什么 特么的安卓12有两个api等级 详细看上图 --> 其中一个是专门为平坝电脑适配的
    if (get_android_api_level() == __ANDROID_API_T__){ // 安卓13支持
        // 解除触摸限制
        exec("settings put global block_untrusted_touches 0");
        handle = dlblob(&native_surface_13_64, sizeof(native_surface_13_64)); // 64位支持
    } else if (get_android_api_level() == __ANDROID_API_S__ || get_android_api_level() == 32){ // 安卓12支持
        // 解除限制
        exec("settings put global block_untrusted_touches 0"); // --> 安卓12以上需要解除本次限制
        handle = dlblob(&native_surface_12_64, sizeof(native_surface_12_64));    // 64位支持
    } else if (get_android_api_level() == __ANDROID_API_R__) { // 安卓11支持
        handle = dlblob(&native_surface_11_64, sizeof(native_surface_11_64));    // 64位支持
    } else if (get_android_api_level() == __ANDROID_API_Q__){ // 安卓10支持
        handle = dlblob(&native_surface_10_64, sizeof(native_surface_10_64)); // 64位支持
    } else { // 不在安卓10-12中 不在支持列表
        printf("Sorry, Don't Support~");
        exit(0);
    }
    funcPointer.func_createNativeWindow = dlsym(handle, "_Z18createNativeWindowPKcjjbPv"); // 创建surface
    funcPointer.func_getDisplayInfo = dlsym(handle, "_Z14getDisplayInfov"); // 获取屏幕信息
    funcPointer.func_setSize = dlsym(handle, "_Z7setSizejj"); // 设置大小
    return true;
}