#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <vector>

#include "edt/math/matrix.hpp"
#include "verlet/emitters/emitter.hpp"

namespace verlet
{
class VerletApp;
class RadialEmitterConfig;
class FlatEmitterConfig;

struct ParsedAppState
{
    edt::Vec2<uint32_t> window_size;
    std::optional<size_t> max_objects_count;
    std::optional<float> max_objects_saturation;
    std::vector<std::unique_ptr<Emitter>> emitters;
};

class JSONHelpers
{
public:
    class Internal;

    static const nlohmann::json& GetKey(const nlohmann::json& object, const std::string_view& key);

    static nlohmann::json VectorToJSON(const edt::Vec2f& v);
    static nlohmann::json VectorToJSON(const edt::Vec2i& v);
    static edt::Vec2f Vec2fFromJSON(const nlohmann::json& json);
    static edt::Vec2i Vec2iFromJSON(const nlohmann::json& json);

    static nlohmann::json RadialEmitterToJSON(const RadialEmitterConfig& emitter);
    static RadialEmitterConfig RadialEmitterFromJSON(const nlohmann::json& json);

    static nlohmann::json FlatEmitterToJSON(const FlatEmitterConfig& emitter);
    static FlatEmitterConfig FlatEmitterFromJSON(const nlohmann::json& json);

    static nlohmann::json EmitterToJSON(const Emitter& emitter);
    static std::unique_ptr<Emitter> EmitterFromJSON(const nlohmann::json& json);

    static nlohmann::json AppStateToJSON(const VerletApp& app);
    static ParsedAppState AppStateFromJSON(const nlohmann::json& json);
};
}  // namespace verlet
