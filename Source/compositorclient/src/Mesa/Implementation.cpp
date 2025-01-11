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
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
}

#include <com/com.h>

#include <interfaces/IComposition.h>
#include <virtualinput/virtualinput.h>
#include <privilegedrequest/PrivilegedRequest.h>

#include <interfaces/ICompositionBuffer.h>
#include <compositorbuffer/CompositorBufferType.h>

#include <compositor/Client.h>
#include "RenderAPI.h"

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
            class EGLBuffer : public Compositor::ClientBuffer {
            public:
                EGLBuffer(EGLBuffer&&) = delete;
                EGLBuffer(const EGLBuffer&) = delete;
                EGLBuffer& operator=(EGLBuffer&&) = delete;
                EGLBuffer& operator=(const EGLBuffer&) = delete;

                EGLBuffer(SurfaceImplementation& parent)
                    : Compositor::ClientBuffer()
                    , _parent(parent)
                    , _display(EGL_NO_DISPLAY)
                    , _textureId(0)
                    , _frameBuffer(0)
                    , _eglImage(EGL_NO_IMAGE)
                    , _eglSync()
                {
                }
                ~EGLBuffer()
                {
                    if (_frameBuffer != 0) {
                        glDeleteFramebuffers(1, &_frameBuffer);
                    }
                    if (_textureId != 0) {
                        glDeleteTextures(1, &_textureId);
                    }
                    if (_eglSync != nullptr) {
                        _egl.eglDestroySync(_display, _eglSync);
                    }
                    if (_eglImage != EGL_NO_IMAGE) {
                        _egl.eglDestroyImage(_display, _eglImage);
                    }
                }

            public:
                /*
                 * @brief   Creates a EGL image using the current EGL context.
                 *
                 * @note    This should be called AFTER an EGL is initialised in the
                 *          parent process.
                 */
                EGLImage CreateImage()
                {
                    ICompositionBuffer::IIterator* planes = Acquire(Core::infinite);

                    EGLImage eglImage = EGL_NO_IMAGE;

                    if (planes != nullptr) {
                        uint32_t width = Compositor::ClientBuffer::Width();
                        uint32_t height = Compositor::ClientBuffer::Height();
                        uint32_t format = Compositor::ClientBuffer::Format();
                        uint64_t modifier = Compositor::ClientBuffer::Modifier();

                        _display = eglGetCurrentDisplay();

                        ASSERT(_display != EGL_NO_DISPLAY);

                        _eglSync = _egl.eglCreateSync(_display, EGL_SYNC_FENCE, NULL);

                        planes->Next();
                        ASSERT(planes->IsValid() == true);

                        Compositor::API::Attributes<EGLAttrib> imageAttributes;

                        imageAttributes.Append(EGL_WIDTH, width);
                        imageAttributes.Append(EGL_HEIGHT, height);
                        imageAttributes.Append(EGL_LINUX_DRM_FOURCC_EXT, format);

                        imageAttributes.Append(EGL_DMA_BUF_PLANE0_FD_EXT, planes->Descriptor());
                        imageAttributes.Append(EGL_DMA_BUF_PLANE0_OFFSET_EXT, planes->Offset());
                        imageAttributes.Append(EGL_DMA_BUF_PLANE0_PITCH_EXT, planes->Stride());
                        imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, modifier & 0xFFFFFFFF);
                        imageAttributes.Append(EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, modifier >> 32);

                        TRACE(Trace::Information, (_T("Add Plane 0 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), planes->Descriptor(), planes->Offset(), planes->Stride(), modifier));

                        if (planes->Next() == true) {
                            imageAttributes.Append(EGL_DMA_BUF_PLANE1_FD_EXT, planes->Descriptor());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE1_OFFSET_EXT, planes->Offset());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE1_PITCH_EXT, planes->Stride());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, modifier & 0xFFFFFFFF);
                            imageAttributes.Append(EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, modifier >> 32);

                            TRACE(Trace::Information, (_T("Add Plane 1 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), planes->Descriptor(), planes->Offset(), planes->Stride(), modifier));
                        }

                        if (planes->Next() == true) {
                            imageAttributes.Append(EGL_DMA_BUF_PLANE2_FD_EXT, planes->Descriptor());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE2_OFFSET_EXT, planes->Offset());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE2_PITCH_EXT, planes->Stride());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, modifier & 0xFFFFFFFF);
                            imageAttributes.Append(EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT, modifier >> 32);

                            TRACE(Trace::Information, (_T("Add Plane 2 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), planes->Descriptor(), planes->Offset(), planes->Stride(), modifier));
                        }

                        if (planes->Next() == true) {
                            imageAttributes.Append(EGL_DMA_BUF_PLANE3_FD_EXT, planes->Descriptor());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE3_OFFSET_EXT, planes->Offset());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE3_PITCH_EXT, planes->Stride());
                            imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, (modifier & 0xFFFFFFFF));
                            imageAttributes.Append(EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT, (modifier >> 32));

                            TRACE(Trace::Information, (_T("Add Plane 3 fd=%d, offset=%d, stride=%d, modifier=%" PRIu64), planes->Descriptor(), planes->Offset(), planes->Stride(), modifier));
                        }

                        Relinquish();

                        imageAttributes.Append(EGL_IMAGE_PRESERVED_KHR, EGL_TRUE);

                        eglImage = _egl.eglCreateImage(_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, imageAttributes);

                        glFlush(); // Mandatory

                        TRACE(Trace::Information, (_T("Created image %dx%d(hxb) for buffer, _eglImage=%p, format=0x%04X"), height, width, eglImage, format));
                    } else {
                        TRACE(Trace::Error, (_T("Could not acquire the buffer planes in time")));
                    }

                    return eglImage;
                }
                bool Render()
                {
                    bool succeeded = false;

                    // Make sure this done at the right time, after eglInitialize and only needed once.
                    if (_eglImage == EGL_NO_IMAGE) {
                        _eglImage = CreateImage();
                    }

                    ASSERT(_eglImage != EGL_NO_IMAGE);

                    // Lock the buffer
                    if (Acquire(100) != nullptr) {
                        constexpr const GLuint target = GL_TEXTURE_2D;
                        constexpr const GLuint filter = GL_LINEAR;
                        constexpr const GLuint wrap = GL_CLAMP_TO_EDGE;

                        // Just an arbitrary selected unit
                        glActiveTexture(GL_TEXTURE0);

                        if (_textureId != 0) {
                            glDeleteTextures(1, &_textureId);
                        }

                        // GLES (extension: GL_OES_EGL_image_external): Create GL texture from EGL image
                        glGenTextures(1, &_textureId);
                        glBindTexture(target, _textureId);
                        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
                        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
                        glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
                        glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);

                        _gl.glEGLImageTargetTexture2DOES(target, _eglImage);

                        if (_frameBuffer != 0) {
                            glDeleteFramebuffers(1, &_frameBuffer);
                        }

                        glGenFramebuffers(1, &_frameBuffer);
                        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);

                        // Bind the created texture as one of the buffers of the frame buffer object
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _textureId, 0 /* level */);

                        succeeded = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

                        if (succeeded != true) {
                            glDeleteFramebuffers(1, &_frameBuffer);
                            glDeleteTextures(1, &_textureId);

                            _frameBuffer = 0;
                            _textureId = 0;
                        }

                        // Wait for all EGL actions to be completed
                        _egl.eglClientWaitSync(_display, _eglSync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);

                        Relinquish();

                        // Signal the other side we have a completed buffer, ready to show...
                        RequestRender();
                    }
                    return (succeeded);
                }
                void Rendered() override
                {
                    _parent.Rendered();
                }
                void Published() override
                {
                    _parent.Published();
                }

            private:
                SurfaceImplementation& _parent;
                EGLDisplay _display;
                GLuint _textureId;
                GLuint _frameBuffer;
                EGLImage _eglImage;
                EGLSync _eglSync;
                Compositor::API::EGL _egl;
                Compositor::API::GL _gl;
            };

        public:
            SurfaceImplementation() = delete;
            SurfaceImplementation(SurfaceImplementation&&) = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(SurfaceImplementation&&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
                : _adminLock()
                , _id(Core::InterlockedIncrement(_surfaceIndex))
                , _name()
                , _display(display)
                , _remoteClient(nullptr)
                , _keyboard(nullptr)
                , _wheel(nullptr)
                , _pointer(nullptr)
                , _touchpanel(nullptr)
                , _surface(nullptr)
                , _buffer(*this)
            {
                TRACE(Trace::Information, (_T("Construct surface[%d] %s  %dx%d (hxb)"), _id, name.c_str(), height, width));

                _display.AddRef();

                _remoteClient = _display.CreateRemoteSurface(name, width, height);

                if (_remoteClient != nullptr) {
                    TRACE(Trace::Information, (_T("Created remote surface %s  %dx%d"), name.c_str(), width, height));

                    Core::PrivilegedRequest::Container descriptors;
                    Core::PrivilegedRequest request;

                    _name = _remoteClient->Name();

                    if (request.Request(1000, BufferConnector(), _remoteClient->Native(), descriptors) == Core::ERROR_NONE) {
                        _buffer.Load(descriptors);

                        _surface = gbm_surface_create(
                            static_cast<gbm_device*>(_display.Native()),
                            _buffer.Width(), _buffer.Height(), _buffer.Format(),
                            GBM_BO_USE_RENDERING /* used for rendering */);

                        ASSERT(_surface != nullptr);

                        Core::ResourceMonitor::Instance().Register(_buffer);

                        TRACE(Trace::Information, (_T("Surface %p ready %dx%d format=0x%04X"), _surface, _buffer.Width(), _buffer.Height(), _buffer.Format()));
                    }
                }

                _display.Register(this);
            }
            ~SurfaceImplementation() override
            {
                _display.Unregister(this);

                Core::ResourceMonitor::Instance().Unregister(_buffer);

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

                if (_surface != nullptr) {
                    gbm_surface_destroy(_surface);
                }
            }

        public:
            EGLNativeWindowType Native() const override
            {
                return (static_cast<EGLNativeWindowType>(_surface));
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
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;
                _keyboard->AddRef();
            }
            void Pointer(Compositor::IDisplay::IPointer* pointer) override
            {
                assert((_pointer == nullptr) ^ (pointer == nullptr));
                _pointer = pointer;
                _pointer->AddRef();
            }
            void Wheel(Compositor::IDisplay::IWheel* wheel) override
            {
                assert((_wheel == nullptr) ^ (wheel == nullptr));
                _wheel = wheel;
                _wheel->AddRef();
            }
            void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
            {
                assert((_touchpanel == nullptr) ^ (touchpanel == nullptr));
                _touchpanel = touchpanel;
                _touchpanel->AddRef();
            }
            int32_t Width() const override
            {
                return (static_cast<int32_t>(_buffer.Width()));
            }
            int32_t Height() const override
            {
                return (static_cast<int32_t>(_buffer.Height()));
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
                _display.Rendered(this);
            }
            void Published()
            {
                _display.Published(this);
            }

            uint32_t Process()
            {
                return (_buffer.Render() == true) ? Core::ERROR_NONE : Core::ERROR_BAD_REQUEST;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            const uint8_t _id;
            string _name;
            Display& _display;
            Exchange::IComposition::IClient* _remoteClient;
            IKeyboard* _keyboard;
            IWheel* _wheel;
            IPointer* _pointer;
            ITouchPanel* _touchpanel;
            struct gbm_surface* _surface;
            Core::SinkType<EGLBuffer> _buffer;

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

        ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override;

        int Process(const uint32_t data VARIABLE_IS_NOT_USED) override
        {
            std::unique_lock<std::mutex> lock(_rendering);

            for (auto begin = _surfaces.begin(), it = begin, end = _surfaces.end(); it != end; it++) {
                SurfaceImplementation* surface = *it;

                if (surface->Process() == Core::ERROR_NONE) {
                    _pendingSurfaces |= (1 << surface->Id());
                }
            }

            while (_pendingSurfaces != 0) {
                _published.wait(lock);
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

        void Rendered(SurfaceImplementation* surface)
        {
        }

        void Published(SurfaceImplementation* surface)
        {
            _pendingSurfaces &= ~(1 << surface->Id());
            _published.notify_all();
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
        uint32_t _pendingSurfaces;
        std::mutex _rendering;
        std::condition_variable _published;
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
        , _pendingSurfaces(0)
        , _rendering()
        , _published()
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

    Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
    {
        Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, width, height));
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
