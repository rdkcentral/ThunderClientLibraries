/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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
#include <interfaces/IMemory.h>

#include <IModel.h>
#include <Renderer.h>
#include <TextureBounce.h>

#include "ClientCompositorRender.h"

namespace Thunder {
namespace Plugin {
    class ClientCompositorRenderImplementation : public PluginHost::IStateControl {
    public:
        ClientCompositorRenderImplementation(const ClientCompositorRenderImplementation&) = delete;
        ClientCompositorRenderImplementation& operator=(const ClientCompositorRenderImplementation&) = delete;

        ClientCompositorRenderImplementation()
            : _adminLock()
            , _observers()
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _requestedCommand(PluginHost::IStateControl::SUSPEND)
            , _job(*this)
            , _renderer()
            , _model()
        {
        }

        ~ClientCompositorRenderImplementation() override = default;

        friend Core::ThreadPool::JobType<ClientCompositorRenderImplementation&>;

        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result(Core::ERROR_NONE);

            ASSERT(service != nullptr);
            ClientCompositorRender::Config config;
            config.FromString(service->ConfigLine());

            if (_renderer.Configure(config.CanvasWidth.Value(), config.CanvasHeight.Value())) {
                Compositor::TextureBounce::Config model_config;
                model_config.Image = service->DataPath() + config.Image.Value();
                model_config.ImageCount = config.ImageCount.Value();

                std::string configStr;
                model_config.ToString(configStr);

                if (!_renderer.Register(&_model, configStr)) {
                    fprintf(stderr, "Failed to initialize model\n");
                    result = Core::ERROR_OPENING_FAILED;
                }

            } else {
                result = Core::ERROR_ILLEGAL_STATE;
            }

            return result;
        }

        PluginHost::IStateControl::state State() const override
        {
            PluginHost::IStateControl::state state;
            _adminLock.Lock();
            state = _state;
            _adminLock.Unlock();
            return state;
        }

        void Dispatch()
        {
            bool stateChanged = false;
            
            _adminLock.Lock();
            if (_requestedCommand == PluginHost::IStateControl::RESUME) {
                if ((_state == PluginHost::IStateControl::UNINITIALIZED || _state == PluginHost::IStateControl::SUSPENDED)) {
                    _renderer.Start();
                    stateChanged = true;
                    _state = PluginHost::IStateControl::RESUMED;
                }
            } else {
                if (_state == PluginHost::IStateControl::RESUMED || _state == PluginHost::IStateControl::UNINITIALIZED) {
                    _renderer.Stop();
                    stateChanged = true;
                    _state = PluginHost::IStateControl::SUSPENDED;
                }
            }
            if (stateChanged) {
                for (auto& observer : _observers) {
                    observer->StateChange(_state);
                }
            }
            _adminLock.Unlock();
        }

        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            _adminLock.Lock();
            _requestedCommand = command;
            _job.Submit();
            _adminLock.Unlock();
            return (Core::ERROR_NONE);
        }

        void Register(PluginHost::IStateControl::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();

            // Only subscribe an interface once.
            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));
            ASSERT(index == _observers.end());

            if (index == _observers.end()) {
                // We will keep a reference to this observer, reference it..
                notification->AddRef();
                _observers.push_back(notification);
            }
            _adminLock.Unlock();
        }

        void Unregister(PluginHost::IStateControl::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();
            // Only subscribe an interface once.
            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            // Unregister only once :-)
            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                // We will keep a reference to this observer, reference it..
                (*index)->Release();
                _observers.erase(index);
            }
            _adminLock.Unlock();
        }

        BEGIN_INTERFACE_MAP(ClientCompositorRenderImplementation)
        INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        mutable Core::CriticalSection _adminLock;
        std::list<PluginHost::IStateControl::INotification*> _observers;
        PluginHost::IStateControl::state _state;
        PluginHost::IStateControl::command _requestedCommand;
        Core::WorkerPool::JobType<ClientCompositorRenderImplementation&> _job;

        Compositor::Render _renderer;
        Compositor::TextureBounce _model;
    };

    SERVICE_REGISTRATION(ClientCompositorRenderImplementation, 1, 0)

} /* namespace Plugin */

namespace ClientCompositorRender {
    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
        MemoryObserverImpl(const RPC::IRemoteConnection* connection)
            : _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
        {
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        uint64_t Resident() const override
        {
            return _main.Resident();
        }
        uint64_t Allocated() const override
        {
            return _main.Allocated();
        }
        uint64_t Shared() const override
        {
            return _main.Shared();
        }
        uint8_t Processes() const override
        {
            return (IsOperational() ? 1 : 0);
        }
        bool IsOperational() const override
        {
            return _main.IsActive();
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        Core::ProcessInfo _main;
    };

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        ASSERT(connection != nullptr);
        Exchange::IMemory* result = Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}
} // namespace ClientCompositorRender
