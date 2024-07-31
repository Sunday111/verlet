#pragma once

#include <cstddef>
#include <vector>

namespace cppreflection
{
class Type;
}

namespace klgl::events
{
class IEventListener
{
public:
    using CallbackFunction = void (*)(IEventListener* listener, const void* event_data);

    virtual ~IEventListener() = default;
    virtual std::vector<const cppreflection::Type*> GetEventTypes() const = 0;
    virtual CallbackFunction MakeCallbackFunction(const size_t index) = 0;
};
}  // namespace klgl::events
