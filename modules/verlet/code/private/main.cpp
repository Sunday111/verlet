#include "fmt/format.h"
#include "yae_example_header_only/yae_example_header_only.hpp"
#include "yae_example_library/yae_example_library.hpp"

int main()
{
    yae::example_header_only::Test test_a;
    fmt::print("{}\n", test_a.GetValue());

    yae::example_library::Test test_b;
    fmt::print("{}\n", test_b.GetValue());
}
