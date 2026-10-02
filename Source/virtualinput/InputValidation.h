#pragma once

#include <cstdint>
#include <string>

#ifndef __WINDOWS__
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Thunder {
namespace VirtualInput {

inline bool ValidKeyAction(const uint32_t action) { return action <= 3; }
inline bool ValidMouseAction(const uint32_t action) { return action <= 3; }
inline bool ValidTouchAction(const uint32_t action) { return action <= 2; }
inline bool ValidTouchIndex(const uint32_t index) { return index < 16; }

inline bool TrustedEndpoint(const std::string& endpoint)
{
#ifdef __WINDOWS__
    return false;
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
