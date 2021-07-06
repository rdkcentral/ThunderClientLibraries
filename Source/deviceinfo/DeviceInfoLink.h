#pragma once

#include "Module.h"

#include "deviceinfo.h"
#include <interfaces/IDeviceInfo.h>

using namespace WPEFramework;

class DeviceInfoLink : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities>;

public:
    DeviceInfoLink();
    ~DeviceInfoLink() override;
    static DeviceInfoLink& Instance();

private:
    void Operational(const bool upAndRunning) override;
    PluginHost::ISubSystem*  _subsysInterface;
    const PluginHost::ISubSystem::IIdentifier* _identifierInterface;
    Exchange::IDeviceCapabilities* _deviceCapabilitiesInterface; 
    Exchange::IDeviceMetadata* _deviceMetaDataInterface ;

public:
    uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length);
    uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length);
    uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length);
    uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length);
    uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length);
    uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value);
    uint32_t Deviceinfo_hdr(bool* supportsHDR);
    uint32_t Deviceinfo_atmos(bool* supportsAtmos);
    uint32_t Deviceinfo_cec(bool* supportsCEC);
    uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP);
};
