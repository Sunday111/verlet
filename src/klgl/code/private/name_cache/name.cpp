#include "klgl/name_cache/name.hpp"

#include <stdexcept>

#include "klgl/name_cache/name_cache.hpp"

namespace klgl
{

Name::Name(const char* strptr) : Name(std::string_view(strptr)) {}

Name::Name(const std::string& string) : Name(std::string_view(string)) {}

Name::Name(const std::string_view& view)
{
    id_ = name_cache_impl::NameCache::Get().GetId(view);
}

[[nodiscard]] bool Name::IsValid() const noexcept
{
    return id_ != kInvalidNameId;
}

std::string_view Name::GetView() const
{
    [[unlikely]] if (!IsValid())
    {
        return "";
    }
    [[likely]] if (auto maybe_str = name_cache_impl::NameCache::Get().FindView(id_))
    {
        return *maybe_str;
    }
    throw std::logic_error("Invalid string id");
}

}  // namespace klgl
