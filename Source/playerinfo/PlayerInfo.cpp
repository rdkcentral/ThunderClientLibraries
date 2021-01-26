#include <com/com.h>
#include <core/core.h>
#include <stdlib.h>
#include <tracing/tracing.h>

#include <interfaces/IDolby.h>
#include <interfaces/IPlayerInfo.h>
#include <playerinfo.h>

#include <interfaces/IDictionary.h>

namespace WPEFramework {
namespace RPC {
    template <typename INTERFACE>
    class InterfaceType {
    private:
        using Engine = InvokeServerType<2, 0, 8>;

        class Channel : public CommunicatorClient {
        public:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

            Channel(const Core::NodeId& remoteNode, const Core::ProxyType<Engine>& handler)
                : CommunicatorClient(remoteNode, Core::ProxyType<Core::IIPCServer>(handler))
            {
                handler->Announcements(CommunicatorClient::Announcement());
            }
            ~Channel() override = default;

        public:
            uint32_t Initialize()
            {
                return (CommunicatorClient::Open(Core::infinite));
            }
            void Deintialize()
            {
                CommunicatorClient::Close(Core::infinite);
            }
            INTERFACE* Aquire(const string className, const uint32_t version)
            {
                return RPC::CommunicatorClient::Aquire<INTERFACE>(Core::infinite, className, version);
            }
        };

        friend class Core::SingletonType<InterfaceType<INTERFACE>>;
        InterfaceType()
            : _engine(Core::ProxyType<Engine>::Create())
        {
        }

    public:
        InterfaceType(const InterfaceType<INTERFACE>&) = delete;
        InterfaceType<INTERFACE>& operator=(const InterfaceType<INTERFACE>&) = delete;

        static InterfaceType<INTERFACE>& Instance()
        {
            static InterfaceType<INTERFACE>& singleton(Core::SingletonType<InterfaceType<INTERFACE>>::Instance());
            return (singleton);
        }
        ~InterfaceType() = default;

    public:
        INTERFACE* Aquire(const uint32_t waitTime, const Core::NodeId& nodeId, const string className, const uint32_t version = ~0)
        {
            INTERFACE* result = nullptr;

            ASSERT(_engine.IsValid() == true);

            Core::ProxyType<Channel> channel = _comChannels.Instance(nodeId, _engine);

            if (channel.IsValid() == true) {
                result = channel->Aquire(className, version);
            }

            return (result);
        }
        inline void Submit(const Core::ProxyType<Core::IDispatch>& job)
        {
            // The engine has to be running :-)
            ASSERT(_engine.IsValid() == true);

            _engine->Submit(job, Core::infinite);
        }

    private:
        Core::ProxyType<Engine> _engine;
        Core::ProxyMapType<Core::NodeId, Channel> _comChannels;
    };

    template <typename HANDLER>
    class PluginMonitorType {
    private:
        class Sink : public PluginHost::IPlugin::INotification {
        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            Sink(PluginMonitorType<HANDLER>& parent)
                : _parent(parent)
            {
            }
            ~Sink() override = default;

        public:
            void StateChange(PluginHost::IShell* plugin, const string& name) override
            {
                _parent.StateChange(plugin, name);
            }
            void Dispatch()
            {
                _parent.Dispatch();
            }

            BEGIN_INTERFACE_MAP(Sink)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            PluginMonitorType<HANDLER>& _parent;
        };
        class Job {
        public:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

            Job(PluginMonitorType<HANDLER>& parent)
                : _parent(parent)
            {
            }
            ~Job() = default;

        public:
            void Dispatch()
            {
                _parent.Dispatch();
            }

        private:
            PluginMonitorType<HANDLER>& _parent;
        };

    public:
        PluginMonitorType() = delete;
        PluginMonitorType(const PluginMonitorType<HANDLER>&) = delete;
        PluginMonitorType<HANDLER>& operator=(const PluginMonitorType<HANDLER>&) = delete;

        template <typename... Args>
        PluginMonitorType(Args&&... args)
            : _adminLock()
            , _reporter(std::forward<Args>(args)...)
            , _callsign()
            , _node()
            , _sink(*this)
            , _job(*this)
            , _controller(nullptr)
            , _reportedActive(false)
            , _administrator(InterfaceType<PluginHost::IShell>::Instance())
        {
        }
        ~PluginMonitorType() = default;

    public:
        uint32_t Open(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        {

            _adminLock.Lock();

            ASSERT(_controller == nullptr);

            if (_controller != nullptr) {
                _adminLock.Unlock();
            } else {
                _controller = _administrator.Aquire(waitTime, node, _T(""), ~0);

                if (_controller == nullptr) {
                    _adminLock.Unlock();
                } else {
                    _node = node;
                    _callsign = callsign;

                    _adminLock.Unlock();

                    _controller->Register(&_sink);
                }
            }

            return (_controller != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        uint32_t Close(const uint32_t waitTime)
        {
            _adminLock.Lock();
            if (_controller != nullptr) {
                _controller->Unregister(&_sink);
                _controller->Release();
                _controller = nullptr;
            }
            _adminLock.Unlock();
            return (Core::ERROR_NONE);
        }

    private:
        void Dispatch()
        {

            _adminLock.Lock();
            PluginHost::IShell* evaluate = _designated;
            _designated = nullptr;
            _adminLock.Unlock();

            if (evaluate != nullptr) {

                PluginHost::IShell::state current = evaluate->State();

                if (current == PluginHost::IShell::ACTIVATED) {
                    _reporter.Activated(evaluate);
                    _reportedActive = true;
                } else if (current == PluginHost::IShell::DEACTIVATION) {
                    if (_reportedActive == true) {
                        _reporter.Deactivated(evaluate);
                    }
                }
                evaluate->Release();
            }
        }
        void StateChange(PluginHost::IShell* plugin, const string& callsign)
        {
            if (callsign == _callsign) {
                _adminLock.Lock();

                if (_designated == nullptr) {
                    _designated = plugin;
                    _designated->AddRef();
                    Core::ProxyType<Core::IDispatch> job(_job.Aquire());
                    if (job.IsValid() == true) {
                        _administrator.Submit(job);
                    }
                }

                _adminLock.Unlock();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        HANDLER _reporter;
        string _callsign;
        Core::NodeId _node;
        Core::Sink<Sink> _sink;
        Core::ThreadPool::JobType<Job> _job;
        PluginHost::IShell* _designated;
        PluginHost::IShell* _controller;
        bool _reportedActive;
        InterfaceType<PluginHost::IShell>& _administrator;
    };

    template <typename INTERFACE>
    class SmartInterfaceType {
    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        SmartInterfaceType()
            : _adminLock()
            , _monitor(*this)
            , _smartType(nullptr)
        {
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

        virtual ~SmartInterfaceType()
        {
            Close(Core::infinite);
        }

    public:
        inline bool IsOperational() const
        {
            return (_smartType != nullptr);
        }
        uint32_t Open(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        {
            return (_monitor.Open(waitTime, node, callsign));
        }
        uint32_t Close(const uint32_t waitTime)
        {
            Deactivated(nullptr);
            return (_monitor.Close(waitTime));
        }

        // IMPORTANT NOTE:
        // If you aquire the interface here, take action on the interface and release it. Do not maintain/stash it
        // since the interface might require to be dropped in the mean time.
        // So usage on the interface should be deterministic and short !!!
        INTERFACE* Interface()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            INTERFACE* result = _smartType;

            if (result != nullptr) {
                result->AddRef();
            }

            return (result);
        }
        const INTERFACE* Interface() const
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            const INTERFACE* result = _smartType;

            if (result != nullptr) {
                result->AddRef();
            }

            return (result);
        }

        // Allow a derived class to take action on a new interface, or almost dissapeared interface..
        virtual void Operational(const bool upAndRunning)
        {
        }

    private:
        friend class PluginMonitorType<SmartInterfaceType<INTERFACE>&>;
        void Activated(PluginHost::IShell* plugin)
        {
            ASSERT(plugin != nullptr);
            _adminLock.Lock();
            DropInterface();
            _smartType = plugin->QueryInterface<INTERFACE>();
            _adminLock.Unlock();
            Operational(true);
        }
        void Deactivated(PluginHost::IShell* /* plugin */)

        {
            Operational(false);
            _adminLock.Lock();
            DropInterface();
            _adminLock.Unlock();
        }
        void DropInterface()
        {
            if (_smartType != nullptr) {
                _smartType->Release();
                _smartType = nullptr;
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        PluginMonitorType<SmartInterfaceType<INTERFACE>&> _monitor;
        INTERFACE* _smartType;
    };
}

class PlayerInfo : protected RPC::SmartInterfaceType<Exchange::IPlayerProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPlayerProperties>;
    using Callbacks = std::map<playerinfo_dolby_audio_updated_cb, void*>;

    PlayerInfo(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass()
        , _callsign(callsign)
    {
        BaseClass::Open(waitTime, node, callsign);
    }
    PlayerInfo(const PlayerInfo&) = delete;
    PlayerInfo& operator=(const PlayerInfo&) = delete;

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

    void Operational(const bool upAndRunning) override
    {
        printf("Operational state of PlayerInfo: %s\n", upAndRunning ? _T("true") : _T("false"));
    }

    static std::unique_ptr<PlayerInfo> _instance;
    std::string _callsign;
    Callbacks _callbacks;

public:
    //Object management
    ~PlayerInfo()
    {
        BaseClass::Close(Core::infinite);
    }

    static PlayerInfo* Instance()
    {
        if (_instance == nullptr) {
            _instance.reset(new PlayerInfo(Core::infinite, Connector(), "Dictionary")); //no make_unique in C++11 :/
            //TODO should I worry about this???
            //if (!_instance->IsProperlyConstructed()) {
            //    delete _instance;
            //    _instance = nullptr;
            // }
        }
        return _instance.get();
    }
    void DestroyInstance()
    {
        _instance.reset();
    }

public:
    //Methods goes here
    const string& Name() const
    {
        return _callsign;
    }

    void Updated(const Exchange::Dolby::IOutput::SoundModes mode, const bool enabled)
    {
        Callbacks::iterator index(_callbacks.begin());

        while (index != _callbacks.end()) {
            index->first(index->second);
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

    uint32_t IsAudioEquivalenceEnabled(bool& outIsEnabled) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->IsAudioEquivalenceEnabled(outIsEnabled);
            impl->Release();
        }

        return errorCode;
    }

    uint32_t PlaybackResolution(Exchange::IPlayerProperties::PlaybackResolution& outResolution) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            errorCode = impl->Resolution(outResolution);
            impl->Release();
        }

        return errorCode;
    }

    int8_t VideoCodecs(playerinfo_videocodec_t array[], const uint8_t length) const
    {
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();
        Exchange::IPlayerProperties::IVideoCodecIterator* videoCodecs;
        int8_t value = 0;

        if (impl != nullptr) {
            if (impl->VideoCodecs(videoCodecs) == Core::ERROR_NONE && videoCodecs != nullptr) {

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
                    ::memcpy(array, newArray, numberOfCodecs * sizeof(playerinfo_videocodec_t)); //TODO change this later
                } else {
                    value = -numberOfCodecs;
                }
            }
            impl->Release();
        }

        return value;
    }
    int8_t AudioCodecs(playerinfo_audiocodec_t array[], const uint8_t length) const
    {
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();
        Exchange::IPlayerProperties::IAudioCodecIterator* audioCodecs;
        int8_t value = 0;

        if (impl != nullptr) {

            if (impl->AudioCodecs(audioCodecs) == Core::ERROR_NONE && audioCodecs != nullptr) {

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
            impl->Release();
        }
        return value;
    }

    bool IsAtmosMetadataSupported() const
    {
        bool isSupported = false;

        const Exchange::IPlayerProperties* impl = BaseClass::Interface();
        if (impl != nullptr) {
            const Exchange::Dolby::IOutput* dolby = impl->QueryInterface<const Exchange::Dolby::IOutput>();

            if (dolby != nullptr) {
                if (dolby->AtmosMetadata(isSupported) != Core::ERROR_NONE) {
                    isSupported = false;
                }
                dolby->Release();
            }
            impl->Release();
        }
        return isSupported;
    }

    uint32_t DolbySoundMode(Exchange::Dolby::IOutput::SoundModes& mode) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            const Exchange::Dolby::IOutput* dolby = impl->QueryInterface<const Exchange::Dolby::IOutput>();

            if (dolby != nullptr) {
                errorCode = dolby->SoundMode(mode);
                dolby->Release();
            }
            impl->Release();
        }

        return errorCode;
    }
    uint32_t EnableAtmosOutput(const bool enabled)
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {

            Exchange::Dolby::IOutput* dolby = impl->QueryInterface<Exchange::Dolby::IOutput>();
            if (dolby != nullptr) {
                errorCode = dolby->EnableAtmosOutput(enabled);
                dolby->Release();
            }
            impl->Release();
        }

        return errorCode;
    }
    uint32_t SetDolbyMode(const Exchange::Dolby::IOutput::Type& mode)
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {

            Exchange::Dolby::IOutput* dolby = impl->QueryInterface<Exchange::Dolby::IOutput>();
            if (dolby != nullptr) {
                errorCode = dolby->Mode(mode);
                dolby->Release();
            }
            impl->Release();
        }

        return errorCode;
    }
    uint32_t GetDolbyMode(Exchange::Dolby::IOutput::Type& outMode) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* impl = BaseClass::Interface();

        if (impl != nullptr) {
            const Exchange::Dolby::IOutput* dolby = impl->QueryInterface<const Exchange::Dolby::IOutput>();

            if (dolby != nullptr) {
                errorCode = dolby->Mode(outMode);
                dolby->Release();
            }
            impl->Release();
        }
        return errorCode;
    }
};

std::unique_ptr<PlayerInfo> PlayerInfo::_instance;

} //namespace WPEFramework

using namespace WPEFramework;
extern "C" {

struct playerinfo_type* playerinfo_instance(const char name[])
{
    if (name != NULL) {
        return reinterpret_cast<playerinfo_type*>(PlayerInfo::Instance());
    }
    return NULL;
}

void playerinfo_release(struct playerinfo_type* instance)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->DestroyInstance();
    }
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

uint32_t playerinfo_playback_resolution(struct playerinfo_type* instance, playerinfo_playback_resolution_t* resolution)
{

    if (instance != NULL && resolution != NULL) {
        Exchange::IPlayerProperties::PlaybackResolution value = Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_UNKNOWN;

        if (reinterpret_cast<PlayerInfo*>(instance)->PlaybackResolution(value) == Core::ERROR_NONE) {
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
                return Core::ERROR_UNKNOWN_KEY;
            }
            return Core::ERROR_NONE;
        }
    }
    return Core::ERROR_UNAVAILABLE;
}

uint32_t playerinfo_is_audio_equivalence_enabled(struct playerinfo_type* instance, bool* is_enabled)
{
    if (instance != NULL && is_enabled != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->IsAudioEquivalenceEnabled(*is_enabled);
    }
    return Core::ERROR_UNAVAILABLE;
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

bool playerinfo_is_dolby_atmos_supported(struct playerinfo_type* instance)
{
    if (instance != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->IsAtmosMetadataSupported();
    }
    return false;
}

uint32_t playerinfo_set_dolby_sound_mode(struct playerinfo_type* instance, playerinfo_dolby_sound_mode_t* sound_mode)
{
    if (instance != NULL && sound_mode != NULL) {
        Exchange::Dolby::IOutput::SoundModes value = Exchange::Dolby::IOutput::SoundModes::UNKNOWN;
        if (reinterpret_cast<PlayerInfo*>(instance)->DolbySoundMode(value) == Core::ERROR_NONE) {
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
                return Core::ERROR_UNKNOWN_KEY;
                break;
            }
            return Core::ERROR_NONE;
        }
    }
    return Core::ERROR_UNAVAILABLE;
}
uint32_t playerinfo_enable_atmos_output(struct playerinfo_type* instance, const bool is_enabled)
{
    if (instance != NULL) {
        return reinterpret_cast<PlayerInfo*>(instance)->EnableAtmosOutput(is_enabled);
    }
    return Core::ERROR_UNAVAILABLE;
}

uint32_t playerinfo_set_dolby_mode(struct playerinfo_type* instance, const playerinfo_dolby_mode_t mode)
{
    if (instance != NULL) {
        switch (mode) {
        case PLAYERINFO_DOLBY_MODE_AUTO:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::AUTO);
        case PLAYERINFO_DOLBY_MODE_DIGITAL_PCM:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PCM);
        case PLAYERINFO_DOLBY_MODE_DIGITAL_AC3:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_AC3);
        case PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PLUS);
        case PLAYERINFO_DOLBY_MODE_MS12:
            return reinterpret_cast<PlayerInfo*>(instance)->SetDolbyMode(Exchange::Dolby::IOutput::Type::MS12);
        default:
            fprintf(stderr, "Unknown enum value, not included in playerinfo_dolby_mode_type?\n");
            return Core::ERROR_UNKNOWN_KEY;
        }
    }
    return Core::ERROR_UNAVAILABLE;
}

uint32_t playerinfo_get_dolby_mode(struct playerinfo_type* instance, playerinfo_dolby_mode_t* mode)
{
    if (instance != NULL && mode != NULL) {

        Exchange::Dolby::IOutput::Type value = Exchange::Dolby::IOutput::Type::AUTO;
        if (reinterpret_cast<PlayerInfo*>(instance)->GetDolbyMode(value) == Core::ERROR_NONE) {
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
                return Core::ERROR_UNKNOWN_KEY;
            }
            return Core::ERROR_NONE;
        }
    }
    return Core::ERROR_UNAVAILABLE;
}
}