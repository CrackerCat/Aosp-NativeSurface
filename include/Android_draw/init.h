//
// Created by 12981 on 2023/6/13.
//

#ifndef SGAMEJQY_INIT_H
#define SGAMEJQY_INIT_H
#include <EGL/egl.h>
#include <GLES/gl.h>
#include "Android_utils.h"
#include <android/native_window.h>
#include <Aosp_surface.h>
#include <imgui_impl_opengl3.h> // IMGUI库
#include <imgui_impl_android.h> // IMGUI库
#include <thread>
//includ font
#include <font.h>
#include <stb_image.h>
#include <syscall.h>



void init_imgui();
void init_screen_config();
void release_android_draw();
bool init_android_draw();
void drawBegin();
void drawEnd();

//TextureInfo ImAgeHeadFile(const char* ImagePath);

extern EGLDisplay display;
extern EGLSurface surface;
extern MDisplayInfo displayInfo;
extern int32_t init_screen_width;
extern int32_t init_screen_height;
//extern TextureInfo icon;




#endif //SGAMEJQY_INIT_H
