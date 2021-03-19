#include "Module.h"

#include "deviceinfo.h"
#include <interfaces/IDeviceInfo.h>

using namespace WPEFramework;
namespace {
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
        PluginHost::ISubSystem* r = nullptr;

        PluginHost::IShell* shell = Aquire<PluginHost::IShell>(RPC::CommunicationTimeOut, BaseClass::Connector(), _T(""), ~0);

        if (shell != nullptr) {
            r = shell->SubSystems();
        }

        return r;
    }

private:
    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning == false) {
        }
    }
};

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

} // nameless namespace

extern "C" {
uint32_t deviceinfo_architecure(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    PluginHost::ISubSystem* subsys = DeviceInfoLink::Instance().SubSystem();

    if (subsys != nullptr) {
        const PluginHost::ISubSystem::IIdentifier* identifier(subsys->Get<PluginHost::ISubSystem::IIdentifier>());

        if (identifier != nullptr) {
            std::string newValue = Core::ToString(identifier->Architecture());

            if (newValue.size() <= *length) {
                strncpy(buffer, newValue.c_str(), *length);
                *length = static_cast<uint8_t>(strlen(buffer));
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
                result = Core::ERROR_WRITE_ERROR;
            }

            identifier->Release();
        }
        subsys->Release();
    }

    return result;
}

uint32_t deviceinfo_chipset(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    PluginHost::ISubSystem* subsys = DeviceInfoLink::Instance().SubSystem();

    if (subsys != nullptr) {
        const PluginHost::ISubSystem::IIdentifier* identifier(subsys->Get<PluginHost::ISubSystem::IIdentifier>());

        if (identifier != nullptr) {
            std::string newValue = Core::ToString(identifier->Chipset());

            if (newValue.size() <= *length) {
                strncpy(buffer, newValue.c_str(), *length);
                *length = static_cast<uint8_t>(strlen(buffer));
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
                result = Core::ERROR_WRITE_ERROR;
            }

            identifier->Release();
        }
        subsys->Release();
    }

    return result;
}

uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    PluginHost::ISubSystem* subsys = DeviceInfoLink::Instance().SubSystem();

    if (subsys != nullptr) {
        const PluginHost::ISubSystem::IIdentifier* identifier(subsys->Get<PluginHost::ISubSystem::IIdentifier>());

        if (identifier != nullptr) {
            std::string newValue = Core::ToString(identifier->FirmwareVersion());

            if (newValue.size() <= *length) {
                strncpy(buffer, newValue.c_str(), *length);
                *length = static_cast<uint8_t>(strlen(buffer));
                result = Core::ERROR_NONE;
            } else {
                *length = 0;
                result = Core::ERROR_WRITE_ERROR;
            }

            identifier->Release();
        }
        subsys->Release();
    }

    return result;
}

uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    PluginHost::ISubSystem* subsys = DeviceInfoLink::Instance().SubSystem();

    if (subsys != nullptr) {
        const PluginHost::ISubSystem::IIdentifier* identifier(subsys->Get<PluginHost::ISubSystem::IIdentifier>());

        if (identifier != nullptr) {
            *length = identifier->Identifier((*length) - 1, buffer);

            identifier->Release();

            result = Core::ERROR_NONE;
        }

        subsys->Release();
    }

    return result;
}

uint32_t deviceinfo_id_str(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    PluginHost::ISubSystem* subsys = DeviceInfoLink::Instance().SubSystem();

    if (subsys != nullptr) {
        const PluginHost::ISubSystem::IIdentifier* identifier(subsys->Get<PluginHost::ISubSystem::IIdentifier>());

        if (identifier != nullptr) {

            uint8_t id_buffer[64] = {};

            id_buffer[0] = identifier->Identifier(sizeof(id_buffer) - 1, &(id_buffer[1]));

            string id = Core::SystemInfo::Instance().Id(id_buffer, ~0);

            if (*length >= id.size()) {
                memcpy(buffer, id.c_str(), id.size() + 1);
                *length = id.size();
                result = Core::ERROR_NONE;
            } else {
                result = Core::ERROR_WRITE_ERROR;
            }

            identifier->Release();
        }

        subsys->Release();
    }

    return result;
}

uint32_t deviceinfo_output_resolutions(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceCapabilities::IOutputResolutionIterator* index = nullptr;

        accessor->Resolutions(index);
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
        }
        accessor->Release();
    }
    return result;
}

uint32_t deviceinfo_video_outputs(deviceinfo_video_output_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceCapabilities::IVideoOutputIterator* index = nullptr;

        accessor->VideoOutputs(index);
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
        }
        accessor->Release();
    }
    return result;
}

uint32_t deviceinfo_audio_outputs(deviceinfo_audio_output_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceCapabilities::IAudioOutputIterator* index = nullptr;

        accessor->AudioOutputs(index);
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
        }
        accessor->Release();
    }
    return result;
}

uint32_t deviceinfo_maximum_output_resolution(deviceinfo_output_resolution_t* value)
{
    deviceinfo_output_resolution_type resolutions[32];
    uint8_t maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_type);

    uint32_t result = deviceinfo_output_resolutions(resolutions, &maxLength);

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

uint32_t deviceinfo_hdr(bool* supportsHDR)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        result = accessor->HDR(*supportsHDR);

        accessor->Release();
    }

    return result;
}

uint32_t deviceinfo_atmos(bool* supportsAtmos)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        result = accessor->Atmos(*supportsAtmos);

        accessor->Release();
    }

    return result;
}

uint32_t deviceinfo_cec(bool* supportsCEC)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();
    if (accessor != nullptr) {
        result = accessor->CEC(*supportsCEC);

        accessor->Release();
    }

    return result;
}

uint32_t deviceinfo_hdcp(deviceinfo_hdcp_t* supportedHDCP)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceCapabilities::CopyProtection cp;
        result = accessor->HDCP(cp);

        *supportedHDCP = Convert(cp);

        accessor->Release();
    }
    return result;
}
} // extern "C"