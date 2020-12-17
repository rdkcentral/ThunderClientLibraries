#pragma once

#include <com/com.h>
#include <core/core.h>
#include <plugins/IShell.h>

/*TODO:
1. Change catalog name 
2. Catalog must be observable
3. Catalog must connect and get systemInterface by itself
4. Make mock class for IObserver
*/

struct IObserver {
    virtual void NotifyActivation(const std::string& callsign) = 0;
    virtual void NotifyDeactivation(const std::string& callsign) = 0;
};

struct IObservable {
    virtual void Register(const std::string& callsign, IObserver* observer) = 0;
    virtual void Unregister(const std::string& callsign, IObserver* observer) = 0;
};

namespace WPEFramework {
class Catalog : public ::IObservable {
public:
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;

    Catalog()
        : _systemInterface(nullptr)
        , _callsign(string())
        , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 8>>::Create())
        , _comChannel(Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(_engine)))
        , _notification(this)
    {
        ASSERT(_engine != nullptr);
        ASSERT(_comChannel != nullptr);
        _engine->Announcements(_comChannel->Announcement());

        _systemInterface = _comChannel->Open<PluginHost::IShell>(string());
        ASSERT(_systemInterface != nullptr);
        _notification.Initialize(_systemInterface);

        _callsign = "PlayerInfo";
    }
    ~Catalog()
    {
        _notification.Deinitialize();
    }

    void Register(const std::string& callsign, IObserver* observer) override
    {
        ASSERT(observer != nullptr);
        _adminLock.Lock();

        _observers.emplace(std::piecewise_construct,
            std::forward_as_tuple(callsign),
            std::forward_as_tuple(observer));

        _adminLock.Unlock();
    }
    void Unregister(const std::string& callsign, IObserver* observer) override
    {
        _adminLock.Lock();

        auto lower = _observers.lower_bound(callsign);
        auto upper = _observers.upper_bound(callsign);

        for (auto it = lower; it != upper; ++it) {
            if (lower->first == callsign) {
                if (lower->second == observer) {
                    _observers.erase(it);
                }
            }
        }

        _adminLock.Unlock();
    }

private:
    class Notification : protected PluginHost::IPlugin::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;
        ~Notification() = default;

        explicit Notification(Catalog* parent)
            : _parent(*parent)
            , _client(nullptr)
            , _isRegistered(false)
        {
            ASSERT(parent != nullptr);
        }

        void Initialize(PluginHost::IShell* client)
        {
            ASSERT(client != nullptr);
            _client = client;
            _client->AddRef();
            _client->Register(this);
            _isRegistered = true;
        }
        void Deinitialize()
        {
            ASSERT(_client != nullptr);
            if (_client != nullptr) {
                _client->Unregister(this);
                _isRegistered = false;
                _client->Release();
                _client = nullptr;
            }
        }

        void StateChange(PluginHost::IShell* plugin) override
        {
            ASSERT(plugin != nullptr);

            if (_isRegistered) {
                _parent.StateChange(plugin);
            }
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        Catalog& _parent;
        bool _isRegistered;
        PluginHost::IShell* _client;
    };

    void StateChange(PluginHost::IShell* plugin)
    {
        std::string callsign = plugin->Callsign();
        auto observer = _observers.find(callsign);
        if (observer == _observers.end()) {
            return;
        }

        if (plugin->State() == PluginHost::IShell::ACTIVATION) {
            fprintf(stderr, "Activating\n");
            observer->second->NotifyActivation(callsign);
        }
        if (plugin->State() == PluginHost::IShell::DEACTIVATION) {
            fprintf(stderr, "Deactivating\n");
            observer->second->NotifyDeactivation(callsign);
        }
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

private:
    std::multimap<std::string, ::IObserver*> _observers;
    std::string _callsign;
    PluginHost::IShell* _systemInterface;
    Core::Sink<Notification> _notification;
    mutable Core::CriticalSection _adminLock;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 8>> _engine;
    Core::ProxyType<RPC::CommunicatorClient> _comChannel;
};
}