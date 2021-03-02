#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>
#include <tracing/tracing.h>

#include <deviceinfo.h>
#include <interfaces/IDeviceInfo.h>

namespace WPEFramework {
class DeviceInfo : public Core::IReferenceCounted {
private:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    DeviceInfo(const string& deviceName, Exchange::IDeviceInfo::IProperties* interface)
        : _refCount(1)
        , _name(deviceName)
        , _deviceProperties(interface)
        , _deviceCapabilities(interface != nullptr ?  interface->QueryInterface<Exchange::IDeviceInfo::ICapabilities>() : nullptr)
        , _identifier(interface != nullptr ? interface->QueryInterface<PluginHost::ISubSystem::IIdentifier>() : nullptr)
    {
        ASSERT(_deviceProperties != nullptr);
        _deviceProperties->AddRef();
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
    ~DeviceInfo()
    {
        if (_deviceProperties != nullptr) {
            _deviceProperties->Release();
        }
        
        if (_deviceCapabilities != nullptr) {
            _deviceCapabilities->Release();
        }
        
        if (_identifier != nullptr) {
            _identifier->Release();
        }
    }
    class DeviceInfoAdministration : protected std::list<DeviceInfo*> {
    public:
        DeviceInfoAdministration(const DeviceInfoAdministration&) = delete;
        DeviceInfoAdministration& operator=(const DeviceInfoAdministration&) = delete;

        DeviceInfoAdministration()
            : _adminLock()
            , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 8>>::Create())
            , _comChannel(Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(_engine)))
        {
            ASSERT(_engine != nullptr);
            ASSERT(_comChannel != nullptr);
            _engine->Announcements(_comChannel->Announcement());
        }

        ~DeviceInfoAdministration()
        {
            std::list<DeviceInfo*>::iterator index(std::list<DeviceInfo*>::begin());

            while (index != std::list<DeviceInfo*>::end()) {
                TRACE_L1(_T("Closing %s"), (*index)->Name().c_str());
                ++index;
            }

            ASSERT(std::list<DeviceInfo*>::size() == 0);
        }

        DeviceInfo* Instance(const string& name)
        {
            DeviceInfo* result(nullptr);

            _adminLock.Lock();

            result = Find(name);

            if (result == nullptr) {
                Exchange::IDeviceInfo::IProperties* interface = _comChannel->Open<Exchange::IDeviceInfo::IProperties>(name);

                if (interface != nullptr) {
                    result = new DeviceInfo(name, interface);
                    std::list<DeviceInfo*>::emplace_back(result);
                    interface->Release();
                }
            }
            _adminLock.Unlock();

            return (result);
        }

        static Core::NodeId Connector()
        {
            const TCHAR* comPath = ::getenv(_T("COMMUNICATOR_PATH"));

            if (comPath == nullptr) {
#ifdef __WINDOWS__
                comPath = _T("127.0.0.1:62000");
#else
                comPath = _T("/tmp/communicator");
#endif
            }

            return Core::NodeId(comPath);
        }

        uint32_t Delete(const DeviceInfo* deviceInfo, int& refCount)
        {
            uint32_t result(Core::ERROR_NONE);

            _adminLock.Lock();

            if (Core::InterlockedDecrement(refCount) == 0) {
                std::list<DeviceInfo*>::iterator index(
                    std::find(std::list<DeviceInfo*>::begin(), std::list<DeviceInfo*>::end(), deviceInfo));

                ASSERT(index != std::list<DeviceInfo*>::end());

                if (index != std::list<DeviceInfo*>::end()) {
                    std::list<DeviceInfo*>::erase(index);
                }
                delete const_cast<DeviceInfo*>(deviceInfo);
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            _adminLock.Unlock();

            return result;
        }

    private:
        DeviceInfo* Find(const string& name)
        {
            DeviceInfo* result(nullptr);

            std::list<DeviceInfo*>::iterator index(std::list<DeviceInfo*>::begin());

            while ((index != std::list<DeviceInfo*>::end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != std::list<DeviceInfo*>::end()) {
                result = *index;
                result->AddRef();
            }

            return result;
        }

        Core::CriticalSection _adminLock;
        Core::ProxyType<RPC::InvokeServerType<1, 0, 8>> _engine;
        Core::ProxyType<RPC::CommunicatorClient> _comChannel;
    };

    mutable int _refCount;
    const string _name;
    Exchange::IDeviceInfo::IProperties* _deviceProperties;
    Exchange::IDeviceInfo::ICapabilities* _deviceCapabilities;
    
    PluginHost::ISubSystem::IIdentifier* _identifier;
    static DeviceInfo::DeviceInfoAdministration _administration;

public:
    DeviceInfo() = delete;
    DeviceInfo(const DeviceInfo&) = delete;
    DeviceInfo& operator=(const DeviceInfo&) = delete;

    static DeviceInfo* Instance(const string& name)
    {
        return _administration.Instance(name);
    }
    void AddRef() const
    {
        Core::InterlockedIncrement(_refCount);
    }
    uint32_t Release() const
    {
        return _administration.Delete(this, _refCount);
    }

    const string& Name() const { return _name; }

    int16_t Chipset(char buffer[], const uint8_t length) const
    {
        int16_t stringLength = 0;

        if (_deviceProperties != nullptr) {
            string chipset = _deviceProperties->Chipset();
            stringLength = chipset.length();

            if (length > stringLength) {
                strncpy(buffer, chipset.c_str(), length);
            } else {
                return -stringLength;
            }
        }
        return stringLength;
    }

    int16_t FirmwareVersion(char buffer[], const uint8_t length) const
    {
        int16_t stringLength = 0;

        if (_deviceProperties != nullptr) {
            string firmware = _deviceProperties->FirmwareVersion();
            stringLength = firmware.length();

            if (length > stringLength) {
                strncpy(buffer, firmware.c_str(), length);
            } else {
                return -stringLength;
            }
        }
        return stringLength;
    }

    int16_t Identifier(uint8_t buffer[], const uint8_t length) const
    {
        int16_t result = 0;

        if (_identifier != nullptr) {

            result = _identifier->Identifier(length, buffer);

            if (result == length) {
                uint8_t newBuffer[2048];
                return -_identifier->Identifier(sizeof(newBuffer), newBuffer);
            }
        }
        return result;
    }
    
    Exchange::IDeviceInfo::ICapabilities::OutputResolution OutputResolution() const
    {
        Exchange::IDeviceInfo::ICapabilities::OutputResolution value =
            Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_UNKNOWN;

        if (_deviceCapabilities != nullptr) {
            Exchange::IDeviceInfo::ICapabilities::IOutputResolutionIterator* resolutionIt = nullptr;
            if (_deviceCapabilities->Resolutions(resolutionIt) == Core::ERROR_NONE && resolutionIt != nullptr) {
                value = resolutionIt->Current();
            }
        }
        return value;
    }
};
/* static */ DeviceInfo::DeviceInfoAdministration DeviceInfo::_administration;

} // namespace WPEFramework

using namespace WPEFramework;
extern "C" {

struct deviceinfo_type* deviceinfo_instance(const char name[])
{
    if (name != NULL) {
        return reinterpret_cast<deviceinfo_type*>(DeviceInfo::Instance(string(name)));
    }
    return NULL;
}

void deviceinfo_release(struct deviceinfo_type* instance)
{
    if (instance != NULL) {
        reinterpret_cast<DeviceInfo*>(instance)->Release();
    }
}

int16_t deviceinfo_chipset(struct deviceinfo_type* instance, char buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceInfo*>(instance)->Chipset(buffer, length);
    }
    return 0;
}

int16_t deviceinfo_firmware_version(struct deviceinfo_type* instance, char buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceInfo*>(instance)->FirmwareVersion(buffer, length);
    }
    return 0;
}

int16_t deviceinfo_id(struct deviceinfo_type* instance, uint8_t buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceInfo*>(instance)->Identifier(buffer, length);
    }
    return 0;
}

deviceinfo_output_resolution_t deviceinfo_output_resolution(struct deviceinfo_type* instance)
{
    deviceinfo_output_resolution_t result = DEVICEINFO_RESOLUTION_UNKNOWN;
    if (instance != NULL) {
        switch (reinterpret_cast<DeviceInfo*>(instance)->OutputResolution()) {
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_480I:
            result = DEVICEINFO_RESOLUTION_480I;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_480P:
            result = DEVICEINFO_RESOLUTION_480P;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_576I:
            result = DEVICEINFO_RESOLUTION_576I;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_576P:
            result = DEVICEINFO_RESOLUTION_576P;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_720P:
            result = DEVICEINFO_RESOLUTION_720P;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_1080I:
            result = DEVICEINFO_RESOLUTION_1080I;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_1080P:
            result = DEVICEINFO_RESOLUTION_1080P;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_2160P30:
            result = DEVICEINFO_RESOLUTION_2160P30;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_2160P60:
            result = DEVICEINFO_RESOLUTION_2160P60;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_4320P30:
            result = DEVICEINFO_RESOLUTION_4320P30;
            break;
        case Exchange::IDeviceInfo::ICapabilities::OutputResolution::RESOLUTION_4320P60:
            result = DEVICEINFO_RESOLUTION_4320P60;
            break;
        default:
            result = DEVICEINFO_RESOLUTION_UNKNOWN;
            break;
        }
    }
    return result;
}
}
