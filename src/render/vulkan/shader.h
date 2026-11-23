#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "core/check.h"
#include "render/vulkan/vk.h"

namespace ih::vulkan {

// Reads a SPIR-V binary produced by the build-time glslang step.
inline std::vector<uint32_t> loadSpirv (const std::string& path) {
    std::ifstream file (path, std::ios::binary | std::ios::ate);
    IH_CHECK (file.is_open ());
    const auto size = file.tellg ();
    IH_CHECK (size > 0 && size % sizeof (uint32_t) == 0);
    std::vector<uint32_t> code (static_cast<size_t> (size) / sizeof (uint32_t));
    file.seekg (0);
    file.read (reinterpret_cast<char*> (code.data ()), size);
    return code;
}

inline vk::raii::ShaderModule loadShaderModule (vk::raii::Device& device, const std::string& path) {
    const auto code = loadSpirv (path);
    vk::ShaderModuleCreateInfo info{};
    info.codeSize = code.size () * sizeof (uint32_t);
    info.pCode = code.data ();
    return vk::raii::ShaderModule (device, info);
}

}  // namespace ih::vulkan
