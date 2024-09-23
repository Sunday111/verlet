#pragma once

#include <memory>

#include "ass/enum_set.hpp"
#include "emitter_flag.hpp"
#include "emitter_type.hpp"

namespace verlet
{

class VerletApp;

class Emitter
{
public:
    virtual void Tick(VerletApp& app) = 0;
    virtual void GUI() = 0;
    virtual constexpr EmitterType GetType() const = 0;
    virtual std::unique_ptr<Emitter> Clone() const = 0;
    virtual ~Emitter() = default;

    [[nodiscard]] bool HasFlag(const EmitterFlag flag) const { return flags_.Contains(flag); }
    void SetFlag(const EmitterFlag flag, bool value)
    {
        if (value)
        {
            flags_.Add(flag);
        }
        else
        {
            flags_.Remove(flag);
        }
    }

protected:
    void DeleteButton();
    void CloneButton();
    void EnabledCheckbox();

private:
    ass::EnumSet<EmitterFlag> flags_;
};

}  // namespace verlet
