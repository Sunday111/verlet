#include "klgl/application.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <chrono>
#include <concepts>
#include <utility>

#include "klgl/events/event_manager.hpp"
#include "klgl/opengl/debug/annotations.hpp"
#include "klgl/opengl/debug/gl_debug_messenger.hpp"
#include "klgl/platform/os/os.hpp"
#include "klgl/reflection/register_types.hpp"
#include "klgl/window.hpp"
#include "platform/glfw/glfw_state.hpp"

namespace klgl
{

struct Application::State
{
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    GlfwState glfw_;
    std::unique_ptr<Window> window_;
    std::filesystem::path executable_dir_;

    void InitTime()
    {
        app_start_time_ = GetTime();
        std::ranges::fill(frame_start_time_history_, app_start_time_);
    }

    void RegisterFrameStartTime()
    {
        const TimePoint previous_frame_start_time = frame_start_time_history_[current_frame_time_index_];
        current_frame_time_index_ = (current_frame_time_index_ + 1) % frame_start_time_history_.size();
        const TimePoint current_frame_start_time = Clock::now();
        const TimePoint oldest_frame_start_time =
            std::exchange(frame_start_time_history_[current_frame_time_index_], current_frame_start_time);

        framerate_ = static_cast<float>(
            static_cast<double>(frame_start_time_history_.size()) /
            DurationToSeconds<double>(current_frame_start_time - oldest_frame_start_time));

        last_frame_duration_seconds_ = DurationToSeconds<float>(current_frame_start_time - previous_frame_start_time);
    }

    static TimePoint GetTime() { return Clock::now(); }

    template <std::floating_point Result = float, typename Duration>
    static Result DurationToSeconds(Duration&& duration)
    {
        return std::chrono::duration_cast<std::chrono::duration<Result, std::chrono::seconds::period>>(
                   std::forward<Duration>(duration))
            .count();
    }

    float GetRelativeTimeSeconds() const { return State::DurationToSeconds(GetTime() - app_start_time_); }

    float GetCurrentFrameStartTime() const
    {
        return State::DurationToSeconds(frame_start_time_history_[current_frame_time_index_] - app_start_time_);
    }

    void AlignWithFramerate()
    {
        if (target_framerate_.has_value())
        {
            const float frame_start = GetCurrentFrameStartTime();
            constexpr float target_frame_duration = (1 / 60.f) * 0.9995f;
            while (GetRelativeTimeSeconds() - frame_start < target_frame_duration)
            {
            }
        }
    }

    TimePoint app_start_time_{};
    static constexpr size_t kFrameTimeHistorySize = 128;
    std::array<TimePoint, kFrameTimeHistorySize> frame_start_time_history_{};
    float last_frame_duration_seconds_ = 0.f;
    float framerate_ = 0.0f;
    uint8_t current_frame_time_index_ = kFrameTimeHistorySize - 1;
    std::optional<float> target_framerate_;
    events::EventManager event_manager_;
};

Application::Application()
{
    state_ = std::make_unique<State>();
}

Application::~Application() = default;

int InitializeGLAD_impl()
{
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))  // NOLINT
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    return 42;
}

void InitializeGLAD()
{
    [[maybe_unused]] static int once = InitializeGLAD_impl();
}

void Application::Initialize()
{
    state_->executable_dir_ = os::GetExecutableDir();

    InitializeReflectionTypes();

    state_->glfw_.Initialize();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    {
        uint32_t window_width = 900;
        uint32_t window_height = 900;
        if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
        {
            float x_scale = 0.f, y_scale = 0.f;
            glfwGetMonitorContentScale(monitor, &x_scale, &y_scale);
            window_width = static_cast<uint32_t>(static_cast<float>(window_width) * x_scale);
            window_height = static_cast<uint32_t>(static_cast<float>(window_height) * y_scale);
        }

        state_->window_ = std::make_unique<Window>(*this, window_width, window_height);
    }

    // GLAD can be initialized only when glfw has window context
    state_->window_->MakeContextCurrent();
    InitializeGLAD();
    GlDebugMessenger::Start();

    glfwSwapInterval(0);
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(state_->window_->GetGlfwWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
    {
        float xscale = 0, yscale = 0;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);

        ImGui::GetStyle().ScaleAllSizes(2);
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig font_config{};
        font_config.SizePixels = 13 * xscale;
        io.Fonts->AddFontDefault(&font_config);
    }

    state_->InitTime();
}

void Application::Run()
{
    Initialize();
    MainLoop();
}

void Application::PreTick()
{
    OpenGl::Viewport(
        0,
        0,
        static_cast<GLint>(state_->window_->GetWidth()),
        static_cast<GLint>(state_->window_->GetHeight()));

    OpenGl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::Tick() {}

void Application::PostTick()
{
    {
        ScopeAnnotation imgui_render("ImGUI");
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    state_->window_->SwapBuffers();
    glfwPollEvents();
}

void Application::MainLoop()
{
    while (!state_->window_->ShouldClose())
    {
        ScopeAnnotation frame_annotation("Frame");
        state_->RegisterFrameStartTime();

        PreTick();
        Tick();
        PostTick();
        state_->AlignWithFramerate();
    }
}

void Application::InitializeReflectionTypes()
{
    RegisterReflectionTypes();
}

Window& Application::GetWindow()
{
    return *state_->window_;
}

const Window& Application::GetWindow() const
{
    return *state_->window_;
}

const std::filesystem::path& Application::GetExecutableDir() const
{
    return state_->executable_dir_;
}

float Application::GetTimeSeconds() const
{
    return state_->GetRelativeTimeSeconds();
}

float Application::GetCurrentFrameStartTime() const
{
    return state_->GetCurrentFrameStartTime();
}

float Application::GetFramerate() const
{
    return state_->framerate_;
}

float Application::GetLastFrameDurationSeconds() const
{
    return state_->last_frame_duration_seconds_;
}

void Application::SetTargetFramerate(std::optional<float> framerate)
{
    state_->target_framerate_ = framerate;
}

events::EventManager& Application::GetEventManager()
{
    return state_->event_manager_;
}

}  // namespace klgl
