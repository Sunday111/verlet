#include <benchmark/benchmark.h>

#include "verlet/coloring/spawn_color/spawn_color_strategy_rainbow.hpp"
#include "verlet/emitters/burst_emitter.hpp"
#include "verlet/verlet_app.hpp"

namespace
{
void BurstSimulation(benchmark::State& state)
{
    verlet::VerletApp app;
    app.solver.SetThreadsCount(static_cast<size_t>(state.range(0)));
    app.solver.SetSimArea({.x = {-200.f, 200.f}, .y = {-150.f, 150.f}});
    app.max_objects_count_ = 100000;
    app.spawn_color_strategy_ = std::make_unique<verlet::SpawnColorStrategyRainbow>(app);
    verlet::BurstEmitter emitter;
    emitter.enabled = true;
    emitter.Tick(app);

    for (auto iteration : state)
    {
        benchmark::DoNotOptimize(app.solver.Update());
    }
}

BENCHMARK(BurstSimulation)->Arg(1)->Arg(8)->Arg(32)->Iterations(300)->UseRealTime()->Unit(benchmark::kMillisecond);
}  // namespace
