//
// Created by 12981 on 2023/6/18.
//

#ifndef SGAMEJQY_IMGUIDRAW_HPP
#define SGAMEJQY_IMGUIDRAW_HPP
#include <imgui.h>
//颜色


static struct Color{
    ImVec4 Red={255/255.f,0/255.f,0/255.f,255/255.f};
    ImVec4 Red_={255/255.f,0/255.f,0/255.f,50/255.f};
    ImVec4 Green={0/255.f,255/255.f,0/255.f,255/255.f};
    ImVec4 Green_={0/255.f,255/255.f,0/255.f,50/255.f};
    ImVec4 White={1.0,1.0,1.0,1.0};

}Colors;


void DrawCircleFilled(int x, int y, int radius, ImVec4 color, int segments)
{
    ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x, y), radius,
                                                    ImGui::ColorConvertFloat4ToU32(color),
                                                    segments);
}

void DrawCircleFilled(int x, int y, int radius, ImColor color, int segments)
{
    ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x, y), radius,
                                                    (color),
                                                    segments);
}

void DrawLine(int x1, int y1, int x2, int y2, ImVec4 color, int size)
{

    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2),
                                            ImGui::ColorConvertFloat4ToU32(color), size);
}

void DrawRect(int x, int y, int w, int h, ImVec4 color, int size,float round)
{								// rounding 方框边缘曲率
    // //rounding_corners_flags
    // 方框边缘弯曲类型
    // 1.ImDrawCornerFlags_All
    // 2.ImDrawCornerFlags_Top 3.
    // ImDrawCornerFlags_Bot 4.
    // ImDrawCornerFlags_Left 5.
    // ImDrawCornerFlags_Right
    ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
                                            ImGui::ColorConvertFloat4ToU32(color), round, 0, size);
}

void DrawRect2(float x, float y, float x2, float y2, ImVec4 color, int size)
{								// rounding 方框边缘曲率
    // //rounding_corners_flags
    // 方框边缘弯曲类型
    // 1.ImDrawCornerFlags_All
    // 2.ImDrawCornerFlags_Top 3.
    // ImDrawCornerFlags_Bot 4.
    // ImDrawCornerFlags_Left 5.
    // ImDrawCornerFlags_Right
    ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x2, y2), ImColor(color), 0, 0, size);
}

void DrawRectFilled(int x, int y, int w, int h, ImVec4 color)
{
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                                                  ImGui::ColorConvertFloat4ToU32(color), 0, 0);
}

void DrawCircle(int x, int y, float radius, ImVec4 color, int segments, int thickness)
{

    ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), radius,
                                              ImGui::ColorConvertFloat4ToU32(color), segments,
                                              thickness);
}


void DrawStrokeText2(int x, int y, ImVec4 color, const char *str, float size)
{

    ImGui::GetBackgroundDrawList()->AddText(NULL, size, ImVec2(x, y),
                                            ImGui::ColorConvertFloat4ToU32(color), str);
}

void DrawStrokeText(int x, int y, ImVec4 color, const char *str)
{
    DrawStrokeText2(x, y, color, str, 20);
}

void DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, ImVec4 Colors, int T)
{
    ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2),
                                                      ImVec2(x3, y3),
                                                      ImGui::ColorConvertFloat4ToU32(Colors));
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, ImVec4 Colors, int T) {
    ImGui::GetBackgroundDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2),
                                                      ImVec2(x3, y3),
                                                      ImGui::ColorConvertFloat4ToU32(Colors));
}

//void DrawTexture(int x, int y, int w, int h, TextureInfo Image){
//    ImGui::GetBackgroundDrawList()->AddImage(Image.textureId, ImVec2(x, y), ImVec2(x + w, y + h));
//}


#endif //SGAMEJQY_IMGUIDRAW_HPP
