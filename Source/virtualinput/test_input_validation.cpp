#include "InputValidation.h"

#ifndef __WINDOWS__
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <cstring>

int main()
{
    if (!Thunder::VirtualInput::ValidKeyAction(3) || Thunder::VirtualInput::ValidKeyAction(4) || !Thunder::VirtualInput::ValidMouseAction(3) || Thunder::VirtualInput::ValidTouchAction(3) || Thunder::VirtualInput::ValidTouchIndex(16)) {
        return 1;
    }
#ifndef __WINDOWS__
    const char path[] = "/tmp/virtualinput-endpoint-test.sock";
    unlink(path);
    const int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
    if ((descriptor < 0) || (bind(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) || (chmod(path, 0600) != 0)) {
        return 2;
    }
    const bool trusted = Thunder::VirtualInput::TrustedEndpoint(path);
    close(descriptor);
    unlink(path);
    if (!trusted || Thunder::VirtualInput::TrustedEndpoint("127.0.0.1:1")) {
        return 3;
    }
#endif
    return 0;
}
