#include "klgl/name_cache/name.hpp"

#include <stdexcept>

#include "klgl/name_cache/name_cache.hpp"

namespace klgl
{

Name::Name(const char* strptr) : Name(std::string_view(strptr)) {}

Name::Name(const std::string& string) : Name(std::string_view(string)) {}

Name::Name(const std::string_view& view)
{
    using namespace name_cache_impl;
    id_ = NameCache::Get().GetId(view);
}

[[nodiscard]] bool Name::IsValid() const noexcept
{
    return id_ != kInvalidNameId;
}

std::string_view Name::GetView() const
{
    using namespace name_cache_impl;
    [[unlikely]] if (!IsValid())
    {
        return "";
    }
    auto maybe_str = NameCache::Get().FindView(id_);
    [[likely]] if (maybe_str)
    {
        return *maybe_str;
    }
    throw std::logic_error("Invalid string id");
}

}  // namespace klgl
