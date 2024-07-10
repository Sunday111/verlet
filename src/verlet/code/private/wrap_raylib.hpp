#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>

namespace raylib
{

enum class MouseButton
{
    Left
};

using Color = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;

void InitWindow(const size_t width, const size_t height, const char* title);
void SetTargetFPS(const int fps);
bool WindowShouldClose();
int GetScreenWidth();
int GetScreenHeight();
float GetScreenWidthF();
float GetScreenHeightF();
std::tuple<float, float> GetMousePos();
float GetTime();
float GetFrameTime();
bool IsMouseButtonPressed(const MouseButton button);
void BeginDrawing();
void EndDrawing();
void ClearBackground(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
void DrawCircle(std::tuple<float, float> pos, float radius, const Color& color);
void CloseWindow();

inline constexpr Color kBlack = {255, 255, 255, 255};
}  // namespace raylib
