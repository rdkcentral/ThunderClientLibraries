#include <com/com.h>
#include <core/core.h>
#include <plugins/Types.h>

#include <stdlib.h>
#include <tracing/tracing.h>

#include <interfaces/IDolby.h>
#include <interfaces/IPlayerInfo.h>
#include <playerinfo.h>

#include <interfaces/IDictionary.h>

namespace WPEFramework {

class PlayerInfo : protected RPC::SmartInterfaceType<Exchange::IPlayerProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPlayerProperties>;
    using DolbyModeAudioUpdateCallbacks = std::map<playerinfo_dolby_audio_updated_cb, void*>;
    using OperationalStateChangeCallbacks = std::map<playerinfo_operational_state_change_cb, void*>;

    //CONSTRUCTORS
    PlayerInfo(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass()
        , _playerInterface(nullptr)
        , _dolbyInterface(nullptr)
        , _callsign(callsign)
        , _dolbyNotification(this)
    {
        BaseClass::Open(waitTime, node, callsign);
    }

    PlayerInfo() = delete;
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

private:
    //NOTIFICATIONS
    void DolbySoundModeUpdated(const Exchange::Dolby::IOutput::SoundModes mode, const bool enabled)
    {
        for (auto& index : _dolbyCallbacks) {
            index.first(index.second);
        }
    }

    void Operational(const bool upAndRunning) override
    {
        switch (upAndRunning) {
        case true:

            if (_playerInterface == nullptr) {
                _playerInterface = BaseClass::Interface();

                if (_playerInterface != nullptr && _dolbyInterface == nullptr) {
                    _dolbyInterface = _playerInterface->QueryInterface<Exchange::Dolby::IOutput>();

                    if (_dolbyInterface != nullptr) {
                        _dolbyInterface->AddRef();
                        _dolbyInterface->Register(&_dolbyNotification);
                    }
                }
            }
            break;

        case false:
        default:
            if (_dolbyInterface != nullptr) {
                _dolbyInterface->Unregister(&_dolbyNotification);
                _dolbyInterface->Release();
                _dolbyInterface = nullptr;
            }
            if (_playerInterface != nullptr) {
                _playerInterface->Release();
                _playerInterface = nullptr;
            }
            break;
        }

        for (auto& index : _operationalStateCallbacks) {
            index.first(upAndRunning, index.second);
        }
    }

    class Notification : public Exchange::Dolby::IOutput::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

        explicit Notification(PlayerInfo* parent)
            : _parent(*parent)
        {
        }

        void AudioModeChanged(const Exchange::Dolby::IOutput::SoundModes mode, const bool enabled) override
        {
            _parent.DolbySoundModeUpdated(mode, enabled);
        }

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::Dolby::IOutput::INotification)
        END_INTERFACE_MAP

    private:
        PlayerInfo& _parent;
    };

private:
    //MEMBERS
    static PlayerInfo* _instance;
    Exchange::IPlayerProperties* _playerInterface;
    Exchange::Dolby::IOutput* _dolbyInterface;
    std::string _callsign;

    DolbyModeAudioUpdateCallbacks _dolbyCallbacks;
    OperationalStateChangeCallbacks _operationalStateCallbacks;
    Core::Sink<Notification> _dolbyNotification;

public:
    //OBJECT MANAGEMENT
    ~PlayerInfo()
    {
        BaseClass::Close(Core::infinite);
    }

    static PlayerInfo* Instance()
    {
        if (_instance == nullptr) {
            _instance = new PlayerInfo(3000, Connector(), "PlayerInfo");
        }
        return _instance;
    }
    void DestroyInstance()
    {
        delete _instance;
        _instance = nullptr;
    }

public:
    //METHODS FROM INTERFACE
    const string& Name() const
    {
        return _callsign;
    }
    void RegisterOperationalStateChangedCallback(playerinfo_operational_state_change_cb callback, void* userdata)
    {
        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index == _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
        }
    }
    void UnregisterOperationalStateChangedCallback(playerinfo_operational_state_change_cb callback)
    {
        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index != _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.erase(index);
        }
    }

    void RegisterDolbyAudioModeChangedCallback(playerinfo_dolby_audio_updated_cb callback, void* userdata)
    {
        DolbyModeAudioUpdateCallbacks::iterator index(_dolbyCallbacks.find(callback));

        if (index == _dolbyCallbacks.end()) {
            _dolbyCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
        }
    }

    void UnregisterDolbyAudioModeChangedCallback(playerinfo_dolby_audio_updated_cb callback)
    {
        DolbyModeAudioUpdateCallbacks::iterator index(_dolbyCallbacks.find(callback));

        if (index != _dolbyCallbacks.end()) {
            _dolbyCallbacks.erase(index);
        }
    }

    uint32_t IsAudioEquivalenceEnabled(bool& outIsEnabled) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* constPlayerInterface = _playerInterface;

        if (constPlayerInterface != nullptr) {
            errorCode = _playerInterface->IsAudioEquivalenceEnabled(outIsEnabled);
        }

        return errorCode;
    }

    uint32_t PlaybackResolution(Exchange::IPlayerProperties::PlaybackResolution& outResolution) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::IPlayerProperties* constPlayerInterface = _playerInterface;

        if (constPlayerInterface != nullptr) {
            errorCode = constPlayerInterface->Resolution(outResolution);
        }

        return errorCode;
    }

    int8_t VideoCodecs(playerinfo_videocodec_t array[], const uint8_t length) const
    {
        const Exchange::IPlayerProperties* constPlayerInterface = _playerInterface;
        Exchange::IPlayerProperties::IVideoCodecIterator* videoCodecs;
        int8_t value = 0;

        if (constPlayerInterface != nullptr) {
            if (constPlayerInterface->VideoCodecs(videoCodecs) == Core::ERROR_NONE && videoCodecs != nullptr) {

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
        }

        return value;
    }
    int8_t AudioCodecs(playerinfo_audiocodec_t array[], const uint8_t length) const
    {
        const Exchange::IPlayerProperties* constPlayerInterface = _playerInterface;
        Exchange::IPlayerProperties::IAudioCodecIterator* audioCodecs;
        int8_t value = 0;

        if (constPlayerInterface != nullptr) {

            if (constPlayerInterface->AudioCodecs(audioCodecs) == Core::ERROR_NONE && audioCodecs != nullptr) {

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
        }
        return value;
    }

    bool IsAtmosMetadataSupported() const
    {
        bool isSupported = false;

        const Exchange::Dolby::IOutput* constDolby = _dolbyInterface;
        if (constDolby != nullptr) {
            if (constDolby->AtmosMetadata(isSupported) != Core::ERROR_NONE) {
                isSupported = false;
            }
        }

        return isSupported;
    }

    uint32_t DolbySoundMode(Exchange::Dolby::IOutput::SoundModes& mode) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;

        const Exchange::Dolby::IOutput* constDolby = _dolbyInterface;

        if (constDolby != nullptr) {
            errorCode = constDolby->SoundMode(mode);
        }

        return errorCode;
    }
    uint32_t EnableAtmosOutput(const bool enabled)
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;

        if (_dolbyInterface != nullptr) {
            errorCode = _dolbyInterface->EnableAtmosOutput(enabled);
        }

        return errorCode;
    }
    uint32_t SetDolbyMode(const Exchange::Dolby::IOutput::Type& mode)
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;

        if (_dolbyInterface != nullptr) {
            errorCode = _dolbyInterface->Mode(mode);
        }

        return errorCode;
    }
    uint32_t GetDolbyMode(Exchange::Dolby::IOutput::Type& outMode) const
    {
        uint32_t errorCode = Core::ERROR_UNAVAILABLE;
        const Exchange::Dolby::IOutput* constDolby = _dolbyInterface;

        if (constDolby != nullptr) {
            errorCode = constDolby->Mode(outMode);
        }

        return errorCode;
    }
};

PlayerInfo* PlayerInfo::_instance = nullptr;

} //namespace WPEFramework

using namespace WPEFramework;
extern "C" {

struct playerinfo_type* playerinfo_instance()
{
    return reinterpret_cast<playerinfo_type*>(PlayerInfo::Instance());
}

void playerinfo_release(struct playerinfo_type* instance)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->DestroyInstance();
    }
}

void playerinfo_register_operational_state_change_callback(struct playerinfo_type* instance,
    playerinfo_operational_state_change_cb callback,
    void* userdata)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->RegisterOperationalStateChangedCallback(callback, userdata);
    }
}

void playerinfo_unregister_operational_state_change_callback(struct playerinfo_type* instance,
    playerinfo_operational_state_change_cb callback)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->UnregisterOperationalStateChangedCallback(callback);
    }
}

void playerinfo_register_dolby_sound_mode_updated_callback(struct playerinfo_type* instance, playerinfo_dolby_audio_updated_cb callback, void* userdata)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->RegisterDolbyAudioModeChangedCallback(callback, userdata);
    }
}

void playerinfo_unregister_dolby_sound_mode_updated_callback(struct playerinfo_type* instance, playerinfo_dolby_audio_updated_cb callback)
{
    if (instance != NULL) {
        reinterpret_cast<PlayerInfo*>(instance)->UnregisterDolbyAudioModeChangedCallback(callback);
    }
}

void playerinfo_name(struct playerinfo_type* instance, char buffer[], const uint8_t length)
{
    string name = reinterpret_cast<PlayerInfo*>(instance)->Name();
    strncpy(buffer, name.c_str(), length);
}

uint32_t playerinfo_playback_resolution(struct playerinfo_type* instance, playerinfo_playback_resolution_t* resolution)
{

    if (instance != NULL && resolution != NULL) {
        Exchange::IPlayerProperties::PlaybackResolution value = Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_UNKNOWN;
        *resolution = PLAYERINFO_RESOLUTION_UNKNOWN;

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
        *mode = PLAYERINFO_DOLBY_MODE_AUTO;

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