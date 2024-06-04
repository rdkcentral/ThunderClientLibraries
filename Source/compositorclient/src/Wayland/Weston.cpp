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

#include "Module.h"

#define EGL_EGLEXT_PROTOTYPES 1

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <compositor/Client.h>
#include "Implementation.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <wayland-client-core.h>
#include <wayland-client.h>

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "xdg-shell-client-protocol.h"
// logical xor
#define XOR(a, b) ((!a && b) || (a && !b))

using namespace Thunder;

#define Trace(fmt, args...) fprintf(stderr, "[pid=%d][Client %s:%d] : " fmt, getpid(), __FILE__, __LINE__, ##args)

#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (0)

static const struct wl_shell_surface_listener g_ShellSurfaceListener = {
    //handle_ping,
    [](void*, struct wl_shell_surface* shell_surface, uint32_t serial) {
        wl_shell_surface_pong(shell_surface, serial);
    },
    //handle_configure,
    [](void*, struct wl_shell_surface*, uint32_t, int32_t width, int32_t height) {
        Trace("handle_configure: width=%d height=%d \n", width, height);
        //  Wayland::Display::Sur *wayland = static_cast<Wayland *>(data);
        // wl_egl_window_resize(wayland->eglWindow, width, height, 0, 0);
    },
    //handle_popup_done
    [](void*, struct wl_shell_surface*) {
    }
};

struct wl_shm_listener shmListener = {
    // shmFormat
    [](void*, struct wl_shm*, uint32_t format) {
        Trace("shm format: %X\n", format);
    }
};

static const struct wl_output_listener outputListener = {
    // outputGeometry
    [](void* data, struct wl_output*, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make, const char* model, int32_t transform) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        Trace("wl_output_listener.outputGeometry x=%d y=%d physical_width=%d physical_height=%d, make=%s: model=%s transform=%d subpixel%d\n",
            x, y, physical_width, physical_height, make, model, transform, subpixel);
        Wayland::Display::Rectangle& rect(const_cast<Wayland::Display::Rectangle&>(context.Physical()));
        rect.X = x;
        rect.Y = y;
        rect.Width = physical_width;
        rect.Height = physical_height;
    },
    // outputMode
    [](void* data, struct wl_output*, uint32_t flags, int32_t width, int32_t height, int32_t) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        const Wayland::Display::Rectangle& rect(context.Physical());

        if ((flags & WL_OUTPUT_MODE_CURRENT) && ((width != rect.Width) || (height != rect.Height))) {
            Wayland::Display::Rectangle& rect(const_cast<Wayland::Display::Rectangle&>(context.Physical()));
            rect.Width = width;
            rect.Height = height;
            Trace("wl_output_listener.outputMode [0,0,%d,%d]\n", width, height);
        }
    },
    // outputDone
    [](void*, struct wl_output*) {
    },
    // outputScale
    [](void*, struct wl_output*, int32_t) {
    }
};

static const struct wl_keyboard_listener keyboardListener = {
    // keyboardKeymap
    [](void* data, struct wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
        } else {
            void* mapping = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
            if (mapping == MAP_FAILED) {
                close(fd);
            } else {

                Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
                context.KeyMapConfiguration(static_cast<const char*>(mapping), size);
                munmap(mapping, size);
                close(fd);
            }
        }
        Trace("wl_keyboard_listener.keyboardKeymap [%d,%d]\n", format, size);
    },
    // keyboardEnter,
    [](void* data, struct wl_keyboard*, uint32_t serial, struct wl_surface* surface, struct wl_array*) {
        Trace("wl_keyboard_listener.keyboardEnter serial=%d\n", serial);
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        context.FocusKeyboard(surface, true);
    },
    // keyboardLeave,
    [](void* data, struct wl_keyboard*, uint32_t serial, struct wl_surface* surface) {
        Trace("wl_keyboard_listener.keyboardLeave serial=%d\n", serial);
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        context.FocusKeyboard(surface, false);
    },
    // keyboardKey
    [](void* data, struct wl_keyboard* keyboard, uint32_t, uint32_t time, uint32_t key, uint32_t state) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        // Have no idea if this is true, just lets see...
        assert(keyboard == context._keyboard);

        Wayland::Display::IKeyboard::state action;
        switch (state) {
        case WL_KEYBOARD_KEY_STATE_RELEASED:
            action = Wayland::Display::IKeyboard::released;
            break;
        case WL_KEYBOARD_KEY_STATE_PRESSED:
            action = Wayland::Display::IKeyboard::pressed;
            break;
        default:
            action = static_cast<Wayland::Display::IKeyboard::state>(state);
        }
        context.Key(key, action, time);

        Trace("wl_keyboard_listener.keyboardKey [0x%02X, %s, 0x%02X ]\n", key, state == WL_KEYBOARD_KEY_STATE_PRESSED ? "Pressed" : "Released", context._keyModifiers);
    },
    // keyboardModifiers
    [](void* data, struct wl_keyboard*, uint32_t, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
        Trace("wl_keyboard_listener.keyboardModifiers [%d,%d,%d]\n", mods_depressed, mods_latched, mods_locked);
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        context.Modifiers(mods_depressed, mods_latched, mods_locked, group);
    },
    // keyboardRepeatInfo
    [](void* data, struct wl_keyboard*, int32_t rate, int32_t delay) {
        Trace("wl_keyboard_listener.keyboardRepeatInfo [%d,%d]\n", rate, delay);
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        context.Repeat(rate, delay);
    }
};

static const struct wl_pointer_listener pointerListener = {
    // pointerEnter
    [](void* data, struct wl_pointer*, uint32_t, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        int x, y;

        x = wl_fixed_to_int(sx);
        y = wl_fixed_to_int(sy);

        Trace("wl_pointer_listener.pointerEnter [%d,%d]\n", x, y);
        context.FocusPointer(surface, true);
    },
    // pointerLeave
    [](void* data, struct wl_pointer*, uint32_t, struct wl_surface* surface) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        Trace("wl_pointer_listener.pointerLeave [%p]\n", surface);
        context.FocusPointer(surface, false);
    },
    // pointerMotion
    [](void* data, struct wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        int x, y;
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        x = wl_fixed_to_int(sx);
        y = wl_fixed_to_int(sy);

        Trace("wl_pointer_listener.pointerMotion [%d,%d]\n", x, y);
        context.SendPointerPosition(x, y);
    },
    // pointerButton
    [](void* data, struct wl_pointer*, uint32_t, uint32_t, uint32_t button, uint32_t state) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));
        Trace("wl_pointer_listener.pointerButton [%u,%u]\n", button, state);

        //align with what WPEBackend-rdk thunder backend is expecting
        if (button >= BTN_MOUSE)
          button = button - BTN_MOUSE;
        else
          button = 0;

        context.SendPointerButton(button, static_cast<Wayland::Display::IPointer::state>(state));
    },
    // pointerAxis
    [](void*, struct wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value) {
        int v;
        v = wl_fixed_to_int(value);
        Trace("wl_pointer_listener.pointerAxis [%u,%d]\n", axis, v);
    },
    // pointerFrame
    [](void*, struct wl_pointer*) {
    },
    // pointerAxisSource
    [](void*, struct wl_pointer*, uint32_t) {
    },
    // pointerAxisStop
    [](void*, struct wl_pointer*, uint32_t, uint32_t) {
    },
    // pointerAxisDiscrete
    [](void*, struct wl_pointer*, uint32_t, int32_t) {
    }
};

static const struct wl_seat_listener seatListener = {
    // seatCapabilities,
    [](void* data, struct wl_seat* seat, uint32_t capabilities) {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        Trace("wl_seat_listener.seatCapabilities [%p,%d]\n", seat, capabilities);

        if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
            context._keyboard = wl_seat_get_keyboard(context._seat);
            wl_keyboard_add_listener(context._keyboard, &keyboardListener, data);
            Trace("wl_seat_listener.keyboard [%p,%p]\n", seat, context._keyboard);
        }
        if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
            context._pointer = wl_seat_get_pointer(context._seat);
            wl_pointer_add_listener(context._pointer, &pointerListener, data);
            Trace("wl_seat_listener.pointer [%p,%p]\n", seat, context._pointer);
        }
        if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
            context._touch = wl_seat_get_touch(context._seat);
            Trace("wl_seat_listener.touch [%p,%p]\n", seat, context._touch);
        }
    },
    // seatName
    [](void*, struct wl_seat* seat, const char* name) {
        Trace("wl_seat_listener.seatName[%p,%s]\n", seat, name);
    }
};

static void
xdg_wm_base_ping(void*, struct xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    xdg_wm_base_ping,
};

static const struct wl_registry_listener globalRegistryListener = {

    // global
    [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
        Trace("wl_registry_listener.global interface=%s name=%d version=%d\n", interface, name, version);

        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        if (::strcmp(interface, "wl_compositor") == 0) {
            // I expect that a compositor is tied to a display, so expect the name here to be the one of the display.
            // Lets check :-)
            context._compositor = static_cast<struct wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
        }
        else if (::strcmp(interface, "wl_seat") == 0) {
            struct wl_seat* result = static_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 4));
            wl_seat_add_listener(result, &seatListener, data);
            context._seat = result;

        } else if (::strcmp(interface, "wl_shell") == 0) {
            // A shell, is probably associated with a client, so I guess we now need to find a client..
            // Save details and bind once xdg_wm_base configured
            context._shell = static_cast<struct wl_shell*>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
        } else if (::strcmp(interface, "wl_output") == 0) {
            struct wl_output* result = static_cast<struct wl_output*>(wl_registry_bind(registry, name, &wl_output_interface, 2));
            wl_output_add_listener(result, &outputListener, data);
            context._output = result;
        } else if (strcmp(interface, "xdg_wm_base") == 0) {
            struct xdg_wm_base* result = static_cast<struct xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));

            xdg_wm_base_add_listener(result, &wm_base_listener, data);
            context._wm_base = result;
        }
    },
    // global_remove
    [](void*, struct wl_registry*, uint32_t) {
        Trace("wl_registry_listener.global_remove\n");
    },
};

static void
handle_surface_configure(void*, struct xdg_surface* id,
             uint32_t serial)
{
    xdg_surface_ack_configure(id, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    handle_surface_configure
};


static void
handle_toplevel_configure(void*, struct xdg_toplevel*,
                          int32_t, int32_t, struct wl_array*)
{
}

static void
handle_toplevel_close(void*, struct xdg_toplevel*)
{
    //running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    handle_toplevel_configure,
    handle_toplevel_close
};

namespace Thunder {

namespace Wayland {
    /*static*/ Display::CriticalSection Display::_adminLock;
    /*static*/ std::string Display::_runtimeDir;
    /*static*/ Display::DisplayMap Display::_displays;
    /*static*/ Display::WaylandSurfaceMap Display::_waylandSurfaces;
    /*static*/ EGLenum Display::ImageImplementation::_eglTarget;
    /*static*/ PFNEGLCREATEIMAGEKHRPROC Display::ImageImplementation::_eglCreateImagePtr = nullptr;
    /*static*/ PFNEGLDESTROYIMAGEKHRPROC Display::ImageImplementation::_eglDestroyImagePtr = nullptr;

    /*
    Copyright (C) 2007 The Android Open Source Project
    Licensed under the Apache License, Version 2.0
    */
    static void printEGLConfiguration(EGLDisplay dpy, EGLConfig config)
    {
#define X(VAL)    \
    {             \
        VAL, #VAL \
    }
        struct {
            EGLint attribute;
            const char* name;
        } names[] = {
            X(EGL_BUFFER_SIZE),
            X(EGL_RED_SIZE),
            X(EGL_GREEN_SIZE),
            X(EGL_BLUE_SIZE),
            X(EGL_ALPHA_SIZE),
            X(EGL_CONFIG_CAVEAT),
            X(EGL_CONFIG_ID),
            X(EGL_DEPTH_SIZE),
            X(EGL_LEVEL),
            X(EGL_MAX_PBUFFER_WIDTH),
            X(EGL_MAX_PBUFFER_HEIGHT),
            X(EGL_MAX_PBUFFER_PIXELS),
            X(EGL_NATIVE_RENDERABLE),
            X(EGL_NATIVE_VISUAL_ID),
            X(EGL_NATIVE_VISUAL_TYPE),
            X(EGL_SAMPLE_BUFFERS),
            X(EGL_SAMPLES),
            X(EGL_SURFACE_TYPE),
            X(EGL_TRANSPARENT_TYPE),
        };
#undef X

        Trace("Config details:\n");
        for (unsigned int j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
            EGLint value = -1;
            EGLBoolean res = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
            if (res) {
                Trace("  - %s: %d (0x%x)\n", names[j].name, value, value);
            }
        }
    }

    Display::SurfaceImplementation::SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
        : _surface(nullptr)
        , _refcount(1)
        , _level(0)
        , _name(name)
        , _id(0)
        , _x(0)
        , _y(0)
        , _width(width)
        , _height(height)
        , _visible(0)
        , _opacity(0)
        , _ZOrder(0)
        , _display(&display)
        , _native(nullptr)
        , _shellSurface(nullptr)
        , _eglSurfaceWindow(EGL_NO_SURFACE)
        , _keyboard(nullptr)
        , _pointer(nullptr)
    {
        assert(display.IsOperational());

        _level = 0;

        _surface = wl_compositor_create_surface(display._compositor);

        if (_surface != nullptr) {

            Trace("### Creating a surface of size: %d x %d _surface=%p\n", width, height, _surface);

            _native = wl_egl_window_create(_surface, width, height);

            assert(EGL_NO_SURFACE != _native);

            if (_display->HasEGLContext() == true) {
                Connect(EGLSurface(EGL_NO_SURFACE));
            }
        }
    }

    Display::SurfaceImplementation::SurfaceImplementation(Display& display, const uint32_t id, struct wl_surface* surface)
        : _surface(surface)
        , _refcount(1)
        , _level(2)
        , _name()
        , _id(id)
        , _width(0)
        , _height(0)
        , _visible(0)
        , _opacity(0)
        , _ZOrder(0)
        , _display(&display)
        , _native(nullptr)
        , _shellSurface(nullptr)
        , _eglSurfaceWindow(EGL_NO_SURFACE)
        , _keyboard(nullptr)
        , _pointer(nullptr)
    {
    }

    Display::SurfaceImplementation::SurfaceImplementation(Display& display, const uint32_t id, const char* name)
        : _surface(nullptr)
        , _refcount(1)
        , _level(2)
        , _name(name)
        , _id(id)
        , _width(0)
        , _height(0)
        , _visible(0)
        , _opacity(0)
        , _ZOrder(0)
        , _display(&display)
        , _native(nullptr)
        , _shellSurface(nullptr)
        , _eglSurfaceWindow(EGL_NO_SURFACE)
        , _keyboard(nullptr)
        , _pointer(nullptr)
    {
    }

    Display::SurfaceImplementation::~SurfaceImplementation()
    {
        if (_xdg_toplevel != nullptr) {
            xdg_toplevel_destroy(_xdg_toplevel);
            _xdg_toplevel = nullptr;
        }

        if (_xdg_surface != nullptr) {
            if (_display != nullptr) {
                // Unlink the surface and remove from th
                _display->Destructed(reinterpret_cast<Display*>(_xdg_surface));
            }
            xdg_surface_destroy(_xdg_surface);
            _xdg_surface = nullptr;
        }
    }

    void Display::SurfaceImplementation::Resize(const int x, const int y, const int w, const int h)
    {
        xdg_surface_set_window_geometry(_xdg_surface, x, y, w, h);
    }

    void Display::SurfaceImplementation::Visibility(const bool)
    {
        Trace("WARNING: Display::SurfaceImplementation::Visibility is not implemented\n");
    }

    void Display::SurfaceImplementation::Opacity(const uint32_t)
    {
        Trace("WARNING: Display::SurfaceImplementation::Opacity is not implemented\n");
    }

    uint32_t Display::SurfaceImplementation::ZOrder(const uint16_t)
    {
        Trace("WARNING: Display::SurfaceImplementation::ZOrder is not implemented\n");
        return (Core::ERROR_UNAVAILABLE);
    }

    void Display::SurfaceImplementation::Dimensions(
        const uint32_t, const int32_t, const int32_t, const int32_t,
        const int32_t, const uint32_t, const uint32_t)
    {
    }

    void Display::SurfaceImplementation::Redraw()
    {
        _display->Trigger();

        // wait for wayland to flush events
        sem_wait(&(_display->_redraw));
        if (_native != nullptr) {
            eglSwapBuffers(_display->_eglDisplay, _eglSurfaceWindow);
        }
    }

    void Display::SurfaceImplementation::Unlink()
    {
        if (_display != nullptr) {

            if (_eglSurfaceWindow != EGL_NO_SURFACE) {

                eglDestroySurface(_display->_eglDisplay, _eglSurfaceWindow);
                _eglSurfaceWindow = EGL_NO_SURFACE;
            }

            if (_native != nullptr) {
                wl_egl_window_destroy(_native);
                _native = nullptr;
            }

            if (_shellSurface != nullptr) {
                wl_shell_surface_destroy(_shellSurface);
                _shellSurface = nullptr;
            }

            if (_surface != nullptr) {
                wl_surface_destroy(_surface);
                _surface = nullptr;
            }

            _display = nullptr;
        }
    }

    bool Display::SurfaceImplementation::Connect(const EGLSurface& surface)
    {
        if (surface != EGL_NO_SURFACE) {
            _eglSurfaceWindow = surface;
        } else {
            if (_display->_eglContext == EGL_NO_CONTEXT) {
                _display->InitializeEGL();
            }

            if (_display->_eglContext != EGL_NO_CONTEXT) {
                /*
                 * Create a window surface
                 */
                _eglSurfaceWindow = eglCreateWindowSurface(
                    _display->_eglDisplay,
                    _display->_eglConfig,
                    reinterpret_cast<EGLNativeWindowType>(_native),
                    nullptr);

                if (_eglSurfaceWindow == EGL_NO_SURFACE) {

                    _eglSurfaceWindow = eglCreateWindowSurface(
                        _display->_eglDisplay,
                        _display->_eglConfig,
                        static_cast<EGLNativeWindowType>(nullptr),
                        nullptr);
                }

                assert(EGL_NO_SURFACE != _eglSurfaceWindow);

                EGLint height(0);
                EGLint width(0);
                eglQuerySurface(_display->_eglDisplay, _eglSurfaceWindow, EGL_WIDTH, &width);
                eglQuerySurface(_display->_eglDisplay, _eglSurfaceWindow, EGL_HEIGHT, &height);

                Trace("EGL window surface is %dx%d\n", height, width);
            }
        }

        if (_eglSurfaceWindow != EGL_NO_SURFACE) {
            /*
             * Establish EGL context for this thread
             */

            int result = eglMakeCurrent(_display->_eglDisplay, _eglSurfaceWindow, _eglSurfaceWindow, _display->_eglContext);
            assert(EGL_FALSE != result);

            if (EGL_FALSE != result) {
                result = eglSwapInterval(_display->_eglDisplay, 1);
                assert(EGL_FALSE != result);
            }
        }

        return (_eglSurfaceWindow != EGL_NO_SURFACE);
    }

    Display::ImageImplementation::ImageImplementation(Display& display, const uint32_t texture, const uint32_t, const uint32_t)
        : _refcount(1)
        , _display(&display)
    {
        if (_eglCreateImagePtr == nullptr) {

            const char* eglExtension = eglQueryString(_display->_eglDisplay, EGL_EXTENSIONS);
            if (strstr(eglExtension, "EGL_KHR_image_base")) {
                _eglTarget = EGL_GL_TEXTURE_2D_KHR;

                _eglCreateImagePtr = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
                // Keep Destroy pointer for later reference.
                _eglDestroyImagePtr = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
            } else {
#if !defined(EGL_GL_TEXTURE_2D) // FIXME: Added to build for not supporting platform too, but has to be removed once support is ready
#define EGL_GL_TEXTURE_2D EGL_GL_TEXTURE_2D_KHR
#endif
                _eglTarget = EGL_GL_TEXTURE_2D;

                _eglCreateImagePtr = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImage"));
                // Keep Destroy pointer for later reference.
                _eglDestroyImagePtr = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImage"));
            }
        }

        _eglImage = _eglCreateImagePtr(_display->_eglDisplay, _display->_eglContext, _eglTarget,
            reinterpret_cast<EGLClientBuffer>(texture), 0);
    }

    Display::ImageImplementation::~ImageImplementation()
    {
        if (_display != nullptr) {
            _eglDestroyImagePtr(_display->_eglDisplay, _eglImage);
        }
    }

    static void* Processor(void* data)
    {
        Wayland::Display& context = *(static_cast<Wayland::Display*>(data));

        while ((sem_wait(&context._trigger) == 0) && (context._display != nullptr)) {
            Trace("Flush Events\n");
            wl_display_flush(context._display);

            context.Redraw();
        }
        return nullptr;
    }
    Display::~Display()
    {
        ASSERT(_refCount == 0);
        DisplayMap::iterator index(_displays.find(_displayName));

        if (index != _displays.end()) {
            _displays.erase(index);
        }
#ifdef BCM_HOST
        bcm_host_deinit();
#endif
    }


    void Display::Initialize()
    {
        Trace("Display::Initialize\n");
        if (_display != nullptr)
            return;

        if (_runtimeDir.empty() == true) {
            const char* envName = ::getenv("XDG_RUNTIME_DIR");
            if (envName != nullptr) {
                _runtimeDir = envName;
            }
        }

        Trace("Initialize Wayland Display on %s\n", _runtimeDir.c_str());
        Trace("Initialize Wayland Display Name %s\n", _displayName.c_str());
        if (_displayId.empty() == false)
            Trace("Connecting to Wayland Display %s\n", _displayId.c_str());

        _display = wl_display_connect(_displayId.empty() == false ? _displayId.c_str() : nullptr);

        assert(_display != nullptr);

        if (_display != nullptr) {
            sem_init(&_trigger, 0, 0);
            _registry = wl_display_get_registry(_display);

            assert(_registry != nullptr);

            if (_registry != nullptr) {

                wl_registry_add_listener(_registry, &globalRegistryListener, this);
                wl_display_roundtrip(_display);

                sem_init(&_redraw, 0, 0);
                Trace("creating communication thread\n");
                if (pthread_create(&_tid, nullptr, Processor, this) != 0) {
                    Trace("[Wayland] Error creating communication thread\n");
                }
            }
        }
    }

    void Display::InitializeEGL()
    {
        /*
         * Get default EGL display
         */
        _eglDisplay = eglGetDisplay(reinterpret_cast<NativeDisplayType>(_display));

        Trace("Display: %p\n", _eglDisplay);

        if (_eglDisplay == EGL_NO_DISPLAY) {
            Trace("Oops bad Display: %p\n", _eglDisplay);
        } else {
            /*
             * Initialize display
             */
            EGLint major, minor;
            if (eglInitialize(_eglDisplay, &major, &minor) != EGL_TRUE) {
                Trace("Unable to initialize EGL: %X\n", eglGetError());
            } else {
                /*
                 * Get number of available configurations
                 */
                EGLint configCount;
                Trace("Vendor: %s\n", eglQueryString(_eglDisplay, EGL_VENDOR));
                Trace("Version: %d.%d\n", major, minor);

                if (eglGetConfigs(_eglDisplay, nullptr, 0, &configCount)) {

                    EGLConfig eglConfigs[configCount];

                    EGLint attributes[] = {
                        EGL_RED_SIZE, RED_SIZE,
                        EGL_GREEN_SIZE, GREEN_SIZE,
                        EGL_BLUE_SIZE, BLUE_SIZE,
                        EGL_DEPTH_SIZE, DEPTH_SIZE,
                        EGL_STENCIL_SIZE, 0,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_NONE
                    };

                    Trace("Configs: %d\n", configCount);
                    /*
                     * Get a list of configurations that meet or exceed our requirements
                     */
                    if (eglChooseConfig(_eglDisplay, attributes, eglConfigs, configCount, &configCount)) {

                        /*
                         * Choose a suitable configuration
                         */
                        int index = 0;

                        while (index < configCount) {
                            EGLint redSize, greenSize, blueSize, alphaSize, depthSize;

                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_RED_SIZE, &redSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_GREEN_SIZE, &greenSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_BLUE_SIZE, &blueSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_ALPHA_SIZE, &alphaSize);
                            eglGetConfigAttrib(_eglDisplay, eglConfigs[index], EGL_DEPTH_SIZE, &depthSize);

                            if ((redSize == RED_SIZE) && (greenSize == GREEN_SIZE) && (blueSize == BLUE_SIZE) && (alphaSize == ALPHA_SIZE) && (depthSize >= DEPTH_SIZE)) {
                                break;
                            }

                            index++;
                        }
                        if (index < configCount) {
                            _eglConfig = eglConfigs[index];

                            EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2 /* ES2 */, EGL_NONE };

                            Trace("Config choosen: %d\n", index);
                            printEGLConfiguration(_eglDisplay, _eglConfig);

                            /*
                             * Create an EGL context
                             */
                            _eglContext = eglCreateContext(_eglDisplay, _eglConfig, EGL_NO_CONTEXT, attributes);

                            Trace("Context created\n");
                        }
                    }
                }
                Trace("Extentions: %s\n", eglQueryString(_eglDisplay, EGL_EXTENSIONS));
            }
        }
    }
    void Display::Deinitialize()
    {
        Trace("Display::Deinitialize\n");

        _adminLock.Lock();

        _keyboardReceiver = nullptr;

        // First notify our client of our destruction...
        SurfaceMap::iterator index(_surfaces.begin());

        while (index != _surfaces.end()) {
            // Remove the entry from global list
            WaylandSurfaceMap::iterator entry(_waylandSurfaces.find(index->second->_surface));

            if (entry != _waylandSurfaces.end()) {
                _waylandSurfaces.erase(entry);
            }

            index->second->Unlink();
            index->second->Release();
            index++;
        }

        _waylandSurfaces.clear();
        _surfaces.clear();

        if (_eglContext != EGL_NO_CONTEXT) {
            eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglTerminate(_eglDisplay);
            eglReleaseThread();
        }

        if (_output != nullptr) {
            wl_output_destroy(_output);
            _output = nullptr;
        }

        if (_wm_base != nullptr) {
            xdg_wm_base_destroy(_wm_base);
            _wm_base = nullptr;
        }

        if (_shell != nullptr) {
            wl_shell_destroy(_shell);
            _shell = nullptr;
        }

        if (_seat != nullptr) {
            wl_seat_destroy(_seat);
            _seat = nullptr;
        }

        if (_keyboard) {
            wl_keyboard_destroy(_keyboard);
            _keyboard = nullptr;
        }

        if (_compositor != nullptr) {
            wl_compositor_destroy(_compositor);
            _compositor = nullptr;
        }

        if (_registry != nullptr) {
            wl_registry_destroy(_registry);
            _registry = nullptr;
        }

        if (_display != nullptr) {
            wl_display_flush(_display);
            wl_display_disconnect(_display);
            _display = nullptr;
        }

        _adminLock.Unlock();

        Trigger();

        if (_tid) {
            pthread_join(_tid, nullptr);
        }
    }

    void Display::LoadSurfaces()
    {
        Trace("Display::LoadSurfaces\n");
    }

    Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
    {
        IDisplay::ISurface* result = nullptr;

        Trace("Display::Create: name = %s\n", name.c_str());
        _adminLock.Lock();

        SurfaceImplementation* surface = new SurfaceImplementation(*this, name, width, height);

        if(_wm_base != nullptr) {
            surface->_xdg_surface = xdg_wm_base_get_xdg_surface(_wm_base, surface->_surface);
            assert(surface->_xdg_surface != NULL);
            xdg_surface_add_listener(surface->_xdg_surface, &xdg_surface_listener, this);

            surface->_xdg_toplevel = xdg_surface_get_toplevel(surface->_xdg_surface);
            assert(surface->_xdg_toplevel != NULL);
            xdg_toplevel_add_listener(surface->_xdg_toplevel, &xdg_toplevel_listener, this);
            xdg_toplevel_set_title(surface->_xdg_toplevel, name.c_str());

            _waylandSurfaces.insert(std::pair<struct wl_surface*, SurfaceImplementation*>(surface->_surface, surface));
            _surfaces.insert(std::pair<void*, SurfaceImplementation*>(reinterpret_cast<Display*>(surface->_xdg_surface), surface));
            wl_surface_commit(surface->_surface);
            result = surface;
        }

        _adminLock.Unlock();

        // Wait till we are fully registered.
        wl_display_roundtrip(_display);

        return (result);
    }

    Display::Image Display::Create(const uint32_t texture, const uint32_t width, const uint32_t height)
    {
        Trace("Display::Create (with texture)\n");

        return (Image(*new ImageImplementation(*this, texture, width, height)));
    }

    void Display::Dimensions(
        const uint32_t, const uint32_t,
        const int32_t, const int32_t, const int32_t, const int32_t,
        const uint32_t, const uint32_t)
    {
    }

    void Display::Constructed(const void*, wl_surface*)
    {
    }

    void Display::Constructed(const void*, const char*)
    {
    }

    void Display::Destructed(const void* id)
    {
        Trace("Display::Destructed\n");

        _adminLock.Lock();

        SurfaceMap::iterator index = _surfaces.find(id);

        if (index != _surfaces.end()) {
            // See if it is in the surfaces map, we need to take it out here as well..
            WaylandSurfaceMap::iterator entry(_waylandSurfaces.find(index->second->_surface));

            assert(entry != _waylandSurfaces.end());

            if (entry != _waylandSurfaces.end()) {
                _waylandSurfaces.erase(entry);
            }

            if (_keyboardReceiver == index->second) {
                _keyboardReceiver = nullptr;
            }

            _surfaces.erase(index);
            index->second->Unlink();
        }
        _adminLock.Unlock();
    }

    /* static */ Display& Display::Instance(const std::string& displayName)
    {
        if (_runtimeDir.empty() == true) {
            const char* envName = ::getenv("XDG_RUNTIME_DIR");
            if (envName != nullptr) {
                _runtimeDir = envName;
            }
        }

        // Please define a runtime directory, or using an environment variable (XDG_RUNTIME_DIR)
        // or by setting it before creating a Display, using Display::RuntimeDirectory(<DIR>),
        // or by passing it as an argument to this method (none empty dir)
        assert(_runtimeDir.empty() == false);

        Display* result(nullptr);

        _adminLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName);
            _displays.insert(std::pair<const std::string, Display*>(displayName, result));
        } else {
            result = index->second;
        }
        result->AddRef();
        _adminLock.Unlock();

        assert(result != nullptr);

        return (*result);
    }

    static void signalHandler(int)
    {
    }

    void Display::Process(Display::IProcess* processloop)
    {

        struct sigaction sigint;

        sigint.sa_handler = signalHandler;
        sigemptyset(&sigint.sa_mask);
        sigint.sa_flags = SA_RESETHAND;
        sigaction(SIGINT, &sigint, nullptr);

        _thread = ::pthread_self();

        Trace("Setup dispatch loop using thread %p signal: %d \n", &_thread, _signal);
        if (_display != nullptr) {
            while ((wl_display_dispatch(_display) != -1) && (processloop->Dispatch() == true)) {
                /* intentionally left empty */
            }
        }
    }

    int Display::Process(const uint32_t data)
    {

        signed int result(0);
        _adminLock.Lock();
        if (_display) {

            while (wl_display_prepare_read(_display) != 0) {
                if (wl_display_dispatch_pending(_display) < 0) {
                    result = -1;
                    break;
                }
            }

            wl_display_flush(_display);

            if (data != 0) {
                if (wl_display_read_events(_display) < 0) {
                    result = -2;
                } else {
                    if (wl_display_dispatch_pending(_display) < 0) {
                        result = 1;
                    }
                }
            } else {
                wl_display_cancel_read(_display);
                result = -3;
            }
        }
        _adminLock.Unlock();
        return result;
    }

    uint32_t Display::AddRef() const
    {
        if (Core::InterlockedIncrement(_refCount) == 1) {
            const_cast<Display*>(this)->Initialize();
        }
        return Core::ERROR_NONE;
    }

    uint32_t Display::Release() const
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            const_cast<Display*>(this)->Deinitialize();

            //Indicate Wayland connection is closed properly
            return (Core::ERROR_DESTRUCTION_SUCCEEDED);
        }
        return (Core::ERROR_NONE);
    }

    int Display::FileDescriptor() const
    {
        return (wl_display_get_fd(_display));
    }

    void Display::Signal()
    {
        TRACE_L1(_T("Received Signal, killing thread %p"), &_thread);
        ::pthread_kill(_thread, SIGINT);
    }
}

/* static */ Compositor::IDisplay* Compositor::IDisplay::Instance(const std::string& displayName)
{
    return (&(Wayland::Display::Instance(displayName)));
}
}
