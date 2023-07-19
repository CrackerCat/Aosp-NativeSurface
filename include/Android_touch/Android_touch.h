#ifndef NATIVESURFACE_ANDROID_TOUCH_H
#define NATIVESURFACE_ANDROID_TOUCH_H
// System libs
#include <iostream>
using namespace std;
// int get_touch_event_num(); // 循环等待用户第一次触摸获ID <-- 启用该方案
// void handleInputEvent(); // 处理触控事件 --> 老的触摸处理方案 已解决触摸穿透 但是会和触摸自瞄产生触摸冲突
void Touch_Down(int id, int x, int y);
void Touch_Move(int id, int x, int y);
void Touch_Up(int id);
// void init_android_touch(); // 新开一个线程
void init_android_touch(bool isReadOnly);
#endif