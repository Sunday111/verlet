#include "klgl/name_cache/name_cache.hpp"

namespace klgl::name_cache_impl
{
NameId NameCache::GetId(std::string_view view)
{
    auto it = view_to_id_.find(view);
    if (it == view_to_id_.end())
    {
        const NameId id = next_id_++;

        std::string& str = id_to_str_[id];
        str = view;

        view_to_id_[str] = id;
        it = view_to_id_.find(view);
    }

    return it->second;
}

std::optional<std::string_view> NameCache::FindView(NameId id) const
{
    auto it = id_to_str_.find(id);
    if (it != id_to_str_.end())
    {
        return it->second;
    }

    return std::optional<std::string_view>();
}

NameCache& NameCache::Get()
{
    static NameCache name_cache;
    return name_cache;
}
}  // namespace klgl::name_cache_impl
