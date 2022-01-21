/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Module.h"

#include <playerinfo.h>

#include <interfaces/IDolby.h>
#include <interfaces/IPlayerInfo.h>

#include <plugins/Types.h>

namespace WPEFramework {

class PlayerInfo : protected RPC::SmartInterfaceType<Exchange::IPlayerProperties> {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IPlayerProperties>;
    using DolbyModeAudioUpdateCallbacks = std::map<playerinfo_dolby_audio_updated_cb, void*>;
    using OperationalStateChangeCallbacks = std::map<playerinfo_operational_state_change_cb, void*>;

    //CONSTRUCTORS
    PlayerInfo(const string& callsign)
        : BaseClass()
        , _dolbyInterface(nullptr)
        , _callsign(callsign)
        , _dolbyNotification(this)
    {
        ASSERT(_singleton==nullptr);
        _singleton=this;
        BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), callsign);
    }

public:
    PlayerInfo() = delete;
    PlayerInfo(const PlayerInfo&) = delete;
    PlayerInfo& operator=(const PlayerInfo&) = delete;

private:
    //NOTIFICATIONS
    void DolbySoundModeUpdated(VARIABLE_IS_NOT_USED const Exchange::Dolby::IOutput::SoundModes mode, VARIABLE_IS_NOT_USED const bool enabled)
    {
        for (auto& index : _dolbyCallbacks) {
            index.first(index.second);
        }
    }

    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning) {
            if (_playerInterface == nullptr) {
                _playerInterface = BaseClass::Interface();

                if (_playerInterface != nullptr && _dolbyInterface == nullptr) {
                    _dolbyInterface = _playerInterface->QueryInterface<Exchange::Dolby::IOutput>();

                    if (_dolbyInterface != nullptr) {
                        _dolbyInterface->Register(&_dolbyNotification);
                    }
                }
            }
        } else {
            if (_dolbyInterface != nullptr) {
                _dolbyInterface->Unregister(&_dolbyNotification);
                _dolbyInterface->Release();
                _dolbyInterface = nullptr;
            }
            if (_playerInterface != nullptr) {
                _playerInterface->Release();
                _playerInterface = nullptr;
            }
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
    Exchange::IPlayerProperties* _playerInterface;
    Exchange::Dolby::IOutput* _dolbyInterface;
    std::string _callsign;

    DolbyModeAudioUpdateCallbacks _dolbyCallbacks;
    OperationalStateChangeCallbacks _operationalStateCallbacks;
    Core::Sink<Notification> _dolbyNotification;
    static PlayerInfo* _singleton;

public:
    //OBJECT MANAGEMENT
    ~PlayerInfo()
    {
        BaseClass::Close(Core::infinite);
        ASSERT(_singleton!=nullptr);
        _singleton = nullptr;
    }

    static PlayerInfo& Instance()
    {
        PlayerInfo* instance = new PlayerInfo("PlayerInfo");
        ASSERT(instance!=nullptr);
        return *instance;
    }

    static void Dispose()
    {
        ASSERT(_singleton != nullptr);
        if(_singleton != nullptr)
        {
            delete _singleton;
        }
    }

public:
    //METHODS FROM INTERFACE
    const string& Name() const
    {
        return _callsign;
    }
    uint32_t RegisterOperationalStateChangedCallback(playerinfo_operational_state_change_cb callback, void* userdata)
    {
        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index == _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
            return Core::ERROR_NONE;
        }
        return Core::ERROR_GENERAL;
    }

    uint32_t UnregisterOperationalStateChangedCallback(playerinfo_operational_state_change_cb callback)
    {
        OperationalStateChangeCallbacks::iterator index(_operationalStateCallbacks.find(callback));

        if (index != _operationalStateCallbacks.end()) {
            _operationalStateCallbacks.erase(index);
            return Core::ERROR_NONE;
        }
        return Core::ERROR_NOT_EXIST;
    }

    uint32_t RegisterDolbyAudioModeChangedCallback(playerinfo_dolby_audio_updated_cb callback, void* userdata)
    {
        DolbyModeAudioUpdateCallbacks::iterator index(_dolbyCallbacks.find(callback));

        if (index == _dolbyCallbacks.end()) {
            _dolbyCallbacks.emplace(std::piecewise_construct,
                std::forward_as_tuple(callback),
                std::forward_as_tuple(userdata));
            return Core::ERROR_NONE;
        }
        return Core::ERROR_GENERAL;
    }

    uint32_t UnregisterDolbyAudioModeChangedCallback(playerinfo_dolby_audio_updated_cb callback)
    {
        DolbyModeAudioUpdateCallbacks::iterator index(_dolbyCallbacks.find(callback));

        if (index != _dolbyCallbacks.end()) {
            _dolbyCallbacks.erase(index);
            return Core::ERROR_NONE;
        }
        return Core::ERROR_NOT_EXIST;
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
                    std::copy(newArray, newArray + numberOfCodecs, array);
                } else {
                    value = -numberOfCodecs;
                }
                videoCodecs->Release();
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
                    std::copy(newArray, newArray + numberOfCodecs, array);
                } else {
                    value = -numberOfCodecs;
                }
                audioCodecs->Release();
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

PlayerInfo* PlayerInfo::_singleton=nullptr;

} //namespace WPEFramework

using namespace WPEFramework;
extern "C" {

uint32_t playerinfo_register_operational_state_change_callback(
    playerinfo_operational_state_change_cb callback,
    void* userdata)
{
    return PlayerInfo::Instance().RegisterOperationalStateChangedCallback(callback, userdata);
}

uint32_t playerinfo_unregister_operational_state_change_callback(
    playerinfo_operational_state_change_cb callback)
{
    return PlayerInfo::Instance().UnregisterOperationalStateChangedCallback(callback);
}

uint32_t playerinfo_register_dolby_sound_mode_updated_callback(playerinfo_dolby_audio_updated_cb callback, void* userdata)
{

    return PlayerInfo::Instance().RegisterDolbyAudioModeChangedCallback(callback, userdata);
}

uint32_t playerinfo_unregister_dolby_sound_mode_updated_callback(playerinfo_dolby_audio_updated_cb callback)
{

    return PlayerInfo::Instance().UnregisterDolbyAudioModeChangedCallback(callback);
}

void playerinfo_name(char buffer[], const uint8_t length)
{
    strncpy(buffer, PlayerInfo::Instance().Name().c_str(), length);
}

uint32_t playerinfo_playback_resolution(playerinfo_playback_resolution_t* resolution)
{
    if (resolution != nullptr) {
        Exchange::IPlayerProperties::PlaybackResolution value = Exchange::IPlayerProperties::PlaybackResolution::RESOLUTION_UNKNOWN;
        *resolution = PLAYERINFO_RESOLUTION_UNKNOWN;

        if (PlayerInfo::Instance().PlaybackResolution(value) == Core::ERROR_NONE) {
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
                TRACE_GLOBAL(Trace::Warning, ("New resolution in the interface, not handled in client library!"));
                *resolution = PLAYERINFO_RESOLUTION_UNKNOWN;
                return Core::ERROR_UNKNOWN_KEY;
            }
            return Core::ERROR_NONE;
        }
    }
    return Core::ERROR_UNAVAILABLE;
}

uint32_t playerinfo_is_audio_equivalence_enabled(bool* is_enabled)
{
    return (is_enabled != nullptr) ? PlayerInfo::Instance().IsAudioEquivalenceEnabled(*is_enabled) : Core::ERROR_UNAVAILABLE;
}

int8_t playerinfo_video_codecs(playerinfo_videocodec_t array[], const uint8_t length)
{
    return PlayerInfo::Instance().VideoCodecs(array, length);
}

int8_t playerinfo_audio_codecs(playerinfo_audiocodec_t array[], const uint8_t length)
{
    return PlayerInfo::Instance().AudioCodecs(array, length);
}

bool playerinfo_is_dolby_atmos_supported()
{
    return PlayerInfo::Instance().IsAtmosMetadataSupported();
}

uint32_t playerinfo_set_dolby_sound_mode(playerinfo_dolby_sound_mode_t* sound_mode)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (sound_mode != nullptr) {
        Exchange::Dolby::IOutput::SoundModes value = Exchange::Dolby::IOutput::SoundModes::UNKNOWN;
        if (PlayerInfo::Instance().DolbySoundMode(value) == Core::ERROR_NONE) {
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
                TRACE_GLOBAL(Trace::Warning, ("New dolby sound mode in the interface, not handled in client library!"));
                *sound_mode = PLAYERINFO_DOLBY_SOUND_UNKNOWN;
                result = Core::ERROR_UNKNOWN_KEY;
                break;
            }
            result = Core::ERROR_NONE;
        }
    }

    return result;
}
uint32_t playerinfo_enable_atmos_output(const bool is_enabled)
{
    return PlayerInfo::Instance().EnableAtmosOutput(is_enabled);
}

uint32_t playerinfo_set_dolby_mode(const playerinfo_dolby_mode_t mode)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    switch (mode) {
    case PLAYERINFO_DOLBY_MODE_AUTO:
        result = PlayerInfo::Instance().SetDolbyMode(Exchange::Dolby::IOutput::Type::AUTO);
        break;
    case PLAYERINFO_DOLBY_MODE_DIGITAL_PCM:
        result = PlayerInfo::Instance().SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PCM);
        break;
    case PLAYERINFO_DOLBY_MODE_DIGITAL_AC3:
        result = PlayerInfo::Instance().SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_AC3);
        break;
    case PLAYERINFO_DOLBY_MODE_DIGITAL_PLUS:
        result = PlayerInfo::Instance().SetDolbyMode(Exchange::Dolby::IOutput::Type::DIGITAL_PLUS);
        break;
    case PLAYERINFO_DOLBY_MODE_MS12:
        result = PlayerInfo::Instance().SetDolbyMode(Exchange::Dolby::IOutput::Type::MS12);
        break;
    default:
        TRACE_GLOBAL(Trace::Warning, ("Unknown enum value, not included in playerinfo_dolby_mode_type?"));
        result = Core::ERROR_UNKNOWN_KEY;
        break;
    }

    return result;
}

uint32_t playerinfo_get_dolby_mode(playerinfo_dolby_mode_t* mode)
{
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (mode != nullptr) {
        Exchange::Dolby::IOutput::Type value = Exchange::Dolby::IOutput::Type::AUTO;
        *mode = PLAYERINFO_DOLBY_MODE_AUTO;

        if (PlayerInfo::Instance().GetDolbyMode(value) == Core::ERROR_NONE) {
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
                TRACE_GLOBAL(Trace::Warning, ("New dolby mode in the interface, not handled in client library!"));

                *mode = PLAYERINFO_DOLBY_MODE_AUTO;
                result = Core::ERROR_UNKNOWN_KEY;
                break;
            }
            result = Core::ERROR_NONE;
        }
    }
    return result;
}

void playerinfo_dispose() {
    PlayerInfo::Dispose();
}
}
