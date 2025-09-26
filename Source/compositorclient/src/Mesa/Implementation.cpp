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
#include <interfaces/IGraphicsBuffer.h>

#include "RenderAPI.h"
#include <compositor/Client.h>

#include <mutex>

namespace Thunder {
namespace Linux {
    namespace {
        const string ConnectorPath()
        {
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("XDG_RUNTIME_DIR"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/Compositor/");
            } else {
                connector = Core::Directory::Normalize(connector);
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

        constexpr std::array<uint32_t, 5> FormatPriority = {
            DRM_FORMAT_ARGB8888, // Best overall - universal support with full alpha
            DRM_FORMAT_ABGR8888, // Alternative byte order, still 32-bit and full alpha
            DRM_FORMAT_XRGB8888, // Best for opaque content
            DRM_FORMAT_XBGR8888, // Alternative opaque format
            DRM_FORMAT_RGB565 // Fallback - memory efficient, widely supported
        };

        const char* GetGbmBackendName(gbm_device* gbmDevice)
        {
            static gbm_device* cachedDevice = nullptr;
            static const char* cachedName = nullptr;

            if (gbmDevice != nullptr && gbmDevice != cachedDevice) {
                cachedName = gbm_device_get_backend_name(gbmDevice);
                cachedDevice = gbmDevice;
                TRACE_GLOBAL(Trace::Information, (_T("GBM Backend: %s"), cachedName));
            }
            return cachedName;
        }

        bool IsGbmBackend(gbm_device* gbmDevice, const char* name)
        {
            const char* backendName = GetGbmBackendName(gbmDevice);
            return (backendName != nullptr) && (strcmp(backendName, name) == 0);
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
            // for now only single plane buffers are supported, e.g. GBM_FORMAT_ARGB8888, GBM_FORMAT_XRGB8888, etc.
            class ContentBuffer : public Graphics::ClientBufferType<1> {
                using BaseClass = Graphics::ClientBufferType<1>;

            public:
                ContentBuffer() = delete;
                ContentBuffer(ContentBuffer&&) = delete;
                ContentBuffer(const ContentBuffer&) = delete;
                ContentBuffer& operator=(ContentBuffer&&) = delete;
                ContentBuffer& operator=(const ContentBuffer&) = delete;

                ContentBuffer(SurfaceImplementation& parent, gbm_bo* frameBuffer)
                    : BaseClass(gbm_bo_get_width(frameBuffer), gbm_bo_get_height(frameBuffer), gbm_bo_get_format(frameBuffer), gbm_bo_get_modifier(frameBuffer), Exchange::IGraphicsBuffer::TYPE_DMA)
                    , _parent(parent)
                    , _bo(frameBuffer)
                {
                    ASSERT(_bo != nullptr);

                    if (_bo != nullptr) {
                        // for now only single plane buffers are supported
                        ASSERT(gbm_bo_get_plane_count(_bo) == 1); 

                        Add(gbm_bo_get_fd_for_plane(_bo, 0),
                            gbm_bo_get_stride_for_plane(_bo, 0),
                            gbm_bo_get_offset(_bo, 0));

                        std::array<int, Core::PrivilegedRequest::MaxDescriptorsPerRequest> descriptors;
                        descriptors.fill(-1);

                        const uint8_t nDescriptors = Descriptors(descriptors.size(), descriptors.data());

                        if (nDescriptors > 0) {
                            Core::PrivilegedRequest::Container container(descriptors.begin(), descriptors.begin() + nDescriptors);
                            Core::PrivilegedRequest request;
                            
                            const string connector = ConnectorPath() + _T("descriptors");

                            if (request.Offer(100, connector, parent.Id(), container) == Core::ERROR_NONE) {
                                TRACE(Trace::Information, (_T("Offered buffer to compositor server")));
                            } else {
                                TRACE(Trace::Error, (_T("Failed to offer buffer to compositor server")));
                            }
                        }
                    }

                    Core::ResourceMonitor::Instance().Register(*this);
                }

                virtual ~ContentBuffer()
                {
                    Core::ResourceMonitor::Instance().Unregister(*this);
                };

                static void Destroyed(gbm_bo* bo, void* data)
                {
                    ASSERT(data != nullptr);
                    ContentBuffer* buffer = static_cast<ContentBuffer*>(data);

                    TRACE_GLOBAL(Trace::Information, (_T("ContentBuffer[%p] Destroyed signaled"), buffer));

                    if ((buffer != nullptr) && (bo == buffer->_bo)) {
                        delete buffer;
                    } else {
                        TRACE_GLOBAL(Trace::Error, (_T("ContentBuffer[%p] Destroyed signaled with mismatched gbm_bo[%p]"), buffer, bo));
                    }
                }

            protected:
                void Rendered() override
                {
                    _parent.Unlock(_bo);
                    _parent.Rendered();
                }

                void Published() override
                {
                    _parent.Published();
                }

            private:
                SurfaceImplementation& _parent;
                gbm_bo* _bo;
            };

        public:
            SurfaceImplementation() = delete;
            SurfaceImplementation(SurfaceImplementation&&) = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(SurfaceImplementation&&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height, ICallback* callback)
                : _display(display)
                , _gbmSurface(display.CreateGbmSurface(width, height))
                , _remoteClient(display.CreateRemoteSurface(name, width, height))
                , _id(_remoteClient->Native())
                , _width(width)
                , _height(height)
                , _gbmBufferLock()
                , _name(name)
                , _keyboard(nullptr)
                , _wheel(nullptr)
                , _pointer(nullptr)
                , _touchpanel(nullptr)
                , _callback(callback)
            {
                _display.AddRef();

                ASSERT(_remoteClient != nullptr);
                ASSERT(_gbmSurface != nullptr);

                TRACE(Trace::Information, (_T("Construct surface[%d] %s  %dx%d (hxb)"), _id, name.c_str(), height, width));

                _display.Register(this);
            }
            ~SurfaceImplementation() override
            {
                _display.Unregister(this);

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

                // lets hope DRM cleans up the gbm buffer objects for us, if so drm should call ContentBuffer::Destroyed()...
                if (_gbmSurface != nullptr) {
                    gbm_surface_destroy(_gbmSurface);
                    _gbmSurface = nullptr;
                }

                _display.Release();
            }

        private:
            gbm_bo* Lock(const uint16_t ms)
            {
                ASSERT((_gbmSurface != nullptr) && "Failed to lock a framebuffer, surface is null");

                gbm_bo* frameBuffer = nullptr;

                if (_gbmBufferLock.try_lock_for(std::chrono::milliseconds(ms))) {
                    frameBuffer = gbm_surface_lock_front_buffer(_gbmSurface);

                    if (frameBuffer == nullptr) {
                        _gbmBufferLock.unlock(); // Unlock the mutex to prevent deadlock
                        TRACE(Trace::Error, (_T("Failed to lock front buffer, surface %p"), _gbmSurface));
                    } else {
                        TRACE(Trace::Information, (_T("Acquired framebuffer[%p]"), frameBuffer));
                    }
                } else {
                    TRACE(Trace::Error, (_T("Failed to lock front buffer within %d ms"), ms));
                }

                return frameBuffer;
            }

            void Unlock(gbm_bo* frameBuffer)
            {
                ASSERT((_gbmSurface != nullptr) && "Failed to release framebuffer, surface is null");

                if ((_gbmSurface != nullptr) && (frameBuffer != nullptr)) {
                    gbm_surface_release_buffer(_gbmSurface, frameBuffer);
                    TRACE(Trace::Information, (_T("Released framebuffer[%p]"), frameBuffer));
                }

                _gbmBufferLock.unlock();
            }

        public:
            EGLNativeWindowType Native() const override
            {
                return static_cast<EGLNativeWindowType>(_gbmSurface);
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
                return _width; // not sure if we need to return the real height or the scaled height
            }
            int32_t Height() const override
            {
                return _height; // not sure if we need to return the real height or the scaled height
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

            void RequestRender() override
            {
                gbm_bo* frameBuffer = Lock(1000);

                if (frameBuffer == nullptr) {
                    // bail-out if we cannot lock the front buffer
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    Rendered(); // we still need call Rendered to request a new frame buffer
                } else {
                    ContentBuffer* buffer = static_cast<ContentBuffer*>(gbm_bo_get_user_data(frameBuffer));

                    if (buffer == nullptr) {
                        buffer = new ContentBuffer(*this, frameBuffer);
                        gbm_bo_set_user_data(frameBuffer, buffer, &ContentBuffer::Destroyed);
                    }

                    ASSERT(buffer != nullptr);
                    buffer->RequestRender();
                }
            }

            uint32_t Process()
            {
                return Core::ERROR_NONE;
            }

        private:
            Display& _display;
            gbm_surface* _gbmSurface;
            Exchange::IComposition::IClient* _remoteClient;
            const uint8_t _id;
            const int32_t _width; // real pixels allocated in the gpu!
            const int32_t _height; // real pixels allocated in the gpu
            std::timed_mutex _gbmBufferLock;
            const string _name;
            IKeyboard* _keyboard;
            IWheel* _wheel;
            IPointer* _pointer;
            ITouchPanel* _touchpanel;
            ISurface::ICallback* _callback;

            static uint32_t _surfaceIndex;
        }; // class SurfaceImplementation

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
            const std::string comrpcPath(ConnectorPath() + _T("comrpc"));

            _adminLock.Lock();

            if (Thunder::Core::WorkerPool::IsAvailable() == true) {
                // If we are in the same process space as where a WorkerPool is registered (Main Process or
                // hosting process) use, it!
                Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(comrpcPath.c_str()), Core::ProxyType<Core::IIPCServer>(engine));
            } else {
                // Seems we are not in a process space initiated from the Main framework process or its hosting process.
                // Nothing more to do than to create a workerpool for RPC our selves !
                Core::ProxyType<RPC::InvokeServerType<2, 0, 8>> engine = Core::ProxyType<RPC::InvokeServerType<2, 0, 8>>::Create();

                _compositorServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(comrpcPath.c_str()), Core::ProxyType<Core::IIPCServer>(engine));
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

        Exchange::IComposition::IClient* CreateRemoteSurface(const std::string& name, const uint32_t width, const uint32_t height)
        {
            return (_remoteDisplay != nullptr ? _remoteDisplay->CreateClient(name, width, height) : nullptr);
        }
        
        gbm_surface* CreateGbmSurface(const uint32_t width, const uint32_t height) const
        {
            gbm_surface* surface(nullptr);

            uint32_t usage(0);

            if (IsGbmBackend(static_cast<gbm_device*>(_gbmDevice), "nvidia") == false) {
                usage |= GBM_BO_USE_RENDERING; // the nvidia backend seems to have issues with usage flags....
            }

            for (uint32_t format : FormatPriority) {
                char* formatName = drmGetFormatName(format);

                ASSERT(formatName != nullptr); // Should always be valid for known DRM formats

                surface = gbm_surface_create(_gbmDevice, width, height, format, usage);
                if (surface != nullptr) {
                    TRACE(Trace::Information, ("Successfully created surface with format: %s", formatName ? formatName : "Unknown"));
                    break;
                }
                TRACE(Trace::Warning, ("Failed to create GBM surface with format: %s, trying next...", formatName ? formatName : "Unknown"));

                if (formatName != nullptr) {
                    free(formatName);
                }
            }

            return surface;
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

        const string connector = ConnectorPath() + _T("descriptors");

        if (request.Request(1000, connector, DisplayId, descriptors) == Core::ERROR_NONE) {
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
