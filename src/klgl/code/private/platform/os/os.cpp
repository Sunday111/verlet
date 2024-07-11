#include "klgl/platform/os/os.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#undef APIENTRY
#include "Windows.h"

namespace klgl::os
{
std::filesystem::path GetExecutableDir()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    const size_t index = std::wstring_view(path).find_last_of(L"\\/");
    path[index] = L'\0';
    return path;
}
}  // namespace klgl::os

#endif

#ifdef __unix__

#include <stdio.h>
#include <unistd.h>

namespace klgl::os
{
std::filesystem::path GetExecutableDir()
{
    char path[1024];  // NOLINT
    readlink("/proc/self/exe", path, sizeof(path) - 1);
    const size_t index = std::string_view(path).find_last_of("\\/");
    path[index] = '\0';
    return path;
}
}  // namespace klgl::os
#endif
