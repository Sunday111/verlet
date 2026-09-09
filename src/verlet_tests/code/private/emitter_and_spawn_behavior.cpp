#include <gtest/gtest.h>

#include <functional>
#include <limits>

#include "verlet/emitters/flat_emitter.hpp"
#include "verlet/emitters/radial_emitter.hpp"
#include "verlet/json/json_helpers.hpp"
#include "verlet/json/json_keys.hpp"
#include "verlet/physics/verlet_solver.hpp"
#include "verlet/random_objects.hpp"
#include "verlet/tools/spawn_random_objects_tool.hpp"
#include "verlet/tools/tool.hpp"
#include "verlet/verlet_app.hpp"

namespace
{
nlohmann::json ValidRadialEmitterJSON()
{
    const verlet::RadialEmitter emitter;
    return verlet::JSONHelpers::EmitterToJSON(emitter);
}

nlohmann::json ValidFlatEmitterJSON()
{
    const verlet::FlatEmitter emitter;
    return verlet::JSONHelpers::EmitterToJSON(emitter);
}

nlohmann::json ValidAppStateJSON()
{
    using verlet::JSONKeys;
    return {
        {JSONKeys::kWindowSize, {{JSONKeys::kX, 900}, {JSONKeys::kY, 700}}},
        {JSONKeys::kMaxObjectsCount, 1234},
        {JSONKeys::kEmitters, {ValidRadialEmitterJSON(), ValidFlatEmitterJSON()}},
    };
}

std::string ExceptionMessage(const std::function<void()>& operation)
{
    try
    {
        operation();
    }
    catch (const std::exception& error)
    {
        return error.what();
    }
    return {};
}

TEST(RadialEmitterConfigTest, AcceptsFiniteSoftRangeValues)  // NOLINT
{
    verlet::RadialEmitterConfig config;
    config.position = {12.f, -7.f};
    config.radius = 3.f;
    config.phase_degrees = 720.f;
    config.rotation_speed = -721.f;

    EXPECT_FALSE(verlet::RadialEmitter::ValidateConfig(config));
}

TEST(RadialEmitterConfigTest, RejectsEveryInvalidInvariant)  // NOLINT
{
    verlet::RadialEmitterConfig config;

    config.position.x() = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kPosition);
    config = {};

    config.radius = -0.01f;
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kRadius);
    config.radius = std::numeric_limits<float>::infinity();
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kRadius);
    config = {};

    config.phase_degrees = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kPhaseDegrees);
    config = {};

    config.sector_degrees = 361.f;
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kSectorDegrees);
    config.sector_degrees = -1.f;
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kSectorDegrees);
    config = {};

    config.speed_factor = 241.f;
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kSpeedFactor);
    config.speed_factor = -241.f;
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kSpeedFactor);
    config = {};

    config.rotation_speed = std::numeric_limits<float>::infinity();
    EXPECT_EQ(verlet::RadialEmitter::ValidateConfig(config), verlet::JSONKeys::kRotationSpeed);
}

TEST(RadialEmitterConfigTest, NormalizesPhaseSymmetrically)  // NOLINT
{
    EXPECT_FLOAT_EQ(verlet::RadialEmitter::NormalizeDegrees(540.f), -180.f);
    EXPECT_FLOAT_EQ(verlet::RadialEmitter::NormalizeDegrees(-540.f), 180.f);
    EXPECT_FLOAT_EQ(verlet::RadialEmitter::NormalizeDegrees(721.f), 1.f);
}

TEST(RadialEmitterConfigTest, ConfigurationResetKeepsEmitterEnabled)  // NOLINT
{
    verlet::RadialEmitter emitter;
    emitter.enabled = true;
    emitter.config.phase_degrees = 45.f;

    emitter.ResetConfigurationState();

    EXPECT_TRUE(emitter.enabled);
    EXPECT_FLOAT_EQ(emitter.state.phase_degrees, 45.f);
}

TEST(RadialEmitterConfigTest, ClonePreparationDisarmsAndResetsState)  // NOLINT
{
    verlet::RadialEmitter emitter;
    emitter.enabled = true;
    emitter.pending_kill = true;
    emitter.clone_requested = true;
    emitter.config.phase_degrees = 30.f;
    emitter.state.phase_degrees = 12.f;

    emitter.PrepareClone();

    EXPECT_FALSE(emitter.enabled);
    EXPECT_FALSE(emitter.pending_kill);
    EXPECT_FALSE(emitter.clone_requested);
    EXPECT_FLOAT_EQ(emitter.state.phase_degrees, 30.f);
}

TEST(FlatEmitterConfigTest, AcceptsZeroDirectionAndFiniteSoftRangeValues)  // NOLINT
{
    verlet::FlatEmitterConfig config;
    config.start = {-10.f, 8.f};
    config.end = {11.f, -9.f};
    config.direction = {};
    config.spacing = 12.f;

    EXPECT_FALSE(verlet::FlatEmitter::ValidateConfig(config));
}

TEST(FlatEmitterConfigTest, RejectsEveryInvalidInvariant)  // NOLINT
{
    verlet::FlatEmitterConfig config;

    config.start.y() = std::numeric_limits<float>::infinity();
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kStart);
    config = {};

    config.end.x() = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kEnd);
    config = {};

    config.direction.y() = std::numeric_limits<float>::infinity();
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kDirection);
    config = {};

    config.spacing = -0.01f;
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kSpacing);
    config.spacing = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kSpacing);
    config = {};

    config.speed_factor = 241.f;
    EXPECT_EQ(verlet::FlatEmitter::ValidateConfig(config), verlet::JSONKeys::kSpeedFactor);
}

TEST(EmitterTest, ClampsSpawnPointEstimateBeforeIntegerConversion)  // NOLINT
{
    EXPECT_EQ(verlet::Emitter::ClampSpawnPointCount(0.f), 1U);
    EXPECT_EQ(verlet::Emitter::ClampSpawnPointCount(12.9f), 12U);
    EXPECT_EQ(
        verlet::Emitter::ClampSpawnPointCount(std::numeric_limits<float>::max()),
        verlet::Emitter::kMaxSpawnPoints);
}

TEST(RandomObjectsTest, CapsSpawnCount)  // NOLINT
{
    verlet::VerletSolver solver;
    const verlet::RandomObjectsParams params{.count = 20, .seed = std::numeric_limits<uint32_t>::max()};

    EXPECT_EQ(verlet::SpawnRandomObjects(solver, params, 7), 7U);
    EXPECT_EQ(solver.objects.ObjectsCount(), 7U);
}

TEST(AppStateJSONTest, ParsesCompletePresetBeforeCommit)  // NOLINT
{
    auto committed = verlet::JSONHelpers::AppStateFromJSON(ValidAppStateJSON());
    EXPECT_EQ(committed.window_size, (edt::Vec2<uint32_t>{900, 700}));
    ASSERT_TRUE(committed.max_objects_count);
    EXPECT_EQ(*committed.max_objects_count, 1234U);
    EXPECT_FALSE(committed.max_objects_saturation);
    ASSERT_EQ(committed.emitters.size(), 2U);

    auto invalid = ValidAppStateJSON();
    invalid[verlet::JSONKeys::kWindowSize][verlet::JSONKeys::kX] = 1600;
    invalid[verlet::JSONKeys::kMaxObjectsCount] = 9999;
    invalid[verlet::JSONKeys::kEmitters][1]["Flat"][verlet::JSONKeys::kSpacing] = -1.f;

    const std::string message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(invalid); });
    EXPECT_NE(message.find("Emitters[1]"), std::string::npos);
    EXPECT_NE(message.find("Spacing"), std::string::npos);
    EXPECT_EQ(committed.window_size, (edt::Vec2<uint32_t>{900, 700}));
    EXPECT_EQ(*committed.max_objects_count, 1234U);
    EXPECT_EQ(committed.emitters.size(), 2U);
}

TEST(AppStateJSONTest, ValidatesWindowFieldBounds)  // NOLINT
{
    auto json = ValidAppStateJSON();
    json[verlet::JSONKeys::kWindowSize][verlet::JSONKeys::kX] = 8192;
    json[verlet::JSONKeys::kWindowSize][verlet::JSONKeys::kY] = 8192;
    const auto maximum = verlet::JSONHelpers::AppStateFromJSON(json);
    EXPECT_EQ(maximum.window_size, (edt::Vec2<uint32_t>{8192, 8192}));

    json = ValidAppStateJSON();
    json[verlet::JSONKeys::kWindowSize][verlet::JSONKeys::kX] = -1;
    std::string message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(message.find("WindowSize.X"), std::string::npos);

    json = ValidAppStateJSON();
    json[verlet::JSONKeys::kWindowSize][verlet::JSONKeys::kY] = 8193;
    message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(message.find("WindowSize.Y"), std::string::npos);
}

TEST(AppStateJSONTest, RejectsInvalidBudgetRepresentations)  // NOLINT
{
    auto json = ValidAppStateJSON();
    json[verlet::JSONKeys::kMaxObjectsSaturation] = 0.5f;
    EXPECT_FALSE(ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); }).empty());

    json = ValidAppStateJSON();
    json.erase(verlet::JSONKeys::kMaxObjectsCount);
    json[verlet::JSONKeys::kMaxObjectsSaturation] = std::numeric_limits<float>::infinity();
    const std::string saturation_message =
        ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(saturation_message.find("MaxObjectsSaturation"), std::string::npos);

    json = ValidAppStateJSON();
    json[verlet::JSONKeys::kMaxObjectsCount] = -1;
    const std::string count_message =
        ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(count_message.find("MaxObjectsCount"), std::string::npos);
}

TEST(AppStateJSONTest, RejectsNonFiniteEmitterFieldWithIndex)  // NOLINT
{
    auto json = ValidAppStateJSON();
    json[verlet::JSONKeys::kEmitters][0]["Radial"][verlet::JSONKeys::kRadius] = std::numeric_limits<float>::quiet_NaN();
    const std::string message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(message.find("Emitters[0]"), std::string::npos);
    EXPECT_NE(message.find("Radius"), std::string::npos);
}

TEST(AppStateJSONTest, RejectsInvalidTypesWithFieldContext)  // NOLINT
{
    auto json = ValidAppStateJSON();
    json[verlet::JSONKeys::kMaxObjectsCount] = "many";
    std::string message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(message.find("MaxObjectsCount"), std::string::npos);

    json = ValidAppStateJSON();
    json[verlet::JSONKeys::kEmitters][1]["Flat"][verlet::JSONKeys::kLocalDirection] = "yes";
    message = ExceptionMessage([&] { std::ignore = verlet::JSONHelpers::AppStateFromJSON(json); });
    EXPECT_NE(message.find("Emitters[1]"), std::string::npos);
    EXPECT_NE(message.find("LocalDirection"), std::string::npos);
}

TEST(VerletSolverTest, ZeroThreadsUsesOneWorker)  // NOLINT
{
    verlet::VerletSolver solver;
    solver.SetThreadsCount(0);
    EXPECT_EQ(solver.GetThreadsCount(), 1U);
}
}  // namespace

TEST(VerletAppTest, ClearingObjectsNotifiesTheActiveToolBeforeReusingIdentifiers)  // NOLINT
{
    class ReferencingTool : public verlet::Tool
    {
    public:
        using Tool::Tool;
        verlet::ObjectId referenced;
        bool cleared_while_live = false;
        void ClearObjectReferences() override
        {
            cleared_while_live = app_.solver.objects.Contains(referenced);
            referenced = verlet::kInvalidObjectId;
        }
        [[nodiscard]] verlet::ToolType GetToolType() const override { return verlet::ToolType::SpawnObjects; }
    };

    for (size_t count : {size_t{1}, size_t{8}})
    {
        verlet::VerletApp app;
        auto tool = std::make_unique<ReferencingTool>(app);
        auto& active_tool = *tool;
        app.tool_ = std::move(tool);
        for (size_t i = 0; i != count; ++i) active_tool.referenced = std::get<0>(app.solver.objects.Alloc());

        app.DeleteAllObjects();

        EXPECT_TRUE(active_tool.cleared_while_live);
        EXPECT_FALSE(active_tool.referenced.IsValid());
        EXPECT_EQ(app.solver.objects.ObjectsCount(), 0U);
        EXPECT_EQ(app.tool_.get(), &active_tool);
        const auto reused = std::get<0>(app.solver.objects.Alloc());
        EXPECT_EQ(reused, verlet::ObjectId::FromValue(0));
        EXPECT_FALSE(active_tool.referenced.IsValid());

        active_tool.referenced = reused;
        active_tool.cleared_while_live = false;
        verlet::SpawnRandomObjectsTool random_tool(app);
        random_tool.GetParams().count = 3;
        EXPECT_EQ(random_tool.ReplaceAll(), 3U);
        EXPECT_TRUE(active_tool.cleared_while_live);
        EXPECT_FALSE(active_tool.referenced.IsValid());
        EXPECT_EQ(app.solver.objects.ObjectsCount(), 3U);
    }
}
