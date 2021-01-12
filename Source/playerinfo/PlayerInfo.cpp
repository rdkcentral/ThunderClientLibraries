#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>
#include <tracing/tracing.h>

#include <condition_variable>
#include <interfaces/IDolby.h>
#include <interfaces/IPlayerInfo.h>
#include <mutex>
#include <playerinfo.h>
#include <thread>

namespace WPEFramework {
class PlayerInfo : public Core::IReferenceCounted {
private:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    PlayerInfo(const string& playerName, Exchange::IPlayerProperties* interface)
        : _refCount(1)
        , _name(playerName)
        , _playerConnection(interface)
        , _dolby(interface != nullptr ? interface->QueryInterface<Exchange::Dolby::IOutput>() : nullptr)
        , _notification(this)
        , _callbacks()
    {
        ASSERT(_playerConnection != nullptr);
        _playerConnection->AddRef();
    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
    ~PlayerInfo()
    {
        if (_playerConnection != nullptr) {
            _playerConnection->Release();
        }
        if (_dolby != nullptr) {
            _dolby->Release();
        }
    }

    typedef std::map<playerinfo_dolby_audio_updated_cb, void*> Callbacks;

    class Notification : public Exchange::Dolby::IOutput::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        Notification(PlayerInfo* parent)
            : _parent(*parent)
        {
        }

        void AudioModeChanged(const Exchange::Dolby::IOutput::SoundModes mode, const bool enabled) override
        {
            _parent.Updated(mode, enabled);
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::Dolby::IOutput::INotification)
        END_INTERFACE_MAP

    private:
        PlayerInfo& _parent;
    };

    class PlayerInfoAdministration : protected std::list<PlayerInfo*> {
    public:
        PlayerInfoAdministration(const PlayerInfoAdministration&) = delete;
        PlayerInfoAdministration& operator=(const PlayerInfoAdministration&) = delete;

        PlayerInfoAdministration()
            : _adminLock()
            , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 8>>::Create())
            , _comChannel(Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(_engine)))
        {
            ASSERT(_engine != nullptr);
            ASSERT(_comChannel != nullptr);
            _engine->Announcements(_comChannel->Announcement());
        }

        ~PlayerInfoAdministration()
        {
            std::list<PlayerInfo*>::iterator index(std::list<PlayerInfo*>::begin());

            while (index != std::list<PlayerInfo*>::end()) {
                TRACE_L1(_T("Closing %s"), (*index)->Name().c_str());
                ++index;
            }

            ASSERT(std::list<PlayerInfo*>::size() == 0);
        }

        PlayerInfo* Instance(const string& name)
        {
            PlayerInfo* result(nullptr);

            _adminLock.Lock();

            result = Find(name);
            if (result == nullptr) {

                Exchange::IPlayerProperties* interface = _comChannel->Open<Exchange::IPlayerProperties>(name);

                if (interface != nullptr) {
                    result = new PlayerInfo(name, interface);
                    std::list<PlayerInfo*>::emplace_back(result);
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

        uint32_t Delete(const PlayerInfo* playerInfo, int& refCount)
        {
            uint32_t result(Core::ERROR_NONE);

            _adminLock.Lock();

            if (Core::InterlockedDecrement(refCount) == 0) {
                std::list<PlayerInfo*>::iterator index(
                    std::find(std::list<PlayerInfo*>::begin(), std::list<PlayerInfo*>::end(), playerInfo));

                ASSERT(index != std::list<PlayerInfo*>::end());

                if (index != std::list<PlayerInfo*>::end()) {
                    std::list<PlayerInfo*>::erase(index);
                }
                delete const_cast<PlayerInfo*>(playerInfo);
                _comChannel->Close(1000);
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            _adminLock.Unlock();

            return result;
        }

    private:
        PlayerInfo* Find(const string& name)
        {
            PlayerInfo* result(nullptr);

            std::list<PlayerInfo*>::iterator index(std::list<PlayerInfo*>::begin());

            while ((index != std::list<PlayerInfo*>::end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != std::list<PlayerInfo*>::end()) {
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
    Exchange::IPlayerProperties* _playerConnection;
    Exchange::Dolby::IOutput* _dolby;
    Core::Sink<Notification> _notification;
    Callbacks _callbacks;
    static PlayerInfo::PlayerInfoAdministration _administration;

public:
    PlayerInfo() = delete;
    PlayerInfo(const PlayerInfo&) = delete;
    PlayerInfo& operator=(const PlayerInfo&) = delete;

    static PlayerInfo* Instance(const string& name)
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

    void Updated(const Exchange::Dolby::IOutput::SoundModes mode, const bool enabled)
    {
        Callbacks::iterator index(_callbacks.begin());

        while (index != _callbacks.end()) {
            index->first(reinterpret_cast<playerinfo_type*>(this), index->second);
            index++;
        }
    }
    void Register(playerinfo_dolby_audio_updated_cb callback, void* userdata)
    {
        Callbacks::iterator index(_callbacks.find(callback));

        if (index == _callbacks.end()) {
            _callbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
        }
    }
    void Unregister(playerinfo_dolby_audio_updated_cb callback)
    {
        Callbacks::iterator index(_callbacks.find(callback));

        if (index != _callbacks.end()) {
            _callbacks.erase(index);
        }
    }

    int8_t IsAudioEquivalenceEnabled(bool& outIsEnabled) const
    {
        ASSERT(_playerConnection != nullptr);

        if (_playerConnection->IsAudioEquivalenceEnabled(outIsEnabled) == Core::ERROR_NONE) {
            return 1;
        }

        return 0;
    }

    int8_t PlaybackResolution(Exchange::IPlayerProperties::PlaybackResolution& outResolution) const
    {
        ASSERT(_playerConnection != nullptr);

        if (_playerConnection->Resolution(outResolution) == Core::ERROR_NONE) {
            return 1;
        }

        return 0;
    }

    int8_t VideoCodecs(playerinfo_videocodec_t array[], const uint8_t length) const
    {
        Exchange::IPlayerProperties::IVideoCodecIterator* videoCodecs;

        int8_t value = 0;

        ASSERT(_playerConnection != nullptr);

        if (_playerConnection->VideoCodecs(videoCodecs) == Core::ERROR_NONE && videoCodecs != nullptr) {

            Exchange::IPlayerProperties::VideoCodec codec;

            uint8_t numberOfCodecs = 0;
            const uint8_t newArraySize = 50;
            playerinfo_videocodec_t newArray[newArraySize];

            while (videoCodecs->Next(codec) && numberOfCodecs < newArraySize) {
                switch (codec) {
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_UNDEFINED:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_UNDEFINED;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_H263:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_H263;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_H264:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_H264;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_H265:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_H265;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_H265_10:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_H265_10;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_MPEG:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_MPEG;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_VP8:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_VP8;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_VP9:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_VP9;
                    break;
                case Exchange::IPlayerProperties::VideoCodec::VIDEO_VP10:
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_VP10;
                    break;
                default:
                    fprintf(stderr, "New video codec in the interface, not handled in client library!\n");
                    ASSERT(false && "Invalid enum");
                    newArray[numberOfCodecs] = PLAYERINFO_VIDEO_UNDEFINED;
                    break;
                }
                ++numberOfCodecs;
            }
            if (numberOfCodecs < length) {
                value = numberOfCodecs;
                ::memcpy(array, newArray, numberOfCodecs * sizeof(playerinfo_videocodec_t));
            } else {
                value = -numberOfCodecs;
            }
        }

        return value;
    }

    int8_t AudioCodecs(playerinfo_audiocodec_t array[], const uint8_t length) const
    {
        Exchange::IPlayerProperties::IAudioCodecIterator* audioCodecs;
        int8_t value = 0;

        ASSERT(_playerConnection != nullptr);

        if (_playerConnection->AudioCodecs(audioCodecs) == Core::ERROR_NONE && audioCodecs != nullptr) {

            Exchange::IPlayerProperties::AudioCodec codec;

            uint8_t numberOfCodecs = 0;
            const uint8_t newArraySize = 50;
            playerinfo_audiocodec_t newArray[newArraySize];

            while (audioCodecs->Next(codec) && numberOfCodecs < newArraySize) {
                switch (codec) {
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_UNDEFINED:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_UNDEFINED;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_AAC:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_AAC;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_AC3:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_AC3;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_AC3_PLUS:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_AC3_PLUS;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_DTS:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_DTS;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_MPEG1:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_MPEG1;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_MPEG2:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_MPEG2;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_MPEG3:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_MPEG3;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_MPEG4:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_MPEG4;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_OPUS:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_OPUS;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_VORBIS_OGG:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_VORBIS_OGG;
                    break;
                case Exchange::IPlayerProperties::AudioCodec::AUDIO_WAV:
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_WAV;
                    break;
                default:
                    fprintf(stderr, "New audio codec in the interface, not handled in client library!\n");
                    ASSERT(false && "Invalid enum");
                    newArray[numberOfCodecs] = PLAYERINFO_AUDIO_UNDEFINED;
                    break;
                }
                ++numberOfCodecs;
            }
            if (numberOfCodecs < length) {
                value = numberOfCodecs;
                ::memcpy(array, newArray, numberOfCodecs * sizeof(playerinfo_audiocodec_t));
            } else {
                value = -numberOfCodecs;
            }
        }

        return value;
    }

    int8_t IsAtmosMetadataSupported(bool& outIsSupported) const
    {
        ASSERT(_dolby != nullptr);

        if (_dolby->AtmosMetadata(outIsSupported) == Core::ERROR_NONE) {
            return 1;
        }

        return 0;
    }

    int8_t DolbySoundMode(Exchange::Dolby::IOutput::SoundModes& mode) const
    {
        ASSERT(_dolby != nullptr);

        if (_dolby->SoundMode(mode) == Core::ERROR_NONE) {
            return 1;
        }
        return 0;
    }

    int8_t EnableAtmosOutput(const bool enabled)
    {
        ASSERT(_dolby != nullptr);

        if (_dolby->EnableAtmosOutput(enabled) == Core::ERROR_NONE) {
            return 1;
        }
        return 0;
    }

    int8_t SetDolbyMode(const Exchange::Dolby::IOutput::Type& mode)
    {
        ASSERT(_dolby != nullptr);

        if (_dolby->Mode(mode) == Core::ERROR_NONE) {
            return 1;
        }
        return 0;
    }

    int8_t GetDolbyMode(Exchange::Dolby::IOutput::Type& mode) const
    {
        ASSERT(_dolby != nullptr);

        const WPEFramework::Exchange::Dolby::IOutput* constDolby = _dolby;
        if (constDolby->Mode(mode) == Core::ERROR_NONE) {
            return 1;
        }
        return 0;
    }
};

/* static */ PlayerInfo::PlayerInfoAdministration PlayerInfo::_administration;

//RECONNECTION
//IObserver and IObservable probably should be moved to the ThunderNanoInterfaces

struct IObserver {
    virtual ~IObserver() {}
    virtual void Notify(const std::string& callsign, PluginHost::IShell::state state) = 0;
};

struct IObservable {
    virtual ~IObservable() {}
    virtual void Register(const std::string& callsign, IObserver* observer) = 0;
    virtual void Unregister(const std::string& callsign, IObserver* observer) = 0;
};

class StateChangeNotifier : public IObservable {
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
                it->second->Notify(callsign, plugin->State());
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
    std::multimap<std::string, IObserver*> _observers;
    PluginHost::IShell* _systemInterface;
    Core::Sink<Notification> _notification;
    mutable Core::CriticalSection _adminLock;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 8>> _engine;
    Core::ProxyType<RPC::CommunicatorClient> _comChannel;
};

class PlayerInfoStateNotifier : public IObserver {
private:
    typedef std::map<playerinfo_state_changed_cb, void*> Callbacks;

    StateChangeNotifier _notifier;
    bool _toNotifyOnActivation;
    Callbacks _callbacks;
    playerinfo_type*& _player;

    bool _timeToEnd;
    std::thread _thread;
    Core::CriticalSection _lock;
    Core::Event _event;

    static PlayerInfoStateNotifier* _instance;

    PlayerInfoStateNotifier(playerinfo_type*& player, bool toInstantiateOnActivation)
        : _notifier()
        , _player(player)
        , _toNotifyOnActivation(toInstantiateOnActivation)
        , _timeToEnd(false)
        , _event(false, true)

    {
        _notifier.Register("PlayerInfo", this);

        _thread = std::thread(&PlayerInfoStateNotifier::CreatePlayerInfoInstance, this);
    }

    ~PlayerInfoStateNotifier()
    {
        _notifier.Unregister("PlayerInfo", this);
        _timeToEnd = true;

        _event.SetEvent();

        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void Notify(const std::string& callsign, PluginHost::IShell::state state) override
    {
        _lock.Lock();

        switch (state) {
        //Creating instance
        case PluginHost::IShell::ACTIVATED:
            ASSERT(_player == NULL);

            if (_toNotifyOnActivation) {
                _event.SetEvent();
            }
            for (const auto& i : _callbacks) {
                i.first(i.second, ACTIVATED);
            }
            break;

        //Destroying instance
        case PluginHost::IShell::DEACTIVATION:
            ASSERT(_player != NULL);

            reinterpret_cast<PlayerInfo*>(_player)->Release();
            _player = NULL;

            for (const auto& i : _callbacks) {
                i.first(i.second, DEACTIVATION);
            }
            break;

        case PluginHost::IShell::DEACTIVATED:
            for (const auto& i : _callbacks) {
                i.first(i.second, DEACTIVATED);
            }
            break;

        case PluginHost::IShell::ACTIVATION:
            for (const auto& i : _callbacks) {
                i.first(i.second, ACTIVATION);
            }
            break;

        case PluginHost::IShell::PRECONDITION:
            for (const auto& i : _callbacks) {
                i.first(i.second, PRECONDITION);
            }
            break;

        case PluginHost::IShell::DESTROYED:
            for (const auto& i : _callbacks) {
                i.first(i.second, PRECONDITION);
            }
            break;

        default:
            break;
        }

        _lock.Unlock();
    }

    void CreatePlayerInfoInstance()
    {
        while (true) {

            _event.Lock();
            _event.ResetEvent();

            if (_timeToEnd) {
                return;
            }
            if (_toNotifyOnActivation) {
                _player = reinterpret_cast<playerinfo_type*>(PlayerInfo::Instance("PlayerInfo"));
            }
        }
    }

public:
    static void CreateInstance(playerinfo_type*& player, bool toInstantiateOnActivation)
    {
        if (_instance == nullptr) {
            _instance = new PlayerInfoStateNotifier(player, toInstantiateOnActivation);
        }
    }
    static void DestroyInstance()
    {
        if (_instance != nullptr) {
            delete _instance;
            _instance = nullptr;
        }
    }

    static PlayerInfoStateNotifier* GetInstance()
    {
        return _instance;
    }

    void RegisterCallback(playerinfo_state_changed_cb callback, void* userdata)
    {
        if (callback != NULL) {
            Callbacks::iterator index(_callbacks.find(callback));

            if (index == _callbacks.end()) {
                _callbacks.emplace(std::piecewise_construct,
                    std::forward_as_tuple(callback),
                    std::forward_as_tuple(userdata));
            }
        }
    }

    void UnregisterCallback(playerinfo_state_changed_cb callback)
    {
        Callbacks::iterator index(_callbacks.find(callback));

        if (index != _callbacks.end()) {
            _callbacks.erase(index);
        }
    }

    void ToInstantiateOnActivation(bool toInstantiate)
    {
        _toNotifyOnActivation = toInstantiate;
    }
};
PlayerInfoStateNotifier* PlayerInfoStateNotifier::_instance;

// \RECONNECTION

} //namespace WPEFramework

using namespace WPEFramework;
extern "C" {

void playerinfo_register_state_change(struct playerinfo_type** type, bool to_instantiate)
{
    if (*type != NULL) {
        PlayerInfoStateNotifier::CreateInstance(*type, to_instantiate);
    }
}

void playerinfo_register_state_change_callback(playerinfo_state_changed_cb callback, void* userdata)
{
    PlayerInfoStateNotifier::GetInstance()->RegisterCallback(callback, userdata);
}

void playerinfo_unregister_state_change_callback(playerinfo_state_changed_cb callback)
{
    PlayerInfoStateNotifier::GetInstance()->UnregisterCallback(callback);
}

void playerinfo_unregister_state_change()
{
    PlayerInfoStateNotifier::DestroyInstance();
}

struct playerinfo_type* playerinfo_instance(const char name[])
{
    if (name != NULL) {
        return reinterpret_cast<playerinfo_type*>(PlayerInfo::Instance(string(name)));
    }
    return NULL;
}

void playerinfo_register(struct playerinfo_type* instance, playerinfo_dolby_audio_updated_cb callback, void* userdata)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->Register(callback, userdata);
    }
}

void playerinfo_unregister(struct playerinfo_type* instance, playerinfo_dolby_audio_updated_cb callback)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->Unregister(callback);
    }
}

void playerinfo_release(struct playerinfo_type* instance)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->Release();
    }
}

int8_t playerinfo_playback_resolution(struct playerinfo_type* instance, playerinfo_playback_resolution_t* resolution)
{
    if (instance != NULL && resolution != NULL) {
        Exchange::IPlayerProperties::PlaybackResolution value = Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_UNKNOWN;

        if (reinterpret_cast<PlayerInfo*>(instance)->PlaybackResolution(value) == 1) {
            switch (value) {
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_UNKNOWN:
                *resolution = PLAYERINFO_RESOLUTION_UNKNOWN;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_480I:
                *resolution = PLAYERINFO_RESOLUTION_480I;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_480P:
                *resolution = PLAYERINFO_RESOLUTION_480P;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_576I:
                *resolution = PLAYERINFO_RESOLUTION_576I;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_576P:
                *resolution = PLAYERINFO_RESOLUTION_576P;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_720P:
                *resolution = PLAYERINFO_RESOLUTION_720P;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_1080I:
                *resolution = PLAYERINFO_RESOLUTION_1080I;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_1080P:
                *resolution = PLAYERINFO_RESOLUTION_1080P;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_2160P30:
                *resolution = PLAYERINFO_RESOLUTION_2160P30;
                break;
            case Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_2160P60:
                *resolution = PLAYERINFO_RESOLUTION_2160P60;
                break;
            default:
                fprintf(stderr, "New resolution in the interface, not handled in client library!\n");
                ASSERT(false && "Invalid enum");
                *resolution = PLAYERINFO_RESOLUTION_UNKNOWN;
                break;
            }
            return 1;
        }
    }
    return 0;
}

int8_t playerinfo_is_audio_equivalence_enabled(struct playerinfo_type* instance, bool* is_enabled)
{
    if (instance != NULL && is_enabled != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->IsAudioEquivalenceEnabled(*is_enabled);
    }
    return 0;
}

int8_t playerinfo_video_codecs(struct playerinfo_type* instance, playerinfo_videocodec_t array[], const uint8_t length)
{
    if (instance != NULL && array != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->VideoCodecs(array, length);
    }
    return 0;
}

int8_t playerinfo_audio_codecs(struct playerinfo_type* instance, playerinfo_audiocodec_t array[], const uint8_t length)
{
    if (instance != NULL && array != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->AudioCodecs(array, length);
    }
    return 0;
}

int8_t playerinfo_dolby_atmos_metadata(struct playerinfo_type* instance, bool* is_supported)
{
    if (instance != NULL && is_supported != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->IsAtmosMetadataSupported(*is_supported);
    }
    return 0;
}

int8_t playerinfo_dolby_soundmode(struct playerinfo_type* instance, playerinfo_dolby_sound_mode_t* sound_mode)
{
    if (instance != NULL && sound_mode != NULL) {
        Exchange::Dolby::IOutput::SoundModes value = Exchange::Dolby::IOutput::SoundModes::UNKNOWN;
        if (reinterpret_cast<PlayerInfo*>(instance)->DolbySoundMode(value) == 1) {
            switch (value) {
            case Exchange::Dolby::IOutput::SoundModes::UNKNOWN:
                *sound_mode = PLAYERINFO_DOLBY_SOUND_UNKNOWN;
                break;
            case Exchange::Dolby::IOutput::SoundModes::MONO:
                *sound_mode = PLAYERINFO_DOLBY_SOUND_MONO;
                break;
            case Exchange::Dolby::IOutput::SoundModes::STEREO:
                *sound_mode = PLAYERINFO_DOLBY_SOUND_STEREO;
                break;
            case Exchange::Dolby::IOutput::SoundModes::SURROUND:
                *sound_mode = PLAYERINFO_DOLBY_SOUND_SURROUND;
                break;
            case Exchange::Dolby::IOutput::SoundModes::PASSTHRU:
                *sound_mode = PLAYERINFO_DOLBY_SOUND_PASSTHRU;
                break;
            default:
                fprintf(stderr, "New dolby sound mode in the interface, not handled in client library!\n");
                ASSERT(false && "Invalid enum");
                *sound_mode = PLAYERINFO_DOLBY_SOUND_UNKNOWN;
                break;
            }
        }
    }
    return 0;
}
int8_t playerinfo_enable_atmos_output(struct playerinfo_type* instance, const bool is_enabled)
{
    if (instance != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->EnableAtmosOutput(is_enabled);
    }
    return 0;
}

int8_t playerinfo_set_dolby_mode(struct playerinfo_type* instance, const playerinfo_dolby_mode_t mode)
{
    if (instance != NULL) {
        switch (mode) {
        case PLAYERINFO_DOLBY_MODE_DIGITAL_PCM:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PCM);
        case PLAYERINFO_DOLBY_MODE_DIGITAL_AC3:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_AC3);
        case PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PLUS);
        default:
            fprintf(stderr, "Unknown enum value, not included in playerinfo_dolby_mode_type?\n");
            return 0;
        }
    }
    return 0;
}

int8_t playerinfo_get_dolby_mode(struct playerinfo_type* instance, playerinfo_dolby_mode_t* mode)
{
    if (instance != NULL && mode != NULL) {

        Exchange::Dolby::IOutput::Type value = Exchange::Dolby::IOutput::Type::AUTO;
        if (reinterpret_cast<PlayerInfo*>(instance)->GetDolbyMode(value) == 1) {
            switch (value) {
            case Exchange::Dolby::IOutput::Type::AUTO:
                *mode = PLAYERINFO_DOLBY_MODE_AUTO;
                break;
            case Exchange::Dolby::IOutput::Type::DIGITAL_AC3:
                *mode = PLAYERINFO_DOLBY_MODE_DIGITAL_AC3;
                break;
            case Exchange::Dolby::IOutput::Type::DIGITAL_PCM:
                *mode = PLAYERINFO_DOLBY_MODE_DIGITAL_PCM;
                break;
            case Exchange::Dolby::IOutput::Type::DIGITAL_PLUS:
                *mode = PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS;
                break;
            case Exchange::Dolby::IOutput::Type::MS12:
                *mode = PLAYERINFO_DOLBY_MODE_MS12;
                break;
            default:
                fprintf(stderr, "New dolby mode in the interface, not handled in client library!\n");
                ASSERT(false && "Invalid enum");
                *mode = PLAYERINFO_DOLBY_MODE_AUTO;
                break;
            }
            return 1;
        }
    }
    return 0;
}
}