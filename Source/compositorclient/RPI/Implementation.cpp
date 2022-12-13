/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

// FIXME: This flag should be included in the preprocessor flags
#define __GBM__

#include "Headless.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <gbm.h>
#include <xf86drm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <fcntl.h>

#ifdef __cplusplus
}
#endif

#include <mutex>
#include <algorithm>
#include <chrono>
#include <core/core.h>
#include <com/com.h>
#include <interfaces/IComposition.h>
#include <virtualinput/virtualinput.h>
#include "../Client.h"

#include "include/traits.h"

// Platform specific types
static_assert(std::is_convertible<struct gbm_device*, EGLNativeDisplayType>::value);
static_assert(std::is_convertible<struct gbm_surface*, EGLNativeWindowType>::value);

#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS
#define _EGL_SIGNALED EGL_SIGNALED
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT
#else
#define _KHRFIX(left, right) left ## right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
#define _EGL_CONDITION_SATISFIED EGL_CONDITION_SATISFIED_KHR
#define _EGL_SYNC_FLUSH_COMMANDS_BIT EGL_SYNC_FLUSH_COMMANDS_BIT_KHR
#define _EGL_SIGNALED EGL_SIGNALED_KHR
#define _EGL_SYNC_STATUS EGL_SYNC_STATUS_KHR
#endif

namespace WPEFramework {
namespace RPI {
    class Display;
}
}
 
namespace WPEFramework {
namespace RPI_INTERNAL {

    class GLResourceMediator {
    public :

        class Native final {
        public :

            class Prime final {
            public :

                Prime() = delete;
                Prime(const Prime&) = delete;
                Prime& operator=(const Prime&) = delete;

                explicit Prime(int, int, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t);
                ~Prime();

                explicit Prime(Prime&&);

                Prime& operator=(Prime&&);

                int Fd() const { return _fd; }
                int SyncFd() const { return _sync_fd; }

                uint64_t Modifier() const { return _modifier; }
                uint32_t Format() const  { return _format; }
                uint32_t Stride() const { return _stride; }

                uint32_t Height() const { return _height; }
                uint32_t Width() const { return _width; }

                bool Lock() const;
                bool Unlock() const;

                static constexpr int InvalidFiledescriptor() { return RenderDevice::GBM::InvalidFiledescriptor(); }
                static constexpr uint64_t InvalidModifier() { return RenderDevice::GBM::InvalidModifier(); }
                static constexpr uint32_t InvalidFormat() { return RenderDevice::GBM::InvalidFormat(); }
                static constexpr uint32_t InvalidStride() { return RenderDevice::GBM::InvalidStride(); }
                static constexpr uint32_t InvalidWidth() { return  RenderDevice::GBM::InvalidWidth(); }
                static constexpr uint32_t InvalidHeight() { return RenderDevice::GBM::InvalidHeight(); } 

                bool IsValid() const;

            private :

                bool InitializeLocks() const;

                int _fd;
                int _sync_fd;

                uint64_t _modifier;
                uint32_t _format;
                uint32_t _stride;

                uint32_t _height;
                uint32_t _width;

                mutable struct flock _acquire;
                mutable struct flock _release;
            };

            Native() = delete;
            Native(const Native&) = delete;
            explicit Native(Native&&) = delete;

            Native& operator=(const Native&) = delete;
            Native& operator=(Native&&) = delete;
 
            explicit Native(const WPEFramework::RPI::Display&, uint32_t, uint32_t);

            ~Native();

            const Prime& GetPrime() const { return _prime; }

            struct gbm_device* GetNativeDisplay() const { return _device; }
            struct gbm_surface* GetNativeSurface() const { return _surf; }

            uint32_t Width() const { return _width; }
            uint32_t Height() const { return _height; }

            static constexpr struct gbm_device* InvalidDevice() { return RenderDevice::GBM::InvalidDevice(); }
            static constexpr struct gbm_surface* InvalidSurface() { return RenderDevice::GBM::InvalidSurface(); }
            static constexpr uint32_t InvalidWidth() { return RenderDevice::GBM::InvalidWidth(); }
            static constexpr uint32_t InvalidHeight() { return RenderDevice::GBM::InvalidHeight(); }

            bool Invalidate();

            // For each valid prime a surface is created
            bool AddPrime(Prime&& prime);

            bool IsValid();

            static struct gbm_device* CreateNativeDisplay();
            static bool DestroyNativeDisplay(struct gbm_device*);

        private :

            struct gbm_surface* CreateNativeSurface(struct gbm_device* device, uint32_t width, uint32_t height);
            bool DestroyNativeSurface(struct gbm_device* device, struct gbm_surface* surface);

            Prime _prime;

            struct gbm_device* _device;
            struct gbm_surface* _surf;

            uint32_t _width;
            uint32_t _height;

            const WPEFramework::RPI::Display& _display;
        };

    public :

        class EGL {
        public :

            class Sync final {
            public :

                Sync() = delete;
                Sync(const Sync& ) = delete;
                Sync(Sync&&) = delete;

                Sync& operator=(const Sync&) = delete;
                Sync& operator=(Sync&&) = delete;

                void* operator new(size_t) = delete;
                void* operator new[](size_t) = delete;
                void operator delete(void*) = delete;
                void operator delete[](void*) = delete;

                explicit Sync(const EGLDisplay);
                ~Sync();

            private :

                static constexpr EGLDisplay InvalidDisplay() { return EGL::InvalidDisplay(); }

                static_assert(std::is_convertible<decltype(_EGL_NO_SYNC), KHRFIX(EGLSync)>::value);
                static constexpr KHRFIX(EGLSync) InvalidSync() { return _EGL_NO_SYNC; }

                const EGLDisplay _dpy;

                const bool _supported;

                KHRFIX(EGLSync) _sync;
            };

            EGL() = delete;
            ~EGL() = delete;

            static constexpr EGLDisplay InvalidDisplay() { return EGL_NO_DISPLAY; }
            static constexpr EGLContext InvalidContext() { return EGL_NO_CONTEXT; }
            static constexpr EGLSurface InvalidSurface() { return EGL_NO_SURFACE; }

            static constexpr EGLNativeDisplayType InvalidDisplayType() { return Native::InvalidDevice(); }
            static constexpr EGLNativeWindowType InvalidWindowType() { return Native::InvalidSurface(); }

            static_assert(std::is_convertible<decltype (_EGL_NO_IMAGE), KHRFIX(EGLImage)>::value);
            static constexpr KHRFIX(EGLImage) InvalidImage() { return _EGL_NO_IMAGE; }
            static KHRFIX(EGLImage) CreateImage(const EGLDisplay, const EGLContext, const EGLNativeWindowType&, const Native::Prime&);
            static bool DestroyImage(KHRFIX(EGLImage)&, const EGLDisplay, const EGLContext);

            // Although compile / build time may succeed, runtime checks are also mandatory
            static bool Supported(const EGLDisplay, const std::string&);

        private :
#ifdef EGL_VERSION_1_5
#else
            using EGLAttrib = EGLint;
#endif
            using EGLuint64KHR = khronos_uint64_t;
        };

        class GLES {
        public :

            GLES() = delete;
            ~GLES() = delete;

            static constexpr GLuint InvalidFramebufferObject(){ return 0; }
            static constexpr GLuint InvalidTexture(){ return 0; }

            static bool ImageAsTarget(const KHRFIX(EGLImage)&, EGLint, EGLint, GLuint&, GLuint&);

            static bool Supported(const std::string&);
        };

    public :

        GLResourceMediator(const GLResourceMediator&) = delete;
        GLResourceMediator(GLResourceMediator&&) = delete;

        GLResourceMediator& operator=(const GLResourceMediator&) = delete;
        GLResourceMediator& operator=(GLResourceMediator &&) = delete;

//        static GLResourceMediator& Instance();

    private :

        GLResourceMediator() = default;
        ~GLResourceMediator() = default;
    };

}
}

static const char* connectorNameVirtualInput = "/tmp/keyhandler";

namespace WPEFramework {
namespace RPI {

    static Core::NodeId Connector();

    class Display : public Compositor::IDisplay {
    public:

        typedef std::map <const string, Display*> DisplayMap;

        Display() = delete;
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;

        ~Display() override;

        static Display* Instance(const string&, Exchange::IComposition::IDisplay*);

        void AddRef() const override;
        uint32_t Release() const override;

        EGLNativeDisplayType Native() const override;

        const std::string& Name() const override;

        int Process(uint32_t) override;

        int FileDescriptor() const override;

        ISurface* SurfaceByName(const std::string&) override;

        ISurface* Create(const std::string&, uint32_t, uint32_t) override;

        bool PrimeFor(const Exchange::IComposition::IClient&, WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime&);

        DisplayResolution Resolution() const override;

    private :

        class SurfaceImplementation;

        using InputFunction = std::function <void (SurfaceImplementation*)>;

        static void VirtualKeyboardCallback(keyactiontype, unsigned int);
        static void VirtualMouseCallback(mouseactiontype, unsigned short , signed short, signed short);
        static void VirtualTouchScreenCallback(touchactiontype, unsigned short, unsigned short, unsigned short);

        class DMATransfer final {
        public :

            DMATransfer(const DMATransfer&) = delete;
            DMATransfer& operator=(const DMATransfer&) = delete;

            DMATransfer(DMATransfer&&) = delete;
            DMATransfer& operator=(DMATransfer&&) = delete;

            DMATransfer();
            ~DMATransfer();

            bool IsValid() const;

            template<size_t N> bool Receive(std::string&, std::array<int, N>&);
            template<size_t N> bool Send(const std::string&, const std::array<int, N>&);

            bool Connect();
            bool Disconnect();

        private :

            static constexpr int InvalidSocket() { return -1; };
            static constexpr int InvalidFiledescriptor() { return -1; };

            bool Initialize();
            bool Deinitialize();

            bool Connect(uint32_t);
            bool Disconnect(uint32_t);

            bool Send(const std::string&, const int*, uint8_t);
            bool Receive(std::string&, int*, uint8_t);

            int _transfer;

            struct sockaddr_un _addr;

            bool _valid;
        };

        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        public:

            SurfaceImplementation() = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

            SurfaceImplementation(Display& display, const std::string& name, uint32_t width, uint32_t height);
            ~SurfaceImplementation() override;

            EGLNativeDisplayType NativeDisplay() const;
            EGLNativeWindowType NativeSurface() const;
            EGLNativeWindowType Native() const override;

            std::string Name() const override;

            void Keyboard(Compositor::IDisplay::IKeyboard*) override;
            void Pointer(Compositor::IDisplay::IPointer*) override;
            void Wheel(Compositor::IDisplay::IWheel*) override;
            void TouchPanel(Compositor::IDisplay::ITouchPanel*) override;

            // Different type from constructor!
            int32_t Width() const override;
            int32_t Height() const override;

            void SendKey(uint32_t, const IKeyboard::state, uint32_t);
            void SendWheelMotion(int16_t, int16_t, uint32_t);
            void SendPointerButton(uint8_t, const IPointer::state, uint32_t);
            void SendPointerPosition(int16_t, int16_t, uint32_t);
            void SendTouch(uint8_t, const ITouchPanel::state, uint16_t, uint16_t, uint32_t);

            inline void ScanOut();
            inline void PreScanOut();
            inline void PostScanOut();

            bool SyncPrimitiveStart() const;
            bool SyncPrimitiveEnd() const;

        private :

            Display& _display;

            IKeyboard* _keyboard;
            IWheel* _wheel;
            IPointer* _pointer;
            ITouchPanel* _touchpanel;

            Exchange::IComposition::IClient* _remoteClient;
            Exchange::IComposition::IRender* _remoteRenderer;

            WPEFramework::RPI_INTERNAL::GLResourceMediator::Native _nativeSurface;
        };


        Display(const std::string&, WPEFramework::Exchange::IComposition::IDisplay*);

        Exchange::IComposition::IClient* CreateRemoteSurface(const std::string&, uint32_t, uint32_t);

        void Register(SurfaceImplementation* surface);
        void Unregister(SurfaceImplementation* surface);

        static void Publish(InputFunction& action);

        void Initialize(WPEFramework::Exchange::IComposition::IDisplay* display);
        void Deinitialize();

        const Exchange::IComposition::IDisplay* RemoteDisplay() const;
        Exchange::IComposition::IDisplay* RemoteDisplay();

    private :

        static DisplayMap _displays;
        static Core::CriticalSection _displaysMapLock;

        std::string _displayName;
        mutable Core::CriticalSection _adminLock;
        void* _virtualinput;
        std::list<SurfaceImplementation*> _surfaces;
        Core::ProxyType<RPC::CommunicatorClient> _compositerServerRPCConnection;
        mutable uint32_t _refCount;

        Exchange::IComposition::IDisplay* _remoteDisplay;
        struct gbm_device* _nativeDisplay;

        DMATransfer* _dma;
        Core::CriticalSection _surfaceLock;
    };

    Display::DisplayMap Display::_displays;
    Core::CriticalSection Display::_displaysMapLock;

    static uint32_t WidthFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution);
    static uint32_t HeightFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution);

} // RPI
} // WPEFramework

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Prime(int fd, int sync_fd, uint64_t modifier, uint32_t format, uint32_t stride, uint32_t height, uint32_t width)
        : _fd{fd}
        , _sync_fd{sync_fd}
        , _modifier{modifier}
        , _format{format}
        , _stride{stride}
        , _height{height}
        , _width{width}
    {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::~Prime()
    {
        /* bool */ Unlock();
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Prime(Prime&& other)
    {
         *this = std::move(other);
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime& WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::operator=(Prime&& other)
    {
        if (this != &other) {
            _fd = other._fd;
            _sync_fd = other._sync_fd;

            _modifier = other._modifier;
            _format = other._format;
            _stride = other._stride;

            _height = other._height;
            _width = other._width;

            other._fd = InvalidFiledescriptor();
            other._sync_fd = InvalidFiledescriptor();

            other._modifier = InvalidModifier();
            other._format = InvalidFormat();
            other._stride = InvalidStride();

            other._height = InvalidHeight();
            other._width = InvalidWidth();
        }

        return *this;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Lock() const
    {
// FIXME: not signal safe
        bool ret =    _sync_fd != InvalidFiledescriptor()
                   && fcntl(_sync_fd, F_SETLKW, &_acquire) != -1
                   ;

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Unlock() const
    {
// FIXME: not signal safe
        bool ret =    _sync_fd != InvalidFiledescriptor()
                   && fcntl(_sync_fd, F_SETLK, &_release) != -1
                   ;

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::IsValid() const
    {
        bool ret =    _fd != InvalidFiledescriptor()
                   && _sync_fd != InvalidFiledescriptor()
                   && _modifier != InvalidModifier()
                   && _format != InvalidFormat()
                   && _stride != InvalidStride()
                   && _height != InvalidHeight()
                   && _width != InvalidWidth();

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InitializeLocks() const
    {
        static bool result = false;

        // Once is enough
        if (!result) {
            // Clear all fields since not all may be set
            /* void * */ memset(&_acquire, 0, sizeof(_acquire));
            _acquire.l_type = F_WRLCK;
            _acquire.l_whence = SEEK_SET;
            _acquire.l_start = 0;
            _acquire.l_len = 0;

            // Clear all fields since not all may be set
            /* void * */ memset(&_release, 0, sizeof(_release));
            _release.l_type = F_UNLCK;
            _release.l_whence = SEEK_SET;
            _release.l_start = 0;
            _release.l_len = 0;

            result = !result;
        }

        return result;
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Native(const WPEFramework::RPI::Display& display, uint32_t width, uint32_t height)
        : _prime{std::move(Prime(Prime::InvalidFiledescriptor(), Prime::InvalidFiledescriptor(), Prime::InvalidModifier(), Prime::InvalidFormat(), Prime::InvalidStride(), height, width))}
        , _device{InvalidDevice()}
        , _surf{InvalidSurface()}
        , _width{width}
        , _height{height}
        , _display(display)
    {
        _display.AddRef();
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::~Native()
    {
        Invalidate();
        _display.Release();
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Invalidate()
    {
        bool result = IsValid();

        if (result) {
            result = DestroyNativeSurface(_device, _surf);
            _surf = InvalidSurface(); 
        }

        return result;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::AddPrime(Prime&& prime)
    {
        if (   !_prime.IsValid()
            && prime.IsValid()
           ) {
            _prime = std::move(prime);

            if (_device == InvalidDevice()) {
                _device = _display.Native();
            }

            if (_device != InvalidDevice()) {
                _surf = CreateNativeSurface(_device, _prime.Width(), _prime.Height());
            }
         }

        return IsValid();
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::IsValid()
    {
        bool ret =    _prime.IsValid()
                   && _device != InvalidDevice()
                   && _surf != InvalidSurface()
                   && _width != InvalidWidth()
                   && _height != InvalidHeight()
                   ;

        return ret;
    }

    /* static */ struct gbm_device* WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::CreateNativeDisplay()
    {
        constexpr const bool enable = true;

        std::vector<std::string> nodes;

        static_assert(   !narrowing<decltype(DRM_NODE_RENDER), uint32_t, enable>::value
                      || (    DRM_NODE_RENDER >= 0
                           && in_unsigned_range <uint32_t, DRM_NODE_RENDER>::value
                         )
                     );
        RenderDevice::GetNodes(static_cast<uint32_t>(DRM_NODE_RENDER), nodes);

        struct gbm_device* device = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidDevice();

        for (auto begin = nodes.begin(), it = begin, end = nodes.end(); it != end; it++) {
            int fd = open(it->c_str(), O_RDWR);

            if (fd != RenderDevice::GBM::InvalidFiledescriptor()) {
                device = gbm_create_device(fd);

                if (device == WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidDevice()) {
                    /* int */ close(fd);
                    fd = RenderDevice::GBM::InvalidFiledescriptor();
                } else {
                    break;
                }
            }
        }

        return device;
    }

    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::DestroyNativeDisplay(struct gbm_device* device)
    {
        bool result = false;

        if (device != WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidDevice()) {
            int fd = gbm_device_get_fd(device);

            if(fd != RenderDevice::DRM::InvalidFiledescriptor()) {
                /* void */ gbm_device_destroy(device);

                result = close(fd) == 0;
            }
        }

        return result;
    }

    struct gbm_surface* WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::CreateNativeSurface(struct gbm_device* device, uint32_t width, uint32_t height)
    {
        ASSERT(gbm_device_get_format_modifier_plane_count(device, _prime.Format(), _prime.Modifier()) == 1);

        ASSERT(    _width == width
                && _prime.Width() == width
                && _height == height
                && _prime.Height() == height
              );
 
// FIXME: gbm_surface_create_with_modifiers
        struct gbm_surface* surf = (   device != InvalidDevice() 
                                    && gbm_device_is_format_supported(device, _prime.Format(), GBM_BO_USE_RENDERING) == 1
                                    && _prime.Width() == width
                                    && _prime.Height() == height
                                   )
                                   ? gbm_surface_create(device, width, height, _prime.Format(), GBM_BO_USE_RENDERING /* used for rendering */)
                                   : InvalidSurface()
                                   ;

        return surf;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::DestroyNativeSurface(struct gbm_device* device, struct gbm_surface* surface)
    {
        bool result =    device != InvalidDevice()
                      && surface != InvalidSurface()
                      ;

        if (result) {
            /* void */ gbm_surface_destroy(surface);
        }

        return result;
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Sync::Sync(const EGLDisplay dpy)
        : _dpy{dpy}
        , _supported{EGL::Supported(_dpy, "EGL_KHR_fence_sync")}
        , _sync{(   _supported
                 && _dpy != InvalidDisplay()
                )
                ? KHRFIX(eglCreateSync)(_dpy, _EGL_SYNC_FENCE, nullptr)
                : InvalidSync()
               }
    {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Sync::~Sync()
    {
        if (_sync == InvalidSync()) {
            // Error : unable to create sync object
            glFinish();
        } else {
            glFlush();

            EGLint val = static_cast<EGLint>(KHRFIX(eglClientWaitSync)(_dpy, _sync, _EGL_SYNC_FLUSH_COMMANDS_BIT, _EGL_FOREVER));

            static_assert(   !narrowing<decltype(EGL_FALSE), EGLint, true>::value
                          || in_signed_range<EGLint, EGL_FALSE>::value
                         );
            static_assert(   !narrowing<decltype(_EGL_CONDITION_SATISFIED), EGLint, true>::value
                          || in_signed_range<EGLint, _EGL_CONDITION_SATISFIED>::value
                         );
            if (   val == static_cast<EGLint>(EGL_FALSE)
                || val != static_cast<EGLint>(_EGL_CONDITION_SATISFIED)) {
                EGLAttrib status;

                bool ret = KHRFIX(eglGetSyncAttrib)(_dpy, _sync, _EGL_SYNC_STATUS, &status) != EGL_FALSE;

                ret =    ret
                      && (status == _EGL_SIGNALED)
                      ;

                if (!ret) {
                    TRACE(Trace::Error, (_T("EGL: synchronization primitive")));
                    ASSERT(false);
                }
            }

            /* EGLBoolean */ val = static_cast<EGLint>(KHRFIX(eglDestroySync)(_dpy, _sync));

            if (val != EGL_TRUE) {
                // Error
            }

            // Consume the (possible) error
            /* ELGint */ glGetError();
            /* ELGint */ eglGetError();
        }
    }

    /* static */ KHRFIX(EGLImage) WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::CreateImage(const EGLDisplay dpy, const EGLContext ctx, const EGLNativeWindowType& win, const Native::Prime& prime)
    {
        KHRFIX(EGLImage) ret = InvalidImage();

        if (    dpy != InvalidDisplay()
             && ctx != InvalidContext()
             && win != InvalidImage()
             && (    Supported(dpy, "EGL_KHR_image")
                  && Supported(dpy, "EGL_KHR_image_base")
                  && Supported(dpy, "EGL_EXT_image_dma_buf_import")
                  && Supported(dpy, "EGL_EXT_image_dma_buf_import_modifiers")
                )
            ) {

            constexpr const char methodName[] = XSTRINGIFY(KHRFIX(eglCreateImage));

            static KHRFIX(EGLImage) (* peglCreateImage)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*) = reinterpret_cast<KHRFIX(EGLImage) (*)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLAttrib*)>(eglGetProcAddress(methodName));

            if (peglCreateImage != nullptr) {
                constexpr const bool enable = false;

                if (    narrowing<uint32_t , EGLAttrib, enable>::value
                     || narrowing<uint64_t, EGLAttrib, enable>::value
                     || narrowing<int, EGLAttrib, enable>::value
                   ) {
                    TRACE_GLOBAL(Trace::Information, (_T("Possible narrowing detected!")));
                }

                // EGL may report differently than GBM / DRM
                // Platform formats are cross referenced against prime settings at construction time

                static EGLBoolean (* peglQueryDmaBufFormatsEXT)(EGLDisplay, EGLint, EGLint*, EGLint*) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufFormatsEXT"));
                static EGLBoolean (* peglQueryDmaBufModifiersEXT)(EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLint, EGLint, EGLuint64KHR*, EGLBoolean*, EGLint*)>(eglGetProcAddress("eglQueryDmaBufModifiersEXT"));

                EGLint count = 0;

                bool valid =    peglQueryDmaBufFormatsEXT != nullptr
                             && peglQueryDmaBufModifiersEXT != nullptr
                             && peglQueryDmaBufFormatsEXT(dpy, 0, nullptr, &count) != EGL_FALSE
                             ;

                std::vector<EGLint> formats(count, 0);

                valid =    valid
                        && peglQueryDmaBufFormatsEXT(dpy, count, formats.data(), &count) != EGL_FALSE
                        ;

                // Format should be listed as supported
                if (valid) {
                    auto it = std::find(formats.begin(), formats.end(), prime.Format());

                    valid = it != formats.end();
                }

                valid =    valid
                        && peglQueryDmaBufModifiersEXT(dpy, prime.Format(), 0, nullptr, nullptr, &count) != EGL_FALSE;

                std::vector<EGLuint64KHR> modifiers(count, 0);
                std::vector<EGLBoolean> external(count, EGL_FALSE);

                // External is required for GL_TEXTURE_EXTERNAL_OES
                valid =    valid
                        && peglQueryDmaBufModifiersEXT(dpy, prime.Format(), count, modifiers.data(), external.data(), &count) != EGL_FALSE;

                if (valid) {
                    static_assert(!narrowing<uint64_t, EGLuint64KHR, enable>::value);
                    auto it = std::find(modifiers.begin(), modifiers.end(), static_cast<EGLuint64KHR>(prime.Modifier()));

                    valid = it != modifiers.end ();

                    // For the compositor not relevant, only relevant for the client
                    if (valid) {
                        valid = !external[std::distance(modifiers.begin(), it)];
                    }
                }

                if (valid) {
                    static_assert(in_signed_range<EGLAttrib, EGL_WIDTH>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_HEIGHT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_LINUX_DRM_FOURCC_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_FD_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_OFFSET_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_PITCH_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_TRUE>::value);
                    static_assert(in_signed_range<EGLAttrib, EGL_NONE>::value);

                    // 64 bit hence the shift 32 and mask 0xFFFFFFF, each half equals 32 bitsF
                    static_assert(sizeof(EGLuint64KHR) == static_cast<size_t>(8));

                    const EGLAttrib attrs[] = {
                        EGL_WIDTH, static_cast<EGLAttrib>(prime.Width()),
                        EGL_HEIGHT, static_cast<EGLAttrib>(prime.Height()),
                        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLAttrib>(prime.Format()),
                        EGL_DMA_BUF_PLANE0_FD_EXT, static_cast<EGLAttrib>(prime.Fd()),
// FIXME: magic constant
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLAttrib>(prime.Stride()),
                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(prime.Modifier()) & 0xFFFFFFFF),
                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast<EGLAttrib>(static_cast<EGLuint64KHR>(prime.Modifier()) >> 32),
//                        EGL_IMAGE_PRESERVED_KHR, static_cast<EGLAttrib>(EGL_TRUE),
                        EGL_NONE
                    };

                    static_assert(std::is_convertible<std::nullptr_t, EGLClientBuffer>::value);
                    ret = peglCreateImage(dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attrs);
                }
            } else {
                // Error
                TRACE_GLOBAL(Trace::Error, (_T("%s is unavailable or invalid parameters.") , methodName));
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T("EGL is not properly initialized.")));
        }

        return ret;
    }

    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::DestroyImage(KHRFIX(EGLImage)& img, EGLDisplay dpy, EGLContext ctx)
    {
        bool ret = img != InvalidImage();

        if (    dpy != InvalidDisplay()
             && ctx != InvalidContext()
             && (   Supported(dpy, "EGL_KHR_image")
                 && Supported(dpy, "EGL_KHR_image_base")
                )
           ) {
            constexpr char const methodName[] = XSTRINGIFY(KHRFIX(eglDestroyImage));

            static EGLBoolean (* peglDestroyImage) (EGLDisplay, KHRFIX(EGLImage)) = reinterpret_cast<EGLBoolean (*)(EGLDisplay, KHRFIX(EGLImage))>(eglGetProcAddress(KHRFIX("eglDestroyImage")));

            if (peglDestroyImage != nullptr) {
                ret = peglDestroyImage(dpy, img) != EGL_FALSE;
                img = InvalidImage();
            } else {
                // Error
                TRACE_GLOBAL(Trace::Error, (_T ("%s is unavailable or invalid parameters are provided."), methodName));
            }
        } else {
            TRACE_GLOBAL(Trace::Error, (_T( "EGL is not properly initialized.")));
        }

        return ret;
    }

    // Although compile / build time may succeed, runtime checks are also mandatory
    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Supported(EGLDisplay dpy, const std::string& name)
    {
        bool ret = false;

        //static_assert ( ( std::is_same < dpy_t , EGLDisplay > :: value ) != false );

#ifdef EGL_VERSION_1_5
        // KHR extentions that have become part of the standard

        // Sync capability
        ret =    name.find("EGL_KHR_fence_sync") != std::string::npos
                 /* CreateImage / DestroyImage */
              || name.find("EGL_KHR_image") != std::string::npos
              || name.find("EGL_KHR_image_base") != std::string::npos
              ;
#endif

        if (!ret) {
            const std::string ext = eglQueryString(dpy, EGL_EXTENSIONS);

            ret =    !ext.empty()
                  && name.size() > 0
                  && ext.find(name) != std::string::npos
                  ;
        }

        return ret;
    }

    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::ImageAsTarget(const KHRFIX(EGLImage)& img, EGLint width, EGLint height, GLuint& tex, GLuint& fbo)
    {
        constexpr const bool enable = true;

        bool ret =    glGetError () == GL_NO_ERROR
                   && img != EGL::InvalidImage()
                   && width > 0
                   && height > 0
                   ;

        static_assert(    !narrowing<decltype(GL_TEXTURE_2D), GLuint, enable>::value 
                      || (   GL_TEXTURE_2D > 0
                          && in_unsigned_range<GLuint, GL_TEXTURE_2D>::value
                         )
                     );
        constexpr const GLuint tgt = GL_TEXTURE_2D;

        if (ret) {
            // Just an arbitrary selected unit
            glActiveTexture(GL_TEXTURE0);
            ret = glGetError() == GL_NO_ERROR;
        }

        if (ret) {
            if (tex != InvalidTexture()) {
                glDeleteTextures(1, &tex);
                ret = glGetError() == GL_NO_ERROR;
            }

            tex = InvalidTexture();

            if (ret) {
                glGenTextures(1, &tex);
                ret = glGetError() == GL_NO_ERROR;
            }

            if (ret) {
                glBindTexture(tgt, tex);
                ret = glGetError() == GL_NO_ERROR;
            }

            if (ret) {
                glTexParameteri(tgt, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                ret = glGetError() == GL_NO_ERROR;
            }

            if (ret) {
                glTexParameteri(tgt, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                ret = glGetError() == GL_NO_ERROR;
            }

            if (ret) {
                glTexParameteri(tgt, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                ret = glGetError() == GL_NO_ERROR;
            }

            if (ret) {
                glTexParameteri(tgt, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                ret = glGetError() == GL_NO_ERROR;
            }

            // A valid GL context should exist for GLES::Supported ()
            EGLContext ctx = eglGetCurrentContext();
            EGLDisplay dpy = ctx != EGL::InvalidContext() ? eglGetCurrentDisplay() : EGL::InvalidDisplay();

            ret =    ret
                  && eglGetError() == EGL_SUCCESS
                  && ctx != EGL::InvalidContext();

            if (    ret
                 && (    Supported("GL_OES_EGL_image")
                      && (   EGL::Supported(dpy, "EGL_KHR_image")
                          || EGL::Supported(dpy, "EGL_KHR_image_base")
                         )
                    )
               ) {
                // Take storage for the texture from the EGLImage; Pixel data becomes undefined
                static void (* pEGLImageTargetTexture2DOES)(GLenum, GLeglImageOES) = reinterpret_cast<void (*)(GLenum, GLeglImageOES)>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

                if (    ret
                     && pEGLImageTargetTexture2DOES != nullptr
                   ) {
                    static_assert(std::is_convertible<KHRFIX(EGLImage), GLeglImageOES>::value);
                    KHRFIX(EGLImage) noconst_img = img;
                    pEGLImageTargetTexture2DOES(tgt, reinterpret_cast<GLeglImageOES>(noconst_img));
                    ret = glGetError() == GL_NO_ERROR;
                } else {
                    ret = false;
                }
            }

            if (ret) {
                if (fbo != InvalidFramebufferObject()) {
                    glDeleteFramebuffers(1, &fbo);
                    ret = glGetError() == GL_NO_ERROR;
                }

                if (ret) {
                    glGenFramebuffers(1, &fbo);
                    ret = glGetError() == GL_NO_ERROR;
                }

                if (ret) {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                    ret = glGetError() == GL_NO_ERROR;
                }

                if (ret) {
                    // Bind the created texture as one of the buffers of the frame buffer object
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tgt, tex, 0 /* level */);
                    ret = glGetError() == GL_NO_ERROR;
                }

                if (ret) {
                    ret = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
                }

                if (!ret) {
                    glDeleteFramebuffers(1, &fbo);
                    glDeleteTextures(1, &tex);

                    fbo = InvalidFramebufferObject();
                    tex = InvalidTexture();
                }
            }
        }

        return ret;
    }

    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::Supported(const std::string& name)
    {
        bool ret = false;

        // Identical underlying types except for signedness
        static_assert(std::is_convertible<GLubyte, unsigned char>::value);
        static_assert(std::is_convertible<std::string::value_type, unsigned char>::value);

        const std::basic_string<unsigned char> uext = static_cast<const unsigned char*>(glGetString(GL_EXTENSIONS));
        const std::string ext = reinterpret_cast<const char*>(uext.data());

        ret =    !ext.empty()
              && name.size() > 0
              && ext.find(name) != std::string::npos
              ;

        return ret;
    }

//    /* static */ WPEFramework::RPI_INTERNAL::GLResourceMediator& WPEFramework::RPI_INTERNAL::GLResourceMediator::Instance()
//    {
//        static GLResourceMediator instance;
//        return instance;
//    }

    /* static */ WPEFramework::Core::NodeId WPEFramework::RPI::Connector()
    {
        string connector;
        if (    !Core::SystemInfo::GetEnvironment(_T("COMPOSITOR"), connector)
             || connector.empty()
           ) {
            connector = _T( "/tmp/compositor");
        }

        return Core::NodeId(connector.c_str());
    }

    WPEFramework::RPI::Display::~Display()
    {
        if (_dma != nullptr)
        {
            delete _dma;
            _dma = nullptr;
        }

        if (WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::DestroyNativeDisplay(static_cast<struct gbm_device*>(_nativeDisplay))) {
            _nativeDisplay = RenderDevice::GBM::InvalidDevice();
        }

        Deinitialize();
    }

    /* static */ WPEFramework::RPI::Display* WPEFramework::RPI::Display::Instance(const string& displayName, Exchange::IComposition::IDisplay* display)
    {
        Display* result(nullptr);

        _displaysMapLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName, display);
            if (    result != nullptr
                 && result -> RemoteDisplay() != nullptr
               ) {
                _displays.insert(std::pair<const std::string, Display*>(displayName, result));

                // If successful inserted
                result->AddRef();
            } else {
                delete result;
                result = nullptr;
            }
        } else {
            result = index->second;
            result->AddRef();
        }

        _displaysMapLock.Unlock();

        return result;
    }

    void WPEFramework::RPI::Display::AddRef() const 
    {
        Core::InterlockedIncrement(_refCount);
    }

    uint32_t WPEFramework::RPI::Display::Release() const
    {
        uint32_t result = static_cast<uint32_t>(Core::ERROR_NONE);

        if (Core::InterlockedDecrement(_refCount) == 0) {
            _displaysMapLock.Lock();

            DisplayMap::iterator display = _displays.find(_displayName);

            if (display != _displays.end()) {
                _displays.erase(display);
            }

            _displaysMapLock.Unlock();

            delete this;

            result = static_cast<uint32_t>(Core::ERROR_CONNECTION_CLOSED);
        }

        return result;
    }

    EGLNativeDisplayType WPEFramework::RPI::Display::Native() const
    {
        return _nativeDisplay;
    }

    const std::string& WPEFramework::RPI::Display::Name() const
    {
        return (_displayName);
    }

    int WPEFramework::RPI::Display::Process(uint32_t)
    {
        auto RefreshRateFromResolution = [](const Exchange::IComposition::ScreenResolution resolution)->int32_t
        {
            // Assume 'unknown' rate equals 60 Hz
            int32_t rate = 60;

            switch (resolution) {
            case Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                                                      rate = 24; break;
            case Exchange::IComposition::ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
                                                                      rate = 30; break;
            case Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
            case Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                                                                      rate = 50; break;
            case Exchange::IComposition::ScreenResolution_480i      : // 720x480
            case Exchange::IComposition::ScreenResolution_480p      : // 720x480 progressive
            case Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution_Unknown   : rate = 60;
            }

            return rate;
        };

        // Signed and at least 45 bits
        constexpr const int32_t milli = 1000;

        static int32_t rate = RefreshRateFromResolution(_remoteDisplay != nullptr ? _remoteDisplay->Resolution() : Exchange::IComposition::ScreenResolution_Unknown);
        static std::chrono::milliseconds delay = std::chrono::milliseconds( milli / rate);

        // Delay the (free running) loop
        auto current_time = std::chrono::steady_clock::now ();

        static decltype(current_time) last_access_time = current_time;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_access_time);

        if (duration.count() < delay.count()) {
            // skip frame, the client is creating frames at a too high rate
            std::this_thread::sleep_for(std::chrono::milliseconds(delay - duration));
        }

        last_access_time = current_time;

        for (auto begin = _surfaces.begin(), it = begin, end = _surfaces.end(); it != end; it++) {
            // THe way the API has been constructed limits the optimal syncing strategy

            (*it)->PreScanOut();  // will render

            // At least fails the very first time
            if ((*it)->SyncPrimitiveEnd()) {
                TRACE(Trace::Error, (_T("Unable to unlock the synchronization primitive.")));
            }

            (*it)->ScanOut(); // actual scan out (at the remote site)

            if ((*it)->SyncPrimitiveStart()) {
                TRACE(Trace::Error, (_T("Unable to lock the synchronization primitive.")));
            }

            (*it)->PostScanOut(); // rendered
        }

        // Arbitrary value
        return 0;
    }

    int WPEFramework::RPI::Display::FileDescriptor() const
    {
        return RenderDevice::DRM::InvalidFiledescriptor();
    }

    WPEFramework::Compositor::IDisplay::ISurface* WPEFramework::RPI::Display::SurfaceByName(const std::string& name)
    {
        auto present = [&name](SurfaceImplementation* impl)->bool
        {
            return impl->Name() == name; 
        };

        IDisplay::ISurface* result = nullptr;

        _adminLock.Lock();

        auto it = std::find_if(_surfaces.begin(), _surfaces.end(), present);

        if (it != _surfaces.end()) {
            result = *it;
            result->AddRef();
        }

        _adminLock.Unlock();

        return result;
    }

    WPEFramework::Compositor::IDisplay::ISurface* WPEFramework::RPI::Display::Create(const std::string& name, uint32_t width, uint32_t height)
    {
        uint32_t realHeight = height;
        uint32_t realWidth = width;

        if (_remoteDisplay != nullptr) {
            Exchange::IComposition::ScreenResolution resolution = _remoteDisplay->Resolution();

            realHeight = HeightFromResolution(resolution);
            realWidth = WidthFromResolution(resolution);

            if (    realWidth != width
                 || realHeight != height
               ) {
                constexpr const bool enable = true;

                static_assert(!narrowing<uint32_t, unsigned int, enable>::value);
                TRACE(Trace::Information, (_T("Requested surface dimensions (%u x %u) differ from true (real) display dimensions (%u x %u). Continuing with the latter!"), width, height, realWidth, realHeight));
            }
        } else {
            TRACE(Trace::Information, (_T("No remote display exist. Unable to query its dimensions. Expect the unexpected!")));
        }

        Core::ProxyType<SurfaceImplementation> retval = Core::ProxyType<SurfaceImplementation>::Create(*this, name, realWidth, realHeight);

        Compositor::IDisplay::ISurface* result = &(*retval);

        result->AddRef();

        return result;
    }

    bool WPEFramework::RPI::Display::PrimeFor(const Exchange::IComposition::IClient& client, WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime& prime)
    {
        constexpr const bool enable = true;

        _surfaceLock.Lock();

        bool ret = false;

        std::string msg = client.Name();

        // <prefix>:width:height:stride:format
        auto str2prop = [&msg, enable](std::string::size_type& pos)->uint64_t
        {
            constexpr const char spacer[] = ":";

            std::string::size_type start = msg.rfind(spacer, pos);

            static_assert(!narrowing <unsigned long, uint64_t, enable>::value);
            uint64_t ret =    start != std::string::npos
                           && start <= msg.length()
                           ? std::stoul(msg.substr(start + 1, pos)) : 0
                           ;

            decltype(ERANGE) err = errno;

            if (err == ERANGE) {
                TRACE_GLOBAL(Trace::Error, (_T("Unbale to determine property value.")));
            }

            pos = start - 1;

            return ret;
        };

        // This does not update our prime since values are const by reference
        std::array<int, 2> handles = {prime.Fd(), prime.SyncFd()};

        if (_dma != nullptr) {
            // Announce ourself to indicate the data exchange is a request for (file descriptor) data
            // Then, after receiving the data DMA is no longer needed for this client
            ret =    _dma->Connect()
                  && _dma->Send(msg, handles)
                  && _dma->Receive(msg, handles)
                  && _dma->Disconnect()
                  ;
        }

        if (ret) {
            std::string::size_type pos = msg.size() - 1;

            // All properties are uint32_t except modifier and file descriptors
            prime = std::move(WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime(
                                  handles[0]
                                , handles[1]
                                , str2prop(pos)
                                , static_cast<uint32_t>(str2prop(pos))
                                , static_cast<uint32_t>(str2prop(pos))
                                , static_cast<uint32_t>(str2prop(pos))
                                , static_cast<uint32_t>(str2prop(pos))
                                )
                    );

            static_assert(!narrowing<uint32_t, unsigned int, enable>::value);
            static_assert(!narrowing<uint64_t, unsigned long, enable>::value);
            TRACE(Trace::Information, (_T ( "Received the following properties via DMA: width %u height %u stride %u format %u modifier %lu."), prime.Width(), prime.Height(), prime.Stride(), prime.Format(), prime.Modifier()));
        } else {
            TRACE(Trace::Error, (_T ("Unable to receive (DMA) data for %s."), client.Name().c_str()));
        }

        _surfaceLock.Unlock();

        return ret;
    }

    template<typename Key, typename Value, std::size_t Size>
    class ResolutionMap
    {
        public :
            std::array<std::pair<Key, Value>, Size> _map;

            /* constexpr */ const Value& at(const Key& key) const
            {
                // std::find_if is not yet constexpr, but is since c++ 20
                auto it = std::find_if(_map.begin(), _map.end(), [&key](const std::pair<Key, Value>& p) {return p.first == key;});

                static_assert(Size > 0);
                return  it != _map.end() ? it->second : _map[0].second;
            }
    };

    WPEFramework::Compositor::IDisplay::DisplayResolution WPEFramework::RPI::Display::Resolution() const
    {
        static constexpr const auto map = ResolutionMap<Exchange::IComposition::ScreenResolution, Compositor::IDisplay::DisplayResolution, 12>
        { { {
            // First pair should contain a value (also) representative for 'key was not found'
            {WPEFramework::Exchange::IComposition::ScreenResolution_Unknown   , WPEFramework::Compositor::IDisplay::DisplayResolution_Unknown},
            {WPEFramework::Exchange::IComposition::ScreenResolution_480i      , WPEFramework::Compositor::IDisplay::DisplayResolution_480i},
            {WPEFramework::Exchange::IComposition::ScreenResolution_480p      , WPEFramework::Compositor::IDisplay::DisplayResolution_480p},
            {WPEFramework::Exchange::IComposition::ScreenResolution_720p      , WPEFramework::Compositor::IDisplay::DisplayResolution_720p},
            {WPEFramework::Exchange::IComposition::ScreenResolution_720p50Hz  , WPEFramework::Compositor::IDisplay::DisplayResolution_720p50Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_1080p24Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_1080p24Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_1080i50Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_1080i50Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_1080p50Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_1080p50Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_1080p60Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_1080p60Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_2160p50Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_2160p60Hz},
            {WPEFramework::Exchange::IComposition::ScreenResolution_2160p30Hz , WPEFramework::Compositor::IDisplay::DisplayResolution_2160p30Hz}
        } } };

        return map.at(_remoteDisplay->Resolution());
    }

    /* static */ void WPEFramework::RPI::Display::VirtualKeyboardCallback(keyactiontype type, unsigned int code)
    {
        if (type != KEY_COMPLETED) {
            const time_t timestamp = time(nullptr);

            const IDisplay::IKeyboard::state state = (  type == KEY_RELEASED
                                                      ? IDisplay::IKeyboard::released
                                                      : (  type == KEY_REPEAT
                                                         ? IDisplay::IKeyboard::repeated
                                                         : IDisplay::IKeyboard::pressed
                                                        )
                                                     )
                                                     ;

            InputFunction action = [=](SurfaceImplementation* s)
            {
                s->SendKey(code, state, timestamp);
            };

            Publish(action);
        }
    }

    /* static */ void WPEFramework::RPI::Display::VirtualMouseCallback(mouseactiontype type, unsigned short button, signed short horizontal, signed short vertical)
    {
        static int32_t pointer_x = 0;
        static int32_t pointer_y = 0;

        const time_t timestamp = time(nullptr);

        InputFunction action;

        pointer_x = pointer_x + horizontal;
        pointer_y = pointer_y + vertical;

        switch (type) {
        case MOUSE_MOTION   :
                                action =[=](SurfaceImplementation* s)
                                {
                                    const int32_t X = std::min(std::max(0, pointer_x), s->Width());
                                    const int32_t Y = std::min(std::max(0, pointer_y), s->Height());
                                    s->SendPointerPosition(X, Y, timestamp);
                                };
                                break;
        case MOUSE_SCROLL   :
                                action =[=](SurfaceImplementation* s)
                                {
                                    s->SendWheelMotion(horizontal, vertical, timestamp);
                                };
                                break;
        case MOUSE_RELEASED :
        case MOUSE_PRESSED  :
                                action = [=](SurfaceImplementation* s)
                                {
                                    s->SendPointerButton(  button
                                                         , (  type == MOUSE_RELEASED 
                                                            ? IDisplay::IPointer::released
                                                            : IDisplay::IPointer::pressed
                                                           )
                                                         , timestamp 
                                                        );
                                };
                                break;
        default             : ASSERT(false);
        }

        Publish(action);
    }

    /* static */ void WPEFramework::RPI::Display::VirtualTouchScreenCallback(touchactiontype type, unsigned short index, unsigned short x, unsigned short y)
    {
        static uint16_t touch_x = ~0;
        static uint16_t touch_y = ~0;

        static touchactiontype last_type = TOUCH_RELEASED;

        // Get touch position in pixels
        // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
        if (      x != touch_x 
               || y != touch_y
               || type != last_type
           ) {

            last_type = type;
            touch_x = x;
            touch_y = y;

            const time_t timestamp = time(nullptr);

            const IDisplay::ITouchPanel::state state =   type == TOUCH_RELEASED
                                                       ? ITouchPanel::released
                                                       : ( 
                                                             type == TOUCH_PRESSED
                                                           ? ITouchPanel::pressed
                                                           : ITouchPanel::motion
                                                        )
                                                       ;

            InputFunction action = [=](SurfaceImplementation* s)
            {
                const uint16_t mapped_x = (s->Width() * x) >> 16;
                const uint16_t mapped_y = (s->Height() * y) >> 16;

                s->SendTouch(index, state, mapped_x, mapped_y, timestamp);
            };

            Publish(action);
        }
    }



    WPEFramework::RPI::Display::DMATransfer::DMATransfer()
        : _transfer{InvalidSocket()}
// FIXME: configurable path
        , _addr{AF_UNIX , "/tmp/Compositor/DMA"}
        , _valid{Initialize()}
    {
    }

    WPEFramework::RPI::Display::DMATransfer::~DMATransfer()
    {
        /* bool */ Deinitialize();
    }

    bool WPEFramework::RPI::Display::DMATransfer::IsValid() const
    {
        return _valid;
    }

    template<size_t N>
    bool WPEFramework::RPI::Display::DMATransfer::Receive(std::string& msg, std::array<int, N>& fds)
    {
        bool ret = IsValid ();

        if (!ret) {
            TRACE(Trace::Information, (_T("Unable to receive (DMA) data.")));
        } else {
            ret = Receive(msg, fds.data(), fds.size());
        }

        return ret;
    }

    template<size_t N>
    bool WPEFramework::RPI::Display::DMATransfer::Send(const std::string& msg, const std::array<int, N>& fds)
    {
        bool ret = IsValid();

        if (!ret) {
            TRACE(Trace::Information, (_T("Unable to send (DMA) data.")));
        } else {
            ret = Send(msg, fds.data(), fds.size());
        }

        return ret;
    }

    bool WPEFramework::RPI::Display::DMATransfer::Connect()
    {
        const uint32_t timeout = Core::infinite;

        return Connect(timeout);
    }

    bool WPEFramework::RPI::Display::DMATransfer::Disconnect()
    {
        const uint32_t timeout = Core::infinite;

        return Disconnect(timeout);
    }

    bool WPEFramework::RPI::Display::DMATransfer::Initialize()
    {
        _transfer = socket(_addr.sun_family /* domain */, SOCK_STREAM /* type */, 0 /* protocol */);

        return _transfer != InvalidSocket();
    }

    bool WPEFramework::RPI::Display::DMATransfer::Deinitialize()
    {
        return Disconnect();
    }

    bool WPEFramework::RPI::Display::DMATransfer::Connect(uint32_t timeout)
    {
        silence(timeout);

        bool ret =    _transfer != InvalidSocket()
                   && connect(_transfer, reinterpret_cast<const struct sockaddr*>(&_addr), sizeof(_addr)) == 0;

        decltype(EISCONN) err = errno;

        // Already connected is not an error
        ret =    ret
              || err == EISCONN;

        return ret;
    }

    bool WPEFramework::RPI::Display::DMATransfer::Disconnect(uint32_t timeout)
    {
        silence(timeout);

        bool ret =    _transfer != InvalidSocket()
                   && close(_transfer) == 0;

        _transfer = InvalidSocket();

        return ret;
    }

    bool WPEFramework::RPI::Display::DMATransfer::Send(const std::string& msg, const int* fd, uint8_t count)
    {
        constexpr const bool enable = true;

        static_assert(!narrowing<std::string::size_type, size_t, enable>::value);
        const size_t bufsize = msg.size();

        bool ret = bufsize > 0;

        if (ret) {
            // Scatter array for vector I/O
            struct iovec iov;

            static_assert(std::is_same<char, std::string::value_type>::value);
            // Starting address for copying data
            char* address = const_cast<char*>(msg.data());
            iov.iov_base = reinterpret_cast<void*>(address);
            // Number of bytes to transfer
            iov.iov_len = bufsize;

            // Actual message
            struct msghdr msgh{};

            // Optional address
            msgh.msg_name = nullptr;
            // Size of address
            msgh.msg_namelen = 0;
            // Scatter array
            msgh.msg_iov = &iov;
            // Elements in msg_iov
            msgh.msg_iovlen = 1;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control[CMSG_SPACE(sizeof(int) * count)];

            // Only valid file descriptor (s) can be sent via extra payload
            for (uint8_t i = 0; i < count && fd != nullptr; i++) {
                ret =    fd[i] != InvalidFiledescriptor()
                      && ret;
            }

            if (ret) {
                // Contruct ancillary data to be added to the transfer via the control message

                // Ancillary data, pointer
                msgh.msg_control = control;

                // Ancillery data buffer length
                msgh.msg_controllen = sizeof(control);

                // Ancillary data should be access via cmsg macros
                // https://linux.die.net/man/2/recvmsg
                // https://linux.die.net/man/3/cmsg
                // https://linux.die.net/man/2/setsockopt
                // https://www.man7.org/linux/man-pages/man7/unix.7.html

                // Control message

                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                struct cmsghdr* cmsgh = CMSG_FIRSTHDR(&msgh);

                if (cmsgh != nullptr) {
                    // Originating protocol
                    // To manipulate options at the sockets API level
                    cmsgh->cmsg_level = SOL_SOCKET;

                    // Protocol specific type
                    // Option at the API level, send or receive a set of open file descriptors from another process
                    cmsgh->cmsg_type = SCM_RIGHTS;

                    // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignmen, eg byte count of control message including header
                    cmsgh->cmsg_len = CMSG_LEN(sizeof(int) * count);

                    // Initialize the payload
                    /* void */ memcpy(CMSG_DATA(cmsgh), fd, sizeof(int) * count);

                    ret = true;
                } else {
                    // Error
                    ret = false;
                }
            } else {
                // No extra payload, ie  file descriptor(s), to include
                msgh.msg_control = nullptr;
                msgh.msg_controllen = 0;

                ret = true;
            }

            if (ret) {
                // Configuration succeeded

                // https://linux.die.net/man/2/sendmsg
                // https://linux.die.net/man/2/write
                // Zero flags is equivalent to write

                ssize_t size = -1;
                socklen_t len = sizeof(size);

                // Only send data if the buffer is large enough to contain all data
                if (getsockopt(_transfer, SOL_SOCKET, SO_SNDBUF, &size, &len) == 0) {
                    // Most options use type int, ssize_t was just a placeholder
                    static_assert(!narrowing<ssize_t, long, enable>::value);
                    TRACE(Trace::Information, (_T("The sending buffer capacity equals %ld bytes."), size));

// FIXME: do not send if the sending buffer is too small
                    size = sendmsg(_transfer, &msgh, 0);
                } else {
                    // Unable to determine buffer capacity
                }

                ret = size > 0;

                if (ret) {
                    static_assert(!narrowing <ssize_t, long, enable>::value);
                    static_assert(!narrowing<decltype(msgh.msg_iov->iov_len + msgh.msg_controllen), unsigned long, enable>::value);
                    TRACE(Trace::Information, (_T("Send %ld bytes out of %lu."), size, msgh.msg_iov->iov_len /* just a single element */ + msgh.msg_controllen));
                } else {
                    TRACE(Trace::Error, (_T("Failed to send data.")));
                }
            }

        } else {
            TRACE(Trace::Error, (_T("A data message to be send cannot be empty!")));
        }

        return ret;
    }

    bool WPEFramework::RPI::Display::DMATransfer::Receive(std::string& msg, int* fd, uint8_t count)
    {
        constexpr const bool enable = true;

        msg.clear();

        static_assert(!narrowing<socklen_t, size_t, enable>::value);
        socklen_t capacity = 0, len = sizeof(capacity);

        if (getsockopt(_transfer, SOL_SOCKET, SO_RCVBUF, &capacity, &len) == 0) {
            static_assert(!narrowing<socklen_t, long, enable>::value);
            TRACE(Trace::Information, (_T("The receiving buffer maximum capacity equals %ld [bytes]."), capacity));

            static_assert(!narrowing<socklen_t, std::string::size_type, enable>::value);
            msg.reserve(capacity);
        } else {
            // Unable to determine buffer capacity
            static_assert(!narrowing<std::string::size_type, unsigned long, enable>::value);
            TRACE(Trace::Information, (_T("Unable to determine buffer maximum cpacity. Using %lu [bytes] instead."), msg.capacity()));

            msg.reserve(0);
        }

        const size_t bufsize = msg.capacity();

        bool ret =    bufsize > 0
                   && count > 0
                   && fd != nullptr;

        if (ret) {
            for (uint8_t i = 0; i < count; i++) {
                fd [i] = InvalidFiledescriptor();
            }

            // Scatter array for vector I/O
            struct iovec iov;

            static_assert(std::is_same<char, std::string::value_type>::value);
            // Starting address for writing data
            char buf[bufsize];
            iov.iov_base = reinterpret_cast<void*>(&buf[0]);
            // Number of bytes to transfer
            iov.iov_len = bufsize;

            // Actual message
            struct msghdr msgh{};

            // Optional address
            msgh.msg_name = nullptr;
            // Size of address
            msgh.msg_namelen = 0;
            // Elements in msg_iov
            msgh.msg_iovlen = 1;
            // Scatter array
            msgh.msg_iov = &iov;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control[CMSG_SPACE(sizeof(int) * count)];

            // Ancillary data, pointer
            msgh.msg_control = control;

            // Ancillary data buffer length
            msgh.msg_controllen = sizeof(control);

            // No flags set
            ssize_t size = recvmsg(_transfer, &msgh, 0);

            ret = size > 0;

            switch (size) {
            case -1 :   // Error
                        {
                            TRACE(Trace::Error, (_T("Error receiving remote (DMA) data.")));
                            break;
                        }
            case 0  :   // Peer has done an orderly shutdown, before any data was received
                        {
                            TRACE(Trace::Error, (_T("Error receiving remote (DMA) data. Client may have become unavailable.")));
                            break;
                        }
            default :   // Data
                        {
                            // Extract the file descriptor information
                            static_assert(!narrowing<ssize_t, long, enable>::value);
                            TRACE(Trace::Information, (_T("Received %ld bytes."), size));

                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                            struct cmsghdr* cmsgh = CMSG_FIRSTHDR(&msgh);

                            // Check for the expected properties the client should have set
                            if (   cmsgh != nullptr
                                && cmsgh->cmsg_level == SOL_SOCKET
                                && cmsgh->cmsg_type == SCM_RIGHTS
                            ) {
                                // The macro returns a pointer to the data portion of a cmsghdr.
                                /* void */ memcpy(fd, CMSG_DATA(cmsgh), sizeof(int) * count);
                            } else {
                                TRACE(Trace::Information, (_T("No (valid) ancillary data received.")));
                            }

                            using common_t = std::common_type<ssize_t, std::string::size_type>::type;

                            if (static_cast<common_t>(size) <= static_cast<common_t>(std::numeric_limits<std::string::size_type>::max())) {
                                msg.assign(buf, size);
                            } else {
                                ret = false;
                            }
                        }
            }
        } else {
            // Error
            TRACE(Trace::Error, (_T("A receiving data buffer (message) cannot be empty!")));
        }

        return ret;
    }

    WPEFramework::RPI::Display::SurfaceImplementation::SurfaceImplementation(Display& display, const std::string& name, uint32_t width, uint32_t height)
        : _display{display}
        , _keyboard{nullptr}
        , _wheel{nullptr}
        , _pointer{nullptr}
        , _touchpanel{nullptr}
        , _remoteClient{nullptr}
        , _remoteRenderer{nullptr}
        , _nativeSurface{_display, width, height}
    {
        _display.AddRef();

        _remoteClient = _display.CreateRemoteSurface(name, width, height);

        if (_remoteClient != nullptr) {
            _remoteRenderer = _remoteClient->QueryInterface<Exchange::IComposition::IRender>();

            if (_remoteRenderer == nullptr) {
                TRACE(Trace::Error, (_T("Unable to aquire remote renderer for surface %s."), name.c_str()));
            } else {
                WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime prime(
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidFiledescriptor(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidFiledescriptor(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidModifier(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidFormat(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidStride(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidHeight(),
                    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::InvalidWidth()
                );
 
                if (!_display.PrimeFor(*_remoteClient, prime)) {
                    TRACE(Trace::Error, (_T("Failed to set up rendering results sharing!")));

                    auto result = _remoteRenderer->Release();
                    ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                    _remoteRenderer = nullptr;

                    /* auto */ result = _remoteClient->Release();
                    ASSERT (result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                    _remoteClient = nullptr;
                } else {
                    _nativeSurface.AddPrime(std::move(prime));
                }
            }
        } else {
            TRACE(Trace::Error, (_T("Unable to create remote surface for local surface %s."), name.c_str()));
        }

        _display.Register(this);
    }

    WPEFramework::RPI::Display::SurfaceImplementation::~SurfaceImplementation()
    {
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

        if (_remoteClient != nullptr) {
            static_assert(std::is_same<std::string::value_type, char>::value);
            TRACE(Trace::Information, (_T ("Destructing surface %s"), _remoteClient->Name().c_str()));

            _display.RemoteDisplay()->InvalidateClient(_remoteClient->Name());

            if (_remoteRenderer != nullptr) {
                auto result = _remoteRenderer->Release();
                silence( result );
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _remoteRenderer = nullptr;
            }

            auto result = _remoteClient->Release();
            silence(result);
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _remoteClient = nullptr;

        }

        _nativeSurface.Invalidate();

        _display.Unregister(this);

        auto result = _display.Release();
        silence(result);
        ASSERT(result = Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    EGLNativeDisplayType WPEFramework::RPI::Display::SurfaceImplementation::NativeDisplay() const
    {
        return _nativeSurface.GetNativeDisplay();
    }

    EGLNativeWindowType WPEFramework::RPI::Display::SurfaceImplementation::Native() const
    {
        return NativeSurface();
    }

    EGLNativeWindowType WPEFramework::RPI::Display::SurfaceImplementation::NativeSurface() const
    {
        return _nativeSurface.GetNativeSurface();
    }

    std::string WPEFramework::RPI::Display::SurfaceImplementation::Name() const
    {
        return (_remoteClient != nullptr ? _remoteClient->Name() : string());
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Keyboard(WPEFramework::Compositor::IDisplay::IKeyboard* keyboard)
    {
        ASSERT(    _keyboard == nullptr
                && keyboard != nullptr
              );

        _keyboard = keyboard;
        _keyboard->AddRef();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Pointer(WPEFramework::Compositor::IDisplay::IPointer* pointer)
    {
        ASSERT(    _pointer == nullptr
                && pointer != nullptr
              );

        _pointer = pointer;
        _pointer->AddRef();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Wheel(WPEFramework::Compositor::IDisplay::IWheel* wheel)
    {
        ASSERT(   _wheel == nullptr
               && wheel != nullptr
              );

        _wheel = wheel;
        _wheel->AddRef();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::TouchPanel(WPEFramework::Compositor::IDisplay::ITouchPanel* touchpanel)
    {
        ASSERT(   _touchpanel == nullptr
               && touchpanel != nullptr
              );

        _touchpanel = touchpanel;
        _touchpanel->AddRef();
    }

    int32_t WPEFramework::RPI::Display::SurfaceImplementation::Width() const
    {
        using common_t = std::common_type<int32_t, uint32_t>::type;

        const bool result =    _nativeSurface.GetNativeSurface() != WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidSurface()
                            && static_cast<common_t>(_nativeSurface.Width()) <= static_cast<common_t>(std::numeric_limits<int32_t>::max())
                            ;

        return result ? _nativeSurface.Width() : WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidWidth();
    }

    int32_t WPEFramework::RPI::Display::SurfaceImplementation::Height() const
    {
        using common_t = std::common_type<int32_t, uint32_t>::type;

        const bool result =    _nativeSurface.GetNativeSurface() != WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidSurface()
                            && static_cast<common_t>(_nativeSurface.Height()) <= static_cast<common_t>(std::numeric_limits<int32_t>::max())
                            ;
 
        return result ? _nativeSurface.Height() : WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidHeight();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendKey(uint32_t key, const IKeyboard::state action, uint32_t)
    {
        if (_keyboard != nullptr) {
            _keyboard->Direct(key, action);
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendWheelMotion(int16_t x, int16_t y, uint32_t)
    {
        if (_wheel != nullptr) {
            _wheel->Direct(x, y);
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendPointerButton(uint8_t button, const IPointer::state state, uint32_t)
    {
        if (_pointer != nullptr) {
            _pointer->Direct(button, state);
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendPointerPosition(int16_t x, int16_t y, uint32_t)
    {
        if (_pointer != nullptr) {
            _pointer->Direct(x , y);
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendTouch(uint8_t index, const ITouchPanel::state state, uint16_t x, uint16_t y, uint32_t)
    {
        if (_touchpanel != nullptr) {
            _touchpanel->Direct(index, state, x, y);
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::ScanOut()
    {
// FIXME: Changes of currents cannot be reliably be monitored
        if (   _nativeSurface.IsValid() 
            && _remoteRenderer != nullptr
            && _remoteClient != nullptr
            && eglGetCurrentDisplay() != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDisplay()
            && eglGetCurrentContext() != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidContext()
            && eglGetCurrentSurface(EGL_DRAW) != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurface()
           ) {
            _remoteRenderer->ScanOut();
        } else {
            TRACE(Trace::Error, (_T("Remote scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?")));
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::PreScanOut()
    {
        EGLint width = 0;
        EGLint height = 0;

        EGLDisplay dpy = eglGetCurrentDisplay();
        EGLContext ctx = eglGetCurrentContext();
        EGLSurface surf = eglGetCurrentSurface(EGL_DRAW);

// FIXME: Changes of currents cannot reliably be monitored

        if (   _nativeSurface.IsValid()
            && _remoteRenderer != nullptr
            && _remoteClient != nullptr
            && dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDisplay()
            && ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidContext()
            && surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurface()
            && eglQuerySurface(dpy, surf, EGL_WIDTH, &width) != EGL_FALSE
            && eglQuerySurface(dpy, surf, EGL_HEIGHT, &height) != EGL_FALSE
           ) {
            static KHRFIX(EGLImage) img = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::CreateImage(dpy, ctx, _nativeSurface.GetNativeSurface(), _nativeSurface.GetPrime());

            static GLuint tex = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidTexture();
            static GLuint fbo = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFramebufferObject();

            using common_t = std::common_type<uint32_t, EGLint>::type;

            if (   img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImage ()
                && static_cast<common_t>(width) == static_cast<common_t>(_nativeSurface.Width())
                && static_cast<common_t>(height) == static_cast<common_t>(_nativeSurface.Height())
                && WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::ImageAsTarget(img , width, height, tex, fbo)
                && img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImage()
                && fbo != WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFramebufferObject()
               ) {
                //  Sync before the remote is using the contents 
                { WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Sync sync(dpy); }

                // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
                _remoteRenderer->PreScanOut();
            }
        } else {
            TRACE(Trace::Error, (_T("Remote pre scan out is not (yet) supported.")));
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::PostScanOut()
    {
// FIXME: Changes of currents cannot be reliably be monitored
        if (  _nativeSurface.IsValid()
            && _remoteRenderer != nullptr
            && _remoteClient != nullptr
            && eglGetCurrentDisplay() != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDisplay()
            && eglGetCurrentContext() != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidContext()
            && eglGetCurrentSurface(EGL_DRAW) != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurface()
           ) {
            _remoteRenderer->PostScanOut();
        } else {
            TRACE(Trace::Error, (_T("Remote post scan out is not (yet) supported.")));
        }
    }

    bool WPEFramework::RPI::Display::SurfaceImplementation::SyncPrimitiveStart() const
    {
        return _nativeSurface.GetPrime().Lock();
    }

    bool WPEFramework::RPI::Display::SurfaceImplementation::SyncPrimitiveEnd() const
    {
        return _nativeSurface.GetPrime().Unlock();
    }

    WPEFramework::RPI::Display::Display(const std::string& displayName, WPEFramework::Exchange::IComposition::IDisplay* display)
        : _displayName{displayName}
        , _adminLock()
        , _virtualinput{nullptr}
        , _surfaces()
        , _compositerServerRPCConnection()
        , _refCount{0}
        , _remoteDisplay{nullptr}
        , _nativeDisplay{RenderDevice::GBM::InvalidDevice()}
        , _dma{nullptr}
    {
        constexpr const bool enable = true;

        Initialize(display);

        _nativeDisplay = static_cast<EGLNativeDisplayType>(WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::CreateNativeDisplay());

        if (_nativeDisplay != WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidDevice()) {
            _dma = new DMATransfer();

            if (    _dma == nullptr
                || !_dma->IsValid()
               ) {
                TRACE(Trace::Error, (_T("DMA transfers are not supported.")));

                delete _dma;
                _dma = nullptr;
            }
        }
    }

    WPEFramework::Exchange::IComposition::IClient* WPEFramework::RPI::Display::CreateRemoteSurface(const std::string& name, uint32_t width, uint32_t height)
    {
        return _remoteDisplay != nullptr ? _remoteDisplay->CreateClient(name, width, height) : nullptr;
    }

    void WPEFramework::RPI::Display::Register(Display::SurfaceImplementation* surface)
    {
        ASSERT(surface != nullptr);

        _adminLock.Lock();

        std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

        if (index == _surfaces.end()) {
            _surfaces.push_back(surface);
        }

        _adminLock.Unlock();
    }

    void WPEFramework::RPI::Display::Unregister(Display::SurfaceImplementation* surface)
    {
        ASSERT(surface != nullptr);

        _adminLock.Lock();

        auto index(std::find(_surfaces.begin(), _surfaces.end(), surface));

        ASSERT(index != _surfaces.end ());

        if (index != _surfaces.end()) {
            _surfaces.erase(index);
        }

        _adminLock.Unlock();
    }

    /* static */ void WPEFramework::RPI::Display::Publish(InputFunction& action)
    {
        if (action != nullptr) {
            _displaysMapLock.Lock();

            for (std::pair<const string, Display*>& entry : _displays) {
                entry.second->_adminLock.Lock();

                std::for_each(begin(entry.second->_surfaces), end(entry.second-> _surfaces), action);

                entry.second->_adminLock.Unlock();
            }

            _displaysMapLock.Unlock();
        }
    }

    void WPEFramework::RPI::Display::Initialize(WPEFramework::Exchange::IComposition::IDisplay* display)
    {
        if (WPEFramework::Core::WorkerPool::IsAvailable()) {
            // If we are in the same process space as where a WorkerPool is registered (Main Process or
            // hosting process) use, it!
            Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        } else {
            // Seems we are not in a process space initiated from the Main framework process or its hosting process.
            // Nothing more to do than to create a workerpool for RPC our selves !
            Core::ProxyType<RPC::InvokeServerType<2, 0, 8>> engine = Core::ProxyType<RPC::InvokeServerType<2, 0, 8>>::Create();

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));

            engine -> Announcements(_compositerServerRPCConnection->Announcement());
        }

        if (display != nullptr) {
            _remoteDisplay = display;
            _remoteDisplay->AddRef();
        } else {
            // Connect to the CompositorServer..
            uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);

            if (result != static_cast<uint32_t>(Core::ERROR_NONE)) {
                TRACE(Trace::Error, (_T("Unable to open connection to Compositor with node %s. Error: %s."), _compositerServerRPCConnection->Source().RemoteId().c_str(), Core::NumberType<uint32_t>(result).Text().c_str ()));
                _compositerServerRPCConnection.Release();
            } else {
// FIXME: arbitrary constant
                _remoteDisplay = _compositerServerRPCConnection->Aquire<Exchange::IComposition::IDisplay>(2000, _displayName, ~0);

                if (_remoteDisplay == nullptr) {
                    TRACE(Trace::Error, (_T("Unable to create remote display for Display %s!"), Name().c_str()));
                }
            }
        }

        _virtualinput = virtualinput_open(_displayName.c_str(), connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE(Trace::Error, (_T("Initialization of virtual input failed for Display %s!"), Name().c_str()));
        }
    }

    void WPEFramework::RPI::Display::Deinitialize()
    {
        if (_virtualinput != nullptr) {
            virtualinput_close(_virtualinput);
            _virtualinput = nullptr;
        }

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            string name = (*index)->Name();

            if ((* index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                TRACE(Trace::Error, (_T("Compositor Surface [%s] is not properly destructed."), name.c_str()));
            }

            index = _surfaces.erase(index);
        }

        if (_remoteDisplay != nullptr) {
            _remoteDisplay->Release();
            _remoteDisplay = nullptr;
        }

        if (_compositerServerRPCConnection.IsValid()) {
            _compositerServerRPCConnection->Close(RPC::CommunicationTimeOut);
            _compositerServerRPCConnection.Release();
        }
    }

    const WPEFramework::Exchange::IComposition::IDisplay* WPEFramework::RPI::Display::RemoteDisplay() const
    {
        return _remoteDisplay;
    }

    WPEFramework::Exchange::IComposition::IDisplay* WPEFramework::RPI::Display::RemoteDisplay()
    {
        return _remoteDisplay;
    }

    /* static */ uint32_t WPEFramework::RPI::WidthFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution resolution)
    {
        uint32_t width = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidWidth();

        switch (resolution) {
        case WPEFramework::Exchange::IComposition::ScreenResolution_480p      : // 720x480
                                                                                width = 720; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
        case WPEFramework::Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 @ 50 Hz
                                                                                width = 1280; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced  @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                                                width = 1920; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                                                width = 3840; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_480i      : // Unknown according to the standards (?)
        case WPEFramework::Exchange::IComposition::ScreenResolution_Unknown   :
        default                                                               : width = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidWidth();
        }

        return width;
    }

    /* static */ uint32_t WPEFramework::RPI::HeightFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution resolution)
    {
        uint32_t height = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidHeight();

        switch ( resolution ) {
        case WPEFramework::Exchange::IComposition::ScreenResolution_480i      : // 720x480
        case WPEFramework::Exchange::IComposition::ScreenResolution_480p      : // 720x480 progressive
                                                                                height = 480; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
        case WPEFramework::Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
                                                                                height = 720; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                                                height = 1080; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p30Hz : // 4K, 3840x2160 progressive @ 30 Hz, HDMI 1.4 bandwidth limited
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
        case WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                                                height = 2160; break;
        case WPEFramework::Exchange::IComposition::ScreenResolution_Unknown   :
        default                                                               : height = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::InvalidHeight();
        }

        return height;
    }

    WPEFramework::Compositor::IDisplay * WPEFramework::Compositor::IDisplay::Instance(const string& displayName, void* display)
    {
        return WPEFramework::RPI::Display::Instance(displayName, reinterpret_cast<WPEFramework::Exchange::IComposition::IDisplay*>(display));
    }

#undef XSTRINGIFY
#undef STRINGIFY
#ifdef _KHRFIX
#undef _KHRFIX
#endif
#undef KHRFIX
#undef _EGL_SYNC_FENCE
#undef _EGL_NO_SYNC
#undef _EGL_FOREVER
#undef _EGL_NO_IMAGE
#undef _EGL_CONDITION_SATISFIED
#undef _EGL_SYNC_FLUSH_COMMANDS_BIT
#undef _EGL_SIGNALED
#undef _EGL_SYNC_STATUS
