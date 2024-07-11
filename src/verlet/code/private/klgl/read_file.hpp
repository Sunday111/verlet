#pragma once

#include <filesystem>
#include <vector>

namespace klgl
{

void ReadFile(const std::filesystem::path& path, std::vector<char>& buffer);

}
