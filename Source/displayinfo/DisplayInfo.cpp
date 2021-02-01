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
    using DisplayOutputUpdatedCallbacks = std::map<displayinfo_display_output_change_cb, void*>;

    //CONSTRUCTORS
    DisplayInfo(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass()
        , _displayConnection(nullptr)
        , _hdrProperties(nullptr)
        , _graphicsProperties(nullptr)
        , _callsign(callsign)
        , _displayUpdatedNotification(this)
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
    void DisplayOutputUpdated(const Exchange::IConnectionProperties::INotification::Source event)
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
                    _hdrProperties->AddRef();
                }
                if (_displayConnection != nullptr && _graphicsProperties == nullptr) {
                    _graphicsProperties = _displayConnection->QueryInterface<Exchange::IGraphicsProperties>();
                    _graphicsProperties->AddRef();
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
                _displayConnection->Register(&_displayUpdatedNotification);
                _displayConnection->Release();
                _displayConnection = nullptr;
            }
        }
    }

private:
    //MEMBERS
    static std::unique_ptr<DisplayInfo> _instance; //in case client forgets to relase the instance

    Exchange::IConnectionProperties* _displayConnection;
    Exchange::IHDRProperties* _hdrProperties;
    Exchange::IGraphicsProperties* _graphicsProperties;
    std::string _callsign;

    DisplayOutputUpdatedCallbacks _displayChangeCallbacks;
    Core::Sink<Notification> _displayUpdatedNotification;

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
    void RegisterDisplayOutputChangeCallback(displayinfo_display_output_change_cb callback, void* userdata)
    {
        DisplayOutputUpdatedCallbacks::iterator index(_displayChangeCallbacks.find(callback));

        if (index == _displayChangeCallbacks.end()) {
            _displayChangeCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
        }
    }

    void UnregisterDolbyAudioModeChangedCallback(displayinfo_display_output_change_cb callback)
    {
        DisplayOutputUpdatedCallbacks::iterator index(_displayChangeCallbacks.find(callback));

        if (index != _displayChangeCallbacks.end()) {
            _displayChangeCallbacks.erase(index);
        }
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

std::unique_ptr<DisplayInfo> DisplayInfo::_instance = nullptr;

} // namespace WPEFramework

using namespace WPEFramework;

extern "C" {
struct displayinfo_type* displayinfo_instance()
{
    return reinterpret_cast<displayinfo_type*>(DisplayInfo::Instance());
}

void displayinfo_release(struct displayinfo_type* displayinfo)
{
    reinterpret_cast<DisplayInfo*>(displayinfo)->DestroyInstance();
}

void displayinfo_register_display_output_change_callback(struct displayinfo_type* instance, displayinfo_display_output_change_cb callback, void* userdata)
{
    if (instance != NULL) {
        reinterpret_cast<DisplayInfo*>(instance)->RegisterDisplayOutputChangeCallback(callback, userdata);
    }
}

void displayinfo_unregister_display_output_change_callback(struct displayinfo_type* instance, displayinfo_display_output_change_cb callback)
{
    if (instance != NULL) {
        reinterpret_cast<DisplayInfo*>(instance)->UnregisterDolbyAudioModeChangedCallback(callback);
    }
}

void displayinfo_name(struct displayinfo_type* instance, char buffer[], const uint8_t length)
{
    string name = reinterpret_cast<DisplayInfo*>(instance)->Name();
    strncpy(buffer, name.c_str(), length);
}

uint32_t displayinfo_is_audio_passthrough(struct displayinfo_type* instance, bool* is_passthrough)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && is_passthrough != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->IsAudioPassthrough(*is_passthrough);
    }

    return errorCode;
}

uint32_t displayinfo_connected(struct displayinfo_type* instance, bool* is_connected)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && is_connected != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->Connected(*is_connected);
    }

    return errorCode;
}

uint32_t displayinfo_width(struct displayinfo_type* instance, uint32_t* width)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && width != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->Width(*width);
    }

    return errorCode;
}

uint32_t displayinfo_height(struct displayinfo_type* instance, uint32_t* height)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && height != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->Height(*height);
    }

    return errorCode;
}

uint32_t displayinfo_vertical_frequency(struct displayinfo_type* instance, uint32_t* vertical_freq)
{

    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && vertical_freq != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->VerticalFreq(*vertical_freq);
    }

    return errorCode;
}

EXTERNAL uint32_t displayinfo_hdr(struct displayinfo_type* instance, displayinfo_hdr_t* hdr)
{

    if (instance != NULL && hdr != NULL) {
        Exchange::IHDRProperties::HDRType value = Exchange::IHDRProperties::HDRType::HDR_OFF;
        *hdr = DISPLAYINFO_HDR_UNKNOWN;

        if (reinterpret_cast<DisplayInfo*>(instance)->HDR(value) == Core::ERROR_NONE) {
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
                fprintf(stderr, "New HDR type in the interface, not handled in client library\n");
                ASSERT(false && "Invalid enum");
                *hdr = DISPLAYINFO_HDR_UNKNOWN;
                return Core::ERROR_UNKNOWN_KEY;
                break;
            }
            return Core::ERROR_NONE;
        }
    }

    return Core::ERROR_UNAVAILABLE;
}

uint32_t displayinfo_hdcp_protection(struct displayinfo_type* instance, displayinfo_hdcp_protection_t* hdcp)
{
    if (instance != NULL && hdcp != NULL) {
        Exchange::IConnectionProperties::HDCPProtectionType value = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_AUTO;
        *hdcp = DISPLAYINFO_HDCP_UNKNOWN;
        if (reinterpret_cast<DisplayInfo*>(instance)->HDCPProtection(value) == Core::ERROR_NONE) {
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
                fprintf(stderr, "New HDCP type in the interface, not handled in client library\n");
                ASSERT(false && "Invalid enum");
                *hdcp = DISPLAYINFO_HDCP_UNKNOWN;
                return Core::ERROR_UNKNOWN_KEY;
                break;
            }
        }
        return Core::ERROR_NONE;
    }
    return Core::ERROR_UNAVAILABLE;
}

uint32_t displayinfo_total_gpu_ram(struct displayinfo_type* instance, uint64_t* total_ram)
{

    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && total_ram != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->TotalGpuRam(*total_ram);
    }

    return errorCode;
}

uint32_t displayinfo_free_gpu_ram(struct displayinfo_type* instance, uint64_t* free_ram)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && free_ram != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->FreeGpuRam(*free_ram);
    }

    return errorCode;
}

uint32_t displayinfo_edid(struct displayinfo_type* instance, uint8_t buffer[], uint16_t* length)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && buffer != NULL && length != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->EDID(*length, buffer);
    }

    return errorCode;
}

uint32_t displayinfo_width_in_centimeters(struct displayinfo_type* instance, uint8_t* width)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && width != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->WidthInCentimeters(*width);
    }

    return errorCode;
}

uint32_t displayinfo_height_in_centimeters(struct displayinfo_type* instance, uint8_t* height)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;

    if (instance != NULL && height != NULL) {
        errorCode = reinterpret_cast<DisplayInfo*>(instance)->HeightInCentimeters(*height);
    }

    return errorCode;
}

bool displayinfo_is_atmos_supported(struct displayinfo_type* displayinfo)
{
    return false;
}

} // extern "C"
