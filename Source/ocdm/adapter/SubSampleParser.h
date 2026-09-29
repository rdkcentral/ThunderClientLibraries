#pragma once

#include "../open_cdm.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace Thunder {
namespace OCDM {

inline bool ParseSubSamples(const uint8_t data[], const uint32_t length, const uint32_t count, const uint32_t sampleLength, std::vector<SubSampleInfo>& entries, uint32_t& encryptedLength)
{
    constexpr uint32_t entryLength = sizeof(uint16_t) + sizeof(uint32_t);
    if ((count > std::numeric_limits<uint8_t>::max()) || (count > (std::numeric_limits<uint32_t>::max() / entryLength)) || (length != count * entryLength) || ((count != 0) && (data == nullptr))) {
        return false;
    }

    entries.clear();
    entries.reserve(count);
    uint64_t sampleOffset = 0;
    uint64_t encryptedOffset = 0;

    for (uint32_t index = 0; index < count; ++index) {
        const uint8_t* entry = data + (index * entryLength);
        const uint16_t clear = static_cast<uint16_t>((static_cast<uint16_t>(entry[0]) << 8) | entry[1]);
        const uint32_t encrypted = (static_cast<uint32_t>(entry[2]) << 24) | (static_cast<uint32_t>(entry[3]) << 16) | (static_cast<uint32_t>(entry[4]) << 8) | entry[5];
        sampleOffset += static_cast<uint64_t>(clear) + encrypted;
        encryptedOffset += encrypted;
        if ((sampleOffset > sampleLength) || (encryptedOffset > std::numeric_limits<uint32_t>::max())) {
            entries.clear();
            return false;
        }
        entries.push_back({ clear, encrypted });
    }

    encryptedLength = static_cast<uint32_t>(encryptedOffset);
    return true;
}

}
}
