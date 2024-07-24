#pragma once

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace klgl
{

class GlfwState
{
public:
    ~GlfwState() { Uninitialize(); }

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
