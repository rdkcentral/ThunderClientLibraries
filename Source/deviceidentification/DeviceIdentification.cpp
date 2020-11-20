#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>
#include <tracing/tracing.h>

#include <deviceidentification.h>
#include <interfaces/IDeviceIdentification.h>

namespace WPEFramework {
class DeviceIdentification : public Core::IReferenceCounted {
private:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    DeviceIdentification(const string& deviceName, Exchange::IDeviceProperties* interface)
        : _refCount(1)
        , _name(deviceName)
        , _deviceConnection(interface)
        , _identifier(interface != nullptr ? interface->QueryInterface<PluginHost::ISubSystem::IIdentifier>() : nullptr)
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
                TRACE_L1(_T("Closing %s"), (*index)->Name().c_str());
                ++index;
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

    mutable int _refCount;
    const string _name;
    Exchange::IDeviceProperties* _deviceConnection;
    PluginHost::ISubSystem::IIdentifier* _identifier;
    static DeviceIdentification::DeviceIdAdministration _administration;

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

    int16_t Chipset(char buffer[], const uint8_t length) const
    {
        int16_t stringLength = 0;

        if (_deviceConnection != nullptr) {
            string chipset = _deviceConnection->Chipset();
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

        if (_deviceConnection != nullptr) {
            string firmware = _deviceConnection->FirmwareVersion();
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
};
/* static */ DeviceIdentification::DeviceIdAdministration DeviceIdentification::_administration;

} // namespace WPEFramework

using namespace WPEFramework;
extern "C" {

struct deviceidentification_type* deviceidentification_instance(const char name[])
{
    if (name != NULL) {
        return reinterpret_cast<deviceidentification_type*>(DeviceIdentification::Instance(string(name)));
    }
    return NULL;
}

void deviceidentification_release(struct deviceidentification_type* instance)
{
    if (instance != NULL) {
        reinterpret_cast<DeviceIdentification*>(instance)->Release();
    }
}

int16_t deviceidentification_chipset(struct deviceidentification_type* instance, char buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceIdentification*>(instance)->Chipset(buffer, length);
    }
    return 0;
}

int16_t deviceidentification_firmware_version(struct deviceidentification_type* instance, char buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceIdentification*>(instance)->FirmwareVersion(buffer, length);
    }
    return 0;
}

int16_t deviceidentification_id(struct deviceidentification_type* instance, uint8_t buffer[], const uint8_t length)
{
    if (instance != NULL && buffer != NULL) {
        return reinterpret_cast<DeviceIdentification*>(instance)->Identifier(buffer, length);
    }
    return 0;
}
}
