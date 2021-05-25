#include "Module.h"

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
    {
        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign());
    }

public:
    ~DeviceInfoLink() override
    {
        BaseClass::Close(WPEFramework::Core::infinite);
    }
    static DeviceInfoLink& Instance()
    {
        return WPEFramework::Core::SingletonType<DeviceInfoLink>::Instance();
    }

    PluginHost::ISubSystem* SubSystem()
    {
        PluginHost::ISubSystem* subsystem = nullptr;

        PluginHost::IShell* shell = Aquire<PluginHost::IShell>(RPC::CommunicationTimeOut, BaseClass::Connector(), _T(""), ~0);

        if (shell != nullptr) {
            subsystem = shell->SubSystems();
            shell->Release();
        }

        return subsystem;
    }

private:
    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning) {
            if (_subsysInterface == nullptr) {
                _subsysInterface = SubSystem();

                if (_subsysInterface != nullptr && _identifierInterface == nullptr) {
                    _identifierInterface = _subsysInterface->Get<PluginHost::ISubSystem::IIdentifier>();
                }

                if (_deviceCapabilitiesInterface ==nullptr) {
                    _deviceCapabilitiesInterface = BaseClass::Interface();
                }

                if (_deviceMetaDataInterface == nullptr &&  _deviceCapabilitiesInterface != nullptr) {
                     _deviceMetaDataInterface = _deviceCapabilitiesInterface->QueryInterface<Exchange::IDeviceMetadata>();
                }
            }
        } else {
            if (_subsysInterface != nullptr) {
                _subsysInterface->Release();
                _subsysInterface = nullptr;
            }
            if (_identifierInterface != nullptr) {
                _identifierInterface->Release();
                _identifierInterface = nullptr;
            }
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

        string modelName;
        
        if (_deviceMetaDataInterface !=nullptr ) {
                result = _deviceMetaDataInterface->ModelName(modelName);
                if (result != Core::ERROR_NONE ) {
                    *length = 0 ;
                    return result ;
                }

                if(*length <= modelName.size()){
                    *length = modelName.size()+1 ;
                    return Core::ERROR_INVALID_INPUT_LENGTH ;
                }

                *length = modelName.size()+1 ;
                strncpy(buffer, modelName.c_str(), *length);
                return Core::ERROR_NONE;

        } else {
            *length=0;
        }
        return result;
    }


    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        uint16_t modelYear = 0;

        if( _deviceMetaDataInterface != nullptr) {
            result = _deviceMetaDataInterface->ModelYear(modelYear);
            if (result != Core::ERROR_NONE ) {
                *length = 0 ;
                return result ;
            }

            string year = Core::ToString(modelYear) ;

            auto size = year.size();

            if(*length <= size){
                *length = size +1;
                return Core::ERROR_INVALID_INPUT_LENGTH ;
            }

            *length = size +1;
            strncpy(buffer, year.c_str(), *length);
            return Core::ERROR_NONE;

        }  else {
            *length = 0;
        }

        return result;
    }



    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string integratorName;
        
        if (_deviceMetaDataInterface != nullptr) {
                result = _deviceMetaDataInterface->SystemIntegratorName(integratorName);
                if (result != Core::ERROR_NONE ) {
                    *length = 0 ;
                    return result ;
                }
                auto size = integratorName.size();
                if(*length <= size){
                    *length = size+1 ;
                    return Core::ERROR_INVALID_INPUT_LENGTH ;
                }

                *length = size+1 ;
                strncpy(buffer, integratorName.c_str(), *length);
                result = Core::ERROR_NONE;

        }  else {
            *length = 0;
        }

        return result;
    }


    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string friendlyName;
        
        if (_deviceMetaDataInterface != nullptr) {
            result = _deviceMetaDataInterface->FriendlyName(friendlyName);
            if (result != Core::ERROR_NONE ) {
                *length = 0 ;
                return result ;
            }
            auto size = friendlyName.size();
            if(*length <= size){
                *length = size+1 ;
                return Core::ERROR_INVALID_INPUT_LENGTH ;
            }

            *length = size+1 ;
            strncpy(buffer, friendlyName.c_str(), *length);
            return Core::ERROR_NONE;

        } else {
            *length=0;
        }

        return result;
    }


    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        string platformName;
        
        if (_deviceMetaDataInterface != nullptr) {
                result = _deviceMetaDataInterface->PlatformName(platformName);
                if (result != Core::ERROR_NONE ) {
                    *length = 0 ;
                    return result ;
                }
                auto size = platformName.size();
                if(*length <= size){
                    *length = size+1 ;
                    return Core::ERROR_INVALID_INPUT_LENGTH ;
                }

                *length = size+1 ;
                strncpy(buffer, platformName.c_str(), *length);
                return Core::ERROR_NONE;

        }  else {
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
                *length = size+1 ;
                return Core::ERROR_INVALID_INPUT_LENGTH ;
            }

            *length = size+1 ;
            strncpy(buffer, newValue.c_str(), *length);
            return Core::ERROR_NONE;

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
                *length = size+1 ;
                return Core::ERROR_INVALID_INPUT_LENGTH ;
            }

            *length = size+1 ;
            strncpy(buffer, newValue.c_str(), *length);
            return Core::ERROR_NONE;

        }  else {
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
                *length = size+1 ;
                return Core::ERROR_INVALID_INPUT_LENGTH ;
            }

            *length = size+1 ;
            strncpy(buffer, newValue.c_str(), *length);
            return Core::ERROR_NONE;

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

            return Core::ERROR_NONE;
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
                return Core::ERROR_NONE;
            } else {
                *length = id.size()+1 ;
                return Core::ERROR_INVALID_INPUT_LENGTH;
            }

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
                return Core::ERROR_NONE;
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
                return Core::ERROR_NONE;
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
                return Core::ERROR_NONE;
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
            return _deviceCapabilitiesInterface->HDR(*supportsHDR);
        }
        return result;
    }

    uint32_t Deviceinfo_atmos(bool* supportsAtmos)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            return _deviceCapabilitiesInterface->Atmos(*supportsAtmos);
        }
        return result;
    }

    uint32_t Deviceinfo_cec(bool* supportsCEC)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_deviceCapabilitiesInterface != nullptr) {
            return _deviceCapabilitiesInterface->CEC(*supportsCEC);
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
} // extern "C"