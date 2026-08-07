#include <chrono>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <tuple>

#include "edt/math/matrix.hpp"
#include "fmt/core.h"
#include "fmt/std.h"  // IWYU pragma: keep
#include "klvk/error_handling.hpp"
#include "klvk/filesystem/filesystem.hpp"
#include "klvk/image/image_decoder.hpp"
#include "klvk/platform/os/os.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy_array.hpp"
#include "verlet/gui/app_gui.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
namespace
{

struct Inputs
{
    std::filesystem::path preset;
    std::filesystem::path image;
    // Nothing means the settled positions are simulated rather than read back
    // from a dump an interactive session wrote.
    std::optional<std::filesystem::path> positions;
};

// A run can go for many minutes without printing anything, which looks exactly
// like a hang. Reporting is throttled by wall clock rather than by a frame count
// because one frame costs anywhere from microseconds to about a second depending
// on how many objects are alive by then.
class ProgressLog
{
public:
    using Clock = std::chrono::steady_clock;

    static constexpr auto kInterval = std::chrono::seconds{2};

    enum class Phase : u8
    {
        Precompute,
        Record,
    };

    // Nothing for record_frames when the run has no frame-based exit, and how far
    // through the whole thing is then unknowable.
    ProgressLog(Clock::time_point started, u64 precompute_frames, std::optional<u64> record_frames)
        : started_{started},
          precompute_frames_{precompute_frames},
          record_frames_{record_frames}
    {
    }

    void Frame(Phase phase, u64 frame, size_t objects)
    {
        const auto now = Clock::now();
        if (now - last_report_ < kInterval) return;
        last_report_ = now;

        const auto elapsed = std::chrono::duration<double>(now - started_).count();
        const bool recording = phase == Phase::Record;
        const std::optional<u64> phase_frames = recording ? record_frames_ : std::optional{precompute_frames_};
        const u64 done = frame + (recording ? precompute_frames_ : 0);

        fmt::println(
            "[{}][{}][{}]: frame {}/{}, {} objects",
            Duration(elapsed),
            GlobalShare(done),
            recording ? "record" : "precompute",
            frame,
            phase_frames ? fmt::format("{}", *phase_frames) : "?",
            objects);

        // Redirected output is block buffered, and a progress report that only
        // appears once the run is over answers nothing. Nothing useful can be
        // done if the flush itself fails.
        std::ignore = std::fflush(stdout);
    }

private:
    [[nodiscard]] std::string GlobalShare(u64 done) const
    {
        if (!record_frames_) return "--%";
        const auto total = static_cast<double>(precompute_frames_ + *record_frames_);
        return fmt::format("{:.0f}%", (100.0 * static_cast<double>(done)) / total);
    }

    [[nodiscard]] static std::string Duration(double seconds)
    {
        const auto total = static_cast<u64>(seconds);
        if (total < 60) return fmt::format("{}s", total);
        return fmt::format("{}m{:02}s", total / 60, total % 60);
    }

    Clock::time_point started_;
    u64 precompute_frames_ = 0;
    std::optional<u64> record_frames_;
    Clock::time_point last_report_ = started_;
};

[[nodiscard]] klvk::DecodedImage ReadImage(const std::filesystem::path& path)
{
    std::string encoded;
    klvk::Filesystem::ReadFile(path, encoded);
    auto decoded = klvk::DecodeImage(std::span{reinterpret_cast<const u8*>(encoded.data()), encoded.size()});
    klvk::ErrorHandling::Ensure(decoded.has_value(), "Failed to decode image at {}", path);
    return std::move(*decoded);
}

[[nodiscard]] std::vector<edt::Vec2f> ReadPositions(const std::filesystem::path& path)
{
    std::string content;
    klvk::Filesystem::ReadFile(path, content);
    std::istringstream stream(std::move(content));

    size_t count = 0;
    klvk::ErrorHandling::Ensure(static_cast<bool>(stream >> count), "Failed to read object count from {}", path);

    std::vector<edt::Vec2f> positions(count);
    for (auto& position : positions)
    {
        klvk::ErrorHandling::Ensure(
            static_cast<bool>(stream >> position.x() >> position.y()),
            "{} declares {} positions but holds fewer",
            path,
            count);
    }

    return positions;
}

[[nodiscard]] std::vector<edt::Vec3u8>
SampleColors(const klvk::DecodedImage& image, std::span<const edt::Vec2f> positions, const edt::FloatRange2Df& sim_area)
{
    const auto max_pixel_coord = image.size.Cast<i32>() - 1;
    const auto image_size_f = image.size.Cast<float>();
    const auto min_coord = sim_area.Min();
    const auto coord_extent = sim_area.Extent();

    std::vector<edt::Vec3u8> colors;
    colors.reserve(positions.size());

    for (const auto& position : positions)
    {
        const auto relative = (position - min_coord) / coord_extent;
        auto pixel_coord = edt::Math::Clamp((relative * image_size_f).Cast<i32>(), edt::Vec2<i32>{}, max_pixel_coord);
        pixel_coord.y() = max_pixel_coord.y() - pixel_coord.y();

        const auto row = static_cast<size_t>(pixel_coord.y());
        const auto column = static_cast<size_t>(pixel_coord.x());
        const size_t offset = 4uz * (row * image.size.x() + column);
        colors.emplace_back(image.pixels[offset], image.pixels[offset + 1], image.pixels[offset + 2]);
    }

    return colors;
}

class VerletVideoApp : public VerletApp
{
public:
    using Super = VerletApp;

    explicit VerletVideoApp(Inputs inputs) : inputs_{std::move(inputs)} {}

    void Initialize() override
    {
        Super::Initialize();
        LoadAppState(inputs_.preset);
        UpdateWorldRange(std::numeric_limits<float>::max());

        // The solver must see the same state in both passes for object i to land
        // where the first pass said it would, so the run is single threaded and
        // the settling loop mirrors what Tick does around UpdateSimulation.
        solver.SetThreadsCount(1);
        StartEmitting();

        const u64 settle_frames = SettleFrames();
        progress_.emplace(started_, settle_frames, GetDiagnosticExitFrame());

        auto color_strategy = std::make_unique<SpawnColorStrategyArray>(*this);
        color_strategy->colors =
            SampleColors(ReadImage(inputs_.image), SettledPositions(settle_frames), solver.GetSimArea());

        solver.DeleteAll();
        for (auto& emitter : GetEmitters()) emitter.ResetRuntimeState();
        spawn_color_strategy_ = std::move(color_strategy);
        StartEmitting();
    }

    void Tick() override
    {
        Super::Tick();
        progress_->Frame(ProgressLog::Phase::Record, ++recorded_frames_, solver.objects.ObjectsCount());
    }

private:
    void StartEmitting()
    {
        time_steps_ = 0;
        EnableAllEmitters();
    }

    // The frame the picture is composed for. It belongs beside 'exit' in the
    // diagnostic configuration because the two are one decision: positions are
    // sampled here, so this is the frame the settled image appears on.
    [[nodiscard]] u64 SettleFrames() const
    {
        static constexpr u64 kDefault = 3600;
        static constexpr std::string_view kKey = "settle_frames";

        const nlohmann::json* application = GetDiagnosticApplicationConfig();
        if (application == nullptr || !application->contains(kKey)) return kDefault;

        const nlohmann::json& value = application->at(kKey);
        klvk::ErrorHandling::Ensure(
            value.is_number_unsigned() && value.get<u64>() != 0,
            "application.{} must be a positive integer, got {}",
            kKey,
            value.dump());

        return value.get<u64>();
    }

    [[nodiscard]] std::vector<edt::Vec2f> SettledPositions(u64 settle_frames)
    {
        if (inputs_.positions) return ReadPositions(*inputs_.positions);

        fmt::println("Simulating {} frames to find where the objects settle", settle_frames);
        for (const u64 frame : std::views::iota(u64{1}, settle_frames + 1))
        {
            UpdateWorldRange();
            UpdateSimulation();
            progress_->Frame(ProgressLog::Phase::Precompute, frame, solver.objects.ObjectsCount());
        }

        std::vector<edt::Vec2f> positions;
        positions.reserve(solver.objects.ObjectsCount());
        for (const auto& object : solver.objects.Objects()) positions.push_back(object.position);
        fmt::println("Settled {} objects", positions.size());

        return positions;
    }

    Inputs inputs_;
    ProgressLog::Clock::time_point started_ = ProgressLog::Clock::now();
    std::optional<ProgressLog> progress_;
    u64 recorded_frames_ = 0;
};

}  // namespace
}  // namespace verlet

void Main(int argc, char** argv)
{
    const std::span arguments{argv, static_cast<size_t>(argc)};
    const auto option = [&](std::string_view name) -> std::optional<std::string_view>
    {
        for (size_t i = 1; i + 1 < arguments.size(); ++i)
        {
            if (name == arguments[i]) return std::string_view{arguments[i + 1]};
        }

        return std::nullopt;
    };

    const auto executable_dir = klvk::os::GetExecutableDir();
    verlet::VerletVideoApp app{{
        .preset = option("--preset")
                      .transform([](auto v) { return std::filesystem::path{v}; })
                      .value_or(executable_dir / verlet::AppGUI::kDefaultPresetFileName),
        .image = option("--image")
                     .transform([](auto v) { return std::filesystem::path{v}; })
                     .value_or(executable_dir / "content" / "target_image.png"),
        .positions = option("--positions").transform([](auto v) { return std::filesystem::path{v}; }),
    }};

    app.RunWithArguments(argc, argv);
}

int main(int argc, char** argv)
{
    return klvk::ErrorHandling::InvokeAndCatchAll([&] { Main(argc, argv); });
}
