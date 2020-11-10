#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>

#include <deviceidentification.h>
#include <interfaces/IDeviceIdentification.h>

#ifndef __DEBUG__
#define Trace(fmt, ...)                                                       \
    do {                                                                      \
        fprintf(stdout, "\033[1;32m[%s:%d](%s){%p}<%d>:" fmt "\n\033[0m",     \
            __FILE__, __LINE__, __FUNCTION__, this, getpid(), ##__VA_ARGS__); \
        fflush(stdout);                                                       \
    } while (0)
#else
#define Trace(fmt, ...)
#endif

namespace WPEFramework {
class DeviceIdentification : public Core::IReferenceCounted {
private:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    DisplayInfo(const string& displayName,
        Exchange::IConnectionProperties* interface)
    {
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
};
} // namespace WPEFramework