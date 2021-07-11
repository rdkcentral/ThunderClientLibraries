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
                // std::cout<<"_deviceMetaDataInterface: "<<_deviceMetaDataInterface<<"\n";
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
private:
    friend class WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>;
    std::string _architecture;
    std::string _chipsetName;
    std::string _modelName;
    std::string _modelYear;
    std::string _systemIntegraterName;
    std::string _friendlyName;
    std::string _platformName;
    std::string _firmwareVersion;
    uint8_t* _id;
    uint8_t _idLength{0};
    std::string _idStr;
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
    uint8_t _hdr_atmos_cec{0};
    deviceinfo_hdcp_t _supportedHdcp{DEVICEINFO_HDCP_LENGTH};
    deviceinfo_output_resolution_t _maxOutputResolution{DEVICEINFO_RESOLUTION_LENGTH};
    deviceinfo_output_resolution_t _outputResolution[DEVICEINFO_RESOLUTION_LENGTH];
    uint8_t _outputResolutionLength{0};
    deviceinfo_video_output_t _videoOutput[DEVICEINFO_VIDEO_LENGTH]{0};
    uint8_t _videoOutputLength{0};
    deviceinfo_audio_output_t _audioOutput[DEVICEINFO_AUDIO_LENGTH]{0};
    uint8_t _audioOutputLength{0};
    DeviceInfoLink& _deviceInfoLink;

public:
    static MemoryCachedDeviceInfo& Instance()
    {
        return WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>::Instance();
    }
private:
    MemoryCachedDeviceInfo():
        _architecture(""),
        _chipsetName(""),
        _modelName(""),
        _modelYear(""),
        _systemIntegraterName(""),
        _friendlyName(""),
        _platformName(""),
        _firmwareVersion(""),
        _id(nullptr),
        _idStr(""),
        _deviceInfoLink(DeviceInfoLink::Instance())
    {
    }

    ~MemoryCachedDeviceInfo()
    {
        if(_id != nullptr)
        {
            delete []_id;
        }
    }

public:
    uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(!_architecture.empty())
        {
            strncpy(buffer, _architecture.c_str(), _architecture.length());
            *length = _architecture.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_architecure(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _architecture.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(!_chipsetName.empty())
        {
            strncpy(buffer, _chipsetName.c_str(), _chipsetName.length());
            *length = _chipsetName.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_chipset(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _chipsetName.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(!_firmwareVersion.empty())
        {
            strncpy(buffer, _firmwareVersion.c_str(), _firmwareVersion.length());
            *length = _firmwareVersion.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_firmware_version(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _firmwareVersion.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(_idLength > 0)
        {
            memcpy(buffer, _id, _idLength);
            *length = _idLength;
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_id(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _id = new uint8_t(*length);
                memcpy(_id, buffer, *length);
                _idLength = *length;
            }
        }
        return result;
    }

    uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(!_idStr.empty())
        {
            strncpy(buffer, _idStr.c_str(), _idStr.length());
            *length = _idStr.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_id_str(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _idStr.assign(buffer, *length);
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
        if(_supportedHdcp != DEVICEINFO_HDCP_LENGTH)
        {
            *supportsHDCP = _supportedHdcp;
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
        if(!_modelName.empty())
        {
            strncpy(buffer, _modelName.c_str(), _modelName.length());
            *length = _modelName.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_model_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _modelName.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length)
    {
        uint32_t result = Core::ERROR_NONE;
        if(!_modelYear.empty())
        {
            strncpy(buffer, _modelYear.c_str(), _modelYear.length());
            *length = _modelYear.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_model_year(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _modelYear.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(!_systemIntegraterName.empty())
        {
            strncpy(buffer, _systemIntegraterName.c_str(), _systemIntegraterName.length());
            *length = _systemIntegraterName.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_system_integrator_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _systemIntegraterName.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(!_friendlyName.empty())
        {
            strncpy(buffer, _friendlyName.c_str(), _friendlyName.length());
            *length = _friendlyName.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_friendly_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _friendlyName.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(!_platformName.empty())
        {
            strncpy(buffer, _platformName.c_str(), _platformName.length());
            *length = _platformName.length();
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_platform_name(buffer,length);
            if(result == Core::ERROR_NONE)
            {
                _platformName.assign(buffer, *length);
            }
        }
        return result;

    }

    uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
    {
        int32_t result = Core::ERROR_NONE;
        if(_maxOutputResolution != DEVICEINFO_RESOLUTION_LENGTH)
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
        if(_outputResolutionLength > 0 )
        {
            arrayCopy(value, _outputResolution, _outputResolutionLength);
            *length = _outputResolutionLength;
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_output_resolutions(value, length);
            if(result == Core::ERROR_NONE)
            {
                arrayCopy(_outputResolution, value, *length);
                _outputResolutionLength = *length;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_videoOutputLength > 0 )
        {
            arrayCopy(value, _videoOutput, _videoOutputLength);
            *length = _videoOutputLength;
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_video_outputs(value, length);
            if(result == Core::ERROR_NONE)
            {
                arrayCopy(_videoOutput, value, *length);
                _videoOutputLength = *length;
            }
        }
        return result;

    }

    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
    {
        int32_t result = Core::ERROR_NONE;
        if(_audioOutputLength > 0 )
        {
            arrayCopy(value, _audioOutput, _audioOutputLength);
            *length = _audioOutputLength;
        }
        else
        {
            result = _deviceInfoLink.Deviceinfo_audio_outputs(value, length);
            if(result == Core::ERROR_NONE)
            {
                arrayCopy(_audioOutput, value, *length);
                _audioOutputLength = *length;
            }
        }
        return result;
    }

private:
    template <typename T>
    void arrayCopy(T* destination, T* source, uint8_t length)
    {
        for(int index = 0; index < length; index++)
        {
            destination[index] = source[index];
        }
    }

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