#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace Thunder {
namespace SecurityAgent {

inline bool ValidRequest(const uint16_t capacity, const uint16_t inputLength, const unsigned char buffer[])
{
    return (buffer != nullptr) && (capacity > 0) && (inputLength <= capacity);
}

inline int CopyToken(const std::string& token, const uint16_t capacity, unsigned char buffer[])
{
    if ((buffer == nullptr) || (token.size() >= capacity)) {
        const size_t required = token.size() + 1;
        return -static_cast<int>(std::min(required, static_cast<size_t>(std::numeric_limits<int>::max())));
    }

    std::copy(token.begin(), token.end(), buffer);
    buffer[token.size()] = '\0';
    return static_cast<int>(token.size());
}

inline int MapError(const uint32_t error)
{
    if (error == 0) {
        return 0;
    }
    return -static_cast<int>(std::min(error, static_cast<uint32_t>(std::numeric_limits<int>::max())));
}

}
}
