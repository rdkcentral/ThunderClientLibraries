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



deviceinfo_hdcp_type Convert(const Exchange::IDeviceCapabilities::CopyProtection from)
{
    static struct LUT {
        Exchange::IDeviceCapabilities::CopyProtection lhs;
        deviceinfo_hdcp_type rhs;
    } lut[] = {
        { Exchange::IDeviceCapabilities::CopyProtection::HDCP_UNAVAILABLE, DEVICEINFO_HDCP_UNAVAILABLE },
        { Exchange::IDeviceCapabilities::CopyProtection::HDCP_14, DEVICEINFO_HDCP_14 },
        { Exchange::IDeviceCapabilities::CopyProtection::HDCP_20, DEVICEINFO_HDCP_20 },
        { Exchange::IDeviceCapabilities::CopyProtection::HDCP_21, DEVICEINFO_HDCP_21 },
        { Exchange::IDeviceCapabilities::CopyProtection::HDCP_22, DEVICEINFO_HDCP_22 },
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_HDCP_UNAVAILABLE);
}

deviceinfo_output_resolution_type Convert(const Exchange::IDeviceCapabilities::OutputResolution from)
{
    static struct LUT {
        Exchange::IDeviceCapabilities::OutputResolution lhs;
        deviceinfo_output_resolution_type rhs;
    } lut[] = {
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_480I, DEVICEINFO_RESOLUTION_480I },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_480P, DEVICEINFO_RESOLUTION_480P },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_576I, DEVICEINFO_RESOLUTION_576I },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_576P, DEVICEINFO_RESOLUTION_576P },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_720P, DEVICEINFO_RESOLUTION_720P },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_1080I, DEVICEINFO_RESOLUTION_1080I },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_1080P, DEVICEINFO_RESOLUTION_1080P },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_2160P30, DEVICEINFO_RESOLUTION_2160P30 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_2160P60, DEVICEINFO_RESOLUTION_2160P60 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_4320P30, DEVICEINFO_RESOLUTION_4320P30 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_4320P60, DEVICEINFO_RESOLUTION_4320P60 }
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_RESOLUTION_480I);
}

deviceinfo_video_output_type Convert(const Exchange::IDeviceCapabilities::VideoOutput from)
{
    static struct LUT {
        Exchange::IDeviceCapabilities::VideoOutput lhs;
        deviceinfo_video_output_type rhs;
    } lut[] = {
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_COMPONENT, DEVICEINFO_VIDEO_COMPONENT },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_COMPOSITE, DEVICEINFO_VIDEO_COMPOSITE },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_DISPLAYPORT, DEVICEINFO_VIDEO_DISPLAYPORT },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_HDMI, DEVICEINFO_VIDEO_HDMI },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_OTHER, DEVICEINFO_VIDEO_OTHER },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_RF_MODULATOR, DEVICEINFO_VIDEO_RF_MODULATOR },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_SCART_RGB, DEVICEINFO_VIDEO_SCART_RGB },
        { Exchange::IDeviceCapabilities::VideoOutput::VIDEO_SVIDEO, DEVICEINFO_VIDEO_SVIDEO }
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_VIDEO_OTHER);
}

deviceinfo_audio_output_type Convert(const Exchange::IDeviceCapabilities::AudioOutput from)
{
    static struct LUT {
        Exchange::IDeviceCapabilities::AudioOutput lhs;
        deviceinfo_audio_output_type rhs;
    } lut[] = {
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_ANALOG, DEVICEINFO_AUDIO_ANALOG },
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_DISPLAYPORT, DEVICEINFO_AUDIO_DISPLAYPORT },
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_HDMI, DEVICEINFO_AUDIO_HDMI },
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_OTHER, DEVICEINFO_AUDIO_OTHER },
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_RF_MODULATOR, DEVICEINFO_AUDIO_RF_MODULATOR },
        { Exchange::IDeviceCapabilities::AudioOutput::AUDIO_SPDIF, DEVICEINFO_AUDIO_SPDIF },
    };
    uint8_t index = 0;
    while ((index < (sizeof(lut) / sizeof(LUT))) && (lut[index].lhs != from))
        index++;

    return (index < (sizeof(lut) / sizeof(LUT)) ? lut[index].rhs : DEVICEINFO_AUDIO_OTHER);
}



static string Callsign()
{
    static constexpr const TCHAR Default[] = _T("DeviceInfo");
    return (Default);
}

class DeviceInfoLink : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities>;

    friend class WPEFramework::Core::SingletonType<DeviceInfoLink>;
    DeviceInfoLink()
        : BaseClass()
        , _lock()
        , _subsysInterface(nullptr)
        , _identifierInterface(nullptr)
        , _deviceCapabilitiesInterface(nullptr)
        , _deviceMetaDataInterface(nullptr)
        , _architecture()
        , _chipsetName()
        , _modelName()
        , _modelYear()
        , _systemIntegraterName()
        , _friendlyName()
        , _platformName()
        , _firmwareVersion()
        , _idStr()
        , _supportedHdcp()
        , _maxOutputResolution()
        , _outputResolution()
        , _audioOutput()
        , _videoOutput()
        , _id()
        , _hdr_atmos_cec(0)
    {
        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign());

        PluginHost::IShell* ControllerInterface = BaseClass::ControllerInterface();

        if(ControllerInterface!= nullptr) {

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
        if (_subsysInterface != nullptr) {
            _subsysInterface->Release();
            _subsysInterface = nullptr;
        }
        if (_identifierInterface != nullptr) {
            _identifierInterface->Release();
            _identifierInterface = nullptr;
        }
        BaseClass::Close(WPEFramework::Core::infinite);
    }
    static DeviceInfoLink& Instance()
    {
        return WPEFramework::Core::SingletonType<DeviceInfoLink>::Instance();
    }

private:
    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning) {
            if (_deviceCapabilitiesInterface ==nullptr) {
                _deviceCapabilitiesInterface = BaseClass::Interface();
            }

            if (_deviceMetaDataInterface == nullptr &&  _deviceCapabilitiesInterface != nullptr) {
                _deviceMetaDataInterface = _deviceCapabilitiesInterface->QueryInterface<Exchange::IDeviceMetadata>();
            }
            
        } else {
            if (_deviceCapabilitiesInterface != nullptr) {
                _deviceCapabilitiesInterface->Release();
                _deviceCapabilitiesInterface = nullptr;
            }
            if (_deviceMetaDataInterface != nullptr) {
                _deviceMetaDataInterface->Release();
                _deviceMetaDataInterface = nullptr;
            }
        }
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

    uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        string modelName;
        _lock.Lock();
        if (_modelName.IsSet() == true )
        {
            modelName = _modelName.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if(_deviceMetaDataInterface !=nullptr )
            {
                result = _deviceMetaDataInterface->ModelName(modelName);
                if (result == Core::ERROR_NONE)
                {
                    _modelName = modelName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            auto size = modelName.size();
            if(*length <= size){
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
        if (_modelYear.IsSet() == true)
        {
            modelYear = _modelYear.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_deviceMetaDataInterface !=nullptr )
            {
                result = _deviceMetaDataInterface->ModelYear(modelYear);
                if(result == Core::ERROR_NONE)
                {
                    _modelYear = modelYear;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            string year = Core::ToString(modelYear) ;
            auto size = year.size();
            if(*length <= size){
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
        if (_systemIntegraterName.IsSet() == true)
        {
            integratorName = _systemIntegraterName.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if( _deviceMetaDataInterface !=nullptr )
            {
                result = _deviceMetaDataInterface->SystemIntegratorName(integratorName);
                if(result == Core::ERROR_NONE)
                {
                    _systemIntegraterName = integratorName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            auto size = integratorName.size();
            if(*length <= size){
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
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
        if (_friendlyName.IsSet() == true)
        {
            friendlyName = _friendlyName.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if(_deviceMetaDataInterface !=nullptr )
            {
                result = _deviceMetaDataInterface->FriendlyName(friendlyName);
                if(result == Core::ERROR_NONE)
                {
                    _friendlyName = friendlyName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            string year = Core::ToString(friendlyName) ;
            auto size = friendlyName.size();
            if(*length <= size){
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
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
        if (_platformName.IsSet() == true)
        {
            platformName = _platformName.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_deviceMetaDataInterface !=nullptr ) {
                result = _deviceMetaDataInterface->PlatformName(platformName);
                if (result == Core::ERROR_NONE)
                {
                    _platformName = platformName;
                }
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE ) {
            string year = Core::ToString(platformName) ;
            auto size = platformName.size();
            if(*length <= size){
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


    uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        std::string newValue;
        _lock.Lock();
        if(_architecture.IsSet() == true)
        {
            newValue = _architecture;
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->Architecture());
                _architecture = newValue;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE)
        {
            auto size = newValue.size();
            if(*length <= size){
                result = Core::ERROR_INVALID_INPUT_LENGTH ;
            } else {
                strncpy(buffer, newValue.c_str(), *length);
                result = Core::ERROR_NONE;
            }
            *length = static_cast<uint8_t>(size + 1);
        }
        else
        {
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
        if (_chipsetName.IsSet() == true)
        {
            newValue = _chipsetName.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->Chipset());
                _chipsetName = newValue;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE)
        {
            auto size = newValue.size();
            if(*length <= size){
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
        if (_firmwareVersion.IsSet() == true)
        {
            newValue = _firmwareVersion.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_identifierInterface != nullptr) {
                newValue = Core::ToString(_identifierInterface->FirmwareVersion());
                _firmwareVersion = newValue;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE)
        {
            auto size = newValue.size();
            if(*length <= size){
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

        if (_id.size() > 0)
        {
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_identifierInterface != nullptr) {
                uint8_t tempBuffer[255] = {'\0'};
                uint8_t size = _identifierInterface->Identifier(sizeof (tempBuffer) - 1, tempBuffer);
                std::copy(tempBuffer, tempBuffer + size, std::back_inserter(_id));
                _id.shrink_to_fit();
                result = Core::ERROR_NONE;
            }
        }
        if (result == Core::ERROR_NONE)
        {
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
        if ( _idStr.IsSet() == true)
        {
            id = _idStr;
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_identifierInterface != nullptr) {
                uint8_t id_buffer[64] = {};
                id_buffer[0] = _identifierInterface->Identifier(sizeof(id_buffer) - 1, &(id_buffer[1]));
                id = Core::SystemInfo::Instance().Id(id_buffer, ~0);
                _idStr = id;
                result = Core::ERROR_NONE;
            }
        }
        _lock.Unlock();
        if (result == Core::ERROR_NONE)
        {
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

    uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        if(_outputResolution.size() > 0)
        {
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                Exchange::IDeviceCapabilities::IOutputResolutionIterator* index = nullptr;

                _deviceCapabilitiesInterface->Resolutions(index);
                if (index != nullptr) {
                    Exchange::IDeviceCapabilities::OutputResolution field;
                    while(index->Next(field) == true)
                    {
                        deviceinfo_output_resolution_type converted = Convert(field);
                        _outputResolution.push_back(converted);
                    }
                    index->Release();
                    _outputResolution.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }

        if (result == Core::ERROR_NONE)
        {
            uint8_t inserted = 0;
            std::vector<deviceinfo_output_resolution_t>::iterator iter = _outputResolution.begin();
            while ((inserted < *length && iter != _outputResolution.end()) ) {
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
        if (_outputResolution.size()>0)
        {
            result = Core::ERROR_NONE;
        }
        else
        {

            if (_deviceCapabilitiesInterface != nullptr) {
                Exchange::IDeviceCapabilities::IVideoOutputIterator* index = nullptr;

                _deviceCapabilitiesInterface->VideoOutputs(index);
                if (index != nullptr) {
                    Exchange::IDeviceCapabilities::VideoOutput field;
                    while(index->Next(field) == true)
                    {
                        deviceinfo_video_output_t converted = Convert(field);
                        _videoOutput.push_back(converted);
                    }
                    index->Release();
                    _videoOutput.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE)
        {

            uint8_t inserted = 0;
            std::vector<deviceinfo_video_output_t>::iterator iter = _videoOutput.begin();

            while ((inserted < *length) && iter != _videoOutput.end()) {
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

    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
    {
        ASSERT(length != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();

        if (_audioOutput.size() > 0)
        {
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                Exchange::IDeviceCapabilities::IAudioOutputIterator* index = nullptr;

                _deviceCapabilitiesInterface->AudioOutputs(index);
                if (index != nullptr) {
                    Exchange::IDeviceCapabilities::AudioOutput field;
                    while(index->Next(field) == true)
                    {
                        deviceinfo_audio_output_type converted = Convert(field);
                        _audioOutput.push_back(converted);
                    }
                    index->Release();
                    _audioOutput.shrink_to_fit();
                    result = Core::ERROR_NONE;
                }
            }
        }
        if (result == Core::ERROR_NONE)
        {

            uint8_t inserted = 0;
            std::vector<deviceinfo_audio_output_t>::iterator iter = _audioOutput.begin();

            while ((inserted < *length) && iter != _audioOutput.end()) {
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

    uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
    {
        ASSERT(value != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();
        if(_maxOutputResolution.IsSet() == true)
        {
            *value = _maxOutputResolution.Value();
            result = Core::ERROR_NONE;
        }
        else
        {
            deviceinfo_output_resolution_type resolutions[32];
            uint8_t maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_type);

            result = Deviceinfo_output_resolutions(resolutions, &maxLength);

            if (result == Core::ERROR_NONE) {
                if (maxLength == 0) {
                    result = Core::ERROR_INVALID_INPUT_LENGTH;
                } else {
                    uint8_t index = 0;
                    *value = DEVICEINFO_RESOLUTION_480I;

                    while (index < maxLength) {
                        if (resolutions[index] > *value) {
                            *value = resolutions[index];
                        }
                        index++;
                    }
                    _maxOutputResolution = *value;
                }
            }
        }
        _lock.Unlock();
        return (result);
    }

    uint32_t Deviceinfo_hdr(bool* supportsHDR)
    {
        ASSERT(supportsHDR != nullptr);
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _lock.Lock();
        if(isHDRSupportCached() == true)
        {
            *supportsHDR = getHDRSupport();
            result = Core::ERROR_NONE;
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                result = _deviceCapabilitiesInterface->HDR(*supportsHDR);
                if(result == Core::ERROR_NONE)
                {
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
        if(isAtmosSupportCached() == true)
        {
            *supportsAtmos = getAtmosSupport();
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                result = _deviceCapabilitiesInterface->Atmos(*supportsAtmos);
                if(result == Core::ERROR_NONE)
                {
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
        if(isCecSupportCached() == true)
        {
            *supportsCEC = getCecSupport();
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                result = _deviceCapabilitiesInterface->CEC(*supportsCEC);
                if(result == Core::ERROR_NONE)
                {
                    setCecSupport(*supportsCEC);
                }
            }
        }
        _lock.Unlock();
        return result;

    }

    uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP)
    {
        ASSERT(supportsHDCP != nullptr);
        uint32_t result = Core::ERROR_NONE;
        _lock.Lock();
        if(_supportedHdcp.IsSet() == true)
        {
            *supportsHDCP = _supportedHdcp.Value();
        }
        else
        {
            if (_deviceCapabilitiesInterface != nullptr) {
                Exchange::IDeviceCapabilities::CopyProtection cp;
                result = _deviceCapabilitiesInterface->HDCP(cp);
                if(result == Core::ERROR_NONE)
                {
                    *supportsHDCP = Convert(cp);
                    _supportedHdcp = *supportsHDCP;
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
    Exchange::IDeviceCapabilities* _deviceCapabilitiesInterface;
    Exchange::IDeviceMetadata* _deviceMetaDataInterface;
    Core::OptionalType<std::string> _architecture;
    Core::OptionalType<std::string> _chipsetName;
    Core::OptionalType<std::string> _modelName;
    Core::OptionalType<uint16_t> _modelYear;
    Core::OptionalType<std::string> _systemIntegraterName;
    Core::OptionalType<std::string> _friendlyName;
    Core::OptionalType<std::string> _platformName;
    Core::OptionalType<std::string> _firmwareVersion;
    Core::OptionalType<std::string> _idStr;
    Core::OptionalType<deviceinfo_hdcp_t> _supportedHdcp;
    Core::OptionalType<deviceinfo_output_resolution_t> _maxOutputResolution;
    std::vector<deviceinfo_output_resolution_t> _outputResolution;
    std::vector<deviceinfo_audio_output_t> _audioOutput;
    std::vector<deviceinfo_video_output_t> _videoOutput;
    std::vector<uint8_t> _id;
    uint8_t _hdr_atmos_cec;
};

}// nameless namespace


extern "C" {

uint32_t deviceinfo_architecure(char buffer[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_architecure(buffer,length);
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

uint32_t deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_output_resolutions(value,length);
}

uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_video_outputs(value,length);
    
}

uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    return DeviceInfoLink::Instance().Deviceinfo_audio_outputs(value,length);
}

uint32_t deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    return DeviceInfoLink::Instance().Deviceinfo_maximum_output_resolution(value);
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

uint32_t deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP)
{
    return DeviceInfoLink::Instance().Deviceinfo_hdcp(supportedHDCP);

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
    Core::Singleton::Dispose();
}

} // extern "C"

