#include "klvk/error_handling.hpp"
#include "verlet/verlet_app.hpp"

void Main(int argc, char** argv)
{
    verlet::VerletApp app;
    // Forwards klvk's own options - diagnostics, input recording, presentation -
    // and ignores everything else, so application arguments still pass through.
    app.RunWithArguments(argc, argv);
}

int main(int argc, char** argv)
{
    klvk::ErrorHandling::InvokeAndCatchAll([&] { Main(argc, argv); });
    return 0;
}
