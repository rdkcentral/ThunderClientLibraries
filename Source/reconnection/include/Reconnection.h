#pragma once

#include <com/com.h>
#include <core/core.h>
#include <plugins/IShell.h>

struct IObserver {
    virtual ~IObserver() {}
    virtual void NotifyActivation(const std::string& callsign) = 0;
    virtual void NotifyDeactivation(const std::string& callsign) = 0;
};

struct IObservable {
    virtual ~IObservable() {}
    virtual void Register(const std::string& callsign, IObserver* observer) = 0;
    virtual void Unregister(const std::string& callsign, IObserver* observer) = 0;
};

namespace WPEFramework {
class Observer : public ::IObserver {
public:
    explicit Observer(int number)
        : _observerNumber(number)
    {
    }
    ~Observer() {}

    void NotifyActivation(const std::string& callsign) override
    {
        fprintf(stderr, "Observer: %d received activation notification of %s plugin\n", _observerNumber, callsign.c_str());
    }
    void NotifyDeactivation(const std::string& callsign) override
    {
        fprintf(stderr, "Observer: %d received deactivation notification of %s plugin\n", _observerNumber, callsign.c_str());
    }

private:
    int _observerNumber;
};

class StateChangeNotifier : public ::IObservable {
public:
    StateChangeNotifier(const StateChangeNotifier&) = delete;
    StateChangeNotifier& operator=(const StateChangeNotifier&) = delete;

    StateChangeNotifier()
        : _systemInterface(nullptr)
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
    }
    ~StateChangeNotifier()
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
            if (it->first == callsign) {
                if (it->second == observer) {
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

        explicit Notification(StateChangeNotifier* parent)
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
        StateChangeNotifier& _parent;
        bool _isRegistered;
        PluginHost::IShell* _client;
    };

    void StateChange(PluginHost::IShell* plugin)
    {
        std::string callsign = plugin->Callsign();

        auto lower = _observers.lower_bound(callsign);
        auto upper = _observers.upper_bound(callsign);

        for (auto it = lower; it != upper; ++it) {
            if (it->first == callsign) {
                if (plugin->State() == PluginHost::IShell::ACTIVATION) {
                    it->second->NotifyActivation(callsign);
                }
                if (plugin->State() == PluginHost::IShell::DEACTIVATION) {
                    it->second->NotifyDeactivation(callsign);
                }
            }
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
    PluginHost::IShell* _systemInterface;
    Core::Sink<Notification> _notification;
    mutable Core::CriticalSection _adminLock;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 8>> _engine;
    Core::ProxyType<RPC::CommunicatorClient> _comChannel;
};
}