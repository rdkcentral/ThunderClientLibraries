/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../Module.h"

extern "C" {
#include <drm_fourcc.h>

#include <gbm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <EGL/egl.h>
}

#include <com/com.h>

#include <interfaces/IComposition.h>
#include <privilegedrequest/PrivilegedRequest.h>
#include <virtualinput/virtualinput.h>

#include <graphicsbuffer/GraphicsBufferType.h>
#include <interfaces/ICompositionBuffer.h>

#include "RenderAPI.h"
#include <compositor/Client.h>

#include <condition_variable>
#include <mutex>

namespace Thunder {
namespace Linux {
    namespace {
        const string BufferConnector()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR_BUFFER_CONNECTOR"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/bufferconnector");
            }
            return connector;
        }

        const string DisplayConnector()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR_DISPLAY_CONNECTOR"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/displayconnector");
            }
            return connector;
        }

        const string InputConnector()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("VIRTUAL_INPUT"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/keyhandler");
            }
            return connector;
        }
    }

    class Display : public Compositor::IDisplay {
    public:
        Display() = delete;
        Display(Display&&) = delete;
        Display(const Display&) = delete;
        Display& operator=(Display&&) = delete;
        Display& operator=(const Display&) = delete;

    private:
        static constexpr uint32_t DisplayId = 0;

        Display(const std::string& displayName);

        class SurfaceImplementation;

        using InputFunction = std::function<void(SurfaceImplementation*)>;

        static void Publish(InputFunction& action);

        static void VirtualKeyboardCallback(keyactiontype, const unsigned int);
        static void VirtualMouseCallback(mouseactiontype, const unsigned short, const signed short, const signed short);
        static void VirtualTouchScreenCallback(touchactiontype, const unsigned short, const unsigned short, const unsigned short);

        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        private:
            class GBMSurface : public Graphics::ClientBufferType<1> {
            private:
                using BaseClass = Graphics::ClientBufferType<1>;
                static void Destroyed(gbm_bo* bo VARIABLE_IS_NOT_USED, void* data VARIABLE_IS_NOT_USED)
                {
                }

            public:
                GBMSurface(SurfaceImplementation& parent, gbm_device* gbmDevice, uint32_t remoteId)
                    : BaseClass()
                    , _parent(parent)
                    , _remoteId(remoteId)
                    , _surface(nullptr)
                    , _frameBuffer(nullptr)
                {

                    Core::PrivilegedRequest::Container descriptors;
                    Core::PrivilegedRequest request;

                    if (request.Request(100, BufferConnector(), _remoteId, descriptors) == Core::ERROR_NONE) {
                        Load(descriptors);
                    } else {
                        TRACE(Trace::Error, (_T ( "Failed to get display file descriptor from compositor server")));
                    }

                    ASSERT(Format() != DRM_FORMAT_INVALID);

                    uint32_t flags = GBM_BO_USE_RENDERING;

                    const uint64_t modifier(Modifier());

                    if (modifier == DRM_FORMAT_MOD_INVALID) {
                        flags |= GBM_BO_USE_LINEAR;
                        _surface = gbm_surface_create(gbmDevice, Width(), Height(), Format(), flags);
                    } else {
                        _surface = gbm_surface_create_with_modifiers2(gbmDevice, Width(), Height(), Format(), &modifier, 1, flags);
                    }

                    ASSERT(_surface != nullptr);
                }

                ~GBMSurface()
                {
                    // fixme: This leads to a SEGFAULT....
                    // if (_frameBuffer != nullptr) {
                    //     gbm_bo_destroy(_frameBuffer);
                    //     _frameBuffer = nullptr;
                    // }

                    if (_surface != nullptr) {
                        gbm_surface_destroy(_surface);
                        _surface = nullptr;
                    }
                }

                bool Render()
                {
                    gbm_bo* bo = gbm_surface_lock_front_buffer(_surface);

                    ASSERT(bo != nullptr);

                    if (gbm_bo_get_user_data(bo) == nullptr) {
                        ASSERT(_frameBuffer == nullptr);

                        uint16_t planes = gbm_bo_get_plane_count(bo);

                        Core::PrivilegedRequest::Container descriptors;
                        Core::PrivilegedRequest request;

                        for (uint16_t index = 0; index < planes; index++) {
                            int descriptor = gbm_bo_get_fd_for_plane(bo, index);
                            descriptors.emplace_back(descriptor);
                            Add(descriptor, gbm_bo_get_stride_for_plane(bo, index), gbm_bo_get_offset(bo, index));
                        }

                        if (request.Offer(100, BufferConnector(), _remoteId, descriptors) == Core::ERROR_NONE) {
                            TRACE(Trace::Information, (_T("Offered buffer to compositor server")));
                        } else {
                            TRACE(Trace::Error, (_T("Failed to offer buffer to compositor server")));
                        }

                        gbm_bo_set_user_data(bo, this, &Destroyed);

                        _frameBuffer = bo;
                    }

                    ASSERT(_frameBuffer != nullptr);
                    ASSERT(gbm_bo_get_handle(bo).u32 == gbm_bo_get_handle(_frameBuffer).u32);

                    RequestRender();
                    return true;
                }

                void Rendered() override
                {
                    gbm_surface_release_buffer(_surface, _frameBuffer);
                    _parent.Rendered();
                }
                void Published() override
                {
                    _parent.Published();
                }

                EGLNativeWindowType Native() const
                {
                    return (static_cast<EGLNativeWindowType>(_surface));
                }

            private:
                SurfaceImplementation& _parent;
                uint32_t _remoteId;
                gbm_surface* _surface;
                gbm_bo* _frameBuffer;
            };

        public:
            SurfaceImplementation() = delete;
            SurfaceImplementation(SurfaceImplementation&&) = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(SurfaceImplementation&&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height, ICallback* callback)
                : _adminLock()
                , _id(Core::InterlockedIncrement(_surfaceIndex))
                , _display(display)
                , _remoteClient(display.CreateRemoteSurface(name, width, height))
                , _surface(*this, static_cast<gbm_device*>(_display.Native()), _remoteClient->Native())
                , _name(_remoteClient->Name())
                , _keyboard(nullptr)
                , _wheel(nullptr)
                , _pointer(nullptr)
                , _touchpanel(nullptr)
                , _callback(callback)
            {
                _display.AddRef();

                ASSERT(_remoteClient != nullptr);
                TRACE(Trace::Information, (_T("Construct surface[%d] %s  %dx%d (hxb)"), _id, name.c_str(), height, width));

                Core::ResourceMonitor::Instance().Register(_surface);

                _display.Register(this);
            }
            ~SurfaceImplementation() override
            {
                _display.Unregister(this);

                Core::ResourceMonitor::Instance().Unregister(_surface);

                if (_keyboard != nullptr) {
                    _keyboard->Release();
                }

                if (_wheel != nullptr) {
                    _wheel->Release();
                }

                if (_pointer != nullptr) {
                    _pointer->Release();
                }

                if (_touchpanel != nullptr) {
                    _touchpanel->Release();
                }

                // Cleanup the remote client buffers
                if (_remoteClient != nullptr) {
                    _remoteClient->Release();
                }

                _display.Release();
            }

        public:
            EGLNativeWindowType Native() const override
            {
                return _surface.Native();
            }
            std::string Name() const override
            {
                return (_name);
            }
            uint32_t Id() const override
            {
                return _id;
            }
            void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
            {
                ASSERT((_keyboard == nullptr) ^ (keyboard == nullptr));

                if (keyboard == nullptr) {
                    if (_keyboard != nullptr) {
                        _keyboard->Release();
                    }
                } else {
                    keyboard->AddRef();
                }

                _keyboard = keyboard;
            }
            void Pointer(Compositor::IDisplay::IPointer* pointer) override
            {
                ASSERT((_pointer == nullptr) ^ (pointer == nullptr));

                if (pointer == nullptr) {
                    if (_pointer != nullptr) {
                        _pointer->Release();
                    }
                } else {
                    pointer->AddRef();
                }

                _pointer = pointer;
            }
            void Wheel(Compositor::IDisplay::IWheel* wheel) override
            {
                ASSERT((_wheel == nullptr) ^ (wheel == nullptr));

                if (wheel == nullptr) {
                    if (_wheel != nullptr) {
                        _wheel->Release();
                    }
                } else {
                    wheel->AddRef();
                }

                _wheel = wheel;
            }
            void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
            {
                ASSERT((_touchpanel == nullptr) ^ (touchpanel == nullptr));

                if (touchpanel == nullptr) {
                    if (_touchpanel != nullptr) {
                        _touchpanel->Release();
                    }
                } else {
                    touchpanel->AddRef();
                }

                _touchpanel = touchpanel;
            }
            int32_t Width() const override
            {
                return _surface.Width();
            }
            int32_t Height() const override
            {
                return _surface.Height();
            }
            inline void SendKey(const uint32_t key, const IKeyboard::state action, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_keyboard != nullptr) {
                    _keyboard->Direct(key, action);
                }
            }
            inline void SendWheelMotion(const int16_t x, const int16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_wheel != nullptr) {
                    _wheel->Direct(x, y);
                }
            }
            inline void SendPointerButton(const uint8_t button, const IPointer::state state, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_pointer != nullptr) {
                    _pointer->Direct(button, state);
                }
            }
            inline void SendPointerPosition(const int16_t x, const int16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_pointer != nullptr) {
                    _pointer->Direct(x, y);
                }
            }
            inline void SendTouch(const uint8_t index, const ITouchPanel::state state, const uint16_t x, const uint16_t y, const uint32_t timestamp VARIABLE_IS_NOT_USED)
            {
                if (_touchpanel != nullptr) {
                    _touchpanel->Direct(index, state, x, y);
                }
            }

            void Rendered()
            {
                if (_callback != nullptr) {
                    _callback->Rendered(this);
                }
            }
            void Published()
            {
                if (_callback != nullptr) {
                    _callback->Published(this);
                }
            }

            uint32_t Process()
            {
                return Core::ERROR_NONE;
            }

            void RequestRender() override
            {
                _surface.Render();
            }

        private:
            mutable Core::CriticalSection _adminLock;
            const uint8_t _id;
            Display& _display;
            Exchange::IComposition::IClient* _remoteClient;
            GBMSurface _surface;
            const string _name;
            IKeyboard* _keyboard;
            IWheel* _wheel;
            IPointer* _pointer;
            ITouchPanel* _touchpanel;
            ISurface::ICallback* _callback;

            static uint32_t _surfaceIndex;
        };

    public:
        using Surfaces = std::vector<SurfaceImplementation*>;
        using Displays = std::unordered_map<string, Display*>;

        virtual ~Display();

        static Display& Instance(const string& displayName)
        {
            Display* result(nullptr);

            _displaysMapLock.Lock();

            Displays::iterator index(_displays.find(displayName));

            if (index == _displays.end()) {
                result = new Display(displayName);
                _displays.insert(std::pair<const std::string, Display*>(displayName, result));
            } else {
                result = index->second;
            }
            result->AddRef();
            _displaysMapLock.Unlock();

            ASSERT(result != nullptr);

            return (*result);
        }

        uint32_t AddRef() const override
        {
            if (Core::InterlockedIncrement(_refCount) == 1) {
                const_cast<Display*>(this)->Initialize();
            }
            return Core::ERROR_NONE;
        }
        uint32_t Release() const override
        {
            if (Core::InterlockedDecrement(_refCount) == 0) {
                _displaysMapLock.Lock();

                Displays::iterator display = _displays.find(_displayName);

                if (display != _displays.end()) {
                    _displays.erase(display);
                }

                _displaysMapLock.Unlock();

                const_cast<Display*>(this)->Deinitialize();

                delete this;

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }
            return (Core::ERROR_NONE);
        }

        EGLNativeDisplayType Native() const override
        {
            return static_cast<EGLNativeDisplayType>(_gbmDevice);
        }

        const std::string& Name() const final override
        {
            return _displayName;
        }

        ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height, ISurface::ICallback* callback) override;

        int Process(const uint32_t data VARIABLE_IS_NOT_USED) override
        {
            for (SurfaceImplementation* surface : _surfaces) {
                surface->Process();
            }

            return Core::ERROR_NONE;
        }

        int FileDescriptor() const override
        {
            return _gpuId;
        }

        ISurface* SurfaceByName(const std::string& name) override
        {
            IDisplay::ISurface* result = nullptr;

            _adminLock.Lock();

            Surfaces::iterator index(_surfaces.begin());
            while ((index != _surfaces.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != _surfaces.end()) {
                result = *index;
                result->AddRef();
            }

            _adminLock.Unlock();

            return result;
        }

        const Exchange::IComposition::IDisplay* RemoteDisplay() const
        {
            return _remoteDisplay;
        }
        Exchange::IComposition::IDisplay* RemoteDisplay()
        {
            return _remoteDisplay;
        }

    private:
        void Initialize()
        {
            TRACE(Trace::Information, (_T("PID: %d: Compositor connector: %s"), getpid(), DisplayConnector().c_str()));
            TRACE(Trace::Information, (_T("PID: %d: Client connector: %s"), getpid(), BufferConnector().c_str()));
            TRACE(Trace::Information, (_T("PID: %d: Input connector: %s"), getpid(), InputConnector().c_str()));

            _adminLock.Lock();

            if (Thunder::Core::WorkerPool::IsAvailable() == true) {
                // If we are in the same process space as where a WorkerPool is registered (Main Process or
                // hosting process) use, it!
                Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(DisplayConnector().c_str()), Core::ProxyType<Core::IIPCServer>(engine));
            } else {
                // Seems we are not in a process space initiated from the Main framework process or its hosting process.
                // Nothing more to do than to create a workerpool for RPC our selves !
                Core::ProxyType<RPC::InvokeServerType<2, 0, 8>> engine = Core::ProxyType<RPC::InvokeServerType<2, 0, 8>>::Create();

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(DisplayConnector().c_str()), Core::ProxyType<Core::IIPCServer>(engine));
            }

            uint32_t result = _compositorServerRPCConnection->Open(RPC::CommunicationTimeOut);

            if (result == Core::ERROR_NONE) {
                _remoteDisplay = _compositorServerRPCConnection->Acquire<Exchange::IComposition::IDisplay>(2000, _displayName, ~0);

                if (_remoteDisplay == nullptr) {
                    TRACE(Trace::Error, (_T ( "Could not create remote display for Display %s!" ), Name().c_str()));
                }
            } else {
                TRACE(Trace::Error, (_T("Could not open connection to Compositor with node %s. Error: %s"), _compositorServerRPCConnection->Source().RemoteId().c_str(), Core::NumberType<uint32_t>(result).Text().c_str()));
                _compositorServerRPCConnection.Release();
            }

            _virtualinput = virtualinput_open(_displayName.c_str(), InputConnector().c_str(), VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

            if (_virtualinput == nullptr) {
                TRACE(Trace::Error, (_T("Initialization of virtual input failed for Display %s!"), Name().c_str()));
            }

            _adminLock.Unlock();
        }

        void Deinitialize()
        {
            _adminLock.Lock();

            if (_virtualinput != nullptr) {
                virtualinput_close(_virtualinput);
                _virtualinput = nullptr;
            }

            Surfaces::iterator index(_surfaces.begin());
            while (index != _surfaces.end()) {
                string name = (*index)->Name();

                if ((*index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) { // note, need cast to prevent ambiguous call
                    TRACE(Trace::Error, (_T("Compositor Surface [%s] is not properly destructed" ), name.c_str()));
                }

                index = _surfaces.erase(index);
            }

            if (_remoteDisplay != nullptr) {
                _remoteDisplay->Release();
                _remoteDisplay = nullptr;
            }

            if (_compositorServerRPCConnection.IsValid() == true) {
                _compositorServerRPCConnection->Close(RPC::CommunicationTimeOut);
                _compositorServerRPCConnection.Release();
            }

            _adminLock.Unlock();
        }

        void Register(SurfaceImplementation* surface)
        {
            ASSERT(surface != nullptr);

            _adminLock.Lock();

            Surfaces::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            ASSERT(index == _surfaces.end());

            if (index == _surfaces.end()) {
                _surfaces.push_back(surface);
            }

            _adminLock.Unlock();
        }

        void Unregister(SurfaceImplementation* surface)
        {
            ASSERT(surface != nullptr);

            _adminLock.Lock();

            auto index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            ASSERT(index != _surfaces.end());

            if (index != _surfaces.end()) {
                _surfaces.erase(index);
            }

            _adminLock.Unlock();
        }

        Thunder::Exchange::IComposition::IClient* CreateRemoteSurface(const std::string& name, const uint32_t width, const uint32_t height)
        {
            return (_remoteDisplay != nullptr ? _remoteDisplay->CreateClient(name, width, height) : nullptr);
        }

    private:
        static Displays _displays;
        static Core::CriticalSection _displaysMapLock;

    private:
        const std::string _displayName;
        mutable Core::CriticalSection _adminLock;
        mutable uint32_t _refCount;
        void* _virtualinput;
        Surfaces _surfaces;
        Core::ProxyType<RPC::CommunicatorClient> _compositorServerRPCConnection;
        Exchange::IComposition::IDisplay* _remoteDisplay;
        int _gpuId;
        gbm_device* _gbmDevice;
    }; // class Display

    uint32_t Display::SurfaceImplementation::_surfaceIndex = 0;

    Display::Displays Display::_displays;
    Core::CriticalSection Display::_displaysMapLock;

    Display::Display(const string& name)
        : _displayName(name)
        , _adminLock()
        , _refCount(0)
        , _virtualinput(nullptr)
        , _surfaces()
        , _compositorServerRPCConnection()
        , _remoteDisplay(nullptr)
        , _gpuId(-1)
        , _gbmDevice(nullptr)
    {
        Core::PrivilegedRequest::Container descriptors;
        Core::PrivilegedRequest request;

        if (request.Request(1000, BufferConnector(), DisplayId, descriptors) == Core::ERROR_NONE) {
            ASSERT(descriptors.size() == 1);
            _gpuId = descriptors[0].Move();
            _gbmDevice = gbm_create_device(_gpuId);
            TRACE(Trace::Information, (_T ( "Opened GBM[%p] device on fd: %d,  RenderDevice: %s"), _gbmDevice, _gpuId, drmGetRenderDeviceNameFromFd(_gpuId)));
        } else {
            TRACE(Trace::Error, (_T ( "Failed to get display file descriptor from compositor server")));
        }

        TRACE(Trace::Information, (_T("Display[%p] Constructed build @ %s"), this, __TIMESTAMP__));
    }

    Display::~Display()
    {
        Deinitialize();

        if (_gbmDevice != nullptr) {
            gbm_device_destroy(_gbmDevice);
            _gbmDevice = nullptr;
        }

        if (_gpuId > 0) {
            ::close(_gpuId);
            _gpuId = -1;
        }

        TRACE(Trace::Information, (_T ( "Display[%p] Destructed"), this));
    }

    Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height, ISurface::ICallback* callback)
    {
        Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, width, height, callback));
        Compositor::IDisplay::ISurface* result = &(*retval);
        result->AddRef();
        return result;
    }

    /* static */ void Display::Publish(InputFunction& action)
    {
        if (action != nullptr) {
            _displaysMapLock.Lock();

            for (std::pair<const string, Display*>& entry : _displays) {
                entry.second->_adminLock.Lock();

                std::for_each(begin(entry.second->_surfaces), end(entry.second->_surfaces), action);

                entry.second->_adminLock.Unlock();
            }

            _displaysMapLock.Unlock();
        }
    }

    /* static */ void Display::VirtualKeyboardCallback(keyactiontype type, unsigned int code)
    {
        if (type != KEY_COMPLETED) {
            time_t timestamp = time(nullptr);
            const IDisplay::IKeyboard::state state = ((type == KEY_RELEASED) ? IDisplay::IKeyboard::released
                                                                             : ((type == KEY_REPEAT) ? IDisplay::IKeyboard::repeated
                                                                                                     : IDisplay::IKeyboard::pressed));

            InputFunction action = [=](SurfaceImplementation* s) {
                s->SendKey(code, state, timestamp);
            };

            Publish(action);
        }
    }

    /* static */ void Display::VirtualMouseCallback(mouseactiontype type, unsigned short button, signed short horizontal, signed short vertical)
    {
        static int32_t pointer_x = 0;
        static int32_t pointer_y = 0;

        time_t timestamp = time(nullptr);
        InputFunction action;
        pointer_x = pointer_x + horizontal;
        pointer_y = pointer_y + vertical;

        switch (type) {
        case MOUSE_MOTION:
            action = [=](SurfaceImplementation* s) {
                int32_t X = std::min(std::max(0, pointer_x), s->Width());
                int32_t Y = std::min(std::max(0, pointer_y), s->Height());
                s->SendPointerPosition(X, Y, timestamp);
            };
            break;
        case MOUSE_SCROLL:
            action = [=](SurfaceImplementation* s) {
                s->SendWheelMotion(horizontal, vertical, timestamp);
            };
            break;
        case MOUSE_RELEASED:
        case MOUSE_PRESSED:
            action = [=](SurfaceImplementation* s) {
                s->SendPointerButton(button, type == MOUSE_RELEASED ? IDisplay::IPointer::released : IDisplay::IPointer::pressed, timestamp);
            };
            break;
        default:
            assert(false);
        }

        Publish(action);
    }

    /* static */ void Display::VirtualTouchScreenCallback(touchactiontype type, unsigned short index, unsigned short x, unsigned short y)
    {
        static uint16_t touch_x = ~0;
        static uint16_t touch_y = ~0;
        static touchactiontype last_type = TOUCH_RELEASED;

        // Get touch position in pixels
        // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
        if ((x != touch_x)
            || (y != touch_y)
            || (type != last_type)) {

            last_type = type;
            touch_x = x;
            touch_y = y;

            time_t timestamp = time(nullptr);
            const IDisplay::ITouchPanel::state state = ((type == TOUCH_RELEASED) ? ITouchPanel::released
                                                                                 : ((type == TOUCH_PRESSED) ? ITouchPanel::pressed
                                                                                                            : ITouchPanel::motion));

            InputFunction action = [=](SurfaceImplementation* s) {
                const uint16_t mapped_x = (s->Width() * x) >> 16;
                const uint16_t mapped_y = (s->Height() * y) >> 16;
                s->SendTouch(index, state, mapped_x, mapped_y, timestamp);
            };

            Publish(action);
        }
    }
} // namespace Linux

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    return (&(Linux::Display::Instance(displayName)));
}
} // namespace Thunder
