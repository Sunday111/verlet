#pragma once

#include "ankerl/unordered_dense.h"

namespace verlet
{

template <typename Identifier>
struct TaggedIdentifierHash
{
    using is_avalanching = void;

    auto operator()(const Identifier& x) const noexcept -> uint64_t
    {
        return ankerl::unordered_dense::detail::wyhash::hash(&x.GetValue(), sizeof(typename Identifier::Repr));
    }
};
}  // namespace verlet
