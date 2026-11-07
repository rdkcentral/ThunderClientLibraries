#include "ProvisionBounds.h"

#include <cstdint>

int main()
{
    uint32_t required = 0;
    if (!IPC::Provisioning::RequiredCapacity(1023, 1, required) || required != 1024) {
        return 1;
    }
    if (IPC::Provisioning::RequiredCapacity(10240, 1, required)) {
        return 2;
    }
    if (IPC::Provisioning::RequiredCapacity(UINT32_MAX, 2, required)) {
        return 3;
    }
    if (IPC::Provisioning::ExpandedCapacity(6000) != 10240) {
        return 4;
    }
    return 0;
}
