#include "ResponseBounds.h"

#include <array>

int main()
{
    std::array<char, 8> output {};
    if (IPC::Provisioning::CopyTextResponse("device", output.size(), output.data()) != 6 || output[6] != '\0') {
        return 1;
    }
    output.fill('x');
    if (IPC::Provisioning::CopyTextResponse("12345678", output.size(), output.data()) >= 0 || output[0] != 'x') {
        return 2;
    }
    if (IPC::Provisioning::MapResponseError(0x80000001U) >= 0) {
        return 3;
    }
    if (IPC::Provisioning::ValidBlobLength(10241, 10240)) {
        return 4;
    }
    return 0;
}
