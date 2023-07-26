//
// Created by JiangNight or RuoNan  on 2023/7/24.
//

#ifndef NATIVESURFACE_SURFACETOOLS_H
#define NATIVESURFACE_SURFACETOOLS_H

#include <init.h>
#include "Aosp_surface.h"
#include "Android_touch.h"


#endif //NATIVESURFACE_SURFACETOOLS_H


namespace SurfaceTools {

    void Init();//初始化imgui的信息
    int32_t GetScreenWidth();
    int32_t GetScreenHeight();
    namespace gui {
        void New(); //创建ImGui窗口
        void End(); //结束ImGui窗口
        void release();//销毁imgui
        ImTextureID createImage(unsigned char *buffer,int len);//绘制图片
    }

    namespace touch {
        void Handle(); //初始化监听 没逼用 可以选择装逼 输出点牛逼东西
        void Move(int id, int x, int y);//手指移动
        void Up(int id);//抬起
        void Down(int id, int x, int y);//按下
    }
}

