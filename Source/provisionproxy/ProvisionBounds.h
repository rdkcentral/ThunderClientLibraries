#pragma once

#include <cstdint>
#include <limits>

namespace IPC {
namespace Provisioning {

inline bool RequiredCapacity(const uint32_t offset, const uint32_t length, uint32_t& required)
{
    constexpr uint32_t maximum = 10 * 1024;
    if ((length > maximum) || (offset > maximum - length)) {
        return false;
    }
    required = offset + length;
    return true;
}

inline uint32_t ExpandedCapacity(const uint32_t required)
{
    constexpr uint32_t maximum = 10 * 1024;
    return required > (maximum / 2) ? maximum : required * 2;
}

}
}
