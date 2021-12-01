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

namespace BluetoothAudioSinkClient {

    class AudioSink : protected RPC::SmartInterfaceType<Exchange::IBluetoothAudioSink> {
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
        using SinkStateUpdateCallbacks = std::map<const bluetoothaudiosink_state_update_cb, void *>;
        friend class WPEFramework::Core::SingletonType<AudioSink>;

    private:
        AudioSink(const string& callsign)
            : SmartInterfaceType()
            , _lock()
            , _bufferLock()
            , _operationalStateCallbacks()
            , _sinkStateCallbacks()
            , _sink(nullptr)
            , _sinkControl(nullptr)
            , _callback(*this)
            , _buffer(nullptr)
        {
            if (SmartInterfaceType::Open(RPC::CommunicationTimeOut, SmartInterfaceType::Connector(), callsign) != Core::ERROR_NONE) {
                PRINT(_T("Failed to open the smart interface!"));
            }
        }

    public:
        AudioSink() = delete;
        AudioSink(const AudioSink&) = delete;
        AudioSink& operator=(const AudioSink&) = delete;

        ~AudioSink() override
        {
            TRACE_L1("Bluetooth audio sink is closing...");

            _bufferLock.Lock();

            if (_buffer != nullptr) {
                delete _buffer;
                _buffer = nullptr;
            }

            _bufferLock.Unlock();

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
                        PRINT(_T("Failed to retrieve the IBluetoothAudioSink interface!"));
                    } else {
                        if (_sink->Callback(&_callback) != Core::ERROR_NONE) {
                            PRINT(_T("Failed to register the sink callback!"));
                        } else {
                            TRACE_L1("Successfully registered the sink callback!");
                        }

                        if (_sinkControl == nullptr) {
                            _sinkControl = _sink->QueryInterface<Exchange::IBluetoothAudioSink::IControl>();

                            if (_sinkControl == nullptr) {
                                PRINT(_T("Failed to retrieve the IBluetoothAudioSink::IControl interface!"));
                            } else {
                                TRACE_L1("Successfully retrieved IBluetoothAudioSink and IControl intefaces!");
                            }
                        }
                    }

                }
            } else {
                TRACE_L1("Bluetooth audio sink is no longer operational!");

                _bufferLock.Lock();

                if (_buffer != nullptr) {
                    delete _buffer;
                    _buffer = nullptr;
                }

                _bufferLock.Unlock();

                if (_sinkControl != nullptr) {
                    _sinkControl->Release();
                    _sinkControl = nullptr;
                }

                if (_sink != nullptr) {
                    _sink->Release();
                    _sink = nullptr;
                }
            }

            auto callbacks = _operationalStateCallbacks;

           _lock.Unlock();

            for (auto& cb : callbacks) {
                cb.first(upAndRunning, cb.second);
            }
        }

    public:
        static AudioSink& Instance()
        {
            return Core::SingletonType<AudioSink>::Instance(_T("BluetoothAudioSink"));
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
                TRACE_L1("Successfully registered a sink state callback");
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

        uint32_t Acquire(const Exchange::IBluetoothAudioSink::IControl::Format& format)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                const uint32_t frameSize = (100UL * format.Resolution * format.Channels * format.SampleRate) / (format.FrameRate * 8);

                TRACE_L1("Acquiring sink with settings: %i Hz, %i channels, %i bits per sample, %i.%i fps, frame size = %i bytes",
                         format.SampleRate,  format.Channels, format.Resolution, format.FrameRate/100, format.FrameRate%100, frameSize);

                _bufferLock.Lock();

                _buffer = new SendBuffer(CONNECTOR, frameSize);
                ASSERT(_buffer != nullptr);

                if (_buffer != nullptr) {
                    result = _sinkControl->Acquire(CONNECTOR, format);
                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("Acquire() failed! [%i]", result);
                        delete _buffer;
                        _buffer = nullptr;
                    } else {
                        TRACE_L1("A client acquired the audio sink");
                    }
                } else {
                    result = Core::ERROR_OPENING_FAILED;
                }

                _bufferLock.Unlock();
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Relinquish()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                result = _sinkControl->Relinquish();
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Relinquish failed! [%i]", result);
                } else {
                    TRACE_L1("A client released the audio sink");
                }

                _bufferLock.Lock();

                if (_buffer != nullptr) {
                    delete _buffer;
                    _buffer = nullptr;
                }

                _bufferLock.Unlock();
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Speed(const int8_t speed)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                result = _sinkControl->Speed(speed);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Speed(%i) failed [%i]", speed, result);
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Frame(const uint16_t length, const uint8_t data[], uint16_t& consumed)
        {
            uint32_t result = Core::ERROR_NONE;

            _bufferLock.Lock();

            if (_buffer != nullptr) {
                consumed = _buffer->Write(length, data);
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            _bufferLock.Unlock();

            return (result);
        }

        uint32_t Time(uint32_t& timeMs)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if ((_sinkControl != nullptr) && (_buffer != nullptr)) {
                result = _sinkControl->Time(timeMs);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Time() failed! [%i]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

        uint32_t Delay(uint32_t& delaySamples)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sinkControl != nullptr) {
                result = _sinkControl->Delay(delaySamples);
                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Delay() failed! [%i]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

    private:
        mutable Core::CriticalSection _lock;
        mutable Core::CriticalSection _bufferLock;
        OperationalStateUpdateCallbacks _operationalStateCallbacks;
        SinkStateUpdateCallbacks _sinkStateCallbacks;
        Exchange::IBluetoothAudioSink* _sink;
        Exchange::IBluetoothAudioSink::IControl* _sinkControl;
        Core::Sink<Callback> _callback;
        SendBuffer* _buffer;
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

uint32_t bluetoothaudiosink_acquire(const bluetoothaudiosink_format_t *format)
{
    if (format == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Acquire({format->sample_rate, format->frame_rate, format->resolution, format->channels}));
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

uint32_t bluetoothaudiosink_delay(uint32_t* delay_samples)
{
    if (delay_samples == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    } else {
        return (BluetoothAudioSinkClient::AudioSink::Instance().Delay(*delay_samples));
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
