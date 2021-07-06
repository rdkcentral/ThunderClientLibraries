#pragma once
#include "DeviceInfoLink.h"
class DeviceInfoAdapterInterface
{
    public:
        virtual uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length) = 0;
        virtual uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value) = 0;
        virtual uint32_t Deviceinfo_hdr(bool* supportsHDR) = 0;
        virtual uint32_t Deviceinfo_atmos(bool* supportsAtmos) = 0;
        virtual uint32_t Deviceinfo_cec(bool* supportsCEC) = 0;
        virtual uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP) = 0;
};

enum class DeviceInfoObjectType
{
    CLASSIC = 0,
    MEMORY_CACHED
};

class SimpleDeviceInfoAdapter: public DeviceInfoAdapterInterface
{
    public:
        static DeviceInfoAdapterInterface& Instance();
    private:
        friend class WPEFramework::Core::SingletonType<SimpleDeviceInfoAdapter>;
        DeviceInfoLink& _deviceInfoLink;
        SimpleDeviceInfoAdapter();
        ~SimpleDeviceInfoAdapter();
        
    public:
        uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_hdr(bool* supportsHDR) override;
        uint32_t Deviceinfo_atmos(bool* supportsAtmos) override;
        uint32_t Deviceinfo_cec(bool* supportsCEC) override;
        uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP) override;
        uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value) override;
        uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length) override;
        uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length) override;
        uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length) override;
};

class MemoryCachedDeviceInfo: public DeviceInfoAdapterInterface
{
    private:
        friend class WPEFramework::Core::SingletonType<MemoryCachedDeviceInfo>;
        struct cacheFlag
        {
            char architectureCached:1;
            char chipsetNameCached:1;
            char modelNameCached:1;
            char modelYearCached:1;
            char systemIntegraterNameCached:1;
            char friendlyNameCached:1;
            char platformNameCached:1;
            char firmwareVersionCached:1;
            char idCached:1;
            char idStringCached:1;
            char outputResolutionCached:1;
            char audioOutputCached:1;
            char videoOutputCached:1;
            char maxOutputResolutionCached:1;
            char hdrCached:1;
            char atmosCached:1;
            char cecCached:1;
            char hdcpCached:1;
        }_cached;
        std::string* _architecture;
        std::string* _chipsetName;
        std::string* _modelName;
        std::string* _modelYear;
        std::string* _systemIntegraterName;
        std::string* _friendlyName;
        std::string* _platformName;
        std::string* _firmwareVersion;
        uint8_t* _id;
        std::string* _idStr;
        bool _hdr;
        bool _atmos;
        bool _cec;
        deviceinfo_hdcp_t _supportedHdcp;
        size_t _idLength;
        deviceinfo_output_resolution_t _maxOutputResolution;
        deviceinfo_output_resolution_t* _outputResolution;
        size_t _outputResolutionLength;
        deviceinfo_video_output_t* _videoOutput;
        size_t _videoOutputLength;
        deviceinfo_audio_output_t* _audioOutput;
        size_t _audioOutputLength;
        DeviceInfoLink& _deviceInfoLink;

    public:
        static DeviceInfoAdapterInterface& Instance();
    private:
        MemoryCachedDeviceInfo();
        ~MemoryCachedDeviceInfo();
        
    public:
        uint32_t Deviceinfo_architecure(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_chipset(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_firmware_version(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_id(uint8_t buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_id_str(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_hdr(bool* supportsHDR) override;
        uint32_t Deviceinfo_atmos(bool* supportsAtmos) override;
        uint32_t Deviceinfo_cec(bool* supportsCEC) override;
        uint32_t Deviceinfo_hdcp(deviceinfo_hdcp_t* supportsHDCP) override;
        uint32_t Deviceinfo_model_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_model_year(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_system_integrator_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_friendly_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_platform_name(char buffer[], uint8_t* length) override;
        uint32_t Deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value) override;
        uint32_t Deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length) override;
        uint32_t Deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length) override;
        uint32_t Deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length) override;
    private:
        template <typename T>
        void copyArray(T* destination, T* source, uint8_t length)
        {
            for(int index = 0; index < length; index++)
            {
                destination[index] = source[index];
            }
        }
};
