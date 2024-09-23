#pragma once

#include <string_view>

namespace verlet
{
class JSONKeys
{
public:
    static constexpr std::string_view kX = "X";
    static constexpr std::string_view kY = "Y";
    static constexpr std::string_view kZ = "Z";
    static constexpr std::string_view kType = "Type";
    static constexpr std::string_view kEmitters = "Emitters";
    static constexpr std::string_view kPosition = "Position";
    static constexpr std::string_view kRadius = "Radius";
    static constexpr std::string_view kPhaseDegrees = "PhaseDegrees";
    static constexpr std::string_view kSectorDegrees = "SectorDegrees";
    static constexpr std::string_view kSpeedFactor = "SpeedFactor";
    static constexpr std::string_view kWindowSize = "WindowSize";
    static constexpr std::string_view kMaxObjectsCount = "MaxObjectsCount";
};

}  // namespace verlet
