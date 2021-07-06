#include "IDeviceInfoAdapter.h"

DeviceInfoAdapterInterface& MemoryCachedDeviceInfo::Instance()
{
    return WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>::Instance();
}
MemoryCachedDeviceInfo::MemoryCachedDeviceInfo():
    _architecture(nullptr),
    _chipsetName(nullptr),
    _modelName(nullptr),
    _modelYear(nullptr),
    _systemIntegraterName(nullptr),
    _friendlyName(nullptr),
    _platformName(nullptr),
    _firmwareVersion(nullptr),
    _id(nullptr),
    _idStr(nullptr),
    _idLength(0),
    _maxOutputResolution(DEVICEINFO_RESOLUTION_UNKNOWN),
    _outputResolution(nullptr),
    _outputResolutionLength(0),
    _videoOutput(nullptr),
    _videoOutputLength(0),
    _audioOutput(nullptr),
    _audioOutputLength(0),
    _deviceInfoLink(DeviceInfoLink::Instance())

{
    ::memset(&_cached, 0, sizeof(_cached));
}
MemoryCachedDeviceInfo::~MemoryCachedDeviceInfo()
{
    if(_architecture != nullptr)
    {
        delete _architecture;
        _architecture = nullptr;
    }
    if(_chipsetName != nullptr)
    {
        delete _chipsetName;
        _chipsetName = nullptr;
    }
    if(_modelName != nullptr)
    {
        delete _modelName;
        _modelName = nullptr;
    }
    if(_modelYear != nullptr)
    {
        delete _modelYear;
        _modelYear = nullptr;
    }
    if(_systemIntegraterName != nullptr)
    {
        delete _systemIntegraterName;
        _systemIntegraterName = nullptr;
    }

    if(_audioOutput != nullptr)
    {
        delete []_audioOutput;
        _audioOutput = nullptr;
    }
    if(_videoOutput != nullptr)
    {
        delete []_videoOutput;
        _videoOutput = nullptr;
    }
    if(_outputResolution != nullptr)
    {
        delete []_outputResolution;
        _outputResolution = nullptr;
    }
    if(_id!= nullptr)
    {
        delete _id;
        _id = nullptr;
    }
    if(_idStr != nullptr)
    {
        delete _idStr;
        _idStr = nullptr;
    }
    if(_firmwareVersion != nullptr)
    {
        delete _firmwareVersion;
        _firmwareVersion = nullptr;
    }
    if(_platformName != nullptr)
    {
        delete _platformName;
        _platformName = nullptr;
    }
    if(_friendlyName != nullptr)
    {
        delete _friendlyName;
        _friendlyName = nullptr;
    }
}
uint32_t MemoryCachedDeviceInfo::Deviceinfo_architecure(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.architectureCached)
    {
        strncpy(buffer, _architecture->c_str(), _architecture->length());
        *length = _architecture->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_architecure(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _architecture = new std::string(buffer);
            _cached.architectureCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_chipset(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.chipsetNameCached)
    {
        strncpy(buffer, _chipsetName->c_str(), _chipsetName->length());
        *length = _chipsetName->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_chipset(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _chipsetName = new std::string(buffer);
            _cached.chipsetNameCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.firmwareVersionCached)
    {
        strncpy(buffer, _firmwareVersion->c_str(), _firmwareVersion->length());
        *length = _firmwareVersion->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_firmware_version(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _firmwareVersion = new std::string(buffer);
            _cached.firmwareVersionCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.idCached)
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
            _cached.idCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_id_str(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.idStringCached)
    {
        strncpy(buffer, _idStr->c_str(), _idStr->length());
        *length = _idStr->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_id_str(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _idStr = new std::string(buffer);
            _cached.idStringCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_hdr(bool* supportsHDR)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.hdrCached)
    {
        *supportsHDR = _hdr;
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_hdr(supportsHDR);
        if(result == Core::ERROR_NONE)
        {
            _hdr = *supportsHDR;
            _cached.hdrCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_atmos(bool* supportsAtmos)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.atmosCached)
    {
        *supportsAtmos = _atmos;
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_atmos(supportsAtmos);
        if(result == Core::ERROR_NONE)
        {
            _atmos = *supportsAtmos;
            _cached.atmosCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_cec(bool* supportsCEC)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.cecCached)
    {
        *supportsCEC = _cec;
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_cec(supportsCEC);
        if(result == Core::ERROR_NONE)
        {
            _cec = *supportsCEC;
            _cached.cecCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.hdcpCached)
    {
        *supportsHDCP = _supportedHdcp;
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_hdcp(supportsHDCP);
        if(result == Core::ERROR_NONE)
        {
            _supportedHdcp = *supportsHDCP;
            _cached.hdcpCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_model_name(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.modelNameCached)
    {
        strncpy(buffer, _modelName->c_str(), _modelName->length());
        *length = _modelName->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_model_name(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _modelName = new std::string(buffer);
            _cached.modelNameCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_model_year(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.modelYearCached)
    {
        strncpy(buffer, _modelYear->c_str(), _modelYear->length());
        *length = _modelYear->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_model_year(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _modelYear = new std::string(buffer);
            _cached.modelYearCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.systemIntegraterNameCached)
    {
        strncpy(buffer, _systemIntegraterName->c_str(), _systemIntegraterName->length());
        *length = _systemIntegraterName->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_system_integrator_name(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _systemIntegraterName = new std::string(buffer);
            _cached.systemIntegraterNameCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_friendly_name(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.friendlyNameCached)
    {
        strncpy(buffer, _friendlyName->c_str(), _friendlyName->length());
        *length = _friendlyName->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_friendly_name(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _friendlyName = new std::string(buffer);
            _cached.friendlyNameCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_platform_name(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.platformNameCached)
    {
        strncpy(buffer, _platformName->c_str(), _platformName->length());
        *length = _platformName->length();
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_platform_name(buffer,length);
        if(result == Core::ERROR_NONE)
        {
            _platformName = new std::string(buffer);
            _cached.platformNameCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.maxOutputResolutionCached)
    {
        *value = _maxOutputResolution;
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_maximum_output_resolution(value);
        if(result == Core::ERROR_NONE)
        {
            _maxOutputResolution = *value;
            _cached.maxOutputResolutionCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.outputResolutionCached)
    {
        *length = _outputResolutionLength;
        copyArray<deviceinfo_output_resolution_t>(value, _outputResolution, _outputResolutionLength);
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_output_resolutions(value, length);
        if(result == Core::ERROR_NONE)
        {
            _outputResolution = new deviceinfo_output_resolution_t[*length];
            copyArray<deviceinfo_output_resolution_t>(_outputResolution, value, *length);
            _outputResolutionLength = *length;
            _cached.outputResolutionCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.videoOutputCached)
    {
        *length = _videoOutputLength;
        copyArray<deviceinfo_video_output_t>(value, _videoOutput, _videoOutputLength);
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_video_outputs(value, length);
        if(result == Core::ERROR_NONE)
        {
            _videoOutput = new deviceinfo_video_output_t[*length];
            copyArray<deviceinfo_video_output_t>(_videoOutput, value, *length);
            _videoOutputLength = *length;
            _cached.videoOutputCached = 1;
        }
    }
    return result;
}

uint32_t MemoryCachedDeviceInfo::Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_NONE;
    if(_cached.audioOutputCached)
    {
        *length = _audioOutputLength;
        copyArray<deviceinfo_audio_output_t>(value, _audioOutput, _audioOutputLength);
    }
    else
    {
        result = _deviceInfoLink.Deviceinfo_audio_outputs(value, length);
        if(result == Core::ERROR_NONE)
        {
            _audioOutput = new deviceinfo_audio_output_t[*length];
            copyArray<deviceinfo_audio_output_t>(_audioOutput, value, *length);
            _audioOutputLength = *length;
            _cached.audioOutputCached = 1;
        }
    }
    return result;
}

