#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace klgl::events
{
class EventManager;
}

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
    virtual void PreTick();
    virtual void Tick();
    virtual void PostTick();
    virtual void MainLoop();
    virtual void InitializeReflectionTypes();

    Window& GetWindow();
    const Window& GetWindow() const;

    const std::filesystem::path& GetExecutableDir() const;

    events::EventManager& GetEventManager();

    // Current time. Relative to app start
    float GetTimeSeconds() const;

    // Time (in seconds) when the current fame started. Relative to app start
    float GetCurrentFrameStartTime() const;

    // How many ticks app does per second (on average among last 128 ticks)
    float GetFramerate() const;

    // Duration of the previous tick (in seconds)
    float GetLastFrameDurationSeconds() const;

    void SetTargetFramerate(std::optional<float> framerate);

private:
    std::unique_ptr<State> state_;
};

}  // namespace klgl
