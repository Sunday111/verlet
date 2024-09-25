#pragma once

#include <nlohmann/json.hpp>

#include "EverydayTools/Math/Matrix.hpp"

namespace verlet
{
class VerletApp;
class Emitter;
class RadialEmitterConfig;

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

    static nlohmann::json EmitterToJSON(const Emitter& emitter);
    static std::unique_ptr<Emitter> EmitterFromJSON(const nlohmann::json& json);

    static nlohmann::json AppStateToJSON(const VerletApp& app);
};
}  // namespace verlet
