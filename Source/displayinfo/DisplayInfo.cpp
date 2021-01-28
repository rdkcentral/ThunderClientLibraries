/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include <com/com.h>
#include <core/core.h>
#include <plugins/Types.h>

#include <stdlib.h>

#include <displayinfo.h>
#include <interfaces/IDisplayInfo.h>

namespace WPEFramework {
class DisplayInfo : protected RPC::SmartInterfaceType<Exchange::IConnectionProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IConnectionProperties>;
    //CONSTRUCTORS
    DisplayInfo(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass()
        , _callsign(callsign)
    {
        BaseClass::Open(waitTime, node, callsign);
    }

    DisplayInfo() = delete;
    DisplayInfo(const DisplayInfo&) = delete;
    DisplayInfo& operator=(const DisplayInfo&) = delete;

    static Core::NodeId Connector()
    {
        const TCHAR* comPath = ::getenv(_T("COMMUNICATOR_PATH"));

        if (comPath == nullptr) {
#ifdef __WINDOWS__
            comPath = _T("127.0.0.1:62000");
#else
            comPath = _T("/tmp/communicator");
#endif
        }

        return Core::NodeId(comPath);
    }

private:
    //MEMBERS
    static std::unique_ptr<DisplayInfo> _instance; //in case client forgets to relase the instance
    std::string _callsign;

public:
    //OBJECT MANAGEMENT
    ~DisplayInfo()
    {
        BaseClass::Close(Core::infinite);
    }

    static DisplayInfo* Instance()
    {
        if (_instance == nullptr) {
            _instance.reset(new DisplayInfo(3000, Connector(), "DisplayInfo")); //no make_unique in C++11 :/
        }
        return _instance.get();
    }
    void DestroyInstance()
    {
        _instance.reset(nullptr);
    }

public:
    //METHODS FROM INTERFACE
    const string& Name() const
    {
        return _callsign;
    }
    uint32_t IsAudioPassthrough(bool& outIsEnabled) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->IsAudioPassthrough(outIsEnabled);
            impl->Release();
        }

        return errorCode;
    }
    uint32_t Connected(bool& outIsConnected) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->Connected(outIsConnected);
            impl->Release();
        }

        return errorCode;
    }
    uint32_t Width(uint32_t& outWidth) const
    {

        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->Width(outWidth);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t Height(uint32_t& outHeight) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->Height(outHeight);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t WidthInCentimeters(uint8_t& outWidthInCentimeters) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->WidthInCentimeters(outWidthInCentimeters);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t HeightInCentimeters(uint8_t& outHeightInCentimeters) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->HeightInCentimeters(outHeightInCentimeters);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t VerticalFreq(uint32_t& outVerticalFreq) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->VerticalFreq(outVerticalFreq);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t EDID(uint16_t& len, uint8_t outData[])
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->EDID(len, outData);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t HDR(Exchange::IHDRProperties::HDRType& outHdrType) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            const Exchange::IHDRProperties* hdr = impl->QueryInterface<const Exchange::IHDRProperties>();

            if (hdr != nullptr) {
                errorCode = hdr->HDRSetting(outHdrType);
                hdr->Release();
            }

            impl->Release();
        }

        return errorCode;
    }

    uint32_t HDCPProtection(Exchange::IConnectionProperties::HDCPProtectionType& outType) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {

            errorCode = impl->HDCPProtection(outType);

            impl->Release();
        }

        return errorCode;
    }
    uint32_t TotalGpuRam(uint64_t& outTotalRam) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            const Exchange::IGraphicsProperties* graphicsProperties = impl->QueryInterface<const Exchange::IGraphicsProperties>();

            if (graphicsProperties != nullptr) {
                errorCode = graphicsProperties->TotalGpuRam(outTotalRam);
                graphicsProperties->Release();
            }

            impl->Release();
        }

        return errorCode;
    }

    uint32_t FreeGpuRam(uint64_t& outFreeRam) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            const Exchange::IGraphicsProperties* graphicsProperties = impl->QueryInterface<const Exchange::IGraphicsProperties>();

            if (graphicsProperties != nullptr) {
                errorCode = graphicsProperties->FreeGpuRam(outFreeRam);
                graphicsProperties->Release();
            }

            impl->Release();
        }

        return errorCode;
    }
};

} // namespace WPEFramework

using namespace WPEFramework;

extern "C" {
struct displayinfo_type* displayinfo_instance(const char displayName[] = "DisplayInfo")
{
    return reinterpret_cast<displayinfo_type*>(DisplayInfo::Instance());
}

void displayinfo_release(struct displayinfo_type* displayinfo)
{
    reinterpret_cast<DisplayInfo*>(displayinfo)->DestroyInstance();
}

void displayinfo_register(struct displayinfo_type* displayinfo, displayinfo_updated_cb callback, void* userdata)
{
    // TODO reinterpret_cast<DisplayInfo*>(displayinfo)->Register(callback, userdata);
}

void displayinfo_unregister(struct displayinfo_type* displayinfo, displayinfo_updated_cb callback)
{
    // TODO reinterpret_cast<DisplayInfo*>(displayinfo)->Unregister(callback);
}

void displayinfo_name(struct displayinfo_type* displayinfo, char buffer[], const uint8_t length)
{
    string name = reinterpret_cast<DisplayInfo*>(displayinfo)->Name();
    strncpy(buffer, name.c_str(), length);
}

bool displayinfo_is_audio_passthrough(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->IsAudioPassthrough();
}

bool displayinfo_connected(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->Connected();
}

uint32_t displayinfo_width(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->Width();
}

uint32_t displayinfo_height(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->Height();
}

uint32_t displayinfo_vertical_frequency(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->VerticalFreq();
}

displayinfo_hdr_t displayinfo_hdr(struct displayinfo_type* displayinfo)
{
    displayinfo_hdr_t result = DISPLAYINFO_HDR_UNKNOWN;

    switch (reinterpret_cast<DisplayInfo*>(displayinfo)->HDR()) {
    case Exchange::IHDRProperties::HDR_OFF:
        result = DISPLAYINFO_HDR_OFF;
        break;
    case Exchange::IHDRProperties::HDR_10:
        result = DISPLAYINFO_HDR_10;
        break;
    case Exchange::IHDRProperties::HDR_10PLUS:
        result = DISPLAYINFO_HDR_10PLUS;
        break;
    case Exchange::IHDRProperties::HDR_DOLBYVISION:
        result = DISPLAYINFO_HDR_DOLBYVISION;
        break;
    case Exchange::IHDRProperties::HDR_TECHNICOLOR:
        result = DISPLAYINFO_HDR_TECHNICOLOR;
        break;
    default:
        result = DISPLAYINFO_HDR_UNKNOWN;
        break;
    }

    return result;
}

displayinfo_hdcp_protection_t displayinfo_hdcp_protection(struct displayinfo_type* displayinfo)
{

    displayinfo_hdcp_protection_t type = DISPLAYINFO_HDCP_UNKNOWN;

    switch (reinterpret_cast<DisplayInfo*>(displayinfo)->HDCPProtection()) {
    case Exchange::IConnectionProperties::HDCP_Unencrypted:
        type = DISPLAYINFO_HDCP_UNENCRYPTED;
        break;
    case Exchange::IConnectionProperties::HDCP_1X:
        type = DISPLAYINFO_HDCP_1X;
        break;
    case Exchange::IConnectionProperties::HDCP_2X:
        type = DISPLAYINFO_HDCP_2X;
        break;
    default:
        type = DISPLAYINFO_HDCP_UNKNOWN;
        break;
    }

    return type;
}

uint64_t displayinfo_total_gpu_ram(struct displayinfo_type* instance)
{
    return reinterpret_cast<DisplayInfo*>(instance)->TotalGpuRam();
}

uint64_t displayinfo_free_gpu_ram(struct displayinfo_type* instance)
{
    return reinterpret_cast<DisplayInfo*>(instance)->FreeGpuRam();
}

uint32_t displayinfo_edid(struct displayinfo_type* displayinfo, uint8_t buffer[], uint16_t* length)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->EDID(*length, buffer);
}

uint8_t displayinfo_width_in_centimeters(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->WidthInCentimeters();
}

uint8_t displayinfo_height_in_centimeters(struct displayinfo_type* displayinfo)
{
    return reinterpret_cast<DisplayInfo*>(displayinfo)->HeightInCentimeters();
}

bool displayinfo_is_atmos_supported(struct displayinfo_type* displayinfo)
{
    return false;
}

} // extern "C"
