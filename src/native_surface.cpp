//
// Created by Ssage on 2022/3/18.
//
// Self lib
#include <cstdlib>
// User libs
#include <Android_draw.h> // 绘制
#include <Android_touch.h> // 触摸
#include <string>

int main(int argc, char const *argv[])
__attribute((__annotate__(("fla cse icall indgv indbr")))) {
    bool is_succeed = init_aosp_function(); // 适配10-12安卓版本
    if (!is_succeed) { // 初始化失败了
        printf("init failed!\n");
        exit(0); // exit since init_aosp_function failed
    }

    init_screen_config(); // 屏幕信息线程
    while (true) { // 死循环执行
        if (init_screen_width != 0 && init_screen_height != 0) { // 屏幕初始化ok了
            init_android_draw(); // 绘制初始化
            init_android_touch(false); // 触摸线程
            init_imgui();
            break;
        }
    }


    while (true) {
        drawBegin();
        DrawUi();
        drawEnd();
    }

    release_android_draw();
    exit(0);
    return 0;
}