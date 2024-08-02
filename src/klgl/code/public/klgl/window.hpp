#pragma once

#include <EverydayTools/Template/TaggedIdentifier.hpp>

#include "EverydayTools/Math/Matrix.hpp"

struct GLFWwindow;

namespace klgl
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

class Application;

class Window
{
public:
    Window(Application& app, uint32_t width, uint32_t height);
    ~Window();

    void MakeContextCurrent();

    [[nodiscard]] bool ShouldClose() const noexcept;
    [[nodiscard]] uint32_t GetWidth() const noexcept { return width_; }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return height_; }

    Vec2<uint32_t> GetSize() const { return {width_, height_}; }
    Vec2f GetSize2f() const { return GetSize().Cast<float>(); }
    [[nodiscard]] GLFWwindow* GetGlfwWindow() const noexcept { return window_; }
    [[nodiscard]] float GetAspect() const noexcept
    {
        return static_cast<float>(GetWidth()) / static_cast<float>(GetHeight());
    }

    void SetSize(size_t width, size_t height);
    void SetTitle(const char* title);

    void SwapBuffers() noexcept;

    bool IsKeyPressed(int key) const;

private:
    static uint32_t MakeWindowId();
    static Window* GetWindow(GLFWwindow* glfw_window) noexcept;

    template <auto method, typename... Args>
    static void CallWndMethod(GLFWwindow* glfw_window, Args&&... args)
    {
        [[likely]] if (Window* window = GetWindow(glfw_window))
        {
            (window->*method)(std::forward<Args>(args)...);
        }
    }

    static void FrameBufferSizeCallback(GLFWwindow* glfw_window, int width, int height);
    static void MouseCallback(GLFWwindow* glfw_window, double x, double y);
    static void MouseButtonCallback(GLFWwindow* glfw_window, int button, int action, int mods);
    static void MouseScrollCallback(GLFWwindow* glfw_window, double x_offset, double y_offset);

    void Create();
    void Destroy();
    void OnResize(int width, int height);
    void OnMouseMove(Vec2f new_cursor);
    void OnMouseButton(int button, int action, [[maybe_unused]] int mods);
    void OnMouseScroll([[maybe_unused]] float dx, float dy);

private:
    Application* app_ = nullptr;
    GLFWwindow* window_ = nullptr;
    Vec2f cursor_;
    uint32_t id_;
    uint32_t width_;
    uint32_t height_;
    bool input_mode_ = false;
};

}  // namespace klgl
