#include "matrix.hpp"

static_assert(
    []
    {
        Matrix<int, 3, 3> m;
        for (size_t r = 0; r != 3; ++r)
        {
            for (size_t c = 0; c != 3; ++c)
            {
                if (m(r, c) != 0)
                {
                    return false;
                }
            }
        }

        m(0, 1) = 42;
        m(1, 0) = 24;

        if (m(0, 1) != 42) return false;
        if (m(1, 0) != 24) return false;

        return true;
    }());

static_assert(
    []
    {
        auto m = Matrix<int, 5, 3>::Identity();

        int s = 0;
        for (size_t c = 0; c != m.NumColumns(); ++c)
        {
            for (size_t r = 0; r != m.NumRows(); ++r)
            {
                s += m(r, c);
                if (c == r && m(r, c) != 1) return false;
            }
        }

        return s == 3;
    }(),
    "Identity test");

static_assert(
    []
    {
        Vec3i a{{1, 2, 3}};
        Vec3i b{{4, 5, 6}};
        return a.Dot(b) == 1 * 4 + 2 * 5 + 3 * 6;
    }(),
    "Dot product test");

static_assert(
    []
    {
        Vec3i a{{1, 2, 3}};
        Vec3i b{{4, 5, 6}};
        return a.Cross(b) == Vec3i{{-3, 6, -3}};
    }(),
    "Cross product test");

static_assert(
    []
    {
        // clang-format off
        Matrix<int, 4, 3> a{{
            0, 1, 2,
            3, 4, 5,
            6, 7, 8,
            9, 10, 11
        }};
        Matrix<int, 3, 5> b{{
            14, 13, 12, 11, 10,
            9, 8, 7, 6, 5,
            4, 3, 2, 1, 0
        }};
        Matrix<int, 4, 5> expected{{
            17, 14, 11, 8, 5,
            98, 86, 74, 62, 50,
            179, 158, 137, 116, 95,
            260, 230, 200, 170, 140
        }};
        // clang-format on

        Matrix<int, 4, 5> actual = a.MatMul(b);
        return actual == expected;
    }(),
    "Matrix multiplication test");

static_assert(
    []
    {
        // clang-format off
        Matrix<int, 3, 4> m{{
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11
        }};
        Matrix<int, 1, 4> expected{{
            4, 5, 6, 7,
        }};
        // clang-format on

        Matrix<int, 1, 4> actual = m.GetRow(1);
        return actual == expected;
    }(),
    "GetRow Test");

static_assert(
    []
    {
        // clang-format off
        Matrix<int, 3, 4> m{{
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11
        }};
        Matrix<int, 3, 1> expected{{
            1, 5, 9
        }};
        // clang-format on

        Matrix<int, 3, 1> actual = m.GetColumn(1);
        return actual == expected;
    }(),
    "GetColumn Test");

static_assert(
    []
    {
        // clang-format off
        Matrix<int, 3, 4> m{{
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11
        }};
        Matrix<int, 4, 3> expected{{
            0, 4, 8,
            1, 5, 9,
            2, 6, 10,
            3, 7, 11
        }};
        // clang-format on

        Matrix<int, 4, 3> actual = m.Transposed();
        return actual == expected;
    }(),
    "Transposed Test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = a + 10;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != static_cast<int>(row * b.NumColumns() + column) + 10)
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Matrix + scalar test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = 10 + a;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != static_cast<int>(row * b.NumColumns() + column) + 10)
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Scalar + matrix test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = a - 10;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != static_cast<int>(row * b.NumColumns() + column) - 10)
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Matrix minus scalar test");

static_assert(
    []
    {
        Matrix<size_t, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = 10uz - a;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != 10uz - (row * b.NumColumns() + column))
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Scalar minus matrix test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = a * 10;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != static_cast<int>(row * b.NumColumns() + column) * 10)
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Matrix times scalar test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = 10 * a;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != 10 * static_cast<int>(row * b.NumColumns() + column))
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Scalar times matrix test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}};

        auto b = a / 10;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != static_cast<int>(row * b.NumColumns() + column) / 10)
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Matrix divided by scalar test");

static_assert(
    []
    {
        Matrix<int, 5, 3> a{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

        auto b = 10 / a;

        for (const size_t row : b.RowIndices())
        {
            for (const size_t column : b.ColumnIndices())
            {
                if (b(row, column) != 10 / static_cast<int>(row * b.NumColumns() + column + 1))
                {
                    return false;
                }
            }
        }

        return true;
    }(),
    "Scalar divided by matrix test");
