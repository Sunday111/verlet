#pragma once

namespace verlet
{

class VerletApp;

class Emitter
{
public:
    virtual void Tick(VerletApp& app) = 0;
    virtual void GUI() = 0;
    bool ShouldBeDeleted() const { return should_be_deleted_; }
    virtual ~Emitter() = default;

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }
    void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }

protected:
    void DeleteButton();
    void EnabledCheckbox();

private:
    bool should_be_deleted_ = false;
    bool enabled_ = false;
};

}  // namespace verlet
