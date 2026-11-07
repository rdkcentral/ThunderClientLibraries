#include "OutputBounds.h"

#include <array>

int main()
{
    std::array<uint8_t, 8> output {};
    if (!Thunder::Cryptography::Implementation::CanCopyOutput(8, output.size(), output.data())) {
        return 1;
    }
    if (Thunder::Cryptography::Implementation::CanCopyOutput(9, output.size(), output.data())) {
        return 2;
    }
    if (Thunder::Cryptography::Implementation::CanCopyOutput(1, 1, nullptr)) {
        return 3;
    }
    return 0;
}
