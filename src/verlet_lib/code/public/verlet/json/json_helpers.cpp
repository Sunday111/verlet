#include "verlet/json/json_helpers.hpp"

#include <limits>

#include "ass/fixed_unordered_map.hpp"
#include "edt/math/math.hpp"
#include "klvk/error_handling.hpp"
#include "klvk/macro/ensure_enum_size.hpp"
#include "klvk/template/constexpr_string_hash.hpp"
#include "magic_enum/magic_enum.hpp"
#include "verlet/emitters/flat_emitter.hpp"
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
    ass::FixedUnorderedMap<num_entries, std::string_view, T, klvk::ConstexprStringHasher> m;
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
            klvk::ErrorHandling::ThrowWithMessage(
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

        throw klvk::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not a string! json: \n {}",
            key,
            json.dump(4, ' '));
    }

    template <typename T>
        requires(std::same_as<T, float>)
    static float GetKey(const nlohmann::json& json, const std::string_view& key)
    {
        if (const nlohmann::json& value = JSONHelpers::GetKey(json, key); value.is_number())
        {
            return value;
        }

        throw klvk::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not a float! json: \n {}",
            key,
            json.dump(4, ' '));
    }

    template <typename T>
        requires(std::same_as<T, bool>)
    static bool GetKey(const nlohmann::json& json, const std::string_view& key)
    {
        if (const nlohmann::json& value = JSONHelpers::GetKey(json, key); value.is_boolean())
        {
            return value;
        }

        throw klvk::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not a boolean! json: \n {}",
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

        throw klvk::ErrorHandling::RuntimeErrorWithMessage(
            "json[{}] is not an int! json: \n {}",
            key,
            json.dump(4, ' '));
    }
};

namespace
{
[[nodiscard]] size_t ObjectCountFromJSON(const nlohmann::json& json)
{
    if (json.is_number_unsigned())
    {
        const uint64_t value = json.get<uint64_t>();
        if (value > std::numeric_limits<size_t>::max())
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage("is too large");
        }
        return static_cast<size_t>(value);
    }
    if (json.is_number_integer())
    {
        const int64_t value = json.get<int64_t>();
        if (value < 0) throw klvk::ErrorHandling::RuntimeErrorWithMessage("must be nonnegative");
        return static_cast<size_t>(value);
    }
    throw klvk::ErrorHandling::RuntimeErrorWithMessage("must be a nonnegative integer");
}
}  // namespace

const nlohmann::json& JSONHelpers::GetKey(const nlohmann::json& json, const std::string_view& key)
{
    [[unlikely]] if (!json.is_object())
    {
        klvk::ErrorHandling::ThrowWithMessage("JSON is not an object: \n ", json.dump(4, ' '));
    }

    [[unlikely]] if (!json.contains(key))
    {
        klvk::ErrorHandling::ThrowWithMessage("Missing required property {} in object: \n {}", key, json.dump(4, ' '));
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

nlohmann::json JSONHelpers::FlatEmitterToJSON(const FlatEmitterConfig& emitter)
{
    nlohmann::json json;

    json[JSONKeys::kStart] = VectorToJSON(emitter.start);
    json[JSONKeys::kEnd] = VectorToJSON(emitter.end);
    json[JSONKeys::kDirection] = VectorToJSON(emitter.direction);
    json[JSONKeys::kLocalDirection] = emitter.local_direction;
    json[JSONKeys::kSpacing] = emitter.spacing;
    json[JSONKeys::kSpeedFactor] = emitter.speed_factor;

    return json;
}

FlatEmitterConfig JSONHelpers::FlatEmitterFromJSON(const nlohmann::json& json)
{
    FlatEmitterConfig e{};

    e.start = Vec2fFromJSON(GetKey(json, JSONKeys::kStart));
    e.end = Vec2fFromJSON(GetKey(json, JSONKeys::kEnd));
    e.direction = Vec2fFromJSON(GetKey(json, JSONKeys::kDirection));
    e.local_direction = Internal::GetKey<bool>(json, JSONKeys::kLocalDirection);
    e.spacing = Internal::GetKey<float>(json, JSONKeys::kSpacing);
    e.speed_factor = Internal::GetKey<float>(json, JSONKeys::kSpeedFactor);

    return e;
}

nlohmann::json JSONHelpers::EmitterToJSON(const Emitter& emitter)
{
    nlohmann::json json;

    const auto type = emitter.GetType();
    const std::string_view type_str = magic_enum::enum_name(type);
    json[JSONKeys::kType] = type_str;
    KLVK_ENSURE_ENUM_SIZE(EmitterType, 2);
    switch (type)
    {
    case EmitterType::Radial:
        json[type_str] = RadialEmitterToJSON(static_cast<const RadialEmitter&>(emitter).config);
        break;
    case EmitterType::Flat:
        json[type_str] = FlatEmitterToJSON(static_cast<const FlatEmitter&>(emitter).config);
        break;
    }

    return json;
}

std::unique_ptr<Emitter> JSONHelpers::EmitterFromJSON(const nlohmann::json& json)
{
    const std::string_view type_str = Internal::GetKey<std::string>(json, JSONKeys::kType);
    const EmitterType type = Internal::ParseEnum(Internal::kEmitterTypeParseMap, type_str);
    const nlohmann::json& inner = GetKey(json, type_str);

    KLVK_ENSURE_ENUM_SIZE(EmitterType, 2);
    switch (type)
    {
    case EmitterType::Radial:
    {
        const RadialEmitterConfig config = RadialEmitterFromJSON(inner);
        if (const auto invalid = RadialEmitter::ValidateConfig(config))
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage("{}.{} is invalid", type_str, *invalid);
        }
        return std::make_unique<RadialEmitter>(config);
    }

    case EmitterType::Flat:
    {
        const FlatEmitterConfig config = FlatEmitterFromJSON(inner);
        if (const auto invalid = FlatEmitter::ValidateConfig(config))
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage("{}.{} is invalid", type_str, *invalid);
        }
        return std::make_unique<FlatEmitter>(config);
    }

    default:
        throw klvk::ErrorHandling::RuntimeErrorWithMessage("Unhandled type of emitter: {}", type_str);
    }
}

nlohmann::json JSONHelpers::AppStateToJSON(const VerletApp& app)
{
    nlohmann::json json;
    json[JSONKeys::kWindowSize] = VectorToJSON(app.GetWindow().GetSize().Cast<int>());
    if (app.max_objects_saturation_)
    {
        json[JSONKeys::kMaxObjectsSaturation] = *app.max_objects_saturation_;
    }
    else
    {
        json[JSONKeys::kMaxObjectsCount] = app.max_objects_count_;
    }
    json[JSONKeys::kEmitters] = nlohmann::json::array();

    auto& array = json[JSONKeys::kEmitters];

    for (auto& emitter : app.GetEmitters())
    {
        array.push_back(EmitterToJSON(emitter));
    }

    return json;
}

ParsedAppState JSONHelpers::AppStateFromJSON(const nlohmann::json& json)
{
    ParsedAppState state;

    try
    {
        const edt::Vec2i window_size = Vec2iFromJSON(GetKey(json, JSONKeys::kWindowSize));
        constexpr int minimum_window_extent = 100;
        constexpr int maximum_window_extent = 5000;
        klvk::ErrorHandling::Ensure(
            window_size.x() >= minimum_window_extent && window_size.x() <= maximum_window_extent,
            "{}.{} must be within [{}, {}], got {}",
            JSONKeys::kWindowSize,
            JSONKeys::kX,
            minimum_window_extent,
            maximum_window_extent,
            window_size.x());
        klvk::ErrorHandling::Ensure(
            window_size.y() >= minimum_window_extent && window_size.y() <= maximum_window_extent,
            "{}.{} must be within [{}, {}], got {}",
            JSONKeys::kWindowSize,
            JSONKeys::kY,
            minimum_window_extent,
            maximum_window_extent,
            window_size.y());
        state.window_size = window_size.Cast<uint32_t>();
    }
    catch (const std::exception& error)
    {
        throw klvk::ErrorHandling::RuntimeErrorWithMessage("{}: {}", JSONKeys::kWindowSize, error.what());
    }

    const bool has_count = json.contains(JSONKeys::kMaxObjectsCount);
    const bool has_saturation = json.contains(JSONKeys::kMaxObjectsSaturation);
    klvk::ErrorHandling::Ensure(
        has_count != has_saturation,
        "A preset must contain exactly one of '{}' and '{}'",
        JSONKeys::kMaxObjectsCount,
        JSONKeys::kMaxObjectsSaturation);

    if (has_saturation)
    {
        try
        {
            const float saturation = Internal::GetKey<float>(json, JSONKeys::kMaxObjectsSaturation);
            klvk::ErrorHandling::Ensure(
                edt::Math::IsFinite(saturation) && saturation >= 0.f && saturation <= 1.f,
                "must be finite and within [0, 1], got {}",
                saturation);
            state.max_objects_saturation = saturation;
        }
        catch (const std::exception& error)
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage("{}: {}", JSONKeys::kMaxObjectsSaturation, error.what());
        }
    }
    else
    {
        try
        {
            state.max_objects_count = ObjectCountFromJSON(GetKey(json, JSONKeys::kMaxObjectsCount));
        }
        catch (const std::exception& error)
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage("{}: {}", JSONKeys::kMaxObjectsCount, error.what());
        }
    }

    const auto& emitters = GetKey(json, JSONKeys::kEmitters);
    klvk::ErrorHandling::Ensure(emitters.is_array(), "{} must be an array", JSONKeys::kEmitters);
    state.emitters.reserve(emitters.size());
    for (size_t emitter_index = 0; emitter_index != emitters.size(); ++emitter_index)
    {
        try
        {
            state.emitters.push_back(EmitterFromJSON(emitters[emitter_index]));
        }
        catch (const std::exception& error)
        {
            throw klvk::ErrorHandling::RuntimeErrorWithMessage(
                "{}[{}]: {}",
                JSONKeys::kEmitters,
                emitter_index,
                error.what());
        }
    }

    return state;
}

}  // namespace verlet
