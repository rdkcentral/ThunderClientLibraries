#include "IDeviceInfoAdapter.h"

DeviceInfoAdapterInterface& getDeviceInfoInterface(const DeviceInfoObjectType & objectType = DeviceInfoObjectType::CLASSIC)
{
    if(objectType == DeviceInfoObjectType::MEMORY_CACHED)
    {
        return MemoryCachedDeviceInfo::Instance();
    }
    else
    {
        return SimpleDeviceInfoAdapter::Instance();
    }
}

extern "C" {

uint32_t deviceinfo_architecure(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_architecure(buffer,length);
}

uint32_t deviceinfo_chipset(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_chipset(buffer,length);
}

uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_firmware_version(buffer,length);
}

uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_id(buffer,length);
}

uint32_t deviceinfo_id_str(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_id_str(buffer,length);
}

uint32_t deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_output_resolutions(value,length);
}

uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_video_outputs(value,length);
    
}

uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_audio_outputs(value,length);
}

uint32_t deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_maximum_output_resolution(value);
}

uint32_t deviceinfo_hdr(bool* supportsHDR)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_hdr(supportsHDR);
}

uint32_t deviceinfo_atmos(bool* supportsAtmos)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_atmos(supportsAtmos);
}

uint32_t deviceinfo_cec(bool* supportsCEC)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_cec(supportsCEC);
}

uint32_t deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_hdcp(supportedHDCP);

}


uint32_t deviceinfo_model_name(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_model_name(buffer,length);
}

uint32_t deviceinfo_model_year(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_model_year(buffer,length);
}



uint32_t deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_system_integrator_name(buffer,length);
}


uint32_t deviceinfo_friendly_name(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_friendly_name(buffer,length);
}



uint32_t deviceinfo_platform_name(char buffer[], uint8_t* length)
{
    return getDeviceInfoInterface(DeviceInfoObjectType::MEMORY_CACHED).Deviceinfo_platform_name(buffer,length);
}
} // extern "C"