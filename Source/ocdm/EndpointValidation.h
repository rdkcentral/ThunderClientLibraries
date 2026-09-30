#pragma once

#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace Thunder {
namespace OCDM {

inline bool TrustedEndpoint(const std::string& endpoint)
{
    struct stat info;
    if ((endpoint.empty()) || (endpoint[0] != '/') || (lstat(endpoint.c_str(), &info) != 0)) {
        return false;
    }
    return S_ISSOCK(info.st_mode) && (info.st_uid == geteuid()) && ((info.st_mode & (S_IWGRP | S_IWOTH)) == 0);
}

}
}
