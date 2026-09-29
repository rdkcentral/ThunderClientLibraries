#include "TokenResponse.h"

#include <array>
#include <cstdint>
#include <string>

int main()
{
    std::array<unsigned char, 8> output {};
    if (Thunder::SecurityAgent::CopyToken("token", output.size(), output.data()) != 5 || output[5] != '\0') {
        return 1;
    }

    output.fill(0x5a);
    if (Thunder::SecurityAgent::CopyToken("12345678", output.size(), output.data()) >= 0 || output[0] != 0x5a) {
        return 2;
    }

    if (Thunder::SecurityAgent::CopyToken("", 0, nullptr) >= 0) {
        return 3;
    }

    if (Thunder::SecurityAgent::MapError(0x80000001U) >= 0) {
        return 4;
    }

    if (Thunder::SecurityAgent::MapError(1) != -1) {
        return 5;
    }

    return 0;
}
