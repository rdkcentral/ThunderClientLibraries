#include "EndpointValidation.h"

#ifndef __WINDOWS__
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <cstring>

int main()
{
#ifdef __WINDOWS__
    return Thunder::SecurityAgent::TrustedEndpoint("127.0.0.1:63000") ? 0 : 1;
#else
    const char path[] = "/tmp/securityagent-endpoint-test.sock";
    unlink(path);
    const int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor < 0) {
        return 1;
    }
    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
    if ((bind(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) || (chmod(path, 0600) != 0)) {
        close(descriptor);
        unlink(path);
        return 2;
    }
    const bool trusted = Thunder::SecurityAgent::TrustedEndpoint(path);
    close(descriptor);
    unlink(path);
    return trusted && !Thunder::SecurityAgent::TrustedEndpoint("127.0.0.1:1") ? 0 : 3;
#endif
}
