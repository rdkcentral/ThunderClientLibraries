#include "adapter/SubSampleParser.h"

#include <array>
#include <cstdint>
#include <vector>

int main()
{
    std::vector<SubSampleInfo> entries;
    uint32_t encryptedLength = 0;

    const std::array<uint8_t, 12> valid { 0, 2, 0, 0, 0, 4, 0, 1, 0, 0, 0, 3 };
    if (!Thunder::OCDM::ParseSubSamples(valid.data(), valid.size(), 2, 10, entries, encryptedLength) || entries.size() != 2 || encryptedLength != 7) {
        return 1;
    }

    const std::array<uint8_t, 5> truncated { 0, 0, 0, 0, 1 };
    if (Thunder::OCDM::ParseSubSamples(truncated.data(), truncated.size(), 1, 1, entries, encryptedLength)) {
        return 2;
    }

    const std::array<uint8_t, 6> oversizedClear { 0, 2, 0, 0, 0, 1 };
    if (Thunder::OCDM::ParseSubSamples(oversizedClear.data(), oversizedClear.size(), 1, 2, entries, encryptedLength)) {
        return 3;
    }

    const std::array<uint8_t, 12> overflowingEncrypted { 0, 0, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 1 };
    if (Thunder::OCDM::ParseSubSamples(overflowingEncrypted.data(), overflowingEncrypted.size(), 2, UINT32_MAX, entries, encryptedLength)) {
        return 4;
    }

    if (Thunder::OCDM::ParseSubSamples(valid.data(), valid.size(), 1, 10, entries, encryptedLength)) {
        return 5;
    }

    const std::vector<uint8_t> excessiveCount(256 * 6);
    if (Thunder::OCDM::ParseSubSamples(excessiveCount.data(), excessiveCount.size(), 256, 0, entries, encryptedLength)) {
        return 6;
    }

    if (!Thunder::OCDM::ParseSubSamples(nullptr, 0, 0, 0, entries, encryptedLength) || !entries.empty() || encryptedLength != 0) {
        return 7;
    }

    if (Thunder::OCDM::ParseSubSamples(nullptr, 6, 1, 1, entries, encryptedLength)) {
        return 8;
    }

    return 0;
}
