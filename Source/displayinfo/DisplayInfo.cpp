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
#include "ExtendedDisplayIdentification.h"

namespace WPEFramework {
class DisplayInfo : protected RPC::SmartInterfaceType<Exchange::IConnectionProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IConnectionProperties>;
    using DisplayOutputUpdatedCallbacks = std::map<displayinfo_display_output_change_cb, void*>;
    using OperationalStateChangeCallbacks = std::map<displayinfo_operational_state_change_cb, void*>;

    //CONSTRUCTORS
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
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
POP_WARNING()

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

uint32_t displayinfo_parse_edid(const uint8_t buffer[], uint16_t length, displayinfo_edid_base_info_t* edid_info)
{
    uint32_t errorCode = Core::ERROR_GENERAL;

    if (buffer != nullptr && length != 0 && edid_info != nullptr) {
        Plugin::ExtendedDisplayIdentification edid;
        uint32_t len = (length > edid.Length()) ? edid.Length() : length;
        memcpy((edid.Segment(0)), buffer, len);

        if(edid.IsValid()) {
            ::memset(edid_info, 0, sizeof(*edid_info));
            memcpy(edid_info->manufacturer_id, edid.Manufacturer().c_str(), sizeof(edid_info->manufacturer_id));
            edid_info->product_code = edid.ProductCode();
            edid_info->serial_number = edid.Serial();
            edid_info->manufacture_week = edid.Week();
            edid_info->manufacture_year = edid.Year();
            edid_info->version = edid.Major();
            edid_info->revision = edid.Minor();
            edid_info->digital = edid.Digital();
            if (edid.Digital() == true) {
                edid_info->bits_per_color = edid.BitsPerColor();
                edid_info->video_interface = edid.VideoInterface();
                edid_info->display_type = edid.DisplayType();
            }
            edid_info->width_in_centimeters = edid.WidthInCentimeters();
            edid_info->height_in_centimeters = edid.HeightInCentimeters();
            edid_info->preferred_width_in_pixels = edid.PreferredWidthInPixels();
            edid_info->preferred_height_in_pixels = edid.PreferredHeightInPixels();
            errorCode = Core::ERROR_NONE;
        }
    }
    return errorCode;
}

uint32_t displayinfo_edid_cea_extension_info(const uint8_t buffer[], const uint16_t length, displayinfo_edid_cea_extension_info_t *cea_info)
{
    uint32_t errorCode = Core::ERROR_GENERAL;

    if (buffer != nullptr && length != 0 && cea_info != nullptr) {
        Plugin::ExtendedDisplayIdentification edid;
        uint32_t max_seg_len = edid.Length();
        uint32_t segments = (length / max_seg_len);

        if(length % max_seg_len > 0) {
            segments += 1;
        }

        uint32_t copied = 0;
        for(uint32_t seg_iter=0; seg_iter < segments; ++seg_iter) {
            uint32_t begin = seg_iter * max_seg_len;
            uint32_t seg_len = ((length - copied) > max_seg_len) ? max_seg_len : (length - copied);
            memcpy((edid.Segment(seg_iter)), buffer + begin, seg_len);
            copied += seg_len;
        }

        ASSERT(copied == length);

        if(edid.IsValid()) {
            Plugin::ExtendedDisplayIdentification::Iterator segment = edid.CEASegment();
            if(segment.IsValid() == true) {
                ::memset(cea_info, 0, sizeof(*cea_info));

                Plugin::ExtendedDisplayIdentification::CEA cae(segment.Current());
                cea_info->version = cae.Version();
                cea_info->audio_formats = cae.AudioFormats();
                cea_info->color_spaces = cae.ColorSpaces();
                cea_info->color_formats = cae.ColorFormats();
                cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_RGB] = cae.RGBColorDepths();
                cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR444] = cae.YCbCr444ColorDepths();
                cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR422] = cae.YCbCr422ColorDepths();
                cea_info->color_depths[DISPLAYINFO_EDID_COLOR_DEPTH_INDEX_YCBCR420] = cae.YCbCr420ColorDepths();

                std::vector<uint8_t> vic_list;
                cae.Timings(vic_list);
                ASSERT(vic_list.size() <= sizeof(cea_info->timings));

                for (uint8_t i = 0; i < vic_list.size(); ++i) {
                    cea_info->timings[i] = vic_list[i];
                }

                cea_info->number_of_timings = static_cast<uint8_t>(vic_list.size());

                errorCode = Core::ERROR_NONE;
            } else {
                errorCode = Core::ERROR_UNAVAILABLE;
            }
        }
    }
    return errorCode;
}

uint32_t displayinfo_edid_vic_to_standard_timing(const uint8_t vic, displayinfo_edid_standard_timing_t* result)
{
    static const displayinfo_edid_standard_timing_t standardTimingMap[] = {
        { 1 /* DMT0659 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 640, 480 },
        { 2 /* 480P */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 720, 480 },
        { 3 /* 480PH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59940, 720, 480 },
        { 4 /* 720P */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 1280, 720 },
        { 5 /* 1080I */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 1920, 540 },
        { 6 /* 480I */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 1440, 240 },
        { 7 /* 480IH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59940, 1440, 240 },
        { 8 /* 240P */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59826, 1440, 240 },
        { 9 /* 240PH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59826, 1440, 240 },
        { 10 /* 480I4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 2880, 240 },
        { 11 /* 480I4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59940, 2880, 240 },
        { 12 /* 240P4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_60000, 2880, 240 },
        { 13 /* 240P4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 2880, 240 },
        { 14 /* 480P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 1440, 480 },
        { 15 /* 480P2XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59940, 1440, 480 },
        { 16 /* 1080P */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 1920, 1080 },
        { 17 /* 576P */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 720, 576 },
        { 18 /* 576PH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 720, 576 },
        { 19 /* 720P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1280, 720 },
        { 20 /* 1080I25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1920, 540 },
        { 21 /* 576I */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 288 },
        { 22 /* 576IH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 288 },
        { 23 /* 288P */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 288 },
        { 24 /* 288PH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 288 },
        { 25 /* 576I4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 288 },
        { 26 /* 576I4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 288 },
        { 27 /* 288P4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 288 },
        { 28 /* 288P4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 288 },
        { 29 /* 576P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 576 },
        { 30 /* 576P2XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1440, 576 },
        { 31 /* 1080P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1920, 1080 },
        { 32 /* 1080P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 1920, 1080 },
        { 33 /* 1080P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_25000, 1920, 1080 },
        { 34 /* 1080P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 1920, 1080 },
        { 35 /* 480P4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_59940, 2880, 240 },
        { 36 /* 480P4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_59940, 2880, 240 },
        { 37 /* 576P4X */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 576 },
        { 38 /* 576P4XH */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 2880, 576 },
        { 39 /* 1080I25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 1920, 540 },
        { 40 /* 1080I50 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 1920, 540 },
        { 41 /* 720P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 1280, 720 },
        { 42 /* 576P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_100000, 720, 576 },
        { 43 /* 576P100H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 720, 576 },
        { 44 /* 576I50 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_100000, 1440, 576 },
        { 45 /* 576I50H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 1440, 576 },
        { 46 /* 1080I60 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1920, 540 },
        { 47 /* 720P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1280, 720 },
        { 48 /* 480P119 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 720, 576 },
        { 49 /* 480P119H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 720, 576 },
        { 50 /* 480I59 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1440, 576 },
        { 51 /* 480I59H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1440, 576 },
        { 52 /* 576P200 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_200000, 720, 576 },
        { 53 /* 576P200H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_200000, 720, 576 },
        { 54 /* 576I100 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_200000, 1440, 288 },
        { 55 /* 576I100H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_200000, 1440, 288 },
        { 56 /* 480P239 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_239760, 720, 480 },
        { 57 /* 480P239H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_239760, 720, 480 },
        { 58 /* 480I119 */, DISPLAYINFO_EDID_ASPECT_RATIO_4_TO_3, DISPLAYINFO_EDID_FREQUENCY_239760, 1440, 240 },
        { 59 /* 480I119H */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_239760, 1440, 240 },
        { 60 /* 720P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 1280, 720 },
        { 61 /* 720P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_25000, 1280, 720 },
        { 62 /* 720P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 1280, 720 },
        { 63 /* 1080P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1920, 1080 },
        { 64 /* 1080P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 1920, 1080 },
        { 65 /* 720P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 1280, 720 },
        { 66 /* 720P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 1280, 720 },
        { 67 /* 720P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 1280, 720 },
        { 68 /* 720P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 1280, 720 },
        { 69 /* 720P */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 1650, 750 },
        { 70 /* 720P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 1280, 720 },
        { 71 /* 720P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1280, 720 },
        { 72 /* 1080P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 1920, 1080 },
        { 73 /* 1080P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 1920, 1080 },
        { 74 /* 1080P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 1920, 1080 },
        { 75 /* 1080P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 1920, 1080 },
        { 76 /* 1080P */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 1920, 1080 },
        { 77 /* 1080P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 1920, 1080 },
        { 78 /* 1080P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1920, 1080 },
        { 79 /* 720P2X24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 1680, 720 },
        { 80 /* 720P2X25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 1680, 720 },
        { 81 /* 720P2X30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 1680, 720 },
        { 82 /* 720P2X50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 1680, 720 },
        { 83 /* 720P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 1680, 720 },
        { 84 /* 720P2X100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 1680, 720 },
        { 85 /* 720P2X120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 1680, 720 },
        { 86 /* 1080P2X24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 2560, 1080 },
        { 87 /* 1080P2X25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 2560, 1080 },
        { 88 /* 1080P2X30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 2560, 1080 },
        { 89 /* 1080P2X50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 2560, 1080 },
        { 90 /* 1080P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 2560, 1080 },
        { 91 /* 1080P2X100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 2560, 1080 },
        { 92 /* 1080P2X120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 2560, 1080 },
        { 93 /* 2160P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 3840, 2160 },
        { 94 /* 2160P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_25000, 3840, 2160 },
        { 95 /* 2160P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 3840, 2160 },
        { 96 /* 2160P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 3840, 2160 },
        { 97 /* 2160P60 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 3840, 2160 },
        { 98 /* 2160P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 4096, 2160 },
        { 99 /* 2160P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_25000, 4096, 2160 },
        { 100 /* 2160P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 4096, 2160 },
        { 101 /* 2160P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_50000, 4096, 2160 },
        { 102 /* 2160P */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_60000, 4096, 2160 },
        { 103 /* 2160P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 3840, 2160 },
        { 104 /* 2160P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 3840, 2160 },
        { 105 /* 2160P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 3840, 2160 },
        { 106 /* 2160P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 3840, 2160 },
        { 107 /* 2160P */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 3840, 2160 },
        { 108 /* 720P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 1280, 720 },
        { 109 /* 720P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 1280, 720 },
        { 110 /* 720P2X48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 1680, 720 },
        { 111 /* 1080P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 1920, 1080 },
        { 112 /* 1080P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 1920, 1080 },
        { 113 /* 1080P2X48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 2560, 1080 },
        { 114 /* 2160P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 3840, 2160 },
        { 115 /* 2160P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 4096, 2160 },
        { 116 /* 2160P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 3840, 2160 },
        { 117 /* 2160P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 3840, 2160 },
        { 118 /* 2160P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 3840, 2160 },
        { 119 /* 2160P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 3840, 2160 },
        { 120 /* 2160P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 3840, 2160 },
        { 121 /* 2160P2X24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 5120, 2160 },
        { 122 /* 2160P2X25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 5120, 2160 },
        { 123 /* 2160P2X30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 5120, 2160 },
        { 124 /* 2160P2X48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 5120, 2160 },
        { 125 /* 2160P2X50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 5120, 2160 },
        { 126 /* 2160P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 5120, 2160 },
        { 127 /* 2160P2X100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 5120, 2160 },
        // ---
        { 193 /* 2160P2X120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 5120, 2160 },
        { 194 /* 4320P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 7680, 4320 },
        { 195 /* 4320P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_25000, 7680, 4320 },
        { 196 /* 4320P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 7680, 4320 },
        { 197 /* 4320P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 7680, 4320 },
        { 198 /* 4320P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_50000, 7680, 4320 },
        { 199 /* 4320P */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_60000, 7680, 4320 },
        { 200 /* 4320P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_100000, 7680, 4320 },
        { 201 /* 4320P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_16_TO_9, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 7680, 4320 },
        { 202 /* 4320P24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 7680, 4320 },
        { 203 /* 4320P25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 7680, 4320 },
        { 204 /* 4320P30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 7680, 4320 },
        { 205 /* 4320P48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 7680, 4320 },
        { 206 /* 4320P50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 7680, 4320 },
        { 207 /* 4320P */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 7680, 4320 },
        { 208 /* 4320P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 7680, 4320 },
        { 209 /* 4320P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 7680, 4320 },
        { 210 /* 4320P2X24 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_23980_OR_24000, 10240, 4320 },
        { 211 /* 4320P2X25 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_25000, 10240, 4320 },
        { 212 /* 4320P2X30 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_29970_OR_30000, 10240, 4320 },
        { 213 /* 4320P2X48 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_47960_OR_48000, 10240, 4320 },
        { 214 /* 4320P2X50 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_50000, 10240, 4320 },
        { 215 /* 4320P2X */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_60000, 10240, 4320 },
        { 216 /* 4320P2X100 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_100000, 10240, 4320 },
        { 217 /* 4320P2X120 */, DISPLAYINFO_EDID_ASPECT_RATIO_64_TO_27, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 10240, 4320 },
        { 218 /* 160P100 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_100000, 4096, 2160 },
        { 219 /* 2160P120 */, DISPLAYINFO_EDID_ASPECT_RATIO_256_TO_135, DISPLAYINFO_EDID_FREQUENCY_119880_OR_120000, 4096, 2160}
    };

    uint32_t errorCode = Core::ERROR_UNAVAILABLE;
    if (result != nullptr) {
        for(uint8_t index = 0; index < (sizeof(standardTimingMap)/sizeof(displayinfo_edid_standard_timing_t)); ++index) {
            if(standardTimingMap[index].vic == vic) {
                memcpy(result, &standardTimingMap[index], sizeof(displayinfo_edid_standard_timing_t));
                errorCode = Core::ERROR_NONE;
                break;
            }
        }
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
