//
// Created by Ssage on 2022/6/29.
//
#ifndef NATIVESURFACE_AOSP_SURFACE_H
#define NATIVESURFACE_AOSP_SURFACE_H

#include <cstdint>
// 屏幕信息
struct MDisplayInfo {
    uint32_t override_width{0}; // 触摸屏
    uint32_t override_height{0}; // 触摸屏
    uint32_t orientation{0};
    uint32_t physical_width{0}; // 物理屏
    uint32_t physical_height{0}; // 物理屏
};
// 方法指针
struct FuncPointer {
    void *func_createNativeWindow;
    void *func_getDisplayInfo;
    void *func_setSize;
};
extern FuncPointer funcPointer;
bool init_aosp_function(); // 初始化aosp的api
#endif //NATIVESURFACE_AOSP_SURFACE_H
