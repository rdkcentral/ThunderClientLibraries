#include "IDeviceInfoAdapter.h"

DeviceInfoAdapterInterface& SimpleDeviceInfoAdapter::Instance()
{
    return WPEFramework::Core::SingletonType<SimpleDeviceInfoAdapter>::Instance();
}

SimpleDeviceInfoAdapter::SimpleDeviceInfoAdapter():
    _deviceInfoLink(DeviceInfoLink::Instance())

{
}
SimpleDeviceInfoAdapter::~SimpleDeviceInfoAdapter(){}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_architecure(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_architecure(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_chipset(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_chipset(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_firmware_version(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_id(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_id_str(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_id_str(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_hdr(bool* supportsHDR)
{
    return _deviceInfoLink.Deviceinfo_hdr(supportsHDR);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_atmos(bool* supportsAtmos)
{
    return _deviceInfoLink.Deviceinfo_atmos(supportsAtmos);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_cec(bool* supportsCEC)
{
    return _deviceInfoLink.Deviceinfo_cec(supportsCEC);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP)
{
    return _deviceInfoLink.Deviceinfo_hdcp(supportsHDCP);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_model_name(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_model_name(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_model_year(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_model_year(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_system_integrator_name(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_system_integrator_name(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_friendly_name(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_friendly_name(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_platform_name(char buffer[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_platform_name(buffer,length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    return _deviceInfoLink.Deviceinfo_maximum_output_resolution(value);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_output_resolutions(value, length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_video_outputs(value, length);
}

uint32_t SimpleDeviceInfoAdapter::Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    return _deviceInfoLink.Deviceinfo_audio_outputs(value, length);
}
