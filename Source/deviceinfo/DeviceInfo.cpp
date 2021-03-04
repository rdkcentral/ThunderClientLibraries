#include <core/core.h>
#include <tracing/tracing.h>
#include <plugins/plugins.h>
#include <com/com.h>

#include "deviceinfo.h"
#include <interfaces/IDeviceInfo.h>

namespace {

    static WPEFramework::Core::NodeId Connector() {
#ifdef __WINDOWS__
        static constexpr const TCHAR PluginConnector[] = _T("127.0.0.1:62000");
#else
        static constexpr const TCHAR PluginConnector[] = _T("/tmp/communicator");
#endif

        return (WPEFramework::Core::NodeId(PluginConnector));
    }

    static string Callsign() {
        static constexpr const TCHAR Default[] = _T("DeviceInfo");
        return (Default);
    }

    class DeviceInfoLink : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities> {
    private:
        using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDeviceCapabilities>;
        static constexpr uint32_t TimeOut = 3000;

        friend class WPEFramework::Core::SingletonType<DeviceInfoLink>;
        DeviceInfoLink() : BaseClass()
        {
            BaseClass::Open(TimeOut, Connector(), Callsign());
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

    private:
        void Operational(const bool upAndRunning) override
        {
            if (upAndRunning == false) {
            }
        }
    };

} // nameless namespace

using namespace WPEFramework;
extern "C" {

uint32_t deviceinfo_chipset(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceProperties* properties = accessor->QueryInterface<Exchange::IDeviceProperties>();

        if (properties != nullptr) {
            result = Core::ERROR_NONE;

            std::string newValue = Core::ToString(properties->Chipset());

            strncpy(buffer, newValue.c_str(), *length);

            *length = static_cast<uint8_t>(strlen(buffer));

            properties->Release();
        }
        accessor->Release();
    }

    return (result);
}

uint32_t deviceinfo_firmware_version(char buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceProperties* properties = accessor->QueryInterface<Exchange::IDeviceProperties>();

        if (properties != nullptr) {
            result = Core::ERROR_NONE;

            std::string newValue = Core::ToString(properties->FirmwareVersion());

            strncpy(buffer, newValue.c_str(), *length);

            *length = static_cast<uint8_t>(strlen(buffer));

            properties->Release();
        }
        accessor->Release();
    }
    return (result);
}

uint32_t deviceinfo_id(uint8_t buffer[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        PluginHost::ISubSystem::IIdentifier* identifier = accessor->QueryInterface<PluginHost::ISubSystem::IIdentifier>();

        if (identifier != nullptr) {
            result = Core::ERROR_NONE;

            *length =  identifier->Identifier(*length, buffer);

            identifier->Release();
        }
        accessor->Release();
    }
    return (result);
}

static deviceinfo_output_resolution_type Convert(const Exchange::IDeviceCapabilities::OutputResolution from) {
    static struct LUT {
        Exchange::IDeviceCapabilities::OutputResolution lhs;
        deviceinfo_output_resolution_type rhs;
    }   resolution[] =
    {
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_480I,    DEVICEINFO_RESOLUTION_480I    },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_480P,    DEVICEINFO_RESOLUTION_480P    },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_576I,    DEVICEINFO_RESOLUTION_576I    },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_576P,    DEVICEINFO_RESOLUTION_576P    },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_720P,    DEVICEINFO_RESOLUTION_720P    },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_1080I,   DEVICEINFO_RESOLUTION_1080I   },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_1080P,   DEVICEINFO_RESOLUTION_1080P   },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_2160P30, DEVICEINFO_RESOLUTION_2160P30 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_2160P60, DEVICEINFO_RESOLUTION_2160P60 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_4320P30, DEVICEINFO_RESOLUTION_4320P30 },
        { Exchange::IDeviceCapabilities::OutputResolution::RESOLUTION_4320P60, DEVICEINFO_RESOLUTION_4320P60 }
    };
    uint8_t index = 0;
    while ((index < (sizeof(resolution) / sizeof(LUT))) && (resolution[index].lhs != from)) index++;

    return (index < (sizeof(resolution) / sizeof(LUT)) ? resolution[index].rhs : DEVICEINFO_RESOLUTION_480I);
}

uint32_t deviceinfo_output_resolution(deviceinfo_output_resolution_t value[], uint8_t* length)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    Exchange::IDeviceCapabilities* accessor = DeviceInfoLink::Instance().Interface();

    if (accessor != nullptr) {
        Exchange::IDeviceCapabilities::IOutputResolutionIterator* index;
        accessor->Resolutions(index);
        if (index != nullptr) {
            Exchange::IDeviceCapabilities::OutputResolution field;
            uint8_t inserted = 0;
            while ((inserted < *length) && (index->Next(field) == true)) {
                deviceinfo_output_resolution_type converted = Convert(field);
                uint8_t loop = 0;

                while ( (loop < inserted) && (value[loop] != converted) ) { loop++;  }

                if (loop == inserted) {
                    value[inserted] = converted;
                    inserted++;
                }
            }
            *length = inserted;
            index->Release();
        }
        accessor->Release();
    }
    return (result);
}

uint32_t deviceinfo_maximum_output_resolutions(deviceinfo_output_resolution_t* value) {
    deviceinfo_output_resolution_type resolutions[32];
    uint8_t maxLength = sizeof(resolutions) / sizeof(deviceinfo_output_resolution_type);

    uint32_t result = deviceinfo_output_resolution(resolutions, &maxLength);

    if (result == Core::ERROR_NONE) {
        if (maxLength == 0) {
            result = Core::ERROR_INVALID_INPUT_LENGTH;
        }
        else {
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


}
