#pragma once

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace Thunder {
namespace Compositor {

inline bool TrustedRenderNode(const std::string& path)
{
    char resolved[PATH_MAX];
    struct stat info;
    if ((realpath(path.c_str(), resolved) == nullptr) || (stat(resolved, &info) != 0)) {
        return false;
    }
    const std::string canonical(resolved);
    const std::string prefix("/dev/dri/renderD");
    return S_ISCHR(info.st_mode) && (canonical.compare(0, prefix.size(), prefix) == 0) && (canonical.find('/', prefix.size()) == std::string::npos);
}

}
}
