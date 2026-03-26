/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Common_CompositionClientRender
#endif

#include <core/core.h>
#include <messaging/messaging.h>

#if defined(__WINDOWS__)
#if defined(COMPOSITORCLIENT_EXPORTS)
#undef EXTERNAL
#define EXTERNAL EXTERNAL_EXPORT
#else
#pragma comment(lib, "compositorclient.lib")
#endif
#endif


namespace Thunder {
namespace Trace {
class Timing {
public:
    ~Timing() = default;
    Timing() = delete;
    Timing(const Timing&) = delete;
    Timing& operator=(const Timing&) = delete;
    Timing(const TCHAR formatter[], ...)
    {
        va_list ap;
        va_start(ap, formatter);
        Thunder::Core::Format(_text, formatter, ap);
        va_end(ap);
    }
    explicit Timing(const string& text)
        : _text(Thunder::Core::ToString(text))
    {
    }

public:
    const char* Data() const
    {
        return (_text.c_str());
    }
    uint16_t Length() const
    {
        return (static_cast<uint16_t>(_text.length()));
    }

private:
    std::string _text;
}; // class Timing

} // namespace Trace
} // namespace Thunder



