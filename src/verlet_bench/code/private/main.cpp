#include <algorithm>
#include <charconv>
#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "fmt/core.h"
#include "fmt/os.h"
#include "klvk/error_handling.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "verlet/emitters/burst_emitter.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/random_objects.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
namespace
{
struct Settings
{
    size_t max_objects = 1'000'000;
    size_t step = 10'000;
    size_t window = 20;
    uint32_t seed = 1234;

    // How much of the world the objects take once all of them are in. The world is sized for
    // that from the start, so the grid never changes shape mid-run and every measurement is
    // of one grid holding more and more objects.
    float density = 0.85f;

    float max_speed = 10.f;
    size_t threads = 0;
    std::string_view out = "bench.csv";
};

[[nodiscard]] double Milliseconds(std::chrono::nanoseconds value)
{
    return std::chrono::duration<double, std::milli>(value).count();
}

[[nodiscard]] std::optional<std::string_view> Option(std::span<char*> arguments, std::string_view name)
{
    for (size_t i = 1; i + 1 < arguments.size(); ++i)
    {
        if (name == arguments[i]) return std::string_view{arguments[i + 1]};
    }

    return std::nullopt;
}

template <typename T>
void ReadOption(std::span<char*> arguments, std::string_view name, T& destination)
{
    const auto text = Option(arguments, name);
    if (!text) return;

    const auto result = std::from_chars(text->data(), text->data() + text->size(), destination);
    klvk::ErrorHandling::Ensure(result.ec == std::errc{}, "{} expects a number, got {}", name, *text);
}

void RunBurst(size_t threads, size_t frames)
{
    verlet::VerletApp app;
    app.solver.SetThreadsCount(threads);
    app.solver.SetSimArea({.x = {-200.f, 200.f}, .y = {-150.f, 150.f}});
    app.max_objects_count_ = 100000;
    app.spawn_color_strategy_ = std::make_unique<verlet::SpawnColorStrategyRainbow>(app);
    verlet::BurstEmitter emitter;
    emitter.enabled = true;
    const auto start = std::chrono::steady_clock::now();
    emitter.Tick(app);
    const double spawn_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    verlet::VerletSolver::UpdateStats sum{};
    for (size_t frame = 0; frame < frames; ++frame)
    {
        const auto stats = app.solver.Update();
        sum.total += stats.total;
        sum.rebuild_grid += stats.rebuild_grid;
        sum.solve_collisions += stats.solve_collisions;
        sum.update_positions += stats.update_positions;
    }
    double checksum = 0;
    for (const auto& object : app.solver.objects.Objects())
    {
        klvk::ErrorHandling::Ensure(object.position.IsFinite(), "Non-finite particle position");
        checksum += static_cast<double>(object.position.x()) + 3.0 * static_cast<double>(object.position.y());
    }
    const auto ms = [frames](auto duration)
    {
        return std::chrono::duration<double, std::milli>(duration).count() / static_cast<double>(frames);
    };
    fmt::println(
        "particles={} threads={} frames={} spawn_ms={:.3f} frame_ms={:.3f} grid_ms={:.3f} collision_ms={:.3f} "
        "position_ms={:.3f} checksum={:.12g}",
        app.solver.objects.ObjectsCount(),
        threads,
        frames,
        spawn_ms,
        ms(sum.total),
        ms(sum.rebuild_grid),
        ms(sum.solve_collisions),
        ms(sum.update_positions),
        checksum);
}

void Main(int argc, char** argv)
{
    const std::span arguments{argv, static_cast<size_t>(argc)};

    Settings settings;
    const bool burst =
        std::ranges::any_of(arguments, [](const char* arg) { return std::string_view{arg} == "--burst"; });
    if (burst)
    {
        settings.window = 300;
        settings.threads = 1;
    }
    ReadOption(arguments, "--max-objects", settings.max_objects);
    ReadOption(arguments, "--step", settings.step);
    ReadOption(arguments, "--window", settings.window);
    ReadOption(arguments, "--seed", settings.seed);
    ReadOption(arguments, "--density", settings.density);
    ReadOption(arguments, "--max-speed", settings.max_speed);
    ReadOption(arguments, "--threads", settings.threads);
    if (const auto out = Option(arguments, "--out")) settings.out = *out;

    if (burst)
    {
        klvk::ErrorHandling::Ensure(settings.threads > 0 && settings.window > 0, "Threads and window must be positive");
        RunBurst(settings.threads, settings.window);
        return;
    }

    const auto world = 0.5f * std::sqrt(static_cast<float>(settings.max_objects) / settings.density);

    VerletSolver solver;
    solver.SetSimArea({.x = {.begin = -world, .end = world}, .y = {.begin = -world, .end = world}});
    if (settings.threads != 0) solver.SetThreadsCount(settings.threads);

    auto csv = fmt::output_file(std::string{settings.out});
    csv.print("objects,cells,threads,total_ms,rebuild_ms,solve_ms,positions_ms\n");

    fmt::println(
        "step={} window={} seed={} density={} max_speed={} world={:.0f} threads={}",
        settings.step,
        settings.window,
        settings.seed,
        settings.density,
        settings.max_speed,
        world,
        solver.GetThreadsCount());
    fmt::println("{:>9} {:>9} {:>9} {:>9} {:>9}", "objects", "total", "rebuild", "solve", "positions");

    uint32_t stage = 0;
    while (solver.objects.ObjectsCount() < settings.max_objects)
    {
        SpawnRandomObjects(
            solver,
            {
                .count = std::min(settings.step, settings.max_objects - solver.objects.ObjectsCount()),
                .seed = settings.seed + stage,
                .max_speed = settings.max_speed,
                .movable = true,
            });
        ++stage;

        VerletSolver::UpdateStats sum{};
        for ([[maybe_unused]] const size_t frame : std::views::iota(size_t{0}, settings.window))
        {
            const auto stats = solver.Update();
            sum.total += stats.total;
            sum.rebuild_grid += stats.rebuild_grid;
            sum.solve_collisions += stats.solve_collisions;
            sum.update_positions += stats.update_positions;
        }

        const auto frames = static_cast<double>(settings.window);
        const auto objects = solver.objects.ObjectsCount();
        csv.print(
            "{},{},{},{:.4f},{:.4f},{:.4f},{:.4f}\n",
            objects,
            solver.GetGridCellsCount(),
            solver.GetThreadsCount(),
            Milliseconds(sum.total) / frames,
            Milliseconds(sum.rebuild_grid) / frames,
            Milliseconds(sum.solve_collisions) / frames,
            Milliseconds(sum.update_positions) / frames);
        csv.flush();

        fmt::println(
            "{:>9} {:>9.3f} {:>9.3f} {:>9.3f} {:>9.3f}",
            objects,
            Milliseconds(sum.total) / frames,
            Milliseconds(sum.rebuild_grid) / frames,
            Milliseconds(sum.solve_collisions) / frames,
            Milliseconds(sum.update_positions) / frames);
    }
}

}  // namespace
}  // namespace verlet

int main(int argc, char** argv)
{
    return klvk::ErrorHandling::InvokeAndCatchAll([&] { verlet::Main(argc, argv); });
}
