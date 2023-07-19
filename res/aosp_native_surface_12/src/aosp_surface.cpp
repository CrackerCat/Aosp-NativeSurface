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
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayState.h>
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
    NativeWindowType ret = nullptr;
    gSurfaceComposerClient = android::sp<android::SurfaceComposerClient> {new android::SurfaceComposerClient()};
    if(is_by_pass){ // 过录屏模式
        gSurfaceControl = gSurfaceComposerClient->createSurface(android::String8(surface_name),
                                                                screen_width,
                                                                screen_height,
                                                                oxorany(android::PIXEL_FORMAT_RGBA_8888),
                                                                oxorany(0x40)); // 对过录屏变量实现混淆
    } else { // 不过录屏模式
        gSurfaceControl = gSurfaceComposerClient->createSurface(android::String8(surface_name),
                                                                screen_width,
                                                                screen_height,
                                                                oxorany(android::PIXEL_FORMAT_RGBA_8888),
                                                                oxorany(0)); // 镜像同步
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
    android::sp <android::IBinder> token = android::SurfaceComposerClient::getInternalDisplayToken();
    android::ui::DisplayState state;
    android::ui::DisplayMode physical_displayInfo;
    // 获取手机的屏幕信息 // 触摸
    android::status_t err = android::SurfaceComposerClient::getDisplayState(token, &state);
    // 获取手机的屏幕信息 // 物理
    android::SurfaceComposerClient::getActiveDisplayConfig(token, &physical_displayInfo);
    MDisplayInfo mDisplayInfo;
    if (err != android::NO_ERROR) {
        std::cout << oxorany("getDisplayInfo err....") << std::endl;
        return mDisplayInfo;
    }
    // 屏幕宽 // 触摸屏
    mDisplayInfo.override_width = state.layerStackSpaceRect.width;
    // 屏幕高 // 触摸屏
    mDisplayInfo.override_height = state.layerStackSpaceRect.height;
    // 方向
    mDisplayInfo.orientation = static_cast<int>(state.orientation);
    // 屏幕宽 // 物理屏
    mDisplayInfo.physical_width = physical_displayInfo.resolution.width;
    // 屏幕宽 // 物理屏
    mDisplayInfo.physical_height = physical_displayInfo.resolution.height;
    return mDisplayInfo;
}

void setSize(uint32_t screen_width, uint32_t screen_height){
    /*== New Way From 辛心 ==*/
    gSurfaceControl->updateDefaultBufferSize(screen_width, screen_height); // 12-13才需要这个 10-11不需要
    transaction->setSize(gSurfaceControl, screen_width, screen_height);
    transaction->apply();
}