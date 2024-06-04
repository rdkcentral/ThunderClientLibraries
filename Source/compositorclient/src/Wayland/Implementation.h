/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#pragma once

#include <cassert>
#include <list>
#include <map>
#include <semaphore.h>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

//
// Forward declaration of the wayland specific types.
// We do not want to make this header file (tsemaphore.hhe C++ abstraction)
// dependent on any wayland header file. This would conflict
// with the idea behind the abstraction from C -> C++
//
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_display;
struct wl_seat;
struct wl_output;
struct wl_keyboard;
struct wl_pointer;
struct wl_touch;
struct wl_simple_shell;
struct xdg_wm_base;
struct wl_shell;
struct wl_surface;
struct wl_egl_window;
struct wl_keyboard_listener;
struct wl_callback;
struct wl_callback_listener;
struct wl_shell_surface;

struct xdg_surface;
struct xdg_toplevel;

namespace Thunder {
namespace Wayland {

    class EXTERNAL Display : public Compositor::IDisplay {
    public:
        struct ICallback {
            virtual ~ICallback() {}
            virtual void Attached(const uint32_t id) = 0;
            virtual void Detached(const uint32_t id) = 0;
        };

    private:
        Display();
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;

        class CriticalSection {
        private:
            CriticalSection(const CriticalSection&);
            CriticalSection& operator=(const CriticalSection&);

        public:
            CriticalSection()
            {

                pthread_mutexattr_t structAttributes;

                // Create a recursive mutex for this process (no named version, use semaphore)
                if ((pthread_mutexattr_init(&structAttributes) != 0) || (pthread_mutexattr_settype(&structAttributes, PTHREAD_MUTEX_RECURSIVE) != 0) || (pthread_mutex_init(&_lock, &structAttributes) != 0)) {
                    // That will be the day, if this fails...
                    assert(false);
                }
            }
            ~CriticalSection()
            {
            }

        public:
            void Lock()
            {
                pthread_mutex_lock(&_lock);
            }
            void Unlock()
            {
                pthread_mutex_unlock(&_lock);
            }

        private:
            pthread_mutex_t _lock;
        };

        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        private:
            SurfaceImplementation() = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        public:
            SurfaceImplementation(Display& compositor, const std::string& name, const uint32_t width, const uint32_t height);
            SurfaceImplementation(Display& compositor, const uint32_t id, const char* name);
            SurfaceImplementation(Display& compositor, const uint32_t id, struct wl_surface* surface);
            virtual ~SurfaceImplementation();

        public:
            uint32_t AddRef() const override
            {
                _refcount++;
                return Core::ERROR_NONE;
            }
            uint32_t Release() const override
            {
                if (--_refcount == 0) {
                    delete const_cast<SurfaceImplementation*>(this);
                    return Core::ERROR_DESTRUCTION_SUCCEEDED;
                }
                return Core::ERROR_NONE;
            }
            EGLNativeWindowType Native() const override
            {
                return (reinterpret_cast<EGLNativeWindowType>(_native));
            }
            std::string Name() const override
            {
                return _name;
            }
            int32_t Height() const override
            {
                return (_height);
            }
            int32_t Width() const override
            {
                return (_width);
            }
            void Keyboard(IKeyboard* keyboard) override
            {
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;

                if (_keyboard != nullptr && _display != nullptr) {
                    const std::string& mapping = _display->KeyMapConfiguration();
                    _keyboard->KeyMap(mapping.c_str(), mapping.length());
                }
            }
            inline uint32_t Id() const override
            {
                return (_id);
            }
            inline bool IsVisible() const
            {
                return (_visible != 0);
            }
            inline uint32_t Opacity() const
            {
                return (_opacity);
            }
            inline uint32_t ZOrder() const override
            {
                return (_ZOrder);
            }
            inline void Name(const char* name)
            {
                if (name != nullptr) {
                    _name = name;
                }
            }
            void Pointer(IPointer* pointer) override
            {
                assert((_pointer == nullptr) ^ (pointer == nullptr));
                _pointer = pointer;
            }
            void KeyMapConfiguration(const char information[], const uint16_t size)
            {
                if (_keyboard != nullptr) {
                    _keyboard->KeyMap(information, size);
                }
            }
            bool Connect(const EGLSurface& surface);
            uint32_t ZOrder(const uint16_t order) override;
            void Opacity(const uint32_t opacity) override;
            void Visibility(const bool visible) override;
            void Resize(const int x, const int y, const int w, const int h) override;
            void Dimensions(const uint32_t visible,
                 const int32_t x, const int32_t y, const int32_t width, const int32_t height,
                 const uint32_t opacity, const uint32_t zorder);

        private:
            void Redraw();
            void Unlink();

        public:
            // Called by C interface methods. A bit to much overkill to actually make the private and all kind
            // of friend definitions.
            struct wl_surface* _surface;

            struct xdg_surface *_xdg_surface;
            struct xdg_toplevel *_xdg_toplevel;

        private:
            friend Display;

            mutable uint32_t _refcount;
            int _level;
            std::string _name;
            uint32_t _id;
            int32_t _x;
            int32_t _y;
            int32_t _width;
            int32_t _height;
            uint32_t _visible;
            uint32_t _opacity;
            uint32_t _ZOrder;
            Display* _display;
            struct wl_egl_window* _native;
            struct wl_shell_surface* _shellSurface;
            EGLSurface _eglSurfaceWindow;
            IKeyboard* _keyboard;
            IPointer* _pointer;
        };

        class ImageImplementation {
        private:
            ImageImplementation();
            ImageImplementation(const ImageImplementation&);
            ImageImplementation& operator=(const ImageImplementation&);

        public:
            ImageImplementation(Display& compositor, uint32_t texture, const uint32_t width, const uint32_t height);
            virtual ~ImageImplementation();

        public:
            inline void AddRef()
            {
                _refcount++;
            }
            inline void Release()
            {
                if (--_refcount == 0) {
                    delete this;
                }
            }

            inline EGLImageKHR GetImage() const
            {
                return _eglImage;
            }

        private:
            friend Display;
            uint32_t _refcount;
            EGLImageKHR _eglImage;
            Display* _display;

            static EGLenum _eglTarget;
            static PFNEGLCREATEIMAGEKHRPROC _eglCreateImagePtr;
            static PFNEGLDESTROYIMAGEKHRPROC _eglDestroyImagePtr;
        };

        typedef std::map<const void*, SurfaceImplementation*> SurfaceMap;
        typedef std::map<struct wl_surface*, SurfaceImplementation*> WaylandSurfaceMap;

        Display(const std::string& displayName)
            : _display(nullptr)
            , _registry(nullptr)
            , _seat_name(0)
            , _seat(nullptr)
            , _seat_registry(nullptr)
            , _output_name(0)
            , _output(nullptr)
            , _output_registry(nullptr)
            , _simpleShell(nullptr)
            , _keyboard(nullptr)
            , _pointer(nullptr)
            , _touch(nullptr)
            , _shell(nullptr)
            , _trigger()
            , _redraw()
            , _tid(0)
            , _displayName(displayName)
            , _displayId()
            , _keyboardReceiver(nullptr)
            , _pointerReceiver(nullptr)
            , _keyMapConfiguration()
            , _eglDisplay(EGL_NO_DISPLAY)
            , _eglConfig(0)
            , _eglContext(EGL_NO_CONTEXT)
            , _collect(false)
            , _surfaces()
            , _physical()
            , _clientHandler(nullptr)
            , _signal()
            , _thread()
            , _refCount(0)
        {
#ifdef BCM_HOST
            bcm_host_init();
#endif
#ifdef V3D_DRM_DISABLE
            ::setenv("V3D_DRM_DISABLE", "1", 1);
#endif
            std::string displayId = IDisplay::Configuration();
            if (displayId.empty() == false) {
                _displayId.swap(displayId);
            }
        }

    public:
        struct Rectangle {
            int32_t X;
            int32_t Y;
            int32_t Width;
            int32_t Height;
        };

        struct IProcess {
            virtual ~IProcess() {}

            virtual bool Dispatch() = 0;
        };

        class Surface {
        public:
            inline Surface()
                : _implementation(nullptr)
            {
            }
            inline Surface(Compositor::IDisplay::ISurface* impl)
                : _implementation(impl)
            {
                _implementation->AddRef();
            }
            inline Surface(const Surface& copy)
                : _implementation(copy._implementation)
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
            }
            inline ~Surface()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
            }

            inline Surface& operator=(const Surface& rhs)
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
                _implementation = rhs._implementation;
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return (_implementation != nullptr);
            }
            inline uint32_t Id() const
            {
                assert(IsValid() == true);
                return (_implementation->Id());
            }
            inline const std::string Name() const
            {
                assert(IsValid() == true);
                return (_implementation->Name());
            }
            inline uint32_t Height() const
            {
                assert(IsValid() == true);
                return (_implementation->Height());
            }
            inline uint32_t Width() const
            {
                assert(IsValid() == true);
                return (_implementation->Width());
            }
            inline void Visibility(const bool visible)
            {
                assert(IsValid() == true);
                return (_implementation->Visibility(visible));
            }
            inline void Opacity(const uint32_t opacity)
            {
                assert(IsValid() == true);
                return (_implementation->Opacity(opacity));
            }
            inline void ZOrder(const uint32_t order)
            {
                assert(IsValid() == true);
                _implementation->ZOrder(order);
            }
            inline void Resize(const int x, const int y, const int w, const int h)
            {
                assert(IsValid() == true);
                _implementation->Resize(x, y, w, h);
            }
            inline void Keyboard(IKeyboard* keyboard)
            {
                assert(IsValid() == true);
                return (_implementation->Keyboard(keyboard));
            }
            inline void Pointer(IPointer* pointer)
            {
                assert(IsValid() == true);
                return (_implementation->Pointer(pointer));
            }
            inline void AddRef()
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                    _implementation = nullptr;
                }
            }
            inline void Release()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                    _implementation = nullptr;
                }
            }
            inline EGLNativeWindowType Native() const
            {
                assert(IsValid() == true);
                return (_implementation->Native());
            }

        private:
            Compositor::IDisplay::ISurface* _implementation;
        };

        class Image {
        public:
            inline Image()
                : _implementation(nullptr)
            {
            }
            inline Image(ImageImplementation& impl)
                : _implementation(&impl)
            {
                _implementation->AddRef();
            }
            inline Image(const Image& copy)
                : _implementation(copy._implementation)
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
            }
            inline ~Image()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
            }

            inline Image& operator=(const Image& rhs)
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                }
                _implementation = rhs._implementation;
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                }
                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return (_implementation != nullptr);
            }
            inline void AddRef()
            {
                if (_implementation != nullptr) {
                    _implementation->AddRef();
                    _implementation = nullptr;
                }
            }
            inline void Release()
            {
                if (_implementation != nullptr) {
                    _implementation->Release();
                    _implementation = nullptr;
                }
            }
            inline EGLImageKHR GetImage() const
            {
                assert(_implementation != nullptr);
                return _implementation->GetImage();
            }

        private:
            ImageImplementation* _implementation;
        };

    public:
        typedef std::map<const std::string, Display*> DisplayMap;
        typedef bool (*KeyHandler)(const uint32_t state, const uint32_t code, const uint32_t modifiers);

        static void RuntimeDirectory(const std::string& directory)
        {
            _runtimeDir = directory;
        }

        static Display& Instance(const std::string& displayName);

        virtual ~Display();

    public:
        // Lifetime management
        virtual uint32_t AddRef() const;
        virtual uint32_t Release() const;

        // Methods
        EGLNativeDisplayType Native() const override
        {
            return (reinterpret_cast<EGLNativeDisplayType>(_display));
        }
        const std::string& Name() const override
        {
            return (_displayName);
        }
        int FileDescriptor() const override;
        int Process(const uint32_t data) override;
        ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override;
        ISurface* SurfaceByName(const std::string& name) override
        {
            //iterate through waylandsurface map return wl_surface with matching name
            _adminLock.Lock();

            WaylandSurfaceMap::iterator entry(_waylandSurfaces.begin());

            while (entry != _waylandSurfaces.end()) {
                if (entry->second->Name().compare(name) == 0) {
                    _adminLock.Unlock();
                    //return iSurface to upper layers
                    return entry->second;
                }
                entry++;
            }
            _adminLock.Unlock();
            return nullptr;
        }

        inline bool IsOperational() const
        {
            return (_display != nullptr);
        }
        inline bool HasEGLContext() const
        {
            return (_eglContext != EGL_NO_CONTEXT);
        }
        inline void Callback(ICallback* callback) const
        {
            _adminLock.Lock();
            assert((callback != nullptr) ^ (_clientHandler != nullptr));
            _clientHandler = callback;
            _adminLock.Unlock();
        }
        inline const std::string& RuntimeDirectory() const
        {
            return (_runtimeDir);
        }
        inline const std::string& KeyMapConfiguration() const
        {
            return _keyMapConfiguration;
        }
        const Rectangle& Physical() const
        {
            return (_physical);
        }
        void Get(const void* id, Surface& surface)
        {
            _adminLock.Lock();

            SurfaceMap::iterator index(_surfaces.find(id));

            if (index != _surfaces.end()) {
                surface = Surface(index->second);
            } else {
                surface.Release();
            }
            _adminLock.Unlock();
        }
        void LoadSurfaces();
        Image Create(const uint32_t texture, const uint32_t width, const uint32_t height);
        void Process(IProcess* processLoop);
        void Signal();
        inline EGLDisplay GetDisplay() const
        {
            return _eglDisplay;
        }

        inline void Trigger()
        {
            sem_post(&_trigger);
        }

        inline void Redraw()
        {
            sem_post(&_redraw);
        }

    private:
        void Initialize();
        void Deinitialize();
        void EGLInitialize();

    public:
        // Called by C interface methods. A bit to much overkill to actually make the private and all kind
        // of friend definitions, so left them public, nut should *NOT* be used by users of this class !!!
        inline bool Collect() const
        {
            return (_collect);
        }

        void InitializeEGL();
        void Constructed(const void* id, wl_surface* surface);
        void Constructed(const void* id, const char* name = nullptr);
        void Destructed(const void* id);
        void Dimensions(
            const uint32_t id, const uint32_t visible, const int32_t x, const int32_t y, const int32_t width,
            const int32_t height, const uint32_t opacity, const uint32_t zorder);
        void FocusKeyboard(struct wl_surface* surface, const bool state)
        {
            _adminLock.Lock();
            WaylandSurfaceMap::const_iterator index = _waylandSurfaces.find(surface);

            if (index != _waylandSurfaces.end()) {
                if (state == false) {
                    if (_keyboardReceiver == index->second) {
                        _keyboardReceiver = nullptr;
                    }
                } else {
                    _keyboardReceiver = index->second;
                }
            }
            _adminLock.Unlock();
        }
        void FocusPointer(struct wl_surface* surface, const bool state)
        {
            _adminLock.Lock();
            WaylandSurfaceMap::const_iterator index = _waylandSurfaces.find(surface);

            if (index != _waylandSurfaces.end()) {
                if (state == false) {
                    if (_pointerReceiver == index->second) {
                        _pointerReceiver = nullptr;
                    }
                } else {
                    _pointerReceiver = index->second;
                }
            }

            _adminLock.Unlock();
        }

        void KeyMapConfiguration(const char information[], const uint16_t size)
        {
            _adminLock.Lock();

            WaylandSurfaceMap::iterator index(_waylandSurfaces.begin());

            while (index != _waylandSurfaces.end()) {
                index->second->KeyMapConfiguration(information, size);
                index++;
            }

            _keyMapConfiguration = std::string(information, size);

            _adminLock.Unlock();
        }
        void Key(const uint32_t key, const IKeyboard::state action, const uint32_t)
        {
            _adminLock.Lock();

            if ((_keyboardReceiver != nullptr) && (_keyboardReceiver->_keyboard != nullptr)) {
                _keyboardReceiver->_keyboard->Direct(key, action);
            }

            _adminLock.Unlock();
        }
        void Modifiers(uint32_t depressedMods, uint32_t latchedMods, uint32_t lockedMods, uint32_t group)
        {
            _adminLock.Lock();

            if ((_keyboardReceiver != nullptr) && (_keyboardReceiver->_keyboard != nullptr)) {
                _keyboardReceiver->_keyboard->Modifiers(depressedMods, latchedMods, lockedMods, group);
            }

            _adminLock.Unlock();
        }
        void Repeat(int32_t rate, int32_t delay)
        {
            _adminLock.Lock();

            if ((_keyboardReceiver != nullptr) && (_keyboardReceiver->_keyboard != nullptr)) {
                _keyboardReceiver->_keyboard->Repeat(rate, delay);
            }

            _adminLock.Unlock();
        }

        void SendPointerPosition(const uint16_t x, const uint16_t y)
        {
            _adminLock.Lock();

            if ((_pointerReceiver != nullptr) && (_pointerReceiver->_pointer != nullptr)) {
              _pointerReceiver->_pointer->Direct(x, y);
            }

            _adminLock.Unlock();

        }

        void SendPointerButton(const uint8_t button, const IPointer::state action)
        {
            _adminLock.Lock();

            if ((_pointerReceiver != nullptr) && (_pointerReceiver->_pointer != nullptr)) {
              _pointerReceiver->_pointer->Direct(button, action);
            }

            _adminLock.Unlock();

        }

        // Wayland related info
        struct wl_display* _display;
        struct wl_registry* _registry;
        struct wl_compositor* _compositor;
        uint32_t _seat_name;
        struct wl_seat* _seat;
        struct wl_registry* _seat_registry;
        uint32_t _output_name;
        struct wl_output* _output;
        struct wl_registry* _output_registry;
        struct wl_simple_shell* _simpleShell;
        struct wl_keyboard* _keyboard;
        struct wl_pointer* _pointer;
        struct wl_touch* _touch;
        struct wl_shell* _shell;
        struct xdg_wm_base* _wm_base;

        // KeyBoardInfo
        uint32_t _keyRate;
        uint32_t _keyDelay;
        uint32_t _keyModifiers;

        sem_t _trigger;
        sem_t _redraw;

    private:
        friend class Surface;
        friend class Image;

        pthread_t _tid;

        std::string _displayName;
        std::string _displayId;
        SurfaceImplementation* _keyboardReceiver;
        SurfaceImplementation* _pointerReceiver;

        std::string _keyMapConfiguration;

        // EGL related info, if initialized and used.
        EGLDisplay _eglDisplay;
        EGLConfig _eglConfig;
        EGLContext _eglContext;

        // Abstraction representations
        int _threadId;
        bool _collect;
        SurfaceMap _surfaces;

        Rectangle _physical;
        mutable ICallback* _clientHandler;

        // Signal handler
        int _signal;
        int _thread;

        // Process wide singleton
        static CriticalSection _adminLock;
        static std::string _runtimeDir;
        static DisplayMap _displays;
        static WaylandSurfaceMap _waylandSurfaces;

        mutable uint32_t _refCount;
    };
} // Wayland
} // Thunder
