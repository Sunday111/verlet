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
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/random_objects.hpp"

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

void Main(int argc, char** argv)
{
    const std::span arguments{argv, static_cast<size_t>(argc)};

    Settings settings;
    ReadOption(arguments, "--max-objects", settings.max_objects);
    ReadOption(arguments, "--step", settings.step);
    ReadOption(arguments, "--window", settings.window);
    ReadOption(arguments, "--seed", settings.seed);
    ReadOption(arguments, "--density", settings.density);
    ReadOption(arguments, "--max-speed", settings.max_speed);
    ReadOption(arguments, "--threads", settings.threads);
    if (const auto out = Option(arguments, "--out")) settings.out = *out;

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
