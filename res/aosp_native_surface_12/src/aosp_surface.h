/*
 * Codes from SsageParuders[https://github.com/SsageParuders]
 * 代码由泓清提供
*/
#include <EGL/egl.h>
#include <GLES/gl.h>

struct MDisplayInfo {
    uint32_t override_width{0}; // 触摸屏
    uint32_t override_height{0}; // 触摸屏
    uint32_t orientation{0};
    uint32_t physical_width{0}; // 物理屏
    uint32_t physical_height{0}; // 物理屏
};

// 创建窗口
NativeWindowType createNativeWindow(const char *surface_name ,uint32_t screen_width ,uint32_t screen_height,bool is_by_pass, void* get_func);
// 获取屏幕信息
MDisplayInfo getDisplayInfo();
// 设置窗口大小 --> 辅助窗口旋转变化
void setSize(uint32_t screen_width, uint32_t screen_height);