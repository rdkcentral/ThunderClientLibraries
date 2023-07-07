/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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
#include <string.h>
#include "include/bluetoothaudiosource.h"
#include <interfaces/IBluetoothAudio.h>

namespace WPEFramework {

namespace BluetoothAudioSourceClient {

    class AudioSource : protected RPC::SmartInterfaceType<Exchange::IBluetoothAudio::ISource> {
    private:
        static constexpr uint32_t WriteTimeout = 50;

        class Receiver : public Core::Thread {
        private:
            class ReceiveBuffer : public Core::SharedBuffer {
            public:
                ReceiveBuffer() = delete;
                ReceiveBuffer(const ReceiveBuffer&) = delete;
                ReceiveBuffer& operator=(const ReceiveBuffer&) = delete;
                ~ReceiveBuffer() = default;

                ReceiveBuffer(const string& name)
                    : Core::SharedBuffer(name.c_str())
                {
                }

            public:
                uint32_t Get(const uint32_t length, uint8_t buffer[])
                {
                    uint32_t result = 0;

                    ASSERT(IsValid() == true);
                    ASSERT(Buffer() != nullptr);

                    if (RequestConsume(100) == Core::ERROR_NONE) {
                        ASSERT(BytesWritten() <= length);

                        const uint32_t size = std::min(BytesWritten(), length);
                        ::memcpy(buffer, Buffer(), size);
                        result = size;

                        Consumed();
                    }

                    return (result);
                }
            }; // class ReceiveBuffer

        public:
            Receiver() = delete;
            Receiver(const Receiver&) = delete;
            Receiver& operator=(const Receiver&) = delete;

            Receiver(AudioSource& parent, const string& connector)
                : _parent(parent)
                , _receiveBuffer(connector)
                , _maxFrameSize(0)
                , _buffer()
                , _bufferSize(0)
            {
                TRACE_L1("Receive buffer at '%s'", connector.c_str());

                _maxFrameSize = _receiveBuffer.Size();
                _bufferSize = _maxFrameSize;

                _buffer.reset(new (std::nothrow) uint8_t[_bufferSize]);
                ASSERT(_buffer.get() != nullptr);

                Stop();
            }

            ~Receiver()
            {
                Stop();
            }

        public:
            void Start()
            {
                _running = true;
                Thread::Run();
            }
            void Stop()
            {
                _running = false;
                Thread::Wait(BLOCKED, Core::infinite);
            }

        public:
            bool IsValid() const {
                return (_receiveBuffer.IsValid());
            }

        private:
            uint32_t Worker()
            {
                uint32_t delay = 0;

                const uint16_t bytesRead = _receiveBuffer.Get(_maxFrameSize, _buffer.get());
                ASSERT(bytesRead < _maxFrameSize);

                if (bytesRead != 0) {
                    _parent.OnFrameReceived(_buffer.get(), bytesRead);
                }

                if (_running == false) {
                    Thread::Block();
                    delay = Core::infinite;
                }

                return (delay);
            }

        private:
            AudioSource& _parent;
            ReceiveBuffer _receiveBuffer;
            uint16_t _maxFrameSize;
            std::unique_ptr<uint8_t[]> _buffer;
            uint32_t _bufferSize;
            bool _running;
        }; // class Receiver

    private:
        using OperationalStateUpdateCallbacks = std::map<const bluetoothaudiosource_operational_state_update_cb, void*>;
        using SourceStateChangedCallbacks = std::map<const bluetoothaudiosource_state_changed_cb, void*>;

    private:
        AudioSource(const string& callsign)
            : SmartInterfaceType()
            , _callsign(callsign)
            , _callback(*this)
            , _sinkStream(*this)
            , _lock()
            , _source(nullptr)
            , _sourceControl(nullptr)
            , _sourceStream(nullptr)
            , _sinkLock()
            , _operationalStateCallbacks()
            , _sinkStateCallbacks()
            , _sinkCallbacks(nullptr)
            , _sinkCallbacksUserData(nullptr)
            , _receiver()
        {
            TRACE_L1("Constructing Bluetooth Audio Source client library...");

            ASSERT(_singleton == nullptr);
            _singleton = this;

            if (SmartInterfaceType::Open(RPC::CommunicationTimeOut, SmartInterfaceType::Connector(), callsign) != Core::ERROR_NONE) {
                TRACE_L1("Failed to open the smart interface!");
            } else {
                TRACE_L1("Opened smart interface ('%s')", callsign.c_str());
            }

            TRACE_L1("Bluetooth Audio Source client library constructed");
        }

    public:
        AudioSource() = delete;
        AudioSource(const AudioSource&) = delete;
        AudioSource& operator=(const AudioSource&) = delete;

    private:
        ~AudioSource() override
        {
            TRACE_L1("Destructing Bluetooth Audio Source client library...");

            _lock.Lock();

            Relinquish();

            Cleanup();

            _lock.Unlock();

            SmartInterfaceType::Close(Core::infinite);

            ASSERT(_singleton != nullptr);
            _singleton = nullptr;

            TRACE_L1("Bluetooth Audio Source client library destructed");
        }

        void Cleanup()
        {
            if (_sourceStream != nullptr) {
                _sourceStream->Release();
                _sourceStream = nullptr;
            }

            if (_sourceControl != nullptr) {
                _sourceControl->Sink(nullptr);
                _sourceControl->Release();
                _sourceControl = nullptr;
            }

            if (_source != nullptr) {
                _source->Callback(nullptr);
                _source->Release();
                _source = nullptr;
            }
        }

    private:
        class Callback : public Exchange::IBluetoothAudio::ISource::ICallback {
        public:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator=(const Callback&) = delete;
            ~Callback() = default;

            Callback(AudioSource& parent)
                : _parent(parent)
            {
            }

        public:
            void StateChanged(const Exchange::IBluetoothAudio::state state) override
            {
                _parent.StateChanged(state);
            }

        public:
            BEGIN_INTERFACE_MAP(Callback)
            INTERFACE_ENTRY(Exchange::IBluetoothAudio::ISource::ICallback)
            END_INTERFACE_MAP

        private:
            AudioSource& _parent;
        }; // class Callback

    private:
        class SinkStream : public Exchange::IBluetoothAudio::IStream {
            // Calls from source device to sink component.
        public:
            SinkStream() = delete;
            SinkStream(const SinkStream&) = delete;
            SinkStream& operator=(const SinkStream&) = delete;
            ~SinkStream() = default;

            SinkStream(AudioSource& parent)
                : _parent(parent)
            {
            }

        public:
            Core::hresult Configure(const Exchange::IBluetoothAudio::IStream::Format& format) override
            {
                return (_parent.OnConfigure(format));
            }
            Core::hresult Acquire(const string& connector) override
            {
                return (_parent.OnAcquire(connector));
            }
            Core::hresult Relinquish() override
            {
                return (_parent.OnRelinquish());
            }
            Core::hresult Speed(const int8_t speed) override
            {
                return (_parent.OnSetSpeed(speed));
            }
            Core::hresult Time(uint32_t& timeMs) const override
            {
                return (_parent.OnGetTime(timeMs));
            }
            Core::hresult Delay(uint32_t& delaySamples) const override
            {
                return (_parent.OnGetDelay(delaySamples));
            }

        public:
            BEGIN_INTERFACE_MAP(SinkStream)
            INTERFACE_ENTRY(Exchange::IBluetoothAudio::IStream)
            END_INTERFACE_MAP

        private:
            AudioSource& _parent;
        }; // class SinkStream

    private:
        void StateChanged(const Exchange::IBluetoothAudio::state state)
        {
            _sinkLock.Lock();

            for (auto& cb : _sinkStateCallbacks) {
                cb.first(ConvertState(state), cb.second);
            }

            _sinkLock.Unlock();
        }

    private:
        Core::hresult OnConfigure(const Exchange::IBluetoothAudio::IStream::Format& format)
        {
            Core::hresult result = Core::ERROR_UNAVAILABLE;

            TRACE_L1("Remote source is configuring sink...");

            _sinkLock.Lock();

            if (_sinkCallbacks == nullptr) {
                TRACE_L1("No sink available!");
            }
            else {
                ASSERT(_sinkCallbacks->configure_cb != nullptr);

                bluetoothaudiosource_format_t cFormat;
                cFormat.sample_rate = format.SampleRate;
                cFormat.frame_rate = format.FrameRate;
                cFormat.resolution = format.Resolution;
                cFormat.channels = format.Channels;

                const uint32_t sinkResult = _sinkCallbacks->configure_cb(&cFormat, _sinkCallbacksUserData);

                if (sinkResult != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                    TRACE_L1("Sink configure_cb() failed!");
                    result = Core::ERROR_GENERAL;
                }
                else {
                    result = Core::ERROR_NONE;
                    TRACE_L1("Sink configured (%d Hz, %d bits, %d channels, %d fps)",
                        cFormat.sample_rate, cFormat.resolution, cFormat.channels, cFormat.frame_rate);
                }
            }

            _sinkLock.Unlock();

            return (result);

        }
        Core::hresult OnAcquire(const string& connector)
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

           TRACE_L1("Remote source is acquiring sink...");

            _sinkLock.Lock();

            if (_receiver != nullptr) {
                TRACE_L1("Sink already acquired!");
            }
            else {
                if (_sinkCallbacks == nullptr) {
                    TRACE_L1("No sink available!");
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    ASSERT(_sinkCallbacks->acquire_cb != nullptr);

                    const uint32_t sinkResult = _sinkCallbacks->acquire_cb(_sinkCallbacksUserData);

                    if (sinkResult != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                        TRACE_L1("Sink acquire_cb() failed!");
                        result = Core::ERROR_GENERAL;
                    }
                    else {
                        _receiver.reset(new Receiver(*this, connector));
                        ASSERT(_receiver.get() != nullptr);

                        if (_receiver->IsValid() != true) {
                            _receiver.reset();

                            ASSERT(_sinkCallbacks->relinquish_cb != nullptr);

                            _sinkCallbacks->relinquish_cb(_sinkCallbacksUserData);

                            TRACE_L1("Failed to open the shared buffer!");
                            result = Core::ERROR_OPENING_FAILED;
                        }
                        else {
                            TRACE_L1("Sink acquired");
                            result = Core::ERROR_NONE;
                        }
                    }
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        Core::hresult OnRelinquish()
        {
            Core::hresult result = Core::ERROR_ALREADY_RELEASED;

            _sinkLock.Lock();

            if ((_receiver != nullptr) || (_sinkCallbacks != nullptr)) {

                TRACE_L1("Remote source is relinquishing sink...");

                _receiver.reset();

                if (_sinkCallbacks != nullptr) {
                    ASSERT(_sinkCallbacks->relinquish_cb != nullptr);

                    const uint32_t sinkResult = _sinkCallbacks->relinquish_cb(_sinkCallbacksUserData);

                    if (sinkResult != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                        result = Core::ERROR_GENERAL;
                        TRACE_L1("Sink relinquish_cb() failed!");
                    }
                    else {
                        TRACE_L1("Sink relinquished");
                        result = Core::ERROR_NONE;
                    }
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        Core::hresult OnSetSpeed(const int8_t speed)
        {
            Core::hresult result = Core::ERROR_NONE;

            TRACE_L1("Remote source is changing sink speed to %d%%...", speed);

            _sinkLock.Lock();

            if (_receiver == nullptr) {
                TRACE_L1("Recevier not aquired!");
                result = Core::ERROR_ILLEGAL_STATE;
            }
            else {
                if ((speed == 0) || (speed == 100)) {

                    if (_sinkCallbacks != nullptr) {
                        ASSERT(_sinkCallbacks->set_speed_cb != nullptr);

                        const uint32_t sinkResult = _sinkCallbacks->set_speed_cb(speed, _sinkCallbacksUserData);

                        if (sinkResult != 0) {
                            TRACE_L1("Sink set_speed_cb() failed!");
                            result = Core::ERROR_NOT_SUPPORTED;
                        }
                        else {
                            if (speed == 0) {
                                _receiver->Stop();
                            }
                            else if (speed == 100) {
                                _receiver->Start();
                            }

                            TRACE_L1("Sink speed changed to %d%%", speed);
                        }
                    }
                    else {
                        result = Core::ERROR_UNAVAILABLE;
                    }
                }
                else {
                    TRACE_L1("Unsupported speed setting (%d)", speed);
                    result = Core::ERROR_NOT_SUPPORTED;
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        Core::hresult OnGetTime(uint32_t& timeMs) const
        {
            Core::hresult result = Core::ERROR_UNAVAILABLE;

            _sinkLock.Lock();

            if (_sinkCallbacks != nullptr) {
                ASSERT(_sinkCallbacks->get_time_cb != nullptr);

                const uint32_t sinkResult = _sinkCallbacks->get_time_cb(&timeMs, _sinkCallbacksUserData);

                if (sinkResult != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                    TRACE_L1("Sink get_time_cb() failed!");
                    result = Core::ERROR_NOT_SUPPORTED;
                }
                else {
                    result = Core::ERROR_NONE;
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        Core::hresult OnGetDelay(uint32_t& delaySamples) const
        {
            Core::hresult result = Core::ERROR_UNAVAILABLE;

            _sinkLock.Lock();

            if (_sinkCallbacks != nullptr) {
                ASSERT(_sinkCallbacks->get_delay_cb != nullptr);

                const uint32_t sinkResult = _sinkCallbacks->get_delay_cb(&delaySamples, _sinkCallbacksUserData);

                if (sinkResult != BLUETOOTHAUDIOSOURCE_SUCCESS) {
                    TRACE_L1("Sink get_delay_cb() failed!");
                    result = Core::ERROR_GENERAL;
                }
                else {
                    result = Core::ERROR_NONE;
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        void OnFrameReceived(const uint8_t frame[], const uint16_t length)
        {
            ASSERT(frame != nullptr);

            _sinkLock.Lock();

            if (_sinkCallbacks != nullptr) {
                ASSERT(_sinkCallbacks->frame_cb != nullptr);

                _sinkCallbacks->frame_cb(length, frame, _sinkCallbacksUserData);
            }

            _sinkLock.Unlock();
        }

    private:
        void Operational(const bool upAndRunning) override
        {
            bool notify = false;

            _lock.Lock();

            if (upAndRunning == true) {
                TRACE_L1("'%s' service is now operational!", _callsign.c_str());

                ASSERT(_source == nullptr);
                ASSERT(_sourceControl == nullptr);
                ASSERT(_sourceStream == nullptr);

                if (_source == nullptr) {
                    _source = SmartInterfaceType::Interface();

                    ASSERT(_source != nullptr);

                    if (_source == nullptr) {
                        TRACE_L1("Failed to retrieve the IBluetoothAudio::ISource interface!");
                    }
                    else {
                        if (_source->Callback(&_callback) != Core::ERROR_NONE) {
                            TRACE_L1("Failed to register the source callback!");
                        }

                        if (_sourceControl == nullptr) {
                            _sourceControl = _source->QueryInterface<Exchange::IBluetoothAudio::ISource::IControl>();

                            ASSERT(_sourceControl != nullptr);

                            if (_sourceControl == nullptr) {
                                TRACE_L1("Failed to retrieve the IBluetoothAudio::ISource::IControl interface!");
                            }
                            else if (_sourceControl->Sink(&_sinkStream) != Core::ERROR_NONE) {
                                TRACE_L1("Failed to register source stream callback!");
                            }
                            else {
                                _sourceStream = _sourceControl->QueryInterface<Exchange::IBluetoothAudio::IStream>();
                                ASSERT(_sourceStream != nullptr);

                                if (_sourceStream == nullptr) {
                                    TRACE_L1("Failed retrieve the IBluetoothAudio::IStream interface!");
                                }
                                else {
                                    // All OK!
                                    notify = true;
                                }
                            }
                        }
                    }
                }
            }
            else {
                TRACE_L1("'%s' service is no longer operational!", _callsign.c_str());

                OnRelinquish();

                Cleanup();

                notify = true;
            }

            _lock.Unlock();

            if (notify == true) {
                _sinkLock.Lock();

                for (auto& cb : _operationalStateCallbacks) {
                    cb.first(upAndRunning, cb.second);
                }

                _sinkLock.Unlock();
            }
        }

    public:
        static AudioSource& Instance()
        {
            static AudioSource* instance = new AudioSource("BluetoothAudio");
            ASSERT(instance != nullptr);

            return (*instance);
        }
        static void Dispose()
        {
            ASSERT(_singleton != nullptr);
            delete _singleton;
        }

    public:
        uint32_t RegisterOperationalStateUpdateCallback(const bluetoothaudiosource_operational_state_update_cb callback, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(callback != nullptr);

            if (SmartInterfaceType::ControllerInterface() == nullptr) {
                TRACE_L1("Framework is not running!");
                result = Core::ERROR_UNAVAILABLE;
            }
            else {
                _sinkLock.Lock();

                if (_operationalStateCallbacks.find(callback) == _operationalStateCallbacks.end()) {
                    _operationalStateCallbacks.emplace(std::piecewise_construct, std::forward_as_tuple(callback), std::forward_as_tuple(user_data));
                    result = Core::ERROR_NONE;
                }

                _sinkLock.Unlock();

                if (result == Core::ERROR_NONE) {
                    // Call the callback immediately if the service is operational already.
                    if (SmartInterfaceType::IsOperational() == true) {
                        callback(true, user_data);
                    }
                }
                else {
                    TRACE_L1("Failed to register operational state callback!");
                }
            }

            return (result);
        }
        uint32_t UnregisterOperationalStateUpdateCallback(const bluetoothaudiosource_operational_state_update_cb callback)
        {
            uint32_t result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(callback != nullptr);

            _sinkLock.Lock();

            auto it = _operationalStateCallbacks.find(callback);

            if (it != _operationalStateCallbacks.end()) {
                _operationalStateCallbacks.erase(it);
                result = Core::ERROR_NONE;
            }

            _sinkLock.Unlock();

            return (result);
        }

    public:
        uint32_t RegisterSourceStateChangedCallback(const bluetoothaudiosource_state_changed_cb callback, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            ASSERT(callback != nullptr);

            _sinkLock.Lock();

            if (_sinkStateCallbacks.find(callback) == _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.emplace(std::piecewise_construct, std::forward_as_tuple(callback), std::forward_as_tuple(user_data));
                result = Core::ERROR_NONE;
            }

            _sinkLock.Unlock();

            if (result == Core::ERROR_NONE) {

                Exchange::IBluetoothAudio::state state = Exchange::IBluetoothAudio::DISCONNECTED;

                _lock.Lock();

                if (_source != nullptr) {
                    _source->State(state);
                }

                _lock.Unlock();

                callback(ConvertState(state), user_data);
            }
            else {
                TRACE_L1("Failed to register a source state callback!");
            }

            return (result);
        }
        uint32_t UnregisterSourceStateChangedCallback(const bluetoothaudiosource_state_changed_cb callback)
        {
            uint32_t result = Core::ERROR_ALREADY_RELEASED;

            ASSERT(callback != nullptr);

            _sinkLock.Lock();

            auto it = _sinkStateCallbacks.find(callback);

            if (it != _sinkStateCallbacks.end()) {
                _sinkStateCallbacks.erase(it);
                result = Core::ERROR_NONE;
            }

            _sinkLock.Unlock();

            return (result);
        }

    public:
        uint32_t SetSink(const bluetoothaudiosource_sink_t* sink, void* user_data)
        {
            uint32_t result = Core::ERROR_ALREADY_CONNECTED;

            TRACE_L1("Client is %s sink callbacks...", (sink == nullptr? "clearing" : "setting"));

            _sinkLock.Lock();

            if ((_sinkCallbacks == nullptr) || (sink == nullptr)) {

                if ((_receiver != nullptr) && (sink == nullptr)) {
                    TRACE_L1("Warning: Removing callbacks for an acquired receiver!");
                    Relinquish();
                }

                if ((sink == nullptr) || ((sink->acquire_cb != nullptr) && (sink->frame_cb != nullptr)
                        && (sink->relinquish_cb != nullptr) && (sink->set_speed_cb != nullptr)
                        && (sink->get_time_cb != nullptr) && (sink->get_delay_cb != nullptr))) {

                    _sinkCallbacks = sink;
                    _sinkCallbacksUserData = user_data;

                    result = Core::ERROR_NONE;
                }
                else {
                    TRACE_L1("Invalid sink callbacks!");
                    result = Core::ERROR_BAD_REQUEST;
                }
            }

            _sinkLock.Unlock();

            return (result);
        }
        uint32_t Relinquish()
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE_L1("Client is relinquishing source...");

            _lock.Lock();

            if (_sourceStream != nullptr) {

                result = _sourceStream->Relinquish();

                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ALREADY_RELEASED)) {
                    TRACE_L1("Relinquish() failed! [%d]", result);
                }

                _sinkLock.Lock();

                _receiver.reset();

                _sinkLock.Unlock();

                TRACE_L1("Source relinquished");
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Speed(const int8_t speed)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            TRACE_L1("Client is changing source speed to %d%%...", speed);

            _lock.Lock();

            if (_sourceStream != nullptr) {
                result = _sourceStream->Speed(speed);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Speed() failed! [%d]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Time(uint32_t& timeMs)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_sourceStream != nullptr) {

                result = _sourceStream->Time(timeMs);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Time() failed! [%d]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

    public:
        uint32_t State(Exchange::IBluetoothAudio::state& state) const
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_source != nullptr) {
                result = _source->State(state);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("State() failed! [%d]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Device(string& address)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _lock.Lock();

            if (_source != nullptr) {

                result = _source->Device(address);

                if (result != Core::ERROR_NONE) {
                    TRACE_L1("Device() failed! [%d]", result);
                }
            }

            _lock.Unlock();

            return (result);
        }

    private:
        static bluetoothaudiosource_state_t ConvertState(const Exchange::IBluetoothAudio::state state)
        {
            bluetoothaudiosource_state_t out_state = BLUETOOTHAUDIOSOURCE_STATE_DISCONNECTED;

            switch (state) {
            case Exchange::IBluetoothAudio::UNASSIGNED:
            case Exchange::IBluetoothAudio::DISCONNECTED:
                break;
            case Exchange::IBluetoothAudio::CONNECTING:
                out_state = BLUETOOTHAUDIOSOURCE_STATE_CONNECTING;
                break;
            case Exchange::IBluetoothAudio::CONNECTED:
                out_state = BLUETOOTHAUDIOSOURCE_STATE_CONNECTED;
                break;
            case Exchange::IBluetoothAudio::CONNECTED_BAD:
            case Exchange::IBluetoothAudio::CONNECTED_RESTRICTED:
                out_state = BLUETOOTHAUDIOSOURCE_STATE_CONNECTED_BAD;
                break;
            case Exchange::IBluetoothAudio::READY:
                out_state = BLUETOOTHAUDIOSOURCE_STATE_READY;
                break;
            case Exchange::IBluetoothAudio::STREAMING:
                out_state = BLUETOOTHAUDIOSOURCE_STATE_STREAMING;
                break;
            default:
                TRACE_L1("Unmapped state value!");
                break;
            }

            return (out_state);
        }

    private:
        static AudioSource* _singleton;

        string _callsign;

        Core::Sink<Callback> _callback;
        Core::Sink<SinkStream> _sinkStream;

        mutable Core::CriticalSection _lock;
        Exchange::IBluetoothAudio::ISource* _source;
        Exchange::IBluetoothAudio::ISource::IControl* _sourceControl;
        Exchange::IBluetoothAudio::IStream* _sourceStream;

        mutable Core::CriticalSection _sinkLock;
        OperationalStateUpdateCallbacks _operationalStateCallbacks;
        SourceStateChangedCallbacks _sinkStateCallbacks;
        const bluetoothaudiosource_sink_t* _sinkCallbacks;
        void* _sinkCallbacksUserData;
        std::unique_ptr<Receiver> _receiver;
    };

    AudioSource* AudioSource::_singleton = nullptr;

} // namespace BluetoothAudiSourceClient

}


using namespace WPEFramework;

extern "C" {

uint32_t bluetoothaudiosource_register_operational_state_update_callback(const bluetoothaudiosource_operational_state_update_cb callback, void *user_data)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        return (BluetoothAudioSourceClient::AudioSource::Instance().RegisterOperationalStateUpdateCallback(callback, user_data));
    }
}

uint32_t bluetoothaudiosource_unregister_operational_state_update_callback(const bluetoothaudiosource_operational_state_update_cb callback)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        return (BluetoothAudioSourceClient::AudioSource::Instance().UnregisterOperationalStateUpdateCallback(callback));
    }
}

uint32_t bluetoothaudiosource_register_state_changed_callback(const bluetoothaudiosource_state_changed_cb callback, void *user_data)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        return (BluetoothAudioSourceClient::AudioSource::Instance().RegisterSourceStateChangedCallback(callback, user_data));
    }
}

uint32_t bluetoothaudiosource_unregister_state_changed_callback(const bluetoothaudiosource_state_changed_cb callback)
{
    if (callback == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        return (BluetoothAudioSourceClient::AudioSource::Instance().UnregisterSourceStateChangedCallback(callback));
    }
}

uint32_t bluetoothaudiosource_set_sink(const bluetoothaudiosource_sink_t *sink, void *user_data)
{
    return (BluetoothAudioSourceClient::AudioSource::Instance().SetSink(sink, user_data));
}

uint32_t bluetoothaudiosource_get_state(bluetoothaudiosource_state_t* out_state)
{
    if (out_state == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        Exchange::IBluetoothAudio::state state{};

        uint32_t result = BluetoothAudioSourceClient::AudioSource::Instance().State(state);

        if (result == Core::ERROR_NONE) {
        }

        return (result);
    }
}

uint32_t bluetoothaudiosource_get_device(uint8_t out_address[6])
{
    if (out_address == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        string addressStr;
        uint32_t result = BluetoothAudioSourceClient::AudioSource::Instance().Device(addressStr);

        // Pack the string into byte array.
        for (uint8_t i = 0; i < 6; i++) {
            out_address[i] = strtoul(&addressStr.c_str()[i*3], NULL, 16);
        }

        return (result);
    }
}

uint32_t bluetoothaudiosource_get_time(uint32_t *out_time_ms)
{
    if (out_time_ms == nullptr) {
        return (Core::ERROR_BAD_REQUEST);
    }
    else {
        return (BluetoothAudioSourceClient::AudioSource::Instance().Time(*out_time_ms));
    }
}

uint32_t bluetoothaudiosource_set_speed(const int8_t speed)
{
    return (BluetoothAudioSourceClient::AudioSource::Instance().Speed(speed));
}

uint32_t bluetoothaudiosource_relinquish(void)
{
    return (BluetoothAudioSourceClient::AudioSource::Instance().Relinquish());
}

uint32_t bluetoothaudiosource_dispose(void)
{
    BluetoothAudioSourceClient::AudioSource::Dispose();
    return (Core::ERROR_NONE);
}

} // extern "C"
