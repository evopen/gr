#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

namespace dhh::filesystem
{
    inline std::vector<char> loadFile(const std::filesystem::path& filename, bool is_binary)
    {
        std::fstream file(filename, std::ios::ate | std::ios::in | (is_binary ? std::ios::binary : 0));
        const size_t FileSize = file.tellg();
        std::vector<char> buffer(FileSize);
        file.seekg(0);
        file.read(buffer.data(), FileSize);
        file.close();
        return buffer;
    }
}
