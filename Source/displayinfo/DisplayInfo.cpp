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

    static const displayinfo_edid_standardtiming_t standardTimingMap[] =
    {
         {1,  DISPLAYINFO_EDID_TSN_DMT0659,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 640, 480},
         {2,  DISPLAYINFO_EDID_TSN_480P,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 720, 480},
         {3,  DISPLAYINFO_EDID_TSN_480PH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59940, 720, 480},
         {4,  DISPLAYINFO_EDID_TSN_720P,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 1280, 720},
         {5,  DISPLAYINFO_EDID_TSN_1080I,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 1920, 540},
         {6,  DISPLAYINFO_EDID_TSN_480I,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 1440, 240},
         {7,  DISPLAYINFO_EDID_TSN_480IH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59940, 1440, 240},
         {8,  DISPLAYINFO_EDID_TSN_240P,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59826, 1440, 240},
         {9,  DISPLAYINFO_EDID_TSN_240PH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59826, 1440, 240},
         {10,  DISPLAYINFO_EDID_TSN_480I4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 2880, 240},
         {11,  DISPLAYINFO_EDID_TSN_480I4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59940, 2880, 240},
         {12,  DISPLAYINFO_EDID_TSN_240P4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_60000, 2880, 240},
         {13,  DISPLAYINFO_EDID_TSN_240P4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 2880, 240},
         {14,  DISPLAYINFO_EDID_TSN_480P2X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 1440, 480},
         {15,  DISPLAYINFO_EDID_TSN_480P2XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59940, 1440, 480},
         {16,  DISPLAYINFO_EDID_TSN_1080P,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 1920, 1080},
         {17,  DISPLAYINFO_EDID_TSN_576P,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 720, 576},
         {18,  DISPLAYINFO_EDID_TSN_576PH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 720, 576},
         {19,  DISPLAYINFO_EDID_TSN_720P50,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1280, 720},
         {20,  DISPLAYINFO_EDID_TSN_1080I25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1920, 540},
         {21,  DISPLAYINFO_EDID_TSN_576I,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 1440, 288},
         {22,  DISPLAYINFO_EDID_TSN_576IH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1440, 288},
         {23,  DISPLAYINFO_EDID_TSN_288P,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 1440, 288},
         {24,  DISPLAYINFO_EDID_TSN_288PH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1440, 288},
         {25,  DISPLAYINFO_EDID_TSN_576I4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 2880, 288},
         {26,  DISPLAYINFO_EDID_TSN_576I4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 2880, 288},
         {27,  DISPLAYINFO_EDID_TSN_288P4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 2880, 288},
         {28,  DISPLAYINFO_EDID_TSN_288P4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 2880, 288},
         {29,  DISPLAYINFO_EDID_TSN_576P2X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 1440, 576},
         {30,  DISPLAYINFO_EDID_TSN_576P2XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1440, 576},
         {31,  DISPLAYINFO_EDID_TSN_1080P50,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1920, 1080},
         {32,  DISPLAYINFO_EDID_TSN_1080P24,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_23980_OR_24000, 1920, 1080},
         {33,  DISPLAYINFO_EDID_TSN_1080P25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_25000, 1920, 1080},
         {34,  DISPLAYINFO_EDID_TSN_1080P30,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_29970_OR_30000, 1920, 1080},
         {35,  DISPLAYINFO_EDID_TSN_480P4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_59940, 2880, 240},
         {36,  DISPLAYINFO_EDID_TSN_480P4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_59940, 2880, 240},
         {37,  DISPLAYINFO_EDID_TSN_576P4X,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_50000, 2880, 576},
         {38,  DISPLAYINFO_EDID_TSN_576P4XH,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 2880, 576},
         {39,  DISPLAYINFO_EDID_TSN_1080I25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 1920, 540},
         {40,  DISPLAYINFO_EDID_TSN_1080I50,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 1920, 540},
         {41,  DISPLAYINFO_EDID_TSN_720P100,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 1280, 720},
         {42,  DISPLAYINFO_EDID_TSN_576P100,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_100000, 720, 576},
         {43,  DISPLAYINFO_EDID_TSN_576P100H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 720, 576},
         {44,  DISPLAYINFO_EDID_TSN_576I50,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_100000, 1440, 576},
         {45,  DISPLAYINFO_EDID_TSN_576I50H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 1440, 576},
         {46,  DISPLAYINFO_EDID_TSN_1080I60,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1920, 540},
         {47,  DISPLAYINFO_EDID_TSN_720P120,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1280, 720},
         {48,  DISPLAYINFO_EDID_TSN_480P119,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_119880_OR_120000, 720, 576},
         {49,  DISPLAYINFO_EDID_TSN_480P119H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 720, 576},
         {50,  DISPLAYINFO_EDID_TSN_480I59,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1440, 576},
         {51,  DISPLAYINFO_EDID_TSN_480I59H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1440, 576},
         {52,  DISPLAYINFO_EDID_TSN_576P200,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_200000, 720, 576},
         {53,  DISPLAYINFO_EDID_TSN_576P200H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_200000, 720, 576},
         {54,  DISPLAYINFO_EDID_TSN_576I100,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_200000, 1440, 288},
         {55,  DISPLAYINFO_EDID_TSN_576I100H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_200000, 1440, 288},
         {56,  DISPLAYINFO_EDID_TSN_480P239,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_239760, 720, 480},
         {57,  DISPLAYINFO_EDID_TSN_480P239H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_239760, 720, 480},
         {58,  DISPLAYINFO_EDID_TSN_480I119,  DISPLAYINFO_EDID_DAR_4_TO_3,  DISPLAYINFO_EDID_VF_239760, 1440, 240},
         {59,  DISPLAYINFO_EDID_TSN_480I119H,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_239760, 1440, 240},
         {60,  DISPLAYINFO_EDID_TSN_720P24,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_23980_OR_24000, 1280, 720},
         {61,  DISPLAYINFO_EDID_TSN_720P25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_25000, 1280, 720},
         {62,  DISPLAYINFO_EDID_TSN_720P30,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_29970_OR_30000, 1280, 720},
         {63,  DISPLAYINFO_EDID_TSN_1080P120,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1920, 1080},
         {64,  DISPLAYINFO_EDID_TSN_1080P100,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 1920, 1080},
         {65,  DISPLAYINFO_EDID_TSN_720P24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 1280, 720},
         {66,  DISPLAYINFO_EDID_TSN_720P25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 1280, 720},
         {67,  DISPLAYINFO_EDID_TSN_720P30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 1280, 720},
         {68,  DISPLAYINFO_EDID_TSN_720P50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 1280, 720},
         {69,  DISPLAYINFO_EDID_TSN_720P,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 1650, 750},
         {70,  DISPLAYINFO_EDID_TSN_720P100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 1280, 720},
         {71,  DISPLAYINFO_EDID_TSN_720P120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1280, 720},
         {72,  DISPLAYINFO_EDID_TSN_1080P24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 1920, 1080},
         {73,  DISPLAYINFO_EDID_TSN_1080P25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 1920, 1080},
         {74,  DISPLAYINFO_EDID_TSN_1080P30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 1920, 1080},
         {75,  DISPLAYINFO_EDID_TSN_1080P50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 1920, 1080},
         {76,  DISPLAYINFO_EDID_TSN_1080P,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 1920, 1080},
         {77,  DISPLAYINFO_EDID_TSN_1080P100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 1920, 1080},
         {78,  DISPLAYINFO_EDID_TSN_1080P120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1920, 1080},
         {79,  DISPLAYINFO_EDID_TSN_720P2X24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 1680, 720},
         {80,  DISPLAYINFO_EDID_TSN_720P2X25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 1680, 720},
         {81,  DISPLAYINFO_EDID_TSN_720P2X30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 1680, 720},
         {82,  DISPLAYINFO_EDID_TSN_720P2X50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 1680, 720},
         {83,  DISPLAYINFO_EDID_TSN_720P2X,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 1680, 720},
         {84,  DISPLAYINFO_EDID_TSN_720P2X100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 1680, 720},
         {85,  DISPLAYINFO_EDID_TSN_720P2X120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 1680, 720},
         {86,  DISPLAYINFO_EDID_TSN_1080P2X24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 2560, 1080},
         {87,  DISPLAYINFO_EDID_TSN_1080P2X25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 2560, 1080},
         {88,  DISPLAYINFO_EDID_TSN_1080P2X30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 2560, 1080},
         {89,  DISPLAYINFO_EDID_TSN_1080P2X50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 2560, 1080},
         {90,  DISPLAYINFO_EDID_TSN_1080P2X,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 2560, 1080},
         {91,  DISPLAYINFO_EDID_TSN_1080P2X100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 2560, 1080},
         {92,  DISPLAYINFO_EDID_TSN_1080P2X120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 2560, 1080},
         {93,  DISPLAYINFO_EDID_TSN_2160P24,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_23980_OR_24000, 3840, 2160},
         {94,  DISPLAYINFO_EDID_TSN_2160P25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_25000, 3840, 2160},
         {95,  DISPLAYINFO_EDID_TSN_2160P30,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_29970_OR_30000, 3840, 2160},
         {96,  DISPLAYINFO_EDID_TSN_2160P50,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 3840, 2160},
         {97,  DISPLAYINFO_EDID_TSN_2160P60,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 3840, 2160},
         {98,  DISPLAYINFO_EDID_TSN_2160P24,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_23980_OR_24000, 4096, 2160},
         {99,  DISPLAYINFO_EDID_TSN_2160P25,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_25000, 4096, 2160},
         {100,  DISPLAYINFO_EDID_TSN_2160P30,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_29970_OR_30000, 4096, 2160},
         {101,  DISPLAYINFO_EDID_TSN_2160P50,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_50000, 4096, 2160},
         {102,  DISPLAYINFO_EDID_TSN_2160P,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_60000, 4096, 2160},
         {103,  DISPLAYINFO_EDID_TSN_2160P24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 3840, 2160},
         {104,  DISPLAYINFO_EDID_TSN_2160P25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 3840, 2160},
         {105,  DISPLAYINFO_EDID_TSN_2160P30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 3840, 2160},
         {106,  DISPLAYINFO_EDID_TSN_2160P50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 3840, 2160},
         {107,  DISPLAYINFO_EDID_TSN_2160P,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 3840, 2160},
         {108,  DISPLAYINFO_EDID_TSN_720P48,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_47960_OR_48000, 1280, 720},
         {109,  DISPLAYINFO_EDID_TSN_720P48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 1280, 720},
         {110,  DISPLAYINFO_EDID_TSN_720P2X48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 1680, 720},
         {111,  DISPLAYINFO_EDID_TSN_1080P48,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_47960_OR_48000, 1920, 1080},
         {112,  DISPLAYINFO_EDID_TSN_1080P48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 1920, 1080},
         {113,  DISPLAYINFO_EDID_TSN_1080P2X48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 2560, 1080},
         {114,  DISPLAYINFO_EDID_TSN_2160P48,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_47960_OR_48000, 3840, 2160},
         {115,  DISPLAYINFO_EDID_TSN_2160P48,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_47960_OR_48000, 4096, 2160},
         {116,  DISPLAYINFO_EDID_TSN_2160P48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 3840, 2160},
         {117,  DISPLAYINFO_EDID_TSN_2160P100,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 3840, 2160},
         {118,  DISPLAYINFO_EDID_TSN_2160P120,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 3840, 2160},
         {119,  DISPLAYINFO_EDID_TSN_2160P100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 3840, 2160},
         {120,  DISPLAYINFO_EDID_TSN_2160P120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 3840, 2160},
         {121,  DISPLAYINFO_EDID_TSN_2160P2X24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 5120, 2160},
         {122,  DISPLAYINFO_EDID_TSN_2160P2X25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 5120, 2160},
         {123,  DISPLAYINFO_EDID_TSN_2160P2X30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 5120, 2160},
         {124,  DISPLAYINFO_EDID_TSN_2160P2X48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 5120, 2160},
         {125,  DISPLAYINFO_EDID_TSN_2160P2X50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 5120, 2160},
         {126,  DISPLAYINFO_EDID_TSN_2160P2X,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 5120, 2160},
         {127,  DISPLAYINFO_EDID_TSN_2160P2X100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 5120, 2160},
         {193,  DISPLAYINFO_EDID_TSN_2160P2X120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 5120, 2160},
         {194,  DISPLAYINFO_EDID_TSN_4320P24,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_23980_OR_24000, 7680, 4320},
         {195,  DISPLAYINFO_EDID_TSN_4320P25,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_25000, 7680, 4320},
         {196,  DISPLAYINFO_EDID_TSN_4320P30,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_29970_OR_30000, 7680, 4320},
         {197,  DISPLAYINFO_EDID_TSN_4320P48,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_47960_OR_48000, 7680, 4320},
         {198,  DISPLAYINFO_EDID_TSN_4320P50,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_50000, 7680, 4320},
         {199,  DISPLAYINFO_EDID_TSN_4320P,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_60000, 7680, 4320},
         {200,  DISPLAYINFO_EDID_TSN_4320P100,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_100000, 7680, 4320},
         {201,  DISPLAYINFO_EDID_TSN_4320P120,  DISPLAYINFO_EDID_DAR_16_TO_9,  DISPLAYINFO_EDID_VF_119880_OR_120000, 7680, 4320},
         {202,  DISPLAYINFO_EDID_TSN_4320P24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 7680, 4320},
         {203,  DISPLAYINFO_EDID_TSN_4320P25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 7680, 4320},
         {204,  DISPLAYINFO_EDID_TSN_4320P30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 7680, 4320},
         {205,  DISPLAYINFO_EDID_TSN_4320P48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 7680, 4320},
         {206,  DISPLAYINFO_EDID_TSN_4320P50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 7680, 4320},
         {207,  DISPLAYINFO_EDID_TSN_4320P,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 7680, 4320},
         {208,  DISPLAYINFO_EDID_TSN_4320P100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 7680, 4320},
         {209,  DISPLAYINFO_EDID_TSN_4320P120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 7680, 4320},
         {210,  DISPLAYINFO_EDID_TSN_4320P2X24,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_23980_OR_24000, 10240, 4320},
         {211,  DISPLAYINFO_EDID_TSN_4320P2X25,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_25000, 10240, 4320},
         {212,  DISPLAYINFO_EDID_TSN_4320P2X30,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_29970_OR_30000, 10240, 4320},
         {213,  DISPLAYINFO_EDID_TSN_4320P2X48,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_47960_OR_48000, 10240, 4320},
         {214,  DISPLAYINFO_EDID_TSN_4320P2X50,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_50000, 10240, 4320},
         {215,  DISPLAYINFO_EDID_TSN_4320P2X,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_60000, 10240, 4320},
         {216,  DISPLAYINFO_EDID_TSN_4320P2X100,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_100000, 10240, 4320},
         {217,  DISPLAYINFO_EDID_TSN_4320P2X120,  DISPLAYINFO_EDID_DAR_64_TO_27,  DISPLAYINFO_EDID_VF_119880_OR_120000, 10240, 4320},
         {218,  DISPLAYINFO_EDID_TSN_2160P100,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_100000, 4096, 2160},
         {219,  DISPLAYINFO_EDID_TSN_2160P120,  DISPLAYINFO_EDID_DAR_256_TO_135,  DISPLAYINFO_EDID_VF_119880_OR_120000, 4096, 2160},
    };

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
        memcpy((edid.Segment(0)), buffer, edid.Length());

        if(edid.IsValid()) {
            memcpy(edid_info->manufacturer_id, edid.Manufacturer().c_str(), sizeof(edid_info->manufacturer_id));
            edid_info->product_code = edid.ProductCode();
            edid_info->serial_number = edid.Serial();
            edid_info->manufacture_week = edid.Week();
            edid_info->manufacture_year = edid.Year();
            edid_info->version = edid.Major();
            edid_info->revision = edid.Minor();
            edid_info->digital = edid.Digital();
            edid_info->bits_per_color = edid.BitsPerColor();
            edid_info->video_interface = edid.VideoInterface();
            edid_info->supported_digital_display_types = edid.SupportedDigitalDisplayTypes();
            edid_info->width_in_centimeters = edid.WidthInCentimeters();
            edid_info->height_in_centimeters = edid.HeightInCentimeters();
            edid_info->preferred_width_in_pixels = edid.PreferredWidthInPixels();
            edid_info->preferred_height_in_pixels = edid.PreferredHeightInPixels();
            errorCode = Core::ERROR_NONE;
        }
    }
    return errorCode;
}

uint32_t displayinfo_edid_cea_extension_info(const uint8_t buffer[], const uint16_t length, displayinfo_edid_cea_extension_t *cea_info)
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
                Plugin::ExtendedDisplayIdentification::CEA cae(segment.Current());
                cea_info->version = cae.Version();
                cea_info->supported_color_depths = cae.SupportedColorDepths();
                cea_info->supported_color_format = cae.SupportedColorFormat();
                cea_info->supported_color_formats = cae.SupportedColorFormats();
                cea_info->supported_color_spaces = cae.SupportedColorSpaces();
                cea_info->supported_audio_formats = cae.SupportedAudioFormats();
                std::vector<uint8_t> vic_list;
                cae.SupportedTimings(vic_list);
                for (uint8_t i = 0; i < vic_list.size(); ++i) {
                    cea_info->supported_timings[i] = vic_list[i];
                }
                cea_info->number_of_supported_timings = vic_list.size();
                errorCode = Core::ERROR_NONE;
            } else {
                errorCode = Core::ERROR_UNAVAILABLE;
            }
        }
    }
    return errorCode;
}

uint32_t displayinfo_edid_vic_to_standard_timing(uint8_t vic, displayinfo_edid_standardtiming_t* result)
{
    uint32_t errorCode = Core::ERROR_UNAVAILABLE;
    if (result != nullptr) {
        for(uint8_t index = 0; index < (sizeof(standardTimingMap)/sizeof(displayinfo_edid_standardtiming_t)); ++index) {
            if(standardTimingMap[index].vic == vic) {
                memcpy(result, &standardTimingMap[index], sizeof(displayinfo_edid_standardtiming_t));
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
