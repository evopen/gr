#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

namespace dhh::filesystem
{
    inline std::vector<char> LoadFile(const std::filesystem::path& filename, bool is_binary)
    {
        std::fstream file(filename, std::ios::ate | std::ios::in | (is_binary ? std::ios::binary : 0));
        const size_t kFileSize = file.tellg();
        std::vector<char> buffer(kFileSize);
        file.seekg(0);
        file.read(buffer.data(), kFileSize);
        file.close();
        return buffer;
    }
}
