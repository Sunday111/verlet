#include "window.hpp"

#include <fmt/format.h>

#include <stdexcept>

#include "GLFW/glfw3.h"

namespace klgl
{

Window::Window(uint32_t width, uint32_t height) : id_(MakeWindowId()), width_(width), height_(height)
{
    Create();
}

Window::~Window()
{
    Destroy();
}

void Window::MakeContextCurrent()
{
    glfwMakeContextCurrent(window_);
}

bool Window::ShouldClose() const noexcept
{
    return glfwWindowShouldClose(window_);
}

void Window::SwapBuffers() noexcept
{
    glfwSwapBuffers(window_);
}

void Window::SetSize(size_t width, size_t height)
{
    glfwSetWindowSize(window_, static_cast<int>(width), static_cast<int>(height));
}

void Window::SetTitle(const char* title)
{
    glfwSetWindowTitle(window_, title);
}

bool Window::IsKeyPressed(int key) const
{
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

uint32_t Window::MakeWindowId()
{
    static uint32_t next_id = 0;
    return next_id++;
}

Window* Window::GetWindow(GLFWwindow* glfw_window) noexcept
{
    [[likely]] if (glfw_window)
    {
        void* user_pointer = glfwGetWindowUserPointer(glfw_window);
        [[likely]] if (user_pointer)
        {
            return reinterpret_cast<Window*>(user_pointer);  // NOLINT (inevitable)
        }
    }

    return nullptr;
}

void Window::FrameBufferSizeCallback(GLFWwindow* glfw_window, int width, int height)
{
    CallWndMethod<&Window::OnResize>(glfw_window, width, height);
}

void Window::MouseCallback(GLFWwindow* glfw_window, double x, double y)
{
    CallWndMethod<&Window::OnMouseMove>(glfw_window, Vec2f{{static_cast<float>(x), static_cast<float>(y)}});
}

void Window::MouseButtonCallback(GLFWwindow* glfw_window, int button, int action, int mods)
{
    CallWndMethod<&Window::OnMouseButton>(glfw_window, button, action, mods);
}

void Window::MouseScrollCallback(GLFWwindow* glfw_window, double x_offset, double y_offset)
{
    CallWndMethod<&Window::OnMouseScroll>(glfw_window, static_cast<float>(x_offset), static_cast<float>(y_offset));
}

void Window::Create()
{
    window_ = glfwCreateWindow(static_cast<int>(width_), static_cast<int>(height_), "KLGL", nullptr, nullptr);

    if (!window_)
    {
        throw std::runtime_error(fmt::format("Failed to create window"));
    }

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FrameBufferSizeCallback);
    glfwSetCursorPosCallback(window_, MouseCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetScrollCallback(window_, MouseScrollCallback);

    double cursor_x{};
    double cursor_y{};
    glfwGetCursorPos(window_, &cursor_x, &cursor_y);
    cursor_.x() = static_cast<float>(cursor_x);
    cursor_.y() = static_cast<float>(cursor_y);
}

void Window::Destroy()
{
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

void Window::OnResize(int width, int height)
{
    width_ = static_cast<uint32_t>(width);
    height_ = static_cast<uint32_t>(height);
}

void Window::OnMouseMove(Vec2f new_cursor)
{
    cursor_ = new_cursor;
}

void Window::OnMouseButton(int button, int action, [[maybe_unused]] int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS)
        {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            input_mode_ = true;
        }
        else if (action == GLFW_RELEASE)
        {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            input_mode_ = false;
        }
        break;
    }
}

void Window::OnMouseScroll([[maybe_unused]] float dx, [[maybe_unused]] float dy) {}

}  // namespace klgl
