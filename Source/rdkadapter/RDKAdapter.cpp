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

#include "Module.h"

#include "rdkadapter.h"
#include <interfaces/IRDKAdapter.h>

#define PRINT(format, ...) fprintf(stderr, _T("rdkadapter: ") format _T("\n"), ##__VA_ARGS__)

using namespace WPEFramework;

namespace {

class RDKAdapterImp : public RDKAdapter::IRDKAdapter, protected RPC::SmartInterfaceType<Exchange::IRDKAdapter> {
public:
    RDKAdapterImp(const RDKAdapterImp&) = delete;
    RDKAdapterImp(RDKAdapterImp&&) = delete;
    RDKAdapterImp& operator=(const RDKAdapterImp&) = delete;
    RDKAdapterImp& operator=(RDKAdapterImp&) = delete;
    
    RDKAdapterImp() 
    : RDKAdapter::IRDKAdapter()
    , RPC::SmartInterfaceType<Exchange::IRDKAdapter>()
    {
            if (SmartInterfaceType::Open(RPC::CommunicationTimeOut, SmartInterfaceType::Connector(), _T("RDKAdapter")) != Core::ERROR_NONE) {
                PRINT(_T("Failed to open the smart interface!"));
            }
    }
    ~RDKAdapterImp() override
    {
        SmartInterfaceType::Close(Core::infinite);
    }

    static RDKAdapterImp& Instance(){
        static RDKAdapterImp adapter;
        return adapter;
    } 

    uint32_t Test() const override {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        const Exchange::IRDKAdapter* impl = SmartInterfaceType::Interface();

        if (impl != nullptr) {
            result = impl->Test();
        }

        return result;
    }

};

}

namespace WPEFramework {

RDKAdapter::IRDKAdapter& RDKAdapter::IRDKAdapter::Instance()
{
    return RDKAdapterImp::Instance();
}

}
