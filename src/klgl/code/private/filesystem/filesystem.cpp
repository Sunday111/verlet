#include <fmt/format.h>
#include <fmt/std.h>

#include <fstream>

#include "klgl/filesystem/filesystem.hpp"


namespace klgl
{

void Filesystem::ReadFile(const std::filesystem::path& path, std::vector<char>& buffer)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    [[unlikely]] if (!file.is_open())
    {
        throw std::runtime_error(fmt::format("failed to open file {}", path));
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(static_cast<size_t>(size));

    [[unlikely]] if (!file.read(buffer.data(), size))
    {
        throw std::runtime_error(fmt::format("failed to read {} bytes from file {}", size, path));
    }
}

}  // namespace klgl
