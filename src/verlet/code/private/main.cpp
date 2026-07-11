#include "klvk/error_handling.hpp"
#include "verlet/verlet_app.hpp"

void Main()
{
    verlet::VerletApp app;
    app.Run();
}

int main()
{
    klvk::ErrorHandling::InvokeAndCatchAll(Main);
    return 0;
}
