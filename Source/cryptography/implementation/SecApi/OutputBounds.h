#pragma once

#include <cstddef>
#include <cstdint>

namespace Thunder {
namespace Cryptography {
namespace Implementation {

inline bool CanCopyOutput(const size_t required, const size_t capacity, const uint8_t output[])
{
    return (output != nullptr) && (required <= capacity);
}

}
}
}
