#include "verlet/json/json_helpers.hpp"

#include <magic_enum.hpp>

#include "ass/fixed_unordered_map.hpp"
#include "klgl/error_handling.hpp"
#include "klgl/macro/ensure_enum_size.hpp"
#include "klgl/template/constexpr_string_hash.hpp"
#include "verlet/emitters/radial_emitter.hpp"
#include "verlet/json/json_keys.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{

template <typename T>
    requires(std::is_enum_v<T>)
static constexpr auto MakeEnumParseMap()
{
    constexpr size_t num_entries = magic_enum::enum_count<T>();
    ass::FixedUnorderedMap<num_entries, std::string_view, T, klgl::ConstexprStringHasher> m;
    for (auto kv : magic_enum::enum_entries<T>())
    {
        assert(!m.Contains(std::get<1>(kv)));
        m.Add(std::get<1>(kv), std::get<0>(kv));
    }
    return m;
}

class JSONHelpers::Internal
{
public:
    static constexpr auto kEmitterTypeParseMap = MakeEnumParseMap<EmitterType>();

    template <typename Map>
        requires(std::same_as<typename Map::Key, std::string_view> && std::is_enum_v<typename Map::Value>)
    static constexpr auto ParseEnum(const Map& map, const std::string_view& text)
    {
        using Value = typename Map::Value;
        [[unlikely]] if (!map.Contains(text))
        {
            klgl::ErrorHandling::ThrowWithMessage(
                "Could not parse {} as {}",
                text,
                magic_enum::enum_type_name<Value>());
        }
        return map.Get(text);
    }

    template <typename T>
        requires(std::same_as<T, std::string>)
    static const std::string& GetKey(const nlohmann::json& json, const std::string_view& key)
    {
        if (const nlohmann::json& value = JSONHelpers::GetKey(json, key); value.is_string())
        {
            return value.get_ref<const std::string&>();
        }

        throw klgl::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not a string! json: \n {}",
            key,
            json.dump(4, ' '));
    }

    template <typename T>
        requires(std::same_as<T, float>)
    static float GetKey(const nlohmann::json& json, const std::string_view& key)
    {
        if (const nlohmann::json& value = JSONHelpers::GetKey(json, key); value.is_number_float())
        {
            return value;
        }

        throw klgl::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not a float! json: \n {}",
            key,
            json.dump(4, ' '));
    }

    template <typename T>
        requires(std::same_as<T, int>)
    static int GetKey(const nlohmann::json& json, const std::string_view& key)
    {
        if (const nlohmann::json& value = JSONHelpers::GetKey(json, key); value.is_number_integer())
        {
            return value;
        }

        throw klgl::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not an int! json: \n {}",
            key,
            json.dump(4, ' '));
    }
};

const nlohmann::json& JSONHelpers::GetKey(const nlohmann::json& json, const std::string_view& key)
{
    [[unlikely]] if (!json.is_object())
    {
        klgl::ErrorHandling::ThrowWithMessage("JSON is not an object: \n ", json.dump(4, ' '));
    }

    [[unlikely]] if (!json.contains(key))
    {
        klgl::ErrorHandling::ThrowWithMessage("Missing required property {} in object: \n {}", key, json.dump(4, ' '));
    }

    return json[key];
}

nlohmann::json JSONHelpers::VectorToJSON(const edt::Vec2f& v)
{
    nlohmann::json json;

    json[JSONKeys::kX] = v.x();
    json[JSONKeys::kY] = v.y();

    return json;
}

nlohmann::json JSONHelpers::VectorToJSON(const edt::Vec2i& v)
{
    nlohmann::json json;

    json[JSONKeys::kX] = v.x();
    json[JSONKeys::kY] = v.y();

    return json;
}

edt::Vec2f JSONHelpers::Vec2fFromJSON(const nlohmann::json& json)
{
    return {
        Internal::GetKey<float>(json, JSONKeys::kX),
        Internal::GetKey<float>(json, JSONKeys::kY),
    };
}

edt::Vec2i JSONHelpers::Vec2iFromJSON(const nlohmann::json& json)
{
    return {
        Internal::GetKey<int>(json, JSONKeys::kX),
        Internal::GetKey<int>(json, JSONKeys::kY),
    };
}

nlohmann::json JSONHelpers::RadialEmitterToJSON(const RadialEmitterConfig& emitter)
{
    nlohmann::json json;

    json[JSONKeys::kPosition] = VectorToJSON(emitter.position);
    json[JSONKeys::kRadius] = emitter.radius;
    json[JSONKeys::kPhaseDegrees] = emitter.phase_degrees;
    json[JSONKeys::kSectorDegrees] = emitter.sector_degrees;
    json[JSONKeys::kSpeedFactor] = emitter.speed_factor;
    json[JSONKeys::kRotationSpeed] = emitter.rotation_speed;

    return json;
}

RadialEmitterConfig JSONHelpers::RadialEmitterFromJSON(const nlohmann::json& json)
{
    RadialEmitterConfig e{};

    e.position = Vec2fFromJSON(GetKey(json, JSONKeys::kPosition));
    e.radius = Internal::GetKey<float>(json, JSONKeys::kRadius);
    e.phase_degrees = Internal::GetKey<float>(json, JSONKeys::kPhaseDegrees);
    e.sector_degrees = Internal::GetKey<float>(json, JSONKeys::kSectorDegrees);
    e.speed_factor = Internal::GetKey<float>(json, JSONKeys::kSpeedFactor);
    e.rotation_speed = Internal::GetKey<float>(json, JSONKeys::kRotationSpeed);

    return e;
}

nlohmann::json JSONHelpers::EmitterToJSON(const Emitter& emitter)
{
    nlohmann::json json;

    const auto type = emitter.GetType();
    const std::string_view type_str = magic_enum::enum_name(type);
    json[JSONKeys::kType] = type_str;
    KLGL_ENSURE_ENUM_SIZE(EmitterType, 1);
    switch (type)
    {
    case EmitterType::Radial:
        json[type_str] = RadialEmitterToJSON(static_cast<const RadialEmitter&>(emitter).config);
        break;
    }

    return json;
}

std::unique_ptr<Emitter> JSONHelpers::EmitterFromJSON(const nlohmann::json& json)
{
    const std::string_view type_str = Internal::GetKey<std::string>(json, JSONKeys::kType);
    const EmitterType type = Internal::ParseEnum(Internal::kEmitterTypeParseMap, type_str);
    const nlohmann::json& inner = GetKey(json, type_str);

    KLGL_ENSURE_ENUM_SIZE(EmitterType, 1);
    switch (type)
    {
    case EmitterType::Radial:
        return std::make_unique<RadialEmitter>(RadialEmitterFromJSON(inner));
        break;

    default:
        throw klgl::ErrorHandling::RuntimeErrorWithMessage("Unhandled type of emitter: {}", type_str);
    }
}

nlohmann::json JSONHelpers::AppStateToJSON(const VerletApp& app)
{
    nlohmann::json json;
    json[JSONKeys::kWindowSize] = VectorToJSON(app.GetWindow().GetSize().Cast<int>());
    json[JSONKeys::kMaxObjectsCount] = app.max_objects_count_;
    json[JSONKeys::kEmitters] = nlohmann::json::array();

    auto& array = json[JSONKeys::kEmitters];

    for (auto& emitter : app.GetEmitters())
    {
        array.push_back(EmitterToJSON(emitter));
    }

    return json;
}

}  // namespace verlet
