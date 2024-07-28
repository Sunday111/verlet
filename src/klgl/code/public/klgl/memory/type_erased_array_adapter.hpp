#include "type_erased_array.hpp"

namespace klgl
{

template <typename T, typename ValueArray>
    requires(std::same_as<std::remove_const_t<ValueArray>, TypeErasedArray>)
class TypeErasedArrayAdapter
{
public:
    static constexpr bool kConstValue = std::is_const_v<ValueArray>;
    using ValueType = std::conditional_t<kConstValue, const T, T>;
    using ValueRef = ValueType&;
    explicit TypeErasedArrayAdapter(ValueArray& value_array) : value_array_(&value_array) {}

    [[nodiscard]] size_t Size() const { return value_array_->Size(); }

    [[nodiscard]] ValueRef operator[](const size_t index) const
    {
        return *reinterpret_cast<ValueType*>(value_array_->operator[](index));  // NOLINT
    }

private:
    ValueArray* value_array_{};
};

template <typename T, typename Array>
    requires(std::same_as<std::remove_const_t<Array>, TypeErasedArray>)
[[nodiscard]] inline auto MakeTypeErasedArrayAdapter(Array& array)
{
    return TypeErasedArrayAdapter<T, Array>(array);
}
}  // namespace klgl
