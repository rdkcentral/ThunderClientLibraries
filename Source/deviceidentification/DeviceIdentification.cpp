#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>

#include <deviceidentification.h>
#include <interfaces/IDeviceIdentification.h>

#ifndef __DEBUG__
#define Trace(fmt, ...)                                                       \
    do {                                                                      \
        fprintf(stdout, "\033[1;32m[%s:%d](%s){%p}<%d>:" fmt "\n\033[0m",     \
            __FILE__, __LINE__, __FUNCTION__, this, getpid(), ##__VA_ARGS__); \
        fflush(stdout);                                                       \
    } while (0)
#else
#define Trace(fmt, ...)
#endif

namespace WPEFramework {
class DeviceIdentification : public Core::IReferenceCounted {
private:
    mutable int _refCount;
    const string _name;
    Exchange::IDeviceProperties* _deviceConnection;
    PluginHost::ISubSystem::IIdentifier* _identifier;
    static DisplayInfo::DisplayInfoAdministration _administration;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    DeviceIdentification(const string& deviceName, Exchange::IDeviceProperties* interface)
        : _refCount(1)
        , _name(deviceName)
        , _deviceConnection(interface)
        , _identifier(interface != nullptr ? interface->QueryInterface<PluginHost::ISubSystem::IIdentifier>() : nullptr))
    {
        ASSERT(_deviceConnection != nullptr);
        _deviceConnection->AddRef();
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
    ~DeviceIdentification()
    {
        if (_deviceConnection != nullptr) {
            _deviceConnection->Release();
        }
        if (_identifier != nullptr) {
            _identifier->Release();
        }
    }
    class DeviceIdAdministration : protected std::list<DeviceIdentification*> {
    public:
        DeviceIdAdministration(const DeviceIdAdministration&) = delete;
        DeviceIdAdministration& operator=(const DeviceIdAdministration&) = delete;

        DeviceIdAdministration()
            : _adminLock()
            , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 8>>::Create())
            , _comChannel(Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(_engine)))
        {
            ASSERT(_engine != nullptr);
            ASSERT(_comChannel != nullptr);
            _engine->Announcements(_comChannel->Announcement());
        }

        ~DeviceIdAdministration()
        {
            std::list<DeviceIdentification*>::iterator index(std::list<DeviceIdentification*>::begin());

            while (index != std::list<DeviceIdentification*>::end()) {
                Trace("Closing %s", (*index)->Name().c_str());
            }

            ASSERT(std::list<DeviceIdentification*>::size() == 0);
        }

        DeviceIdentification* Instance(const string& name)
        {
            DeviceIdentification* result(nullptr);

            _adminLock.Lock();

            result = Find(name);

            if (result == nullptr) {
                Exchange::IDeviceProperties* interface = _comChannel->Open<Exchange::IDeviceProperties>(name);

                if (interface != nullptr) {
                    result = new DeviceIdentification(name, interface);
                    std::list<DeviceIdentification*>::emplace_back(result);
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

        uint32_t Delete(const DeviceIdentification* deviceIdentification, int& refCount)
        {
            uint32_t result(Core::ERROR_NONE);

            _adminLock.Lock();

            if (Core::InterlockedDecrement(refCount) == 0) {
                std::list<DeviceIdentification*>::iterator index(
                    std::find(std::list<DeviceIdentification*>::begin(), std::list<DeviceIdentification*>::end(), deviceIdentification));

                ASSERT(index != std::list<DeviceIdentification*>::end());

                if (index != std::list<DeviceIdentification*>::end()) {
                    std::list<DeviceIdentification*>::erase(index);
                }
                delete const_cast<DeviceIdentification*>(deviceIdentification);
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            _adminLock.Unlock();

            return result;
        }

    private:
        DeviceIdentification* Find(const string& name)
        {
            DeviceIdentification* result(nullptr);

            std::list<DeviceIdentification*>::iterator index(std::list<DeviceIdentification*>::begin());

            while ((index != std::list<DeviceIdentification*>::end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != std::list<DeviceIdentification*>::end()) {
                result = *index;
                result->AddRef();
            }

            return result;
        }

        Core::CriticalSection _adminLock;
        Core::ProxyType<RPC::InvokeServerType<1, 0, 8>> _engine;
        Core::ProxyType<RPC::CommunicatorClient> _comChannel;
    };

public:
    DeviceIdentification() = delete;
    DeviceIdentification(const DeviceIdentification&) = delete;
    DeviceIdentification& operator=(const DeviceIdentification&) = delete;

    static DeviceIdentification* Instance(const string& name)
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

    const string& Chipset() const
    {
        if (_deviceConnection != nullptr) {
            return _deviceConnection->Chipset();
        }
        return string();
    }

    const string& FirmwareVersion() const
    {
        if (_deviceConnection != nullptr) {
            return _deviceConnection->FirmwareVersion();
        }
        return string();
    }

    const string& Identifier() const
    {
        if (_identifier != nullptr) {
            return _identifier->Identifier();
        }
        return string();
    }
};
/* static */ DeviceIdentification::DisplayInfoAdministration DisplayInfo::_administration;

} // namespace WPEFramework

using namespace WPEFramework;
extern "C" {

struct deviceidentification_type* deviceidentification_instance(const char name[] = "DeviceIdentification")
{
    return reinterpret_cast<deviceidentification_type*>(DeviceIdentification::Instance(string(name)));
}
void deviceidentification_release(struct deviceidentification_type* instance)
{
    reinterpret_cast<DeviceIdentification*>(instance)->Release();
}
int16_t deviceidentification_chipset(struct deviceidentification_type* instance, char buffer[], const uint8_t length)
{
    string chipset = reinterpret_cast<DeviceIdentification*>(instance)->Chipset();
    strncpy(buffer, chipset.c_str(), length);

    if (length < chipset.length()) {
        return -chipset.length();
    }
    return 0;
}
int16_t deviceidentification_firmware_version(struct deviceidentification_type* instance, char buffer[], const uint8_t length)
{
    string firmware = reinterpret_cast<DeviceIdentification*>(instance)->FirmwareVersion();
    strncpy(buffer, firmware.c_str(), length);

    if (length < firmware.length()) {
        return -firmware.length();
    }
    return 0;
}

int16_t deviceidentification_id(struct deviceidentification_type* instance, char buffer[], const uint8_t length)
{
    string id = reinterpret_cast<DeviceIdentification*>(instance)->Identifier();
    strncpy(buffer, id.c_str(), length);

    if (length < id.length()) {
        return -id.length();
    }
    return 0;
}
}
