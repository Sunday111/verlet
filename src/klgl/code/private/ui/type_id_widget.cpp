#include "klgl/ui/type_id_widget.hpp"

#include <imgui.h>

#include <limits>

#include "CppReflection/GetStaticTypeInfo.hpp"
#include "CppReflection/TypeRegistry.hpp"
#include "klgl/reflection/matrix_reflect.hpp"  // IWYU pragma: keep

namespace klgl
{

using namespace edt::lazy_matrix_aliases;  // NOLINT

template <typename T>
static constexpr ImGuiDataType_ CastDataType() noexcept
{
    if constexpr (std::is_same_v<T, int8_t>)
    {
        return ImGuiDataType_S8;
    }
    if constexpr (std::is_same_v<T, uint8_t>)
    {
        return ImGuiDataType_U8;
    }
    if constexpr (std::is_same_v<T, int16_t>)
    {
        return ImGuiDataType_S16;
    }
    if constexpr (std::is_same_v<T, uint16_t>)
    {
        return ImGuiDataType_U16;
    }
    if constexpr (std::is_same_v<T, int32_t>)
    {
        return ImGuiDataType_S32;
    }
    if constexpr (std::is_same_v<T, uint32_t>)
    {
        return ImGuiDataType_U32;
    }
    if constexpr (std::is_same_v<T, int64_t>)
    {
        return ImGuiDataType_S64;
    }
    if constexpr (std::is_same_v<T, uint64_t>)
    {
        return ImGuiDataType_U64;
    }
    if constexpr (std::is_same_v<T, float>)
    {
        return ImGuiDataType_Float;
    }
    if constexpr (std::is_same_v<T, double>)
    {
        return ImGuiDataType_Double;
    }
}

template <typename T>
bool ScalarProperty(
    edt::GUID type_guid,
    std::string_view name,
    void* address,
    bool& value_changed,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max())
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T* value = reinterpret_cast<T*>(address);  // NOLINT
        const bool c = ImGui::DragScalar(name.data(), CastDataType<T>(), value, 1.0f, &min, &max);
        value_changed = c;
        return true;
    }

    return false;
}

template <typename T, size_t N>
bool VectorProperty(
    std::string_view title,
    edt::Matrix<T, N, 1>& value,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max()) noexcept
{
    return ImGui::DragScalarN(title.data(), CastDataType<T>(), value.data(), N, 0.01f, &min, &max, "%.3f");
}

template <typename T>
bool VectorProperty(edt::GUID type_guid, std::string_view name, void* address, bool& value_changed)
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T& member_ref = *reinterpret_cast<T*>(address);  // NOLINT
        value_changed |= VectorProperty(name, member_ref);
        return true;
    }

    return false;
}

template <typename T, size_t num_rows, size_t num_columns>
bool MatrixProperty(
    const std::string_view title,
    edt::Matrix<T, num_rows, num_columns>& value,
    T min = std::numeric_limits<T>::lowest(),
    T max = std::numeric_limits<T>::max()) noexcept
{
    bool changed = false;
    if (ImGui::TreeNode(title.data()))
    {
        for (size_t row_index = 0; row_index != num_rows; ++row_index)
        {
            edt::Matrix<T, num_columns, 1> row = value.GetRow(row_index).Transposed();
            ImGui::PushID(static_cast<int>(row_index));
            const bool row_changed = ImGui::DragScalarN(
                "",
                CastDataType<T>(),
                row.data(),
                static_cast<int>(num_columns),
                0.01f,
                &min,
                &max,
                "%.3f");
            ImGui::PopID();
            [[unlikely]] if (row_changed)
            {
                value.SetRow(row_index, row);
            }
            changed = changed || row_changed;
        }
        ImGui::TreePop();
    }
    return changed;
}

template <typename T>
bool MatrixProperty(edt::GUID type_guid, std::string_view name, void* address, bool& value_changed)
{
    constexpr auto type_info = cppreflection::GetStaticTypeInfo<T>();
    if (type_info.guid == type_guid)
    {
        T& member_ref = *reinterpret_cast<T*>(address);  // NOLINT
        value_changed |= MatrixProperty(name, member_ref);
        return true;
    }

    return false;
}

void SimpleTypeWidget(edt::GUID type_guid, std::string_view name, void* value, bool& value_changed)
{
    value_changed = false;
    [[maybe_unused]] const bool found_type = ScalarProperty<float>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<double>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<uint8_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<uint16_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<uint32_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<uint64_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<int8_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<int16_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<int32_t>(type_guid, name, value, value_changed) ||
                                             ScalarProperty<int64_t>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Vec2f>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Vec3f>(type_guid, name, value, value_changed) ||
                                             VectorProperty<Vec4f>(type_guid, name, value, value_changed) ||
                                             MatrixProperty<Mat4f>(type_guid, name, value, value_changed);
}

void TypeIdWidget(edt::GUID type_guid, void* base, bool& value_changed)
{
    const cppreflection::Type* type_info = cppreflection::GetTypeRegistry()->FindType(type_guid);
    for (const cppreflection::Field* field : type_info->GetFields())
    {
        void* pmember = field->GetValue(base);
        bool member_changed = false;
        SimpleTypeWidget(field->GetType()->GetGuid(), field->GetName(), pmember, member_changed);
        value_changed |= member_changed;
    }
}

}  // namespace klgl
