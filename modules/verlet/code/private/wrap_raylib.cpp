#include "wrap_raylib.hpp"

#include "raylib.h"

namespace raylib
{

void InitWindow(const size_t width, const size_t height, const char* title)
{
    ::InitWindow(static_cast<int>(width), static_cast<int>(height), title);
}

void SetTargetFPS(const int fps)
{
    ::SetTargetFPS(fps);
}

bool WindowShouldClose()
{
    return ::WindowShouldClose();
}

int GetScreenWidth()
{
    return ::GetScreenWidth();
}
int GetScreenHeight()
{
    return ::GetScreenHeight();
}
float GetScreenWidthF()
{
    return static_cast<float>(GetScreenWidth());
}
float GetScreenHeightF()
{
    return static_cast<float>(GetScreenHeight());
}

std::tuple<float, float> GetMousePos()
{
    auto [x, y] = ::GetMousePosition();
    return {x, y};
}

float GetTime()
{
    return static_cast<float>(::GetTime());
}
float GetFrameTime()
{
    return static_cast<float>(::GetFrameTime());
}

bool IsMouseButtonPressed(const MouseButton button)
{
    switch (button)
    {
    case MouseButton::Left:
        return ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }
}
void BeginDrawing()
{
    ::BeginDrawing();
}
void EndDrawing()
{
    ::EndDrawing();
}
void CloseWindow()
{
    ::CloseWindow();
}
void ClearBackground(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    ::ClearBackground({r, g, b, a});
}
void DrawCircle(std::tuple<float, float> pos, float radius, const Color& color)
{
    ::DrawCircleV(
        {std::get<0>(pos), std::get<1>(pos)},
        radius,
        {std::get<0>(color), std::get<1>(color), std::get<2>(color), std::get<3>(color)});
}
}  // namespace raylib
