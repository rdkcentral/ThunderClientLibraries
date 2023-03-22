/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2020 Metrological
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

#include <stdint.h>
#include <string>

#ifndef EXTERNAL
#ifdef _MSVC_LANG
#ifdef COMPOSITORCLIENT_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#endif
#else
#define EXTERNAL __attribute__ ((visibility ("default")))
#endif
#endif

namespace WPEFramework {
namespace RDKAdapter {

struct EXTERNAL IRDKAdapter {

    virtual ~IRDKAdapter() = default;

    static IRDKAdapter& Instance();

    // interface
    struct INotification {
        virtual ~INotification() = default;

        virtual void InterfaceUpdate(const std::string& interfacename) = 0;
    };

    virtual uint32_t Register(IRDKAdapter::INotification* sink) = 0;
    virtual uint32_t Unregister(IRDKAdapter::INotification* sink) = 0;

    virtual uint32_t InterfaceAvailable(const std::string& interfacename, bool& available) const = 0;
    virtual uint32_t InterfaceAddress(const std::string& interfacename, std::string& primaryaddress) const = 0;
};

}
}