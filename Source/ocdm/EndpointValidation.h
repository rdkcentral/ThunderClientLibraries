#pragma once

#ifndef __WINDOWS__
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <string>

namespace Thunder {
namespace OCDM {

inline bool TrustedEndpoint(const std::string& endpoint)
{
#ifdef __WINDOWS__
    return endpoint == "127.0.0.1:63000";
#else
    struct stat info;
    if ((endpoint.empty()) || (endpoint[0] != '/') || (lstat(endpoint.c_str(), &info) != 0)) {
        return false;
    }
    return S_ISSOCK(info.st_mode) && (info.st_uid == geteuid()) && ((info.st_mode & (S_IWGRP | S_IWOTH)) == 0);
#endif
}

}
}
