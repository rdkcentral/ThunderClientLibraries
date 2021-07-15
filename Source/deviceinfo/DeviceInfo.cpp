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
        ,_subsysInterface(nullptr)
        ,_identifierInterface(nullptr)
        ,_deviceCapabilitiesInterface(nullptr)
        ,_deviceMetaDataInterface(nullptr)

    {
        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign());

        PluginHost::IShell* ControllerInterface = BaseClass::ControllerInterface();

        ASSERT (ControllerInterface!= nullptr);

        _subsysInterface = ControllerInterface->SubSystems();

        ASSERT (_subsysInterface!= nullptr);

        if (_subsysInterface != nullptr) {
            _identifierInterface = _subsysInterface->Get<PluginHost::ISubSystem::IIdentifier>();
        }
        ASSERT (_identifierInterface!= nullptr);
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
    PluginHost::ISubSystem*  _subsysInterface;
    const PluginHost::ISubSystem::IIdentifier* _identifierInterface;
    Exchange::IDeviceCapabilities* _deviceCapabilitiesInterface; 
    Exchange::IDeviceMetadata* _deviceMetaDataInterface ;



    public:
    uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceMetaDataInterface !=nullptr ) {
            string modelName;
            result = _deviceMetaDataInterface->ModelName(modelName);
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

        } else {
            *length=0;
        }

        return result;
    }


    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceMetaDataInterface !=nullptr ) {
            uint16_t modelYear = 0;
            result = _deviceMetaDataInterface->ModelYear(modelYear);
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

        } else {
            *length=0;
        }

        return result;
    }



    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        
        if (_deviceMetaDataInterface !=nullptr ) {
            string integratorName;
            result = _deviceMetaDataInterface->SystemIntegratorName(integratorName);
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

        } else {
            *length=0;
        }

        return result;
    }


    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        
        if (_deviceMetaDataInterface !=nullptr ) {
            string friendlyName;
            result = _deviceMetaDataInterface->FriendlyName(friendlyName);
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

        } else {
            *length=0;
        }

        return result;
    }


    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        
        if (_deviceMetaDataInterface !=nullptr ) {
            string platformName;
            result = _deviceMetaDataInterface->PlatformName(platformName);
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

        } else {
            *length=0;
        }
            
        return result;
    }


    uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_identifierInterface != nullptr) {
            std::string newValue = Core::ToString(_identifierInterface->Architecture());
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

    uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_identifierInterface != nullptr) {
            std::string newValue = Core::ToString(_identifierInterface->Chipset());
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
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_identifierInterface != nullptr) {
            std::string newValue = Core::ToString(_identifierInterface->FirmwareVersion());
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
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_identifierInterface != nullptr) {
            *length = _identifierInterface->Identifier((*length) - 1, buffer);

            result = Core::ERROR_NONE;
        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_identifierInterface != nullptr) {

            uint8_t id_buffer[64] = {};

            id_buffer[0] = _identifierInterface->Identifier(sizeof(id_buffer) - 1, &(id_buffer[1]));

            string id = Core::SystemInfo::Instance().Id(id_buffer, ~0);

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
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            Exchange::IDeviceCapabilities::IOutputResolutionIterator* index = nullptr;

            _deviceCapabilitiesInterface->Resolutions(index);
            if (index != nullptr) {
                Exchange::IDeviceCapabilities::OutputResolution field;

                uint8_t inserted = 0;

                while ((inserted < *length) && (index->Next(field) == true)) {
                    deviceinfo_output_resolution_type converted = Convert(field);
                    uint8_t loop = 0;

                    while ((loop < inserted) && (value[loop] != converted)) {
                        loop++;
                    }

                    if (loop == inserted) {
                        value[inserted] = converted;
                        inserted++;
                    }
                }
                *length = inserted;
                index->Release();
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
            }
        } else {
            *length = 0;
        }

        return result;
    }

    uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            Exchange::IDeviceCapabilities::IVideoOutputIterator* index = nullptr;

            _deviceCapabilitiesInterface->VideoOutputs(index);
            if (index != nullptr) {
                Exchange::IDeviceCapabilities::VideoOutput field;

                uint8_t inserted = 0;

                while ((inserted < *length) && (index->Next(field) == true)) {
                    deviceinfo_video_output_type converted = Convert(field);
                    uint8_t loop = 0;

                    while ((loop < inserted) && (value[loop] != converted)) {
                        loop++;
                    }

                    if (loop == inserted) {
                        value[inserted] = converted;
                        inserted++;
                    }
                }
                *length = inserted;
                index->Release();
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
            }
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            Exchange::IDeviceCapabilities::IAudioOutputIterator* index = nullptr;

            _deviceCapabilitiesInterface->AudioOutputs(index);
            if (index != nullptr) {
                Exchange::IDeviceCapabilities::AudioOutput field;

                uint8_t inserted = 0;

                while ((inserted < *length) && (index->Next(field) == true)) {
                    deviceinfo_audio_output_type converted = Convert(field);
                    uint8_t loop = 0;

                    while ((loop < inserted) && (value[loop] != converted)) {
                        loop++;
                    }

                    if (loop == inserted) {
                        value[inserted] = converted;
                        inserted++;
                    }
                }
                *length = inserted;
                index->Release();
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
            }
        } else {
            *length = 0;
        }
        return result;
    }

    uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
    {
        deviceinfo_output_resolution_type resolutions[32];
        uint8_t maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_type);

        uint32_t result = Deviceinfo_output_resolutions(resolutions, &maxLength);

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
            }
        } 
        return (result);
    }

    uint32_t Deviceinfo_hdr(bool* supportsHDR)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            result =  _deviceCapabilitiesInterface->HDR(*supportsHDR);
        }
        return result;
    }

    uint32_t Deviceinfo_atmos(bool* supportsAtmos)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            result = _deviceCapabilitiesInterface->Atmos(*supportsAtmos);
        }
        return result;
    }

    uint32_t Deviceinfo_cec(bool* supportsCEC)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            result = _deviceCapabilitiesInterface->CEC(*supportsCEC);
        }
        return result;
    }

    uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            Exchange::IDeviceCapabilities::CopyProtection cp;
            result = _deviceCapabilitiesInterface->HDCP(cp);

            *supportedHDCP = Convert(cp);
        }
        return result;
    }

};

class MemoryCachedDeviceInfo
{
public:
    MemoryCachedDeviceInfo& operator=(const MemoryCachedDeviceInfo&) = delete;
    MemoryCachedDeviceInfo(const MemoryCachedDeviceInfo&) = delete;
    static MemoryCachedDeviceInfo& Instance()
    {
        return WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>::Instance();
    }
private:
    MemoryCachedDeviceInfo():
        _hdr_atmos_cec(0),
        _outputResolution(),
        _audioOutput(),
        _videoOutput(),
        _deviceInfoLink(DeviceInfoLink::Instance())
    {
        _architecture.Clear();
        _chipsetName.Clear();
        _modelName.Clear();
        _modelYear.Clear();
        _systemIntegraterName.Clear();
        _friendlyName.Clear();
        _platformName.Clear();
        _firmwareVersion.Clear();
        _idStr.Clear();
        _supportedHdcp.Clear();
        _maxOutputResolution.Clear();
    }

    ~MemoryCachedDeviceInfo()
    {
    }

public:
    uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_architecture.IsSet())
        {
            strncpy(buffer, _architecture.Value().c_str(), _architecture.Value().length());
            *length = _architecture.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_architecure(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _architecture = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_chipsetName.IsSet())
        {
            strncpy(buffer, _chipsetName.Value().c_str(), _chipsetName.Value().length());
            *length = _chipsetName.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_chipset(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _chipsetName = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_firmwareVersion.IsSet())
        {
            strncpy(buffer, _firmwareVersion.Value().c_str(), _firmwareVersion.Value().length());
            *length = _firmwareVersion.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_firmware_version(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _firmwareVersion = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_id.size()>0)
        {
            std::copy(_id.begin(), _id.end(), buffer);
            *length = _id.size();

        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_id(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _id.insert(_id.begin(), buffer, buffer + (*length));
            }
        }
        return result;
    }

    uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_idStr.IsSet())
        {
            strncpy(buffer, _idStr.Value().c_str(), _idStr.Value().length());
            *length = _idStr.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_id_str(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _idStr = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_hdr(bool* supportsHDR)
    {
        uint32_t result = Core::ERROR_NONE;
        if(isHDRSupportCached())
        {
            *supportsHDR = getHDRSupport();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_hdr(supportsHDR);
            if(result == Core::ERROR_NONE)
            {
                setHdrSupport(*supportsHDR);
            }
        }
        return result;
    }

    uint32_t Deviceinfo_atmos(bool* supportsAtmos)
    {
        uint32_t result = Core::ERROR_NONE;
        if(isAtmosSupportCached())
        {
            *supportsAtmos = getAtmosSupport();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_atmos(supportsAtmos);
            if(result == Core::ERROR_NONE)
            {
                setAtmosSupport(*supportsAtmos);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_cec(bool* supportsCEC)
    {
        uint32_t result = Core::ERROR_NONE;
        if(isCecSupportCached())
        {
            *supportsCEC = getCecSupport();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_cec(supportsCEC);
            if(result == Core::ERROR_NONE)
            {
                setCecSupport(*supportsCEC);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_supportedHdcp.IsSet())
        {
            *supportsHDCP = _supportedHdcp.Value();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_hdcp(supportsHDCP);
            if(result == Core::ERROR_NONE)
            {
                _supportedHdcp = *supportsHDCP;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_modelName.IsSet())
        {
            strncpy(buffer, _modelName.Value().c_str(), _modelName.Value().length());
            *length = _modelName.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_model_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _modelName = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_modelYear.IsSet())
        {
            strncpy(buffer, _modelYear.Value().c_str(), _modelYear.Value().length());
            *length = _modelYear.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_model_year(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _modelYear = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_systemIntegraterName.IsSet())
        {
            strncpy(buffer, _systemIntegraterName.Value().c_str(), _systemIntegraterName.Value().length());
            *length = _systemIntegraterName.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_system_integrator_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _systemIntegraterName = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_friendlyName.IsSet())
        {
            strncpy(buffer, _friendlyName.Value().c_str(), _friendlyName.Value().length());
            *length = _friendlyName.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_friendly_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _friendlyName = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_platformName.IsSet())
        {
            strncpy(buffer, _platformName.Value().c_str(), _platformName.Value().length());
            *length = _platformName.Value().length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_platform_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _platformName = buffer;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
    {
        int32_t result = Core::ERROR_NONE;
        if(_maxOutputResolution.IsSet())
        {
            *value = _maxOutputResolution;
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_maximum_output_resolution(value);
            if(result == Core::ERROR_NONE)
            {
                _maxOutputResolution = *value;
            }
        }
        return result;
    }

    uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_outputResolution.size() > 0)
        {
            std::copy(_outputResolution.begin(), _outputResolution.end(), value);
            *length = _outputResolution.size();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_output_resolutions(value, length);
            if(result == Core::ERROR_NONE)
            {
                // copyToVectorFromArray(_outputResolution, value, *length);
                _outputResolution.insert(_outputResolution.begin(), value, value + (*length));
            }
        }
        return result;

    }

    uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_videoOutput.size() > 0)
        {
            std::copy(_videoOutput.begin(), _videoOutput.end(), value);
            *length = _videoOutput.size();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_video_outputs(value, length);
            if(result == Core::ERROR_NONE)
            {
                _videoOutput.insert(_videoOutput.begin(), value, value + (*length));
            }
        }
        return result;

    }

    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_audioOutput.size() > 0)
        {
            std::copy(_audioOutput.begin(), _audioOutput.end(), value);
            *length = _audioOutput.size();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_audio_outputs(value, length);
            if(result == Core::ERROR_NONE)
            {
                _audioOutput.insert( _audioOutput.begin(), value, value + (*length));
            }
        }
        return result;
    }

private:
    // template <typename T1>
    // void copyToVectorFromArray(std::vector<T1>& vecArg, T1 arrArg[], int length)
    // {
    //     for (int index = 0; index< length; index++)
    //     {
    //         vecArg.push_back(arrArg[index]);
    //     }
    // }

    void setHdrSupport(const bool& hdr)
    {
        _hdr_atmos_cec |= (1 & hdr) << 7;
        _hdr_atmos_cec |= 0x04;
    }

    void setAtmosSupport(const bool& atmos)
    {
        _hdr_atmos_cec |= (1 & atmos) << 6;
        _hdr_atmos_cec |= 0x02;
    }

    void setCecSupport(const bool& cec)
    {
        _hdr_atmos_cec |= (1 & cec) << 5;
        _hdr_atmos_cec |= 0x01;
    }

    bool getCecSupport()
    {
        return _hdr_atmos_cec & 0x10;
    }

    bool getAtmosSupport()
    {
        return _hdr_atmos_cec & 0x20;
    }

    bool getHDRSupport()
    {
        return _hdr_atmos_cec & 0x40;
    }

    bool isCecSupportCached()
    {
        return _hdr_atmos_cec & 0x01;
    }

    bool isAtmosSupportCached()
    {
        return _hdr_atmos_cec & 0x02;
    }

    bool isHDRSupportCached()
    {
        return _hdr_atmos_cec & 0x04;
    }
private:
    friend class WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>;
    Core::OptionalType<std::string> _architecture;
    Core::OptionalType<std::string> _chipsetName;
    Core::OptionalType<std::string> _modelName;
    Core::OptionalType<std::string> _modelYear;
    Core::OptionalType<std::string> _systemIntegraterName;
    Core::OptionalType<std::string> _friendlyName;
    Core::OptionalType<std::string> _platformName;
    Core::OptionalType<std::string> _firmwareVersion;
    std::vector<uint8_t> _id;
    Core::OptionalType<std::string> _idStr;
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
    uint8_t _hdr_atmos_cec;
    Core::OptionalType<deviceinfo_hdcp_t> _supportedHdcp;
    Core::OptionalType<deviceinfo_output_resolution_t> _maxOutputResolution;
    std::vector<deviceinfo_output_resolution_t> _outputResolution;
    std::vector<deviceinfo_audio_output_t> _audioOutput;
    std::vector<deviceinfo_video_output_t> _videoOutput;
    DeviceInfoLink& _deviceInfoLink;
};

}// nameless namespace


extern "C" {

uint32_t deviceinfo_architecure(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_architecure(buffer,length);
}

uint32_t deviceinfo_chipset(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_chipset(buffer,length);
}

uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_firmware_version(buffer,length);
}

uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_id(buffer,length);
}

uint32_t deviceinfo_id_str(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_id_str(buffer,length);
}

uint32_t deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_output_resolutions(value,length);
}

uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_video_outputs(value,length);
    
}

uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_audio_outputs(value,length);
}

uint32_t deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_maximum_output_resolution(value);
}

uint32_t deviceinfo_hdr(bool* supportsHDR)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_hdr(supportsHDR);
}

uint32_t deviceinfo_atmos(bool* supportsAtmos)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_atmos(supportsAtmos);
}

uint32_t deviceinfo_cec(bool* supportsCEC)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_cec(supportsCEC);
}

uint32_t deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_hdcp(supportedHDCP);

}


uint32_t deviceinfo_model_name(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_model_name(buffer,length);
}

uint32_t deviceinfo_model_year(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_model_year(buffer,length);
}



uint32_t deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_system_integrator_name(buffer,length);
}


uint32_t deviceinfo_friendly_name(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_friendly_name(buffer,length);
}



uint32_t deviceinfo_platform_name(char buffer[], uint8_t* length)
{
    return MemoryCachedDeviceInfo::Instance().Deviceinfo_platform_name(buffer,length);
}
} // extern "C"