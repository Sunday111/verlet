#pragma once

#include <algorithm>
#include <array>
#include <random>
#include <vector>

enum class ArrayAction
{
    Resize,
    Erase,
    Insert,
};

static constexpr std::array kArrayActions{
    ArrayAction::Resize,
    ArrayAction::Erase,
    ArrayAction::Insert,
};

inline std::vector<ArrayAction> GenerateRandomActions(std::mt19937& rnd, size_t count)
{
    std::uniform_int_distribution<size_t> index_distribution(0, kArrayActions.size() - 1);
    std::vector<ArrayAction> actions;
    actions.reserve(count);
    std::generate_n(std::back_inserter(actions), count, [&]() { return kArrayActions[index_distribution(rnd)]; });
    return actions;
}
