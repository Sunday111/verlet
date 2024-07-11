#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace klgl
{

class Window;

class Application
{
    struct State;

public:
    Application();
    virtual ~Application();

    virtual void Initialize();
    virtual void Run();
    virtual void PreTick(float dt);
    virtual void Tick(float dt);
    virtual void PostTick(float dt);
    virtual void MainLoop();
    virtual void InitializeReflectionTypes();

    Window& GetWindow();
    const Window& GetWindow() const;

    const std::filesystem::path& GetExecutableDir() const;

private:
    std::unique_ptr<State> state_;
};

}  // namespace klgl
