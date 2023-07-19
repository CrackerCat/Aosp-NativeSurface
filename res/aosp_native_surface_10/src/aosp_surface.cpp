/*
 * Codes from SsageParuders[https://github.com/SsageParuders]
 * 代码由泓清提供
*/
// Self libs
#include "aosp_surface.h"
// C/C++ libs
#include <iostream>
#include <thread>
#include <chrono>
// AOSP libs
#include <utils/StrongPointer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
// User libs
#include "oxorany.h" // 混淆

android::sp<android::SurfaceComposerClient> gSurfaceComposerClient;
android::sp<android::SurfaceControl>        gSurfaceControl;
android::SurfaceComposerClient::Transaction *transaction;

/*
 * Next codes mostly withaout changes from Khronos intro
 */

/* Already defined in frameworks/native/opengl/include/EGL/eglplatform.h like
 * typedef struct ANativeWindow*           EGLNativeWindowType;
 * ...
 * typedef EGLNativeWindowType  NativeWindowType;
 */
// typedef ... NativeWindowType;

NativeWindowType createNativeWindow(const char *surface_name ,uint32_t screen_width ,uint32_t screen_height,bool is_by_pass, void* get_func){
    // Just call once
    NativeWindowType ret = nullptr;
    gSurfaceComposerClient = android::sp<android::SurfaceComposerClient> {new android::SurfaceComposerClient()};
    if(is_by_pass){ // 过录屏模式
        android::LayerMetadata metadata;
        metadata.setInt32(android::METADATA_WINDOW_TYPE, oxorany(441731));
        gSurfaceControl = gSurfaceComposerClient->createSurface(android::String8(surface_name),
                                                                screen_width,
                                                                screen_height,
                                                                oxorany(android::PIXEL_FORMAT_RGBA_8888),
                                                                oxorany(0), NULL, metadata);
    } else { // 不过录屏模式
        gSurfaceControl = gSurfaceComposerClient->createSurface(android::String8(surface_name),
                                                                screen_width,
                                                                screen_height,
                                                                oxorany(android::PIXEL_FORMAT_RGBA_8888),
                                                                oxorany(0));
    }
    if(!gSurfaceControl){
        std::cout << oxorany("!gSurfaceControl") << std::endl;
    } else if(!gSurfaceControl->isValid()){
        std::cout << oxorany("!gSurfaceControl->isValid()") << std::endl;
    } else {
        // TODO: !BUGS FINDED : Vector<> have different types
        // fixed bugs from aosp & miui | see https://github.com/DeviceFarmer/minicap/issues/6
        /* patch frameworks/native/libs/gui/include/gui/LayerState.h below
         * [ ] uint32_t width, height;
         * [+] uint8_t _dummy[16]; // patch Google AOSP & MIUI
         * [ ] status_t write(Parcel& output) const;
         * [ ] status_t read(const Parcel& input);
        */
        // android::SurfaceComposerClient::Transaction{}
        //         .setLayer(gSurfaceControl, INT_MAX)
        //         .show(gSurfaceControl)
        //         .apply();
        // TODO: !BUGS FINDED
        /*== New Way From 辛心 ==*/
        android::SurfaceComposerClient::Transaction *(*transaction_get)() = NULL;
        *(void **)&transaction_get = get_func;
        transaction = transaction_get();
        if (!transaction){ // 我感觉没什么必要做为空判断 =_=
            std::cout << oxorany("transaction create failed!") << std::endl;
            // return NULL;
        }
        ret = gSurfaceControl->getSurface().get();
    }
    return ret;
}

MDisplayInfo getDisplayInfo() {
    const auto token = android::SurfaceComposerClient::getInternalDisplayToken();
    android::DisplayInfo mainDisplayInfo;
    // 获取手机的屏幕信息
    android::status_t err = android::SurfaceComposerClient::getDisplayInfo(token, &mainDisplayInfo);
    MDisplayInfo mDisplayInfo;
    if (err != android::NO_ERROR) {
        std::cout << oxorany("getDisplayInfo err....") << std::endl;
        return mDisplayInfo;
    }
    // 屏幕方向
    mDisplayInfo.orientation = mainDisplayInfo.orientation;
    if (mainDisplayInfo.orientation == 0 || mainDisplayInfo.orientation == 2) { // 竖屏 && 倒屏
        // aosp10 没有getDisplayState接口 因此获取到的屏幕分辨率固定 不可随屏幕方向动态
        // 但是屏幕方向为动态 因此我们内部自行做一层转化
        // 屏幕宽
        mDisplayInfo.width = mainDisplayInfo.w;
        // 屏幕高
        mDisplayInfo.height = mainDisplayInfo.h;
    } else if (mainDisplayInfo.orientation == 1 || mainDisplayInfo.orientation == 3) { // 横屏1 && 横屏2
        // 屏幕宽
        mDisplayInfo.width = mainDisplayInfo.h; // 交换
        // 屏幕高
        mDisplayInfo.height = mainDisplayInfo.w; // 交换
    } else {
        // Unknow orientation
    }
    return mDisplayInfo;
}


void setSize(uint32_t screen_width, uint32_t screen_height){
    /*== New Way From 辛心 ==*/
    transaction->setSize(gSurfaceControl, screen_width, screen_height);
    transaction->apply();
}