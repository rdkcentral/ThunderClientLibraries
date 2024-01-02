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


#define CONNECTOR _T("/tmp/bluetoothaudiosink")

#define PRINT(format, ...) fprintf(stderr, _T("bluetoothaudiosink: ") format _T("\n"), ##__VA_ARGS__)


namespace WPEFramework {

static bluetoothaudiosink_state_t ConvertState(const Exchange::IBluetoothAudio::state state)
{
    bluetoothaudiosink_state_t out_state = BLUETOOTHAUDIOSINK_STATE_UNASSIGNED;

    switch (state) {
    case Exchange::IBluetoothAudio::UNASSIGNED:
        out_state = BLUETOOTHAUDIOSINK_STATE_UNASSIGNED;
        break;
    case Exchange::IBluetoothAudio::DISCONNECTED:
        out_state = BLUETOOTHAUDIOSINK_STATE_DISCONNECTED;
        break;
    case Exchange::IBluetoothAudio::CONNECTING:
        out_state = BLUETOOTHAUDIOSINK_STATE_CONNECTING;
        break;
    case Exchange::IBluetoothAudio::CONNECTED:
        out_state = BLUETOOTHAUDIOSINK_STATE_CONNECTED;
        break;
    case Exchange::IBluetoothAudio::CONNECTED_BAD:
        out_state = BLUETOOTHAUDIOSINK_STATE_CONNECTED_BAD;
        break;
    case Exchange::IBluetoothAudio::CONNECTED_RESTRICTED:
        out_state = BLUETOOTHAUDIOSINK_STATE_CONNECTED_RESTRICTED;
        break;
    case Exchange::IBluetoothAudio::READY:
        out_state = BLUETOOTHAUDIOSINK_STATE_READY;
        break;
    case Exchange::IBluetoothAudio::STREAMING:
        out_state = BLUETOOTHAUDIOSINK_STATE_STREAMING;
        break;
    default:
        out_state = BLUETOOTHAUDIOSINK_STATE_UNASSIGNED;
        ASSERT(!"Unmapped state enum!");
        break;
    }

    return (out_state);
}

namespace BluetoothAudioSinkClient {

    class AudioSink : protected RPC::SmartInterfaceType<Exchange::IBluetoothAudio::ISink> {
    private:
        static constexpr uint32_t WriteTimeout = 50;

    private:
        class SendBuffer : public Core::SharedBuffer {
        public:
            SendBuffer() = delete;
            SendBuffer(const SendBuffer&) = delete;
            SendBuffer& operator=(const SendBuffer&) = delete;

            SendBuffer(const char *connector, const uint32_t bufferSize)
                : Core::SharedBuffer(connector, 0777, bufferSize, 0)
                , _bufferSize(bufferSize)
            {
            }

            ~SendBuffer() = default;

        public:
            uint16_t Write(uint32_t length, const uint8_t data[])
            {
                ASSERT(IsValid() == true);
                ASSERT(data != nullptr);

                uint16_t result = 0;

                if (RequestProduce(WriteTimeout) == Core::ERROR_NONE) {
                    result = std::min(length, _bufferSize);
                    Size(result);
                    ::memcpy(Buffer(), data, result);
                    Produced();
                }

                return (result);
            }

        private:
            uint32_t _bufferSize;
        }; // class SendBuffer

    private:
        using OperationalStateUpdateCallbacks = std::map<const bluetoothaudiosink_operational_state_update_cb, void*>;
        using SinkStateChangedCallbacks = std::map<const bluetoothaudiosink_state_changed_cb, void *>;

    private:
        AudioSink(const string& callsign)
            : SmartInterfaceType()
            , _callback(*this)
            , _lock()
            , _operationalStateCallbacks()
            , _sinkStateCallbacks()
            , _sinkLock()
            , _sink(nullptr)
            , _sinkControl(nullptr)
            , _frameSize(0)
            , _buffer()
        {
            TRACE_L5("Bluetooth audio sink is being constructed...");

            ASSERT(_singleton == nullptr);
            _singleton = this;

            if (SmartInterfaceType::Open(RPC::CommunicationTimeOut, SmartInterfaceType::Connector(), callsign) != Core::ERROR_NONE) {
                PRINT(_T("Failed to open the smart interface!"));
            } else {
                TRACE_L1("Opened smart interface ('%s')", callsign.c_str());
            }

            TRACE_L5("Bluetooth audio sink constructed");
        }

    public:
        AudioSink() = delete;
        AudioSink(const AudioSink&) = delete;
        AudioSink& operator=(const AudioSink&) = delete;

    private:
        ~AudioSink() override
        {
            TRACE_L5("Bluetooth audio sink is being destructed...");

            Relinquish();

            _sinkLock.Lock();

            _buffer.reset();

            _sinkLock.Unlock();

            _lock.Lock();

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

            SmartInterfaceType::Close(Core::infinite);

            ASSERT(_singleton != nullptr);
            _singleton = nullptr;

            TRACE_L5("Bluetooth audio sink destructed");
        }

    private:
        class Callback : public Exchange::IBluetoothAudio::ISink::ICallback {
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
            void StateChanged(const Exchange::IBluetoothAudio::state state) override
            {
                _parent.StateChanged(state);
            }

        public:
            BEGIN_INTERFACE_MAP(Callback)
            INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISink::ICallback)
            END_INTERFACE_MAP

        private:
            AudioSink& _parent;
        }; // class Callback

    private:
        void StateChanged(const Exchange::IBluetoothAudio::state state)
        {
            TRACE_L5("State changed to %d", state);

            _lock.Lock();

            for (auto& cb : _sinkStateCallbacks) {
                cb.first(ConvertState(state), cb.second);
            }

            _lock.Unlock();
        }

    private:
        void Operational(const bool upAndRunning) override
        {
            _lock.Lock();

            if (upAndRunning == true) {
                TRACE_L1("Bluetooth audio sink is now operational!");

                if (_sink == nullptr) {
                    _sink = SmartInterfaceType::Interface();

                    ASSERT(_sink != nullptr);

                    if (_sink == nullptr) {
                        PRINT(_T("Failed to retrieve IBluetoothAudio::ISink interface!"));
                    }
                    else if (_sink->Callback(&_callback) != Core::ERROR_NONE) {
                        PRINT(_T("Failed to register the sink callback!"));
                    }
                    else if (_sinkControl == nullptr) {
                        _sinkControl = _sink->QueryInterface<Exchange::IBluetoothAudio::IStream>();

                        ASSERT(_sinkControl != nullptr);

                        if (_sinkControl == nullptr) {
                            PRINT(_T("Failed to retrieve IBluetoothAudio::ISink::IControl interface!"));
                        }
                    }
                }
            } else {
                TRACE_L1("Bluetooth audio sink is no longer operational!");

                _sinkLock.Lock();

                _buffer.reset();

                _sinkLock.Unlock();

                if (_sinkControl != nullptr) {
                    _sinkControl->Release();
                    _sinkControl = nullptr;
                }

                if (_sink != nullptr) {
                    _sink->Release();
                    _sink = nullptr;
                }
            }

            for (auto& cb : _operationalStateCallbacks) {
                cb.first(upAndRunning, cb.second);
            }

            _lock.Unlock();
        }

    public:
        static AudioSink& Instance()
        {
            static AudioSink* instance = new AudioSink("BluetoothAudio");
            ASSERT(instance != nullptr);

            return (*instance);
        }
        static void Dispose()
        {
            ASSERT(_singleton != nullptr);
            delete _singleton;
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
                // Call the callback immediately if the service is operational already.
                if (SmartInterfaceType::IsOperational() == true) {
                    callback(true, user_data);
                }
            }
            else {
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
            }
             else {
                TRACE_L1("Failed to unregister an operational state callback");
            }

            _lock.Unlock();

            return (result);
        }

    public:
        uint32_t RegisterSinkStateChangedCallback(const bluetoothaudiosink_state_changed_cb callback, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(callback != nullptr);

            Exchange::IBluetoothAudio::state state{};

            _lock.Lock();

            if (_sinkStateCallbacks.find(callback) == _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.emplace(std::piecewise_construct, std::forward_as_tuple(callback), std::forward_as_tuple(user_data));

                result = _sink->State(state);
            }
            else {
                TRACE_L1("Failed to register a sink state callback");
            }

            if ((result == Core::ERROR_NONE) && (state != Exchange::IBluetoothAudio::UNASSIGNED)) {
                callback(ConvertState(state), user_data);
            } 

            _lock.Unlock();

            return (result);
        }

        uint32_t UnregisterSinkStateChangedCallback(const bluetoothaudiosink_state_changed_cb callback)
        {
            uint32_t result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(callback != nullptr);

            _lock.Lock();

            auto it = _sinkStateCallbacks.find(callback);

            if (it != _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.erase(it);
                result = Core::ERROR_NONE;
            }
            else {
                TRACE_L1("Failed to unregister a sink state callback");
            }

            _lock.Unlock();

            return (result);
        }

    public:
        uint32_t State(Exchange::IBluetoothAudio::state& state) const
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if (_sink != nullptr) {
                result = _sink->State(state);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("ISink::State() failed! [%i]", result);
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        uint32_t Configure(const Exchange::IBluetoothAudio::IStream::Format& format)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if (_sinkControl != nullptr) {
                _frameSize = (100UL * format.Resolution * format.Channels * format.SampleRate) / (format.FrameRate * 8);

                TRACE_L1("Configuring sink with settings: %i Hz, %i channels, %i bits per sample, %i.%i fps, frame size is %i bytes",
                         format.SampleRate,  format.Channels, format.Resolution, format.FrameRate/100, format.FrameRate%100, _frameSize);

                result = _sinkControl->Configure(format);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("IStream::Configure() failed! [%i]", result);
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        uint32_t Acquire()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if ((_sinkControl != nullptr) && (_frameSize != 0)) {

                TRACE_L1("Acquiring sink with connector '%s'", CONNECTOR);

                _buffer.reset(new SendBuffer(CONNECTOR, _frameSize));
                ASSERT(_buffer.get() != nullptr);

                if (_buffer->IsValid() == true) {
                    result = _sinkControl->Acquire(CONNECTOR);

                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("IStream::Acquire() failed! [%i]", result);
                        _buffer.reset();
                    }
                }
                else {
                    TRACE_L1("Failed to open buffer!");
                    result = Core::ERROR_OPENING_FAILED;
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        uint32_t Relinquish()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if (_sinkControl != nullptr) {

                result = _sinkControl->Relinquish();
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("IStream::Relinquish failed! [%i]", result);
                }
                else {
                    TRACE_L1("Client released the audio sink");
                }

                _buffer.reset();

                _frameSize = 0;
            }

            _sinkLock.Unlock();

            return (result);
        }

        uint32_t Speed(const int8_t speed)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if (_sinkControl != nullptr) {
                result = _sinkControl->Speed(speed);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("IStream::Speed(%i) failed [%i]", speed, result);
                }
            }

            _sinkLock.Unlock();

            return (result);
        }

        uint32_t Frame(const uint16_t length, const uint8_t data[], uint16_t& consumed)
        {
            uint32_t result = Core::ERROR_NONE;

            _sinkLock.Lock();

            if (_buffer != nullptr) {
                consumed = _buffer->Write(length, data);
            }
            else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _sinkLock.Unlock();

            return (result);
        }

        uint32_t Time(uint32_t& timeMs)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if ((_sinkControl != nullptr) && (_buffer != nullptr)) {
                result = _sinkControl->Time(timeMs);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("IStream::Time() failed! [%i]", result);
                }
            }

            _sinkLock.Unlock();

            return (result);
        }

        uint32_t Delay(uint32_t& delaySamples)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _sinkLock.Lock();

            if (_sinkControl != nullptr) {
                result = _sinkControl->Delay(delaySamples);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("IStream::Delay() failed! [%i]", result);
                }
            }

            _sinkLock.Unlock();

            return (result);
        }

    private:
        static AudioSink* _singleton;

        Core::SinkType<Callback> _callback;

        mutable Core::CriticalSection _lock;
        OperationalStateUpdateCallbacks _operationalStateCallbacks;
        SinkStateChangedCallbacks _sinkStateCallbacks;

        mutable Core::CriticalSection _sinkLock;
        Exchange::IBluetoothAudio::ISink* _sink;
        Exchange::IBluetoothAudio::IStream* _sinkControl;
        uint32_t _frameSize;
        std::unique_ptr<SendBuffer> _buffer;
    };

    AudioSink* AudioSink::_singleton = nullptr;

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

uint32_t bluetoothaudiosink_register_state_changed_callback(const bluetoothaudiosink_state_changed_cb callback, void *user_data)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().RegisterSinkStateChangedCallback(callback, user_data));
    }
}

uint32_t bluetoothaudiosink_unregister_state_changed_callback(const bluetoothaudiosink_state_changed_cb callback)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().UnregisterSinkStateChangedCallback(callback));
    }
}

uint32_t bluetoothaudiosink_state(bluetoothaudiosink_state_t* out_state)
{
    if (out_state == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        Exchange::IBluetoothAudio::state state{};

        uint32_t result = BluetoothAudioSinkClient::AudioSink::Instance().State(state);
        if (result == Core::ERROR_NONE) {
            (*out_state) = ConvertState(state);
        }

        return (result);
    }
}

uint32_t bluetoothaudiosink_configure(const bluetoothaudiosink_format_t *format)
{
    if (format == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Configure({format->sample_rate, format->frame_rate, format->resolution, format->channels}));
    }
}

uint32_t bluetoothaudiosink_acquire()
{
    return (BluetoothAudioSinkClient::AudioSink::Instance().Acquire());
}

uint32_t bluetoothaudiosink_relinquish()
{
    return (BluetoothAudioSinkClient::AudioSink::Instance().Relinquish());
}

uint32_t bluetoothaudiosink_speed(const int8_t speed)
{
    return (BluetoothAudioSinkClient::AudioSink::Instance().Speed(speed));
}

uint32_t bluetoothaudiosink_time(uint32_t* out_time_ms)
{
    if (out_time_ms == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Time(*out_time_ms));
    }
}

uint32_t bluetoothaudiosink_delay(uint32_t* out_delay_samples)
{
    if (out_delay_samples == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Delay(*out_delay_samples));
    }
}

uint32_t bluetoothaudiosink_frame(const uint16_t length, const uint8_t data[], uint16_t* out_consumed)
{
    if ((data == nullptr) || (out_consumed == nullptr)) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Frame(length, data, *out_consumed));
    }
}

uint32_t bluetoothaudiosink_dispose()
{
    BluetoothAudioSinkClient::AudioSink::Dispose();
    return (Core::ERROR_NONE);
}

} // extern "C"
