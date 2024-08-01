#pragma once

#include <filesystem>
#include <vector>

namespace klgl
{

class Filesystem
{
public:
    static void ReadFile(const std::filesystem::path& path, std::vector<char>& buffer);
};

}  // namespace klgl
