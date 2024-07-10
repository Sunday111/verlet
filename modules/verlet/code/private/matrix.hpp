#pragma once

#include <array>
#include <cstddef>
#include <ranges>

template <typename T, size_t num_rows, size_t num_columns>
class Matrix;

template <typename MatrixA, typename MatrixB>
inline constexpr bool kMatricesWithSameSize =
    MatrixA::NumRows() == MatrixB::NumRows() && MatrixA::NumColumns() == MatrixB::NumColumns();

template <typename MatrixA, typename MatrixB>
inline constexpr bool kVectorsWithSameSize =
    MatrixA::IsVector() == MatrixB::IsVector() && MatrixA::Size() == MatrixB::Size();

template <typename T, size_t num_rows, size_t num_columns>
class Matrix
{
public:
    // Deducing this covers const/non-const (mainly)
    template <typename Self>
    [[nodiscard]] constexpr auto&& operator()(this Self&& self, size_t row, size_t column)
    {
        return std::forward<Self>(self).data_[row * num_columns + column];
    }
    template <typename Self, typename PureSelf = std::decay_t<Self>>
        requires(PureSelf::IsVector() && PureSelf::Size() > 0)
    [[nodiscard]] constexpr auto&& x(this Self&& self)
    {
        return std::forward<Self>(self)[0];
    }

    template <typename Self, typename PureSelf = std::decay_t<Self>>
        requires(PureSelf::IsVector() && PureSelf::Size() > 1)
    [[nodiscard]] constexpr auto&& y(this Self&& self)
    {
        return std::forward<Self>(self)[1];
    }

    template <typename Self, typename PureSelf = std::decay_t<Self>>
        requires(PureSelf::IsVector() && PureSelf::Size() > 2)
    [[nodiscard]] constexpr auto&& z(this Self&& self)
    {
        return std::forward<Self>(self)[2];
    }

    template <typename Self>
        requires(std::decay_t<Self>::IsVector())
    [[nodiscard]] constexpr auto&& operator[](this Self&& self, const size_t index)
    {
        return std::forward<Self>(self).data_[index];
    }

    [[nodiscard]] static constexpr Matrix Identity() noexcept
    {
        Matrix m;

        for (size_t k = 0; k != num_rows && k != num_columns; ++k)
        {
            m(k, k) = T{1};
        }

        return m;
    }

    [[nodiscard]] static constexpr size_t NumRows() noexcept
    {
        return num_rows;
    }

    [[nodiscard]] static constexpr size_t NumColumns() noexcept
    {
        return num_columns;
    }

    [[nodiscard]] static constexpr bool IsVector()
    {
        return NumRows() == 1 or NumColumns() == 1;
    }

    [[nodiscard]] static constexpr size_t Size() noexcept
    {
        return NumRows() * NumColumns();
    }

    template <size_t other_rows, size_t other_columns>
        requires(kVectorsWithSameSize<Matrix<T, other_rows, other_columns>, Matrix>)
    [[nodiscard]] constexpr T Dot(const Matrix<T, other_rows, other_columns>& other) const
    {
        T sum{0};

        for (size_t i = 0; i != Size(); ++i)
        {
            sum += this->operator[](i) * other[i];
        }

        return sum;
    }

    // https://en.wikipedia.org/wiki/Cross_product
    template <size_t other_rows, size_t other_columns>
        requires(kVectorsWithSameSize<Matrix<T, other_rows, other_columns>, Matrix> && Size() == 3)
    [[nodiscard]] constexpr Matrix Cross(const Matrix<T, other_rows, other_columns>& other) const
    {
        auto& a = *this;
        auto& b = other;
        return Matrix{{
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0],
        }};
    }

    // Two-dimensional "cross product". A hack to obtain a magnitude of 3-dimensional cross product
    template <size_t other_rows, size_t other_columns>
        requires(kVectorsWithSameSize<Matrix<T, other_rows, other_columns>, Matrix> && Size() == 2)
    [[nodiscard]] constexpr Matrix Cross(const Matrix<T, other_rows, other_columns>& other) const
    {
        auto& a = *this;
        auto& b = other;
        return a[0] * b[1] - a[1] * b[0];
    }

    [[nodiscard]] constexpr T Magnitude() const
        requires(IsVector())
    {
        return this->Dot(*this);
    }

    [[nodiscard]] constexpr T SquaredLength() const
        requires(IsVector())
    {
        return Magnitude();
    }

    [[nodiscard]] constexpr auto RowIndices() const
    {
        return std::views::iota(0uz, NumRows());
    }

    [[nodiscard]] constexpr auto ColumnsIndices() const
    {
        return std::views::iota(0uz, NumColumns());
    }

    [[nodiscard]] constexpr auto Indices() const
        requires(IsVector())
    {
        return std::views::iota(0uz, Size());
    }

    template <size_t other_rows, size_t other_columns>
        requires(NumColumns() == other_rows)
    [[nodiscard]] constexpr Matrix<T, NumRows(), other_columns> MatMul(
        const Matrix<T, other_rows, other_columns>& other) const
    {
        Matrix<T, NumRows(), other_columns> result;

        for (const size_t r : result.RowIndices())
        {
            for (const size_t c : result.ColumnsIndices())
            {
                auto& cell = result(r, c);
                for (const size_t k : ColumnsIndices())
                {
                    cell += this->operator()(r, k) * other(k, c);
                }
            }
        }

        return result;
    }

    constexpr Matrix& operator+=(const T& value)
    {
        for (T& element : data_) element += value;
        return *this;
    }

    constexpr Matrix operator+(const T& value) const
    {
        auto copy = *this;
        copy += value;
        return copy;
    }

    constexpr Matrix& operator+=(const Matrix& other)
    {
        for (size_t i = 0; i != data_.size(); ++i) data_[i] += other.data_[i];
        return *this;
    }

    constexpr Matrix operator+(const Matrix& other) const
    {
        auto copy = *this;
        copy += other;
        return copy;
    }

    [[nodiscard]] friend constexpr Matrix operator+(const T& value, const Matrix& m)
    {
        return m + value;
    }

    constexpr Matrix& operator-=(const T& value)
    {
        for (T& element : data_) element -= value;
        return *this;
    }

    constexpr Matrix operator-(const T& value) const
    {
        auto copy = *this;
        copy -= value;
        return copy;
    }

    constexpr Matrix& operator-=(const Matrix& other)
    {
        for (size_t i = 0; i != data_.size(); ++i) data_[i] -= other.data_[i];
        return *this;
    }

    constexpr Matrix operator-(const Matrix& value) const
    {
        auto copy = *this;
        copy -= value;
        return copy;
    }

    [[nodiscard]] friend constexpr Matrix operator-(const T& value, const Matrix& m)
    {
        auto copy = m;
        for (auto& e : copy.data_)
        {
            e = value - e;
        }
        return copy;
    }

    constexpr Matrix& operator*=(const T& value)
    {
        for (T& element : data_) element *= value;
        return *this;
    }

    constexpr Matrix operator*(const T& value) const
    {
        auto copy = *this;
        copy *= value;
        return copy;
    }

    constexpr Matrix& operator*=(const Matrix& other)
    {
        for (size_t i = 0; i != data_.size(); ++i) data_[i] *= other.data_[i];
        return *this;
    }

    constexpr Matrix operator*(const Matrix& value) const
    {
        auto copy = *this;
        copy *= value;
        return copy;
    }

    [[nodiscard]] friend constexpr Matrix operator*(const T& value, const Matrix& m)
    {
        return m * value;
    }

    constexpr Matrix& operator/=(const T& value)
    {
        for (T& element : data_) element /= value;
        return *this;
    }

    constexpr Matrix operator/(const T& value) const
    {
        auto copy = *this;
        copy /= value;
        return copy;
    }

    constexpr Matrix& operator/=(const Matrix& other)
    {
        for (size_t i = 0; i != data_.size(); ++i) data_[i] /= other.data_[i];
        return *this;
    }

    constexpr Matrix operator/(const Matrix& value) const
    {
        auto copy = *this;
        copy /= value;
        return copy;
    }

    [[nodiscard]] friend constexpr Matrix operator/(const T& value, const Matrix& m)
    {
        auto copy = m;
        for (auto& e : copy.data_)
        {
            e = value / e;
        }
        return copy;
    }

    [[nodiscard]] constexpr bool operator==(const Matrix& other) const
    {
        return data_ == other.data_;
    }

    std::array<T, Size()> data_{};
};

template <typename T>
using Vector3 = Matrix<T, 3, 1>;

template <typename T>
using Vector2 = Matrix<T, 2, 1>;

using Vec2f = Vector2<float>;
using Vec2i = Vector2<int>;
using Vec3f = Vector3<float>;
using Vec3i = Vector3<int>;
using Mat3f = Matrix<float, 3, 3>;
