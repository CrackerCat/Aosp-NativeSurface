#include <Android_draw.h>
#include <ImGuiDraw.hpp>
#include <Android_touch.h>


// 测试用窗口
void DrawUi() { // IMGUI的菜单从这里开始写
    static bool a = false;

    ImGui::Begin("测试");
    ImGui::Text("测试窗口");
    ImGui::Checkbox("测试按钮",&a);
    if(ImGui::Button("测试按钮")){
        exit(0);
    }
    ImGui::End();

}