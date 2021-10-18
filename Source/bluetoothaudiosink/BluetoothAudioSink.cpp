/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#define __DEBUG__  // TODO: Remove this eventually

#include "Module.h"
#include "include/bluetoothaudiosink.h"
#include <interfaces/IBluetoothAudio.h>


#define TRACE(format, ...) fprintf(stderr, "bluetoothaudiosink: " format "\n", ##__VA_ARGS__)


namespace WPEFramework {

namespace BluetoothAudioSinkClient {

    class AudioSink : protected RPC::SmartInterfaceType<Exchange::IBluetoothAudioSink> {
    private:
        class Player : public Core::Thread {
        private:
            class SendBuffer : public Core::SharedBuffer {
            public:
                SendBuffer() = delete;
                SendBuffer(const SendBuffer&) = delete;
                SendBuffer& operator=(const SendBuffer&) = delete;

                SendBuffer(const char *connector, const uint32_t maxBufferSize)
                    : Core::SharedBuffer(connector, 0777, maxBufferSize, 0)
                    , _maxBufferSize(maxBufferSize)
                {
                }

                ~SendBuffer() = default;

            public:
                uint16_t Write(uint32_t length, uint8_t data[])
                {
                    ASSERT(IsValid() == true);
                    ASSERT(data != nullptr);

                    uint16_t result = 0;

                    if (RequestProduce(100) == Core::ERROR_NONE) {
                        result = std::min(length, _maxBufferSize);
                        Size(result);
                        ::memcpy(Buffer(), data, result);
                        Produced();
                    }

                    return (result);
                }

            private:
                uint32_t _maxBufferSize;
            }; // class SendBuffer

        public:
            Player(const char* connector, const uint32_t maxFrameSize, uint16_t bufferSize)
                : _lock()
                , _sendBuffer(connector, maxFrameSize)
                , _buffer(nullptr)
                , _capacity(0)
                , _length(0)
                , _EOS(false)
            {
                ASSERT(connector != nullptr);
                ASSERT(maxFrameSize >= 256);
                ASSERT(bufferSize >= 2);

                if ((maxFrameSize != 0) && (bufferSize != 0)) {
                    _capacity = (maxFrameSize * bufferSize);
                    _buffer = static_cast<uint8_t*>(::malloc(_capacity));
                    ASSERT(_buffer != nullptr);

                    TRACE_L1("Player created");
                }

                if (_buffer == nullptr) {
                    TRACE("Failed to allocate a buffer for the player!");
                }
            }
            ~Player() override
            {
                Stop();
                ::free(_buffer);
                TRACE_L1("Player destroyed");
            }

            Player(const Player&) = delete;
            Player& operator=(const Player&) = delete;

        public:
            bool IsValid() const
            {
                return ((_sendBuffer.IsValid() == true) && (_buffer != nullptr));
            }
            bool Play()
            {
                if (IsRunning() == false) {
                    _EOS = false;
                    Run();
                    TRACE_L1("Player started");
                    return (true);
                } else {
                    return (false);
                }
            }
            bool Stop()
            {
                if (IsRunning() == true) {
                    _EOS = true;
                    Thread::Wait(BLOCKED, Core::infinite);
                    TRACE_L1("Player stopped");
                    return (true);
                } else {
                    return (false);
                }
            }
            uint16_t Frame(const uint16_t length, const uint8_t data[])
            {
                ASSERT(data != nullptr);

                uint16_t result = 0;

                _lock.Lock();

                if (_length + length <= _capacity) {
                    ::memcpy(&_buffer[_length], data, length);
                    _length += length;
                    result = length;
                }

                _lock.Unlock();

                return (result);
            }

        private:
            uint32_t Worker()
            {
                uint32_t delay = 0;

                _lock.Lock();

                if (_EOS == true) {
                    Thread::Block();
                    delay = Core::infinite;
                } else if (_length != 0) {
                    uint16_t written = _sendBuffer.Write(_length, _buffer);
                    if (written != 0) {
                        // TODO: opportunity for optimisation
                        ::memmove(_buffer, (_buffer + written), (_length - written));
                        _length -= written;
                    }
                }

                _lock.Unlock();

                return (delay);
            };

        private:
            Core::CriticalSection _lock;
            SendBuffer _sendBuffer;
            uint8_t* _buffer;
            uint32_t _capacity;
            uint32_t _length;
            bool _EOS;
        }; // class Player

    private:
        using OperationalStateUpdateCallbacks = std::map<const bluetoothaudiosink_operational_state_update_cb, void*>;
        using SinkStateUpdateCallbacks = std::map<const bluetoothaudiosink_state_update_cb, void *>;
        friend class WPEFramework::Core::SingletonType<AudioSink>;

    private:
        AudioSink(const string& callsign)
            : SmartInterfaceType()
            , _lock()
            , _operationalStateCallbacks()
            , _sinkStateCallbacks()
            , _sink(nullptr)
            , _sinkControl(nullptr)
            , _callback(*this)
            , _player(nullptr)
        {
            if (SmartInterfaceType::Open(RPC::CommunicationTimeOut, SmartInterfaceType::Connector(), callsign) != Core::ERROR_NONE) {
                TRACE("Failed to open the smart interface!");
            }
        }

    public:
        AudioSink() = delete;
        AudioSink(const AudioSink&) = delete;
        AudioSink& operator=(const AudioSink&) = delete;

        ~AudioSink() override
        {
            if (_player != nullptr) {
                delete _player;
                _player = nullptr;
            }

            if (_sinkControl != nullptr) {
                _sinkControl->Release();
                _sinkControl = nullptr;
            }

            if (_sink != nullptr) {
                _sink->Callback(nullptr);
                _sink->Release();
                _sink = nullptr;
            }

            SmartInterfaceType::Close(Core::infinite);
        }

    private:
        class Callback : public Exchange::IBluetoothAudioSink::ICallback {
        public:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator=(const Callback&) = delete;

            Callback(AudioSink& parent)
                : _parent(parent)
            {
            }

            ~Callback() = default;

        public:
            void Updated() override
            {
                _parent.SinkUpdated();
            }

        public:
            BEGIN_INTERFACE_MAP(Callback)
            INTERFACE_ENTRY(Exchange::IBluetoothAudioSink::ICallback)
            END_INTERFACE_MAP

        private:
            AudioSink& _parent;
        }; // class Callback

    private:
        void SinkUpdated()
        {
            _lock.Lock();

            auto callbacks = _sinkStateCallbacks;

            _lock.Unlock();

            for (auto& cb : callbacks) {
                cb.first(cb.second);
            }
        }

    private:
        void Operational(const bool upAndRunning) override
        {
            _lock.Lock();

            if (upAndRunning == true) {
                TRACE_L1("Bluetooth audio sink is now operational!");

                if (_sink == nullptr) {
                    _sink = SmartInterfaceType::Interface();

                    if (_sink == nullptr) {
                        TRACE("Failed to retrieve the IBluetoothAudioSink interface!");
                    } else {
                        if (_sink->Callback(&_callback) != Core::ERROR_NONE) {
                            TRACE("Failed to register the sink callback!");
                        } else {
                            TRACE_L1("Successfully registered the sink callback!");
                        }

                        if (_sinkControl == nullptr) {
                            _sinkControl = _sink->QueryInterface<Exchange::IBluetoothAudioSink::IControl>();

                            if (_sinkControl == nullptr) {
                                TRACE("Failed to retrieve the IBluetoothAudioSink::IControl interface!");
                            } else {
                                TRACE_L1("Successfully retrieved IBluetoothAudioSink and IControl intefaces!");
                            }
                        }
                    }

                }
            }

            auto callbacks = _operationalStateCallbacks;

           _lock.Unlock();

            for (auto& cb : callbacks) {
                cb.first(upAndRunning, cb.second);
            }

            if (upAndRunning == false) {
                TRACE_L1("Bluetooth audio sink is no longer operational!");

                 _lock.Lock();

                if (_player != nullptr) {
                    delete _player;
                    _player = nullptr;
                }

                if (_sinkControl != nullptr) {
                    _sinkControl->Release();
                    _sinkControl = nullptr;
                }

                if (_sink != nullptr) {
                    _sink->Callback(nullptr);
                    _sink->Release();
                    _sink = nullptr;
                }

                _lock.Unlock();
            }
        }

    public:
        static AudioSink& Instance()
        {
            return Core::SingletonType<AudioSink>::Instance("BluetoothAudioSink");
        }

    public:
        uint32_t RegisterOperationalStateUpdateCallback(const bluetoothaudiosink_operational_state_update_cb callback, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(callback != nullptr);

            _lock.Lock();

            if (_operationalStateCallbacks.find(callback) == _operationalStateCallbacks.end()) {
                _operationalStateCallbacks.emplace(std::piecewise_construct, std::forward_as_tuple(callback), std::forward_as_tuple(user_data));
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            if (result == Core::ERROR_NONE) {
                TRACE_L1("Successfully registered an operational state callback");

                // Call the callback immediately if the service is operational already.
                if (SmartInterfaceType::IsOperational() == true) {
                    callback(true, user_data);
                }
            } else {
                TRACE_L1("Failed to register an operational state callback");
            }

            return (result);
        }

        uint32_t UnregisterOperationalStateUpdateCallback(const bluetoothaudiosink_operational_state_update_cb callback)
        {
            uint32_t result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(callback != nullptr);

            _lock.Lock();

            auto it = _operationalStateCallbacks.find(callback);
            if (it != _operationalStateCallbacks.end()) {
                _operationalStateCallbacks.erase(it);
                result = Core::ERROR_NONE;
            } else {
                TRACE_L1("Failed to unregister an operational state callback");
            }

            _lock.Unlock();

            return (result);
        }

    public:
        uint32_t RegisterSinkStateUpdateCallback(const bluetoothaudiosink_state_update_cb callback, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(callback != nullptr);

            _lock.Lock();

            if (_sinkStateCallbacks.find(callback) == _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.emplace(std::piecewise_construct, std::forward_as_tuple(callback), std::forward_as_tuple(user_data));
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            if (result == Core::ERROR_NONE) {
                callback(user_data);
            } else {
                TRACE_L1("Failed to register a sink state callback");
            }

            return (result);
        }

        uint32_t UnregisterSinkStateUpdateCallback(const bluetoothaudiosink_state_update_cb callback)
        {
            uint32_t result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(callback != nullptr);

            _lock.Lock();

            auto it = _sinkStateCallbacks.find(callback);
            if (it != _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.erase(it);
                result = Core::ERROR_NONE;
            } else {
                TRACE_L1("Failed to unregister a sink state callback");
            }

            _lock.Unlock();

            return (result);
        }

    public:
        uint32_t State(Exchange::IBluetoothAudioSink::state& state) const
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sink != nullptr) {
                result = _sink->State(state);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("State() failed! [%i]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Acquire(const char* connector, const Exchange::IBluetoothAudioSink::IControl::Format& format, const uint16_t bufferSize)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                const uint32_t maxFrameSize = (format.Resolution * format.Channels * format.SampleRate) / (format.FrameRate * 8);
                _player = new Player(connector, maxFrameSize, bufferSize);
                ASSERT(_player != nullptr);

                if (_player != nullptr) {
                    if (_player->IsValid() == false) {
                        TRACE_L1("Created player is not valid");
                        delete _player;
                        _player = nullptr;
                    }
                }

                if (_player != nullptr) {
                    result = _sinkControl->Acquire(connector, format);
                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("Acquire() failed! [%i]", result);
                        delete _player;
                        _player = nullptr;
                    } else {
                        TRACE_L1("A client acquired the audio sink");
                    }
                } else {
                    result = Core::ERROR_OPENING_FAILED;
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Relinquish()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                if (_player != nullptr) {
                    delete _player;
                    _player = nullptr;
                }

                result = _sinkControl->Relinquish();
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Relinquish failed! [%i]", result);
                } else {
                    TRACE_L1("A client released the audio sink");
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Speed(const int8_t speed)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sinkControl != nullptr) && (_player != nullptr)) {
                result = _sinkControl->Speed(speed);
                if (result == Core::ERROR_NONE) {
                    if (speed == 0) {
                        if (_player->Stop() == false) {
                            TRACE_L1("Failed to suspend playback");
                            result = Core::ERROR_GENERAL;
                        }
                    } else if (speed == 100) {
                        if (_player->Play() == false) {
                            TRACE_L1("Failed to resume playback");
                            result = Core::ERROR_GENERAL;
                        }
                    } else {
                       result = Core::ERROR_NOT_SUPPORTED;
                    }
                } else {
                    TRACE_L1("Speed(%i) failed [%i]", speed, result);
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Frame(const uint16_t length, const uint8_t data[], uint16_t& consumed)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sinkControl != nullptr) && (_player != nullptr)) {
                consumed = _player->Frame(length, data);
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Time(uint32_t& timeMs)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sinkControl != nullptr) && (_player != nullptr)) {
                result = _sinkControl->Time(timeMs);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Time() failed! [%i]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

    private:
        mutable Core::CriticalSection _lock;
        OperationalStateUpdateCallbacks _operationalStateCallbacks;
        SinkStateUpdateCallbacks _sinkStateCallbacks;
        Exchange::IBluetoothAudioSink* _sink;
        Exchange::IBluetoothAudioSink::IControl* _sinkControl;
        Core::Sink<Callback> _callback;
        Player* _player;
    };

} // namespace BluetoothAudioSinkClient

}


using namespace WPEFramework;

extern "C" {

uint32_t bluetoothaudiosink_register_operational_state_update_callback(const bluetoothaudiosink_operational_state_update_cb callback, void *user_data)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().RegisterOperationalStateUpdateCallback(callback, user_data));
    }
}

uint32_t bluetoothaudiosink_unregister_operational_state_update_callback(const bluetoothaudiosink_operational_state_update_cb callback)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().UnregisterOperationalStateUpdateCallback(callback));
    }
}

uint32_t bluetoothaudiosink_register_state_update_callback(const bluetoothaudiosink_state_update_cb callback, void *user_data)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().RegisterSinkStateUpdateCallback(callback, user_data));
    }
}

uint32_t bluetoothaudiosink_unregister_state_update_callback(const bluetoothaudiosink_state_update_cb callback)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().UnregisterSinkStateUpdateCallback(callback));
    }
}

uint32_t bluetoothaudiosink_state(bluetoothaudiosink_state_t* state)
{
    if (state == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        (*state) = BLUETOOTHAUDIOSINK_STATE_UNKNOWN;

        Exchange::IBluetoothAudioSink::state newState;
        uint32_t result = BluetoothAudioSinkClient::AudioSink::Instance().State(newState);
        if (result == Core::ERROR_NONE) {
            switch (newState) {
            case Exchange::IBluetoothAudioSink::UNASSIGNED:
                (*state) = BLUETOOTHAUDIOSINK_STATE_UNASSIGNED;
                break;
            case Exchange::IBluetoothAudioSink::DISCONNECTED:
                (*state) = BLUETOOTHAUDIOSINK_STATE_DISCONNECTED;
                break;
            case Exchange::IBluetoothAudioSink::CONNECTED:
                (*state) = BLUETOOTHAUDIOSINK_STATE_CONNECTED;
                break;
            case Exchange::IBluetoothAudioSink::CONNECTED_BAD_DEVICE:
                (*state) = BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD_DEVICE;
                break;
            case Exchange::IBluetoothAudioSink::CONNECTED_RESTRICTED:
                (*state) = BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED;
                break;
            case Exchange::IBluetoothAudioSink::READY:
                (*state) = BLUETOOTHAUDIOSINK_STATE_READY;
                break;
            case Exchange::IBluetoothAudioSink::STREAMING:
                (*state) = BLUETOOTHAUDIOSINK_STATE_STREAMING;
                break;
            default:
                break;
            }
        }

        return (result);
    }
}

uint32_t bluetoothaudiosink_acquire(const char *connector, const bluetoothaudiosink_format_t *format, const uint16_t buffer_size_in_frames)
{
    if ((connector == nullptr) || (format == nullptr)) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Acquire(connector, {format->sample_rate, format->frame_rate, format->resolution, format->channels}, buffer_size_in_frames));
    }
}

uint32_t bluetoothaudiosink_relinquish(void)
{
    return (BluetoothAudioSinkClient::AudioSink::Instance().Relinquish());
}

uint32_t bluetoothaudiosink_speed(const int8_t speed)
{
    return (BluetoothAudioSinkClient::AudioSink::Instance().Speed(speed));
}

uint32_t bluetoothaudiosink_time(uint32_t* time_ms)
{
    if (time_ms == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Time(*time_ms));
    }
}

uint32_t bluetoothaudiosink_frame(const uint16_t length, const uint8_t data[], uint16_t* consumed)
{
    if ((data == nullptr) || (consumed == nullptr)) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Frame(length, data, *consumed));
    }
}

uint32_t bluetoothaudiosink_dispose(void)
{
    Core::Singleton::Dispose();
    return (Core::ERROR_NONE);
}

} // extern "C"
