#pragma once

// clang-format off
#include "klgl/opengl/gl_api.hpp"
#include <GLFW/glfw3.h>
// clang-format on

namespace klgl
{

class GlfwState
{
public:
    ~GlfwState()
    {
        Uninitialize();
    }

    void Initialize()
    {
        [[unlikely]] if (!glfwInit())
        {
            throw std::runtime_error("failed to initialize glfw");
        }

        initialized_ = true;
    }

    void Uninitialize()
    {
        if (initialized_)
        {
            glfwTerminate();
            initialized_ = false;
        }
    }

private:
    bool initialized_ = false;
};

}  // namespace klgl
