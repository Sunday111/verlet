#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "klgl/name_cache/name.hpp"

namespace klgl::name_cache_impl
{
using NameId = Name::NameId;
class NameCache
{
public:
    static NameCache& Get();

    NameId GetId(std::string_view view);
    std::optional<std::string_view> FindView(NameId id) const;

private:
    NameId next_id_ = 0;
    std::unordered_map<std::string_view, NameId> view_to_id_;
    std::unordered_map<NameId, std::string> id_to_str_;
};

}  // namespace klgl::name_cache_impl
