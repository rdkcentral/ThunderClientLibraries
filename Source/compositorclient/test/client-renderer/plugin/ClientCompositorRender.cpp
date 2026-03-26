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

#include "ClientCompositorRender.h"

namespace Thunder {
namespace ClientCompositorRender {
    extern Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection);
}

namespace Plugin {
    namespace {
        static Metadata<ClientCompositorRender> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::GRAPHICS },
            // Terminations
            { subsystem::NOT_GRAPHICS },
            // Controls
            {});
    }

    /* virtual */ const string ClientCompositorRender::Initialize(PluginHost::IShell* service)
    {
        string message;
        ASSERT(_memory == nullptr);
        ASSERT(_statecontrol == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);

        // Setup skip URL for right offset.
        _service = service;
        _service->AddRef();
        _skipURL = static_cast<uint32_t>(_service->WebPrefix().length());

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _statecontrol = _service->Root<PluginHost::IStateControl>(_connectionId, 2000, _T("ClientCompositorRenderImplementation"));

        if (_statecontrol != nullptr) {
            _statecontrol->Register(&_notification);

            uint32_t result = _statecontrol->Configure(_service);

            if (result != Core::ERROR_NONE) {
                message = _T("ClientCompositorRender could not be configured.");
            } else {
                RPC::IRemoteConnection* connection = _service->RemoteConnection(_connectionId);
                if (connection != nullptr) {
                    _memory = Thunder::ClientCompositorRender::MemoryObserver(connection);
                    ASSERT(_memory != nullptr);
                    connection->Release();
                }
            }

            Config config;
            config.FromString(service->ConfigLine());

            if (config.RelativeGeometry.IsSet()) {
                Geometry params;

                params.X = (config.CanvasWidth.Value() * config.RelativeGeometry.X.Value()) / 100;
                params.Y = (config.CanvasHeight.Value() * config.RelativeGeometry.Y.Value()) / 100;
                params.Width = (config.CanvasWidth.Value() * config.RelativeGeometry.Width.Value()) / 100;
                params.Height = (config.CanvasHeight.Value() * config.RelativeGeometry.Height.Value()) / 100;

                string paramsString;
                params.ToString(paramsString);

                PluginHost::IDispatcher* dispatch = service->QueryInterfaceByCallsign<PluginHost::IDispatcher>("Controller");

                string output;
                Core::hresult result = dispatch->Invoke(0, 42, "", "Compositor.1.geometry@" + service->Callsign(), paramsString, output);

                std::cout << "Requested resize: Result=" << result << " response " << output << std::endl;
            }

        } else {
            message = _T("ClientCompositorRender could not be instantiated.");
        }

        return message;
    }

    /* virtual */ void ClientCompositorRender::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == _service);

            _service->Unregister(&_notification);

            if (_statecontrol != nullptr) {
                _statecontrol->Unregister(&_notification);
                _statecontrol->Release();
                _statecontrol = nullptr;
            }

            if (_memory != nullptr) {
                _memory->Release();
                _memory = nullptr;
            }

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));

            // If this was running in a (container) process...
            if (connection != nullptr) {
                // Lets trigger a cleanup sequence for
                // out-of-process code. Which will guard
                // that unwilling processes, get shot if
                // not stopped friendly :~)
                connection->Terminate();
                connection->Release();
            }

            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    /* virtual */ string ClientCompositorRender::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void ClientCompositorRender::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    void ClientCompositorRender::StateChange(const PluginHost::IStateControl::state state)
    {
        switch (state) {
        case PluginHost::IStateControl::RESUMED:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"suspend\":false }"))));
            _service->Notify("{ \"suspended\":false }");
            break;
        case PluginHost::IStateControl::SUSPENDED:
            TRACE(Trace::Information,
                (string(_T("StateChange: { \"suspend\":true }"))));
            _service->Notify("{ \"suspended\":true }");
            break;
        case PluginHost::IStateControl::EXITED:
            Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                    PluginHost::IShell::DEACTIVATED,
                    PluginHost::IShell::REQUESTED));
            break;
        case PluginHost::IStateControl::UNINITIALIZED:
            break;
        default:
            ASSERT(false);
            break;
        }
    }
}
}
