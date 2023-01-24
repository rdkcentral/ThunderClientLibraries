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
#include <vector>

#include "deviceinfo.h"
#include <interfaces/IDeviceInfo.h>

using namespace WPEFramework;
namespace {



deviceinfo_hdcp_type Convert(const Exchange::IDeviceVideoCapabilities::CopyProtection from)
{
    static struct LUT {
        Exchange::IDeviceVideoCapabilities::CopyProtection lhs;
        deviceinfo_hdcp_type rhs;
    } lut[] = {
        { Exchange::IDeviceVideoCapabilities::CopyProtection::HDCP_UNAVAILABLE, DEVICEINFO_HDCP_UNAVAILABLE },
        { Exchange::IDeviceVideoCapabilities::CopyProtection::HDCP_14, DEVICEINFO_HDCP_14 },
        { Exchange::IDeviceVideoCapabilities::CopyProtection::HDCP_20, DEVICEINFO_HDCP_20 },
        { Exchange::IDeviceVideoCapabilities::CopyProtection::HDCP_21, DEVICEINFO_HDCP_21 },
        { Exchange::IDeviceVideoCapabilities::CopyProtection::HDCP_22, DEVICEINFO_HDCP_22 },
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_HDCP_UNAVAILABLE);
}
deviceinfo_output_resolution_type Convert(const Exchange::IDeviceVideoCapabilities::ScreenResolution from)
{
    static struct LUT {
        Exchange::IDeviceVideoCapabilities::ScreenResolution lhs;
        deviceinfo_output_resolution_type rhs;
    } lut[] = {
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_Unknown, DEVICEINFO_RESOLUTION_UNKNOWN },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_480i, DEVICEINFO_RESOLUTION_480I },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_480p, DEVICEINFO_RESOLUTION_480P },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_720p, DEVICEINFO_RESOLUTION_720P },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_720p50Hz, DEVICEINFO_RESOLUTION_720P50HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_1080p24Hz, DEVICEINFO_RESOLUTION_1080P24HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_1080i50Hz, DEVICEINFO_RESOLUTION_1080I50HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_1080p50Hz, DEVICEINFO_RESOLUTION_1080P50HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_1080p60Hz, DEVICEINFO_RESOLUTION_1080P60HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_2160p50Hz, DEVICEINFO_RESOLUTION_2160P50HZ },
        { Exchange::IDeviceVideoCapabilities::ScreenResolution_2160p60Hz, DEVICEINFO_RESOLUTION_2160P60HZ },
    };

    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_RESOLUTION_480I);
}

static struct VideoOutLUT {
    Exchange::IDeviceVideoCapabilities::VideoOutput lhs;
    deviceinfo_video_output_type rhs;
} videoOutLUT[] = {
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_OTHER, DEVICEINFO_VIDEO_OTHER },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_COMPONENT, DEVICEINFO_VIDEO_COMPONENT },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_COMPOSITE, DEVICEINFO_VIDEO_COMPOSITE },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_DISPLAYPORT, DEVICEINFO_VIDEO_DISPLAYPORT },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_HDMI0, DEVICEINFO_VIDEO_HDMI0 },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_HDMI1, DEVICEINFO_VIDEO_HDMI1 },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_RF_MODULATOR, DEVICEINFO_VIDEO_RF_MODULATOR },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_SCART_RGB, DEVICEINFO_VIDEO_SCART_RGB },
    { Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_SVIDEO, DEVICEINFO_VIDEO_SVIDEO }
};

deviceinfo_video_output_type Convert(const Exchange::IDeviceVideoCapabilities::VideoOutput from)
{
    uint8_t index = 0;
    while ((index < (sizeof(videoOutLUT) / sizeof(VideoOutLUT))) && (videoOutLUT[index].lhs != from))
        index++;

    return (index < (sizeof(videoOutLUT) / sizeof(VideoOutLUT)) ? videoOutLUT[index].rhs : DEVICEINFO_VIDEO_OTHER);
}

Exchange::IDeviceVideoCapabilities::VideoOutput Convert(const deviceinfo_video_output_type from)
{
    uint8_t index = 0;
    while ((index < (sizeof(videoOutLUT) / sizeof(VideoOutLUT))) && (videoOutLUT[index].rhs != from))
        index++;

    return (index < (sizeof(videoOutLUT) / sizeof(VideoOutLUT)) ? videoOutLUT[index].lhs : Exchange::IDeviceVideoCapabilities::VideoOutput::VIDEO_OTHER);
}

static struct AudioOutLUT {
    Exchange::IDeviceAudioCapabilities::AudioOutput lhs;
    deviceinfo_audio_output_type rhs;
} audioOutLUT[] = {
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_OTHER, DEVICEINFO_AUDIO_OTHER },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_ANALOG, DEVICEINFO_AUDIO_ANALOG },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_DISPLAYPORT, DEVICEINFO_AUDIO_DISPLAYPORT },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_HDMI0, DEVICEINFO_AUDIO_HDMI0 },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_HDMI1, DEVICEINFO_AUDIO_HDMI1 },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_RF_MODULATOR, DEVICEINFO_AUDIO_RF_MODULATOR },
    { Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_SPDIF, DEVICEINFO_AUDIO_SPDIF },
};

deviceinfo_audio_output_type Convert(const Exchange::IDeviceAudioCapabilities::AudioOutput from)
{
    uint8_t index = 0;
    while ((index < (sizeof(audioOutLUT) / sizeof(AudioOutLUT))) && (audioOutLUT[index].lhs != from))
        index++;

    return (index < (sizeof(audioOutLUT) / sizeof(AudioOutLUT)) ? audioOutLUT[index].rhs : DEVICEINFO_AUDIO_OTHER);
}

Exchange::IDeviceAudioCapabilities::AudioOutput Convert(const deviceinfo_audio_output_type from)
{
    uint8_t index = 0;
    while ((index < (sizeof(audioOutLUT) / sizeof(AudioOutLUT))) && (audioOutLUT[index].rhs != from))
        index++;

    return (index < (sizeof(audioOutLUT) / sizeof(AudioOutLUT)) ? audioOutLUT[index].lhs : Exchange::IDeviceAudioCapabilities::AudioOutput::AUDIO_OTHER);
}

deviceinfo_audio_capability_type Convert(const Exchange::IDeviceAudioCapabilities::AudioCapability from)
{
    static struct LUT {
        Exchange::IDeviceAudioCapabilities::AudioCapability lhs;
        deviceinfo_audio_capability_type rhs;
    } lut[] = {
        { Exchange::IDeviceAudioCapabilities::AudioCapability::AUDIOCAPABILITY_NONE, DEVICEINFO_AUDIO_CAPABILITY_NONE },
        { Exchange::IDeviceAudioCapabilities::AudioCapability::ATMOS, DEVICEINFO_AUDIO_CAPABILITY_ATMOS},
        { Exchange::IDeviceAudioCapabilities::AudioCapability::DD, DEVICEINFO_AUDIO_CAPABILITY_DD},
        { Exchange::IDeviceAudioCapabilities::AudioCapability::DDPLUS, DEVICEINFO_AUDIO_CAPABILITY_DDPLUS},
        { Exchange::IDeviceAudioCapabilities::AudioCapability::DAD, DEVICEINFO_AUDIO_CAPABILITY_DAD},
        { Exchange::IDeviceAudioCapabilities::AudioCapability::DAPV2, DEVICEINFO_AUDIO_CAPABILITY_DAPV2},
        { Exchange::IDeviceAudioCapabilities::AudioCapability::MS12, DEVICEINFO_AUDIO_CAPABILITY_MS12}
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_AUDIO_CAPABILITY_NONE);
}

deviceinfo_audio_ms12_capability_type Convert(const Exchange::IDeviceAudioCapabilities::MS12Capability from)
{
    static struct LUT {
        Exchange::IDeviceAudioCapabilities::MS12Capability lhs;
        deviceinfo_audio_ms12_capability_type rhs;
    } lut[] = {
        { Exchange::IDeviceAudioCapabilities::MS12Capability::MS12CAPABILITY_NONE, DEVICEINFO_AUDIO_MS12_CAPABILITY_NONE },
        { Exchange::IDeviceAudioCapabilities::MS12Capability::DOLBYVOLUME, DEVICEINFO_AUDIO_MS12_CAPABILITY_DOLBYVOLUME},
        { Exchange::IDeviceAudioCapabilities::MS12Capability::INTELIGENTEQUALIZER, DEVICEINFO_AUDIO_MS12_CAPABILITY_INTELIGENTEQUALIZER},
        { Exchange::IDeviceAudioCapabilities::MS12Capability::DIALOGUEENHANCER, DEVICEINFO_AUDIO_MS12_CAPABILITY_DIALOGUEENHANCER},
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_AUDIO_MS12_CAPABILITY_NONE);
}

deviceinfo_audio_ms12_profile_type Convert(const Exchange::IDeviceAudioCapabilities::MS12Profile from)
{
    static struct LUT {
        Exchange::IDeviceAudioCapabilities::MS12Profile lhs;
        deviceinfo_audio_ms12_profile_type rhs;
    } lut[] = {
        { Exchange::IDeviceAudioCapabilities::MS12Profile::MS12PROFILE_NONE, DEVICEINFO_AUDIO_MS12_PROFILE_NONE},
        { Exchange::IDeviceAudioCapabilities::MS12Profile::MUSIC, DEVICEINFO_AUDIO_MS12_PROFILE_MUSIC},
        { Exchange::IDeviceAudioCapabilities::MS12Profile::MOVIE, DEVICEINFO_AUDIO_MS12_PROFILE_MOVIE},
        { Exchange::IDeviceAudioCapabilities::MS12Profile::VOICE, DEVICEINFO_AUDIO_MS12_PROFILE_VOICE}
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_AUDIO_MS12_PROFILE_NONE);
}

static string Callsign()
{
    static constexpr const TCHAR Default[] = _T("DeviceInfo");
    return (Default);
}

class DeviceInfoLink : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceInfo> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceInfo>;
    struct AudioOutputCapability {
        deviceinfo_audio_output_t type;
        std::vector<deviceinfo_audio_capability_t> audioCapabilities;
        std::vector<deviceinfo_audio_ms12_capability_t> ms12Capabilities;
        std::vector<deviceinfo_audio_ms12_profile_t> ms12AuioProfiles;
    };

    struct VideoOutputCapability {
        deviceinfo_video_output_t type;
        Core::OptionalType<deviceinfo_hdcp_t> hdcp;
        std::vector<deviceinfo_output_resolution_t> resolutions;
        Core::OptionalType<deviceinfo_output_resolution_t> defaultResolution;
        Core::OptionalType<deviceinfo_output_resolution_t> maxScreenResolution;
    };
    using AudioOutputMap = std::map<Exchange::IDeviceAudioCapabilities::AudioOutput, AudioOutputCapability>;
    using VideoOutputMap = std::map<Exchange::IDeviceVideoCapabilities::VideoOutput, VideoOutputCapability>;

    DeviceInfoLink()
        : BaseClass()
        , _lock()
        , _subsysInterface(nullptr)
        , _identifierInterface(nullptr)
        , _deviceAudioCapabilitiesInterface(nullptr)
        , _deviceVideoCapabilitiesInterface(nullptr)
        , _deviceInfoInterface(nullptr)
        , _architecture()
        , _chipsetName()
        , _serialNumber()
        , _sku()
        , _make()
        , _deviceType()
        , _modelName()
        , _modelYear()
        , _systemIntegraterName()
        , _friendlyName()
        , _platformName()
        , _firmwareVersion()
        , _idStr()
        , _hostEdid()
        , _videoOutputMap()
        , _audioOutputMap()
        , _id()
        , _hdr_atmos_cec(0)
    {
        ASSERT(_singleton==nullptr);
        
        _singleton = this;
        
        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign());

        PluginHost::IShell* ControllerInterface = BaseClass::ControllerInterface();

        if (ControllerInterface!= nullptr) {

            _subsysInterface = ControllerInterface->SubSystems();

            if (_subsysInterface != nullptr) {
                _identifierInterface = _subsysInterface->Get<PluginHost::ISubSystem::IIdentifier>();
            }
        }
    }

public:
    DeviceInfoLink(const DeviceInfoLink&) = delete;
    DeviceInfoLink& operator=(const DeviceInfoLink&) = delete;
    ~DeviceInfoLink() override
    {
        _lock.Lock();

        if (_subsysInterface != nullptr) {
            _subsysInterface->Release();
            _subsysInterface = nullptr;
        }
        if (_identifierInterface != nullptr) {
            _identifierInterface->Release();
            _identifierInterface = nullptr;
        }

        _lock.Unlock();

        BaseClass::Close(WPEFramework::Core::infinite);
        _singleton = nullptr;
    }

    static DeviceInfoLink& Instance()
    {
        static DeviceInfoLink *instance = new DeviceInfoLink;
        ASSERT(instance!=nullptr);
        return *instance;
    }

    static void Dispose()
    {
        ASSERT(_singleton != nullptr);

        if (_singleton != nullptr) {
            delete _singleton;
        }
    }

private:
    void Operational(const bool upAndRunning) override
    {
        _lock.Lock();

        if (upAndRunning) {
            if (_deviceInfoInterface == nullptr) {
                _deviceInfoInterface = BaseClass::Interface();
            }
            if (_deviceInfoInterface != nullptr) {
                _deviceAudioCapabilitiesInterface = _deviceInfoInterface->QueryInterface<Exchange::IDeviceAudioCapabilities>();
                if (_deviceAudioCapabilitiesInterface != nullptr) {
                    _deviceVideoCapabilitiesInterface = _deviceInfoInterface->QueryInterface<Exchange::IDeviceVideoCapabilities>();
                }
            }
            
        } else {
            if (_deviceAudioCapabilitiesInterface != nullptr) {
                _deviceAudioCapabilitiesInterface->Release();
                _deviceAudioCapabilitiesInterface = nullptr;
            }
            if (_deviceVideoCapabilitiesInterface != nullptr) {
                _deviceVideoCapabilitiesInterface->Release();
                _deviceVideoCapabilitiesInterface = nullptr;
            }
            if (_deviceInfoInterface != nullptr) {
                _deviceInfoInterface->Release();
                _deviceInfoInterface = nullptr;
            }
        }

        _lock.Unlock();
    }

    /*
    * hdr_atmos_cec variable stores value in this format
    * ZX1X2X3 ZY1Y2Y3
    * Z - Unused
    * X1 - hdr value
    * X2 - atmos value
    * X3 - cec value
    * Y1 - hdr cached
    * Y2 - atmos cached
    * Y3 - cec cached
    */
    void setHdrSupport(const bool hdr)
    {
        _hdr_atmos_cec |= hdr << 6;
        _hdr_atmos_cec |= 0x04;
    }

    void setAtmosSupport(const bool atmos)
    {
        _hdr_atmos_cec |= atmos << 5;
        _hdr_atmos_cec |= 0x02;
    }

    void setCecSupport(const bool cec)
    {
        _hdr_atmos_cec |= cec << 4;
        _hdr_atmos_cec |= 0x01;
    }

    bool getCecSupport() const
    {
        return _hdr_atmos_cec & 0x10;
    }

    bool getAtmosSupport() const
    {
        return _hdr_atmos_cec & 0x20;
    }

    bool getHDRSupport() const
    {
        return _hdr_atmos_cec & 0x40;
    }

    bool isCecSupportCached() const
    {
        return _hdr_atmos_cec & 0x01;
    }

    bool isAtmosSupportCached() const
    {
        return _hdr_atmos_cec & 0x02;
    }

    bool isHDRSupportCached() const
    {
        return _hdr_atmos_cec & 0x04;
    }

    public:
    uint32_t Deviceinfo_serial_number(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string serialNumber;
        _lock.Lock();
        if (_modelName.IsSet() == true) {
            serialNumber = _serialNumber.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->SerialNumber(serialNumber);
                if (result == Core::ERROR_NONE) {
                    _serialNumber = serialNumber;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            auto size = serialNumber.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            } else {
                strncpy(buffer, serialNumber.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_sku(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string sku;

        _lock.Lock();
        if (_sku.IsSet() == true) {
            sku = _sku.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->Sku(sku);
                if (result == Core::ERROR_NONE) {
                    _sku = sku;
                }
            }
        }
        _lock.Unlock();

        if (result == Core::ERROR_NONE) {
            auto size = sku.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            } else {
                strncpy(buffer, sku.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_make(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string make;
        _lock.Lock();
        if (_make.IsSet() == true) {
            make = _make.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr ) {
                result = _deviceInfoInterface->Make(make);
                if (result == Core::ERROR_NONE) {
                    _make = make;
                }
            }
        }

        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            auto size = make.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, make.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_device_type(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string deviceType;
        _lock.Lock();
        if (_deviceType.IsSet() == true) {
            deviceType = _deviceType.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->DeviceType(deviceType);
                if (result == Core::ERROR_NONE) {
                    _deviceType = deviceType;
                }
            }
        }
        _lock.Unlock();

        if (result == Core::ERROR_NONE) {
            auto size = deviceType.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            } else {
                strncpy(buffer, deviceType.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string modelName;
        _lock.Lock();
        if (_modelName.IsSet() == true) {
            modelName = _modelName.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->ModelName(modelName);
                if (result == Core::ERROR_NONE) {
                    _modelName = modelName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            auto size = modelName.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, modelName.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;

        uint16_t modelYear = 0;
        _lock.Lock();
        if (_modelYear.IsSet() == true) {
            modelYear = _modelYear.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->ModelYear(modelYear);
                if (result == Core::ERROR_NONE) {
                    _modelYear = modelYear;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            string year = Core::ToString(modelYear);
            auto size = year.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, year.c_str(), *length);
            }

            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string integratorName;
        _lock.Lock();
        if (_systemIntegraterName.IsSet() == true) {
            integratorName = _systemIntegraterName.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->DistributorId(integratorName);
                if (result == Core::ERROR_NONE) {
                    _systemIntegraterName = integratorName;
                }
            }
        }
        _lock.Unlock();

        if (result == Core::ERROR_NONE) {
            auto size = integratorName.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            } else {
                strncpy(buffer, integratorName.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string friendlyName;
        _lock.Lock();
        if (_friendlyName.IsSet() == true) {
            friendlyName = _friendlyName.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->FriendlyName(friendlyName);
                if (result == Core::ERROR_NONE) {
                    _friendlyName = friendlyName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            string year = Core::ToString(friendlyName);
            auto size = friendlyName.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            } else {
                strncpy(buffer, friendlyName.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string platformName;
        _lock.Lock();
        if (_platformName.IsSet() == true) {
            platformName = _platformName.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceInfoInterface != nullptr) {
                result = _deviceInfoInterface->PlatformName(platformName);
                if (result == Core::ERROR_NONE) {
                    _platformName = platformName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            string year = Core::ToString(platformName) ;
            auto size = platformName.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, platformName.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0; 
        }
        return result;
    }

    uint32_t Deviceinfo_architecture(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        std::string newValue;
        _lock.Lock();
        if (_architecture.IsSet() == true) {
            newValue = _architecture;
            result = Core::ERROR_NONE;
        }
        else {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->Architecture());
                _architecture = newValue;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            auto size = newValue.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, newValue.c_str(), *length);
                result = Core::ERROR_NONE;
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        std::string newValue;
        _lock.Lock();
        if (_chipsetName.IsSet() == true) {
            newValue = _chipsetName.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->Chipset());
                _chipsetName = newValue;
                result = Core::ERROR_NONE;
            }
        }

        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            auto size = newValue.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, newValue.c_str(), *length);
                result = Core::ERROR_NONE;
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        std::string newValue;
        _lock.Lock();
        if (_firmwareVersion.IsSet() == true) {
            newValue = _firmwareVersion.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->FirmwareVersion());
                _firmwareVersion = newValue;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            auto size = newValue.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, newValue.c_str(), *length);
                result = Core::ERROR_NONE;
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        if (_id.size() > 0) {
            result = Core::ERROR_NONE;
        }
        else {
            if (_identifierInterface != nullptr) {
                uint8_t tempBuffer[255] = {'\0'};
                uint8_t size = _identifierInterface->Identifier(sizeof (tempBuffer) - 1, tempBuffer);
                std::copy(tempBuffer, tempBuffer + size, std::back_inserter(_id));
                _id.shrink_to_fit();
                result = Core::ERROR_NONE;
            }
        }
        if (result == Core::ERROR_NONE) {
            uint8_t size = static_cast<uint8_t>(_id.size());
            *length = ((size > (*length) - 1) ? (*length) - 1 : size);
            std::copy_n(_id.begin(), *length, buffer);
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string id;
        _lock.Lock();
        if ( _idStr.IsSet() == true) {
            id = _idStr;
            result = Core::ERROR_NONE;
        }
        else {
            if (_identifierInterface != nullptr) {
                uint8_t id_buffer[64] = {};
                id_buffer[0] = _identifierInterface->Identifier(sizeof(id_buffer) - 1, &(id_buffer[1]));
                id = Core::SystemInfo::Instance().Id(id_buffer, ~0);
                _idStr = id;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            if (id.size() < *length) { 
                strncpy(buffer, id.c_str(), *length);
                result = Core::ERROR_NONE;
            } else {
                result = Core::ERROR_INVALID_INPUT_LENGTH;
            }
            *length = static_cast<uint8_t>(id.size() + 1);

        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        if (_audioOutputMap.size() > 0) {
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceAudioCapabilitiesInterface != nullptr) {
                Exchange::IDeviceAudioCapabilities::IAudioOutputIterator* index = nullptr;

                _deviceAudioCapabilitiesInterface->AudioOutputs(index);
                if (index != nullptr) {

                    Exchange::IDeviceAudioCapabilities::AudioOutput field;
                    AudioOutputCapability audioOutputCapability;
                    while (index->Next(field) == true) {
                        audioOutputCapability.type = Convert(field);
                        _audioOutputMap.insert(std::pair<Exchange::IDeviceAudioCapabilities::AudioOutput, AudioOutputCapability>(field, audioOutputCapability));
                    }
                    index->Release();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            AudioOutputMap::iterator iter = _audioOutputMap.begin();

            while ((inserted < *length) && iter != _audioOutputMap.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != iter->second.type)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = iter->second.type;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_audio_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_capability_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceAudioCapabilities::AudioOutput audioPort = Convert(audioOutput);
        AudioOutputMap::iterator index = _audioOutputMap.find(audioPort);
        if ((index != _audioOutputMap.end()) && (index->second.audioCapabilities.size() > 0)) {
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceAudioCapabilitiesInterface != nullptr) && (index != _audioOutputMap.end())) {
                 Exchange::IDeviceAudioCapabilities::IAudioCapabilityIterator* capabilities = nullptr;

                _deviceAudioCapabilitiesInterface->AudioCapabilities(audioPort, capabilities);
                if (capabilities != nullptr) {
                    Exchange::IDeviceAudioCapabilities::AudioCapability field;
                    while (capabilities->Next(field) == true) {
                        deviceinfo_audio_capability_type converted = Convert(field);
                        index->second.audioCapabilities.push_back(converted);
                    }
                    capabilities->Release();
                    index->second.audioCapabilities.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            std::vector<deviceinfo_audio_capability_t>::iterator iter = index->second.audioCapabilities.begin();

            while ((inserted < *length) && iter != index->second.audioCapabilities.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != *iter)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = *iter;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_audio_ms12_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_capability_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceAudioCapabilities::AudioOutput audioPort = Convert(audioOutput);
        AudioOutputMap::iterator index = _audioOutputMap.find(audioPort);
        if ((index != _audioOutputMap.end()) && (index->second.ms12Capabilities.size() > 0)) {
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceAudioCapabilitiesInterface != nullptr) && (index != _audioOutputMap.end())) {
                Exchange::IDeviceAudioCapabilities::IMS12CapabilityIterator* capabilities = nullptr;

                _deviceAudioCapabilitiesInterface->MS12Capabilities(audioPort, capabilities);
                if (capabilities != nullptr) {
                    Exchange::IDeviceAudioCapabilities::MS12Capability field;
                    while (capabilities->Next(field) == true) {
                        deviceinfo_audio_ms12_capability_type converted = Convert(field);
                        index->second.ms12Capabilities.push_back(converted);
                    }
                    capabilities->Release();
                    index->second.ms12Capabilities.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            std::vector<deviceinfo_audio_ms12_capability_t>::iterator iter = index->second.ms12Capabilities.begin();

            while ((inserted < *length) && iter != index->second.ms12Capabilities.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != *iter)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = *iter;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_audio_ms12_audio_profiles(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_profile_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceAudioCapabilities::AudioOutput audioPort = Convert(audioOutput);
        AudioOutputMap::iterator index = _audioOutputMap.find(audioPort);
        if ((index != _audioOutputMap.end()) && (index->second.ms12AuioProfiles.size() > 0)) {
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceAudioCapabilitiesInterface != nullptr) && (index != _audioOutputMap.end())) {
                Exchange::IDeviceAudioCapabilities::IMS12ProfileIterator* profiles = nullptr;

                _deviceAudioCapabilitiesInterface->MS12AudioProfiles(audioPort, profiles);
                if (profiles != nullptr) {
                    Exchange::IDeviceAudioCapabilities::MS12Profile profile;
                    while (profiles->Next(profile) == true) {
                        deviceinfo_audio_ms12_profile_type converted = Convert(profile);
                        index->second.ms12AuioProfiles.push_back(converted);
                    }
                    profiles->Release();
                    index->second.ms12AuioProfiles.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            std::vector<deviceinfo_audio_ms12_profile_t>::iterator iter = index->second.ms12AuioProfiles.begin();

            while ((inserted < *length) && iter != index->second.ms12AuioProfiles.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != *iter)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = *iter;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();
        if (_videoOutputMap.size()>0) {
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceVideoCapabilitiesInterface != nullptr) {
                Exchange::IDeviceVideoCapabilities::IVideoOutputIterator* index = nullptr;

                _deviceVideoCapabilitiesInterface->VideoOutputs(index);
                if (index != nullptr) {
                    Exchange::IDeviceVideoCapabilities::VideoOutput field;
                    VideoOutputCapability videoOutputCapability;
                    while (index->Next(field) == true) {
                        videoOutputCapability.type = Convert(field);
                        _videoOutputMap.insert(std::pair<Exchange::IDeviceVideoCapabilities::VideoOutput, VideoOutputCapability>(field, videoOutputCapability));
                    }
                    index->Release();
                    //_videoOutputMap.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }

        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            VideoOutputMap::iterator iter = _videoOutputMap.begin();

            while ((inserted < *length) && iter != _videoOutputMap.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != iter->second.type)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = iter->second.type;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_output_resolutions(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceVideoCapabilities::VideoOutput videoPort = Convert(videoOutput);
        VideoOutputMap::iterator index = _videoOutputMap.find(videoPort);
        if ((index != _videoOutputMap.end()) && (index->second.resolutions.size() > 0)) {
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceVideoCapabilitiesInterface != nullptr) && (index != _videoOutputMap.end())) {
                Exchange::IDeviceVideoCapabilities::IScreenResolutionIterator* resolutions = nullptr;

                _deviceVideoCapabilitiesInterface->Resolutions(videoPort, resolutions);
                if (resolutions != nullptr) {
                    Exchange::IDeviceVideoCapabilities::ScreenResolution field;
                    while (resolutions->Next(field) == true) {
                        deviceinfo_output_resolution_type converted = Convert(field);
                        index->second.resolutions.push_back(converted);
                    }
                    resolutions->Release();
                    index->second.resolutions.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE) {

            uint8_t inserted = 0;
            std::vector<deviceinfo_output_resolution_t>::iterator iter = index->second.resolutions.begin();

            while ((inserted < *length) && iter != index->second.resolutions.end()) {
                uint8_t loop = 0;

                while ((loop < inserted) && (value[loop] != *iter)) {
                    loop++;
                }

                if (loop == inserted) {
                    value[inserted] = *iter;
                    inserted++;
                }
                iter++;
            }
            *length = inserted;
            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_default_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value)
    {
        ASSERT(value != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceVideoCapabilities::VideoOutput videoPort = Convert(videoOutput);
        VideoOutputMap::iterator index = _videoOutputMap.find(videoPort);
        if ((index != _videoOutputMap.end()) && (index->second.defaultResolution.IsSet() == true)) {
            *value = index->second.defaultResolution.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceVideoCapabilitiesInterface != nullptr) && (index != _videoOutputMap.end())) {
                Exchange::IDeviceVideoCapabilities::ScreenResolution resolution;

                _deviceVideoCapabilitiesInterface->DefaultResolution(videoPort, resolution);
                index->second.defaultResolution = Convert(resolution);
                *value = index->second.defaultResolution.Value();
                result = Core::ERROR_NONE;
            }
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_maximum_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value)
    {
        ASSERT(value != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        Exchange::IDeviceVideoCapabilities::VideoOutput videoPort = Convert(videoOutput);
        VideoOutputMap::iterator index = _videoOutputMap.find(videoPort);
        if ((index != _videoOutputMap.end()) && (index->second.maxScreenResolution.IsSet() == true)) {
            *value = index->second.maxScreenResolution.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceVideoCapabilitiesInterface != nullptr) && (index != _videoOutputMap.end())) {
                deviceinfo_output_resolution_type resolutions[32];
                uint8_t maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_type);

                result = Deviceinfo_output_resolutions(videoOutput, resolutions, &maxLength);

                if (result == Core::ERROR_NONE) {
                    if (maxLength == 0) {
                        result = Core::ERROR_INVALID_INPUT_LENGTH;
                    } else {
                        uint8_t id = 0;
                        *value = DEVICEINFO_RESOLUTION_480I;

                        while (id < maxLength) {
                            if (resolutions[id] > *value) {
                                *value = resolutions[id];
                            }
                            id++;
                        }
                        index->second.maxScreenResolution = *value;
                    }
                }
            }
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_hdcp(const deviceinfo_video_output_t videoOutput, deviceinfo_hdcp_t* supportsHDCP)
    {
        ASSERT(supportsHDCP != nullptr);
        uint32_t result = Core::ERROR_NONE;
        _lock.Lock();

        Exchange::IDeviceVideoCapabilities::VideoOutput videoPort = Convert(videoOutput);
        VideoOutputMap::iterator index = _videoOutputMap.find(videoPort);
        if ((index != _videoOutputMap.end()) && (index->second.hdcp.IsSet() == true)) {
            *supportsHDCP = index->second.hdcp.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if ((_deviceVideoCapabilitiesInterface != nullptr) && (index != _videoOutputMap.end())) {
                Exchange::IDeviceVideoCapabilities::CopyProtection cp;
                result = _deviceVideoCapabilitiesInterface->Hdcp(videoPort, cp);
                if (result == Core::ERROR_NONE) {
                    *supportsHDCP = Convert(cp);
                    index->second.hdcp = *supportsHDCP;
                }

                result = Core::ERROR_NONE;
            }
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_host_edid(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string hostEdid;
        _lock.Lock();
        if (_hostEdid.IsSet() == true) {
            hostEdid = _hostEdid.Value();
            result = Core::ERROR_NONE;
        }
        else {
            if (_deviceVideoCapabilitiesInterface != nullptr) {

                result = _deviceVideoCapabilitiesInterface->HostEDID(hostEdid);
                if (result == Core::ERROR_NONE) {
                    _hostEdid = hostEdid;
                }
            }
        }

        _lock.Unlock();
        if (result == Core::ERROR_NONE) {
            auto size = hostEdid.size();
            if (*length <= size) {
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, hostEdid.c_str(), *length);
            }
            *length = static_cast<uint8_t>(size + 1);
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_hdr(bool* supportsHDR)
    {
        ASSERT(supportsHDR != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();
        if (isHDRSupportCached() == true) {
            *supportsHDR = getHDRSupport();
            result = Core::ERROR_NONE;
        }
        else {

            if (_deviceVideoCapabilitiesInterface != nullptr) {
                result = _deviceVideoCapabilitiesInterface->HDR(*supportsHDR);
                if (result == Core::ERROR_NONE) {
                    setHdrSupport(*supportsHDR);
                }
            }
        }
        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_atmos(bool* supportsAtmos)
    {
        ASSERT(supportsAtmos != nullptr);
        uint32_t result = Core::ERROR_NONE;
        _lock.Lock();
        if (isAtmosSupportCached() == true) {
            *supportsAtmos = getAtmosSupport();
        }
        else {
            if (_deviceVideoCapabilitiesInterface != nullptr) {
                result = _deviceVideoCapabilitiesInterface->Atmos(*supportsAtmos);
                if (result == Core::ERROR_NONE) {
                    setAtmosSupport(*supportsAtmos);
                }
            }
        }

        _lock.Unlock();
        return result;
    }

    uint32_t Deviceinfo_cec(bool* supportsCEC)
    {
        ASSERT(supportsCEC != nullptr);
        uint32_t result = Core::ERROR_NONE;
        _lock.Lock();
        if (isCecSupportCached() == true) {
            *supportsCEC = getCecSupport();
        }
        else {
            if (_deviceVideoCapabilitiesInterface != nullptr) {
                result = _deviceVideoCapabilitiesInterface->CEC(*supportsCEC);
                if (result == Core::ERROR_NONE) {
                    setCecSupport(*supportsCEC);
                }
            }
        }

        _lock.Unlock();
        return result;
    }

private:
    Core::CriticalSection _lock;
    PluginHost::ISubSystem* _subsysInterface;
    const PluginHost::ISubSystem::IIdentifier* _identifierInterface;
    Exchange::IDeviceAudioCapabilities* _deviceAudioCapabilitiesInterface;
    Exchange::IDeviceVideoCapabilities* _deviceVideoCapabilitiesInterface;
    Exchange::IDeviceInfo* _deviceInfoInterface;
    Core::OptionalType<std::string> _architecture;
    Core::OptionalType<std::string> _chipsetName;
    Core::OptionalType<std::string> _serialNumber;
    Core::OptionalType<std::string> _sku;
    Core::OptionalType<std::string> _make;
    Core::OptionalType<std::string> _deviceType;
    Core::OptionalType<std::string> _modelName;
    Core::OptionalType<uint16_t> _modelYear;
    Core::OptionalType<std::string> _systemIntegraterName;
    Core::OptionalType<std::string> _friendlyName;
    Core::OptionalType<std::string> _platformName;
    Core::OptionalType<std::string> _firmwareVersion;
    Core::OptionalType<std::string> _idStr;
    Core::OptionalType<std::string> _hostEdid;
    VideoOutputMap _videoOutputMap;
    AudioOutputMap _audioOutputMap;
    std::vector<uint8_t> _id;
    uint8_t _hdr_atmos_cec;
    static DeviceInfoLink* _singleton;
};

DeviceInfoLink* DeviceInfoLink::_singleton = nullptr;


}// nameless namespace

extern "C" {

uint32_t deviceinfo_architecture(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_architecture(buffer,length);
}

uint32_t deviceinfo_chipset(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_chipset(buffer,length);
}

uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_firmware_version(buffer,length);
}

uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_id(buffer,length);
}

uint32_t deviceinfo_id_str(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_id_str(buffer,length);
}

uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_audio_outputs(value,length);
}

uint32_t deviceinfo_audio_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_capability_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_audio_capabilities(audioOutput, value, length);
}

uint32_t deviceinfo_audio_ms12_capabilities(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_capability_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_audio_ms12_capabilities(audioOutput, value, length);
}

uint32_t deviceinfo_audio_ms12_audio_profiles(const deviceinfo_audio_output_t audioOutput, deviceinfo_audio_ms12_profile_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_audio_ms12_audio_profiles(audioOutput, value, length);
}

uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_video_outputs(value,length);
}

uint32_t deviceinfo_output_resolutions(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_output_resolutions(videoOutput, value,length);
}

uint32_t deviceinfo_default_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value)
{
    return DeviceInfoLink::Instance().Deviceinfo_default_output_resolution(videoOutput, value);
}

uint32_t deviceinfo_maximum_output_resolution(const deviceinfo_video_output_t videoOutput, deviceinfo_output_resolution_t* value)
{
    return DeviceInfoLink::Instance().Deviceinfo_maximum_output_resolution(videoOutput, value);
}

uint32_t deviceinfo_host_edid(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_host_edid(buffer, length);
}

uint32_t deviceinfo_hdr(bool* supportsHDR)
{
    return DeviceInfoLink::Instance().Deviceinfo_hdr(supportsHDR);
}

uint32_t deviceinfo_atmos(bool* supportsAtmos)
{
    return DeviceInfoLink::Instance().Deviceinfo_atmos(supportsAtmos);
}

uint32_t deviceinfo_cec(bool* supportsCEC)
{
    return DeviceInfoLink::Instance().Deviceinfo_cec(supportsCEC);
}

uint32_t deviceinfo_hdcp(const deviceinfo_video_output_t videoOutput, deviceinfo_hdcp_t* hdcp)
{
    return DeviceInfoLink::Instance().Deviceinfo_hdcp(videoOutput, hdcp);

}
uint32_t deviceinfo_serial_number(char buffer[], uint8_t* length)
{
   return DeviceInfoLink::Instance().Deviceinfo_serial_number(buffer,length);
}

uint32_t deviceinfo_sku(char buffer[], uint8_t* length)
{
   return DeviceInfoLink::Instance().Deviceinfo_sku(buffer,length);
}

uint32_t deviceinfo_make(char buffer[], uint8_t* length)
{
   return DeviceInfoLink::Instance().Deviceinfo_make(buffer,length);
}

uint32_t deviceinfo_device_type(char buffer[], uint8_t* length)
{
   return DeviceInfoLink::Instance().Deviceinfo_device_type(buffer,length);
}

uint32_t deviceinfo_model_name(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_model_name(buffer,length);
}

uint32_t deviceinfo_model_year(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_model_year(buffer,length);
}

uint32_t deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_system_integrator_name(buffer,length);
}

uint32_t deviceinfo_friendly_name(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_friendly_name(buffer,length);
}

uint32_t deviceinfo_platform_name(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_platform_name(buffer,length);
}

void deviceinfo_dispose() {
    DeviceInfoLink::Dispose();
}
} // extern "C"

