#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace IPC {
namespace Provisioning {

inline int CopyTextResponse(const std::string& value, const uint16_t capacity, char output[])
{
    const size_t required = value.size() + 1;
    if ((output == nullptr) || (required > capacity)) {
        return -static_cast<int>(std::min(required, static_cast<size_t>(std::numeric_limits<int>::max())));
    }
    std::copy(value.begin(), value.end(), output);
    output[value.size()] = '\0';
    return static_cast<int>(value.size());
}

inline int MapResponseError(const uint32_t error)
{
    return error == 0 ? 0 : -static_cast<int>(std::min(error, static_cast<uint32_t>(std::numeric_limits<int>::max())));
}

inline bool ValidBlobLength(const uint32_t length, const uint32_t capacity)
{
    return length <= capacity;
}

}
}
