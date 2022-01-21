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

#include <plugins/Types.h>

#include <displayinfo.h>
#include <interfaces/IDisplayInfo.h>

namespace WPEFramework {
class DisplayInfo : protected RPC::SmartInterfaceType<Exchange::IConnectionProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IConnectionProperties>;
    using DisplayOutputUpdatedCallbacks = std::map<displayinfo_display_output_change_cb, void*>;
    using OperationalStateChangeCallbacks = std::map<displayinfo_operational_state_change_cb, void*>;

    //CONSTRUCTORS
    #ifdef __WINDOWS__
    #pragma warning(disable: 4355)
    #endif
    DisplayInfo(const string& callsign)
        : BaseClass()
        , _displayConnection(nullptr)
        , _hdrProperties(nullptr)
        , _graphicsProperties(nullptr)
        , _callsign(callsign)
        , _displayUpdatedNotification(this)
    {
        ASSERT(_singleton==nullptr);
        _singleton = this;

        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callsign);
    }
    #ifdef __WINDOWS__
    #pragma warning(default: 4355)
    #endif

public:
    DisplayInfo(const DisplayInfo&) = delete;
    DisplayInfo& operator=(const DisplayInfo&) = delete;

private:
    void DisplayOutputUpdated(VARIABLE_IS_NOT_USED const Exchange::IConnectionProperties::INotification::Source event)
    {
        for (auto& index : _displayChangeCallbacks) {
            index.first(index.second);
        }
    }

    //NOTIFICATIONS
    class Notification : public Exchange::IConnectionProperties::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        Notification(DisplayInfo* parent)
            : _parent(*parent)
        {
        }

        void Updated(const Exchange::IConnectionProperties::INotification::Source event) override
        {
            _parent.DisplayOutputUpdated(event);
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IConnectionProperties::INotification)
        END_INTERFACE_MAP

    private:
        DisplayInfo& _parent;
    };

    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning) {
            if (_displayConnection == nullptr) {
                _displayConnection = BaseClass::Interface();
                if (_displayConnection != nullptr) {
                    _displayConnection->Register(&_displayUpdatedNotification);
                }

                if (_displayConnection != nullptr && _hdrProperties == nullptr) {
                    _hdrProperties = _displayConnection->QueryInterface<Exchange::IHDRProperties>();
                }
                if (_displayConnection != nullptr && _graphicsProperties == nullptr) {
                    _graphicsProperties = _displayConnection->QueryInterface<Exchange::IGraphicsProperties>();
                }
            }
        } else {
            if (_graphicsProperties != nullptr) {
                _graphicsProperties->Release();
                _graphicsProperties = nullptr;
            }
            if (_hdrProperties != nullptr) {
                _hdrProperties->Release();
                _hdrProperties = nullptr;
            }
            if (_displayConnection != nullptr) {
                _displayConnection->Unregister(&_displayUpdatedNotification);
                _displayConnection->Release();
                _displayConnection = nullptr;
            }
        }

        for (auto& index : _operationalStateCallbacks) {
            index.first(upAndRunning, index.second);
        }
    }

private:
    //MEMBERS
    Exchange::IConnectionProperties* _displayConnection;
    Exchange::IHDRProperties* _hdrProperties;
    Exchange::IGraphicsProperties* _graphicsProperties;
    std::string _callsign;

    DisplayOutputUpdatedCallbacks _displayChangeCallbacks;
    OperationalStateChangeCallbacks _operationalStateCallbacks;
    Core::Sink<Notification> _displayUpdatedNotification;
    static DisplayInfo* _singleton;

public:
    //OBJECT MANAGEMENT
    ~DisplayInfo()
    {
        BaseClass::Close(Core::infinite);
        _singleton = nullptr;
    }

    static DisplayInfo& Instance()
    {
        static DisplayInfo *instance = new DisplayInfo("DisplayInfo");
        ASSERT(instance!=nullptr);
        return *instance;
    }

    static void Dispose()
    {
        ASSERT(_singleton != nullptr);

        if(_singleton != nullptr)
        {
            delete _singleton;
        }
    }

public:
    //METHODS FROM INTERFACE
    const string& Name() const
    {
        return _callsign;
    }

    uint32_t RegisterOperationalStateChangedCallback(displayinfo_operational_state_change_cb callback, void* userdata)
    {

        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index == _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
            return Core::ERROR_NONE;
        }
        return Core::ERROR_GENERAL;
    }

    uint32_t UnregisterOperationalStateChangedCallback(displayinfo_operational_state_change_cb callback)
    {
        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index != _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.erase(index);
            return Core::ERROR_NONE;
        }
        return Core::ERROR_NOT_EXIST;
    }

    uint32_t RegisterDisplayOutputChangeCallback(displayinfo_display_output_change_cb callback, void* userdata)
    {
        DisplayOutputUpdatedCallbacks::iterator index(_displayChangeCallbacks.find(callback));

        if (index == _displayChangeCallbacks.end()) {
            _displayChangeCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
            return Core::ERROR_NONE;
        }
        return Core::ERROR_GENERAL;
    }

    uint32_t UnregisterDolbyAudioModeChangedCallback(displayinfo_display_output_change_cb callback)
    {
        DisplayOutputUpdatedCallbacks::iterator index(_displayChangeCallbacks.find(callback));

        if (index != _displayChangeCallbacks.end()) {
            _displayChangeCallbacks.erase(index);
            return Core::ERROR_NONE;
        }
        return Core::ERROR_NOT_EXIST;
    }

    uint32_t IsAudioPassthrough(bool& outIsEnabled) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->IsAudioPassthrough(outIsEnabled);
        }

        return errorCode;
    }
    uint32_t Connected(bool& outIsConnected) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->Connected(outIsConnected);
        }

        return errorCode;
    }
    uint32_t Width(uint32_t& outWidth) const
    {

        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->Width(outWidth);
        }

        return errorCode;
    }

    uint32_t Height(uint32_t& outHeight) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->Height(outHeight);
        }

        return errorCode;
    }

    uint32_t WidthInCentimeters(uint8_t& outWidthInCentimeters) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->WidthInCentimeters(outWidthInCentimeters);
        }

        return errorCode;
    }

    uint32_t HeightInCentimeters(uint8_t& outHeightInCentimeters) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->HeightInCentimeters(outHeightInCentimeters);
        }

        return errorCode;
    }

    uint32_t VerticalFreq(uint32_t& outVerticalFreq) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->VerticalFreq(outVerticalFreq);
        }

        return errorCode;
    }

    uint32_t EDID(uint16_t& len, uint8_t outData[])
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;

        if (_displayConnection != nullptr) {
            errorCode = _displayConnection->EDID(len, outData);
        }

        return errorCode;
    }

    uint32_t HDR(Exchange::IHDRProperties::HDRType& outHdrType) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;

        const Exchange::IHDRProperties* hdr = _hdrProperties;

        if (hdr != nullptr) {
            errorCode = hdr->HDRSetting(outHdrType);
        }

        return errorCode;
    }

    uint32_t HDCPProtection(Exchange::IConnectionProperties::HDCPProtectionType& outType) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IConnectionProperties* impl = _displayConnection;

        if (impl != nullptr) {
            errorCode = impl->HDCPProtection(outType);
        }

        return errorCode;
    }
    uint32_t TotalGpuRam(uint64_t& outTotalRam) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IGraphicsProperties* graphicsProperties = _graphicsProperties;

        if (graphicsProperties != nullptr) {
            errorCode = graphicsProperties->TotalGpuRam(outTotalRam);
        }

        return errorCode;
    }

    uint32_t FreeGpuRam(uint64_t& outFreeRam) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IGraphicsProperties* graphicsProperties = _graphicsProperties;

        if (graphicsProperties != nullptr) {
            errorCode = graphicsProperties->FreeGpuRam(outFreeRam);
        }

        return errorCode;
    }
};

DisplayInfo* DisplayInfo::_singleton = nullptr;
} // namespace WPEFramework

using namespace WPEFramework;

extern "C" {
uint32_t displayinfo_register_operational_state_change_callback(
    displayinfo_operational_state_change_cb callback,
    void* userdata)
{
    return DisplayInfo::Instance().RegisterOperationalStateChangedCallback(callback, userdata);
}

uint32_t displayinfo_unregister_operational_state_change_callback(
    displayinfo_operational_state_change_cb callback)
{
    return DisplayInfo::Instance().UnregisterOperationalStateChangedCallback(callback);
}

uint32_t displayinfo_register_display_output_change_callback(displayinfo_display_output_change_cb callback, void* userdata)
{
    return DisplayInfo::Instance().RegisterDisplayOutputChangeCallback(callback, userdata);
}

uint32_t displayinfo_unregister_display_output_change_callback(displayinfo_display_output_change_cb callback)
{
    return DisplayInfo::Instance().UnregisterDolbyAudioModeChangedCallback(callback);
}

void displayinfo_name(char buffer[], const uint8_t length)
{
    string name = DisplayInfo::Instance().Name();
    strncpy(buffer, name.c_str(), length);
}

uint32_t displayinfo_is_audio_passthrough(bool* is_passthrough)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (is_passthrough != nullptr) {
        errorCode = DisplayInfo::Instance().IsAudioPassthrough(*is_passthrough);
    }

    return errorCode;
}

uint32_t displayinfo_connected(bool* is_connected)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (is_connected != nullptr) {
        return DisplayInfo::Instance().Connected(*is_connected);
    }

    return errorCode;
}

uint32_t displayinfo_width(uint32_t* width)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (width != nullptr) {
        errorCode = DisplayInfo::Instance().Width(*width);
    }

    return errorCode;
}

uint32_t displayinfo_height(uint32_t* height)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (height != nullptr) {
        errorCode = DisplayInfo::Instance().Height(*height);
    }

    return errorCode;
}

uint32_t displayinfo_vertical_frequency(uint32_t* vertical_freq)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (vertical_freq != nullptr) {
        errorCode = DisplayInfo::Instance().VerticalFreq(*vertical_freq);
    }

    return errorCode;
}

uint32_t displayinfo_hdr(displayinfo_hdr_t* hdr)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (hdr != nullptr) {
        Exchange::IHDRProperties::HDRType value = Exchange::IHDRProperties::HDRType::HDR_OFF;
        *hdr = DISPLAYINFO_HDR_UNKNOWN;

        if (DisplayInfo::Instance().HDR(value) == Core::ERROR_NONE) {
            switch (value) {
            case Exchange::IHDRProperties::HDR_OFF:
                *hdr = DISPLAYINFO_HDR_OFF;
                break;
            case Exchange::IHDRProperties::HDR_10:
                *hdr = DISPLAYINFO_HDR_10;
                break;
            case Exchange::IHDRProperties::HDR_10PLUS:
                *hdr = DISPLAYINFO_HDR_10PLUS;
                break;
            case Exchange::IHDRProperties::HDR_DOLBYVISION:
                *hdr = DISPLAYINFO_HDR_DOLBYVISION;
                break;
            case Exchange::IHDRProperties::HDR_TECHNICOLOR:
                *hdr = DISPLAYINFO_HDR_TECHNICOLOR;
                break;
            default:
                TRACE_GLOBAL(Trace::Warning, ("New HDR type in the interface, not handled in client library"));
                *hdr = DISPLAYINFO_HDR_UNKNOWN;
                errorCode = Core::ERROR_UNKNOWN_KEY;
                break;
            }
            errorCode = Core::ERROR_NONE;
        }
    }

    return errorCode;
}

uint32_t displayinfo_hdcp_protection(displayinfo_hdcp_protection_t* hdcp)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (hdcp != nullptr) {
        Exchange::IConnectionProperties::HDCPProtectionType value = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_AUTO;
        *hdcp = DISPLAYINFO_HDCP_UNKNOWN;

        if (DisplayInfo::Instance().HDCPProtection(value) == Core::ERROR_NONE) {
            switch (value) {
            case Exchange::IConnectionProperties::HDCP_Unencrypted:
                *hdcp = DISPLAYINFO_HDCP_UNENCRYPTED;
                break;
            case Exchange::IConnectionProperties::HDCP_1X:
                *hdcp = DISPLAYINFO_HDCP_1X;
                break;
            case Exchange::IConnectionProperties::HDCP_2X:
                *hdcp = DISPLAYINFO_HDCP_2X;
                break;
            default:
                TRACE_GLOBAL(Trace::Warning, ("New HDCP type in the interface, not handled in client library"));
                *hdcp = DISPLAYINFO_HDCP_UNKNOWN;
                errorCode = Core::ERROR_UNKNOWN_KEY;
                break;
            }
            errorCode = Core::ERROR_NONE;
        }
    }

    return errorCode;
}

uint32_t displayinfo_total_gpu_ram(uint64_t* total_ram)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (total_ram != nullptr) {
        errorCode = DisplayInfo::Instance().TotalGpuRam(*total_ram);
    }

    return errorCode;
}

uint32_t displayinfo_free_gpu_ram(uint64_t* free_ram)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (free_ram != nullptr) {
        errorCode = DisplayInfo::Instance().FreeGpuRam(*free_ram);
    }

    return errorCode;
}

uint32_t displayinfo_edid(uint8_t buffer[], uint16_t* length)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (buffer != nullptr && length != nullptr) {
        errorCode = DisplayInfo::Instance().EDID(*length, buffer);
    }

    return errorCode;
}

uint32_t displayinfo_width_in_centimeters(uint8_t* width)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (width != nullptr) {
        errorCode = DisplayInfo::Instance().WidthInCentimeters(*width);
    }

    return errorCode;
}

uint32_t displayinfo_height_in_centimeters(uint8_t* height)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (height != nullptr) {
        errorCode = DisplayInfo::Instance().HeightInCentimeters(*height);
    }

    return errorCode;
}

bool displayinfo_is_atmos_supported()
{
    return false;
}

void displayinfo_dispose()
{
    DisplayInfo::Dispose();
}

} // extern "C"
