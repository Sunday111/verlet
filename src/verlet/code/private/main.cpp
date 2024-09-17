#include "klgl/error_handling.hpp"
#include "verlet/verlet_app.hpp"

void Main()
{
    verlet::VerletApp app;
    app.Run();
}

int main()
{
    klgl::ErrorHandling::InvokeAndCatchAll(Main);
    return 0;
}
