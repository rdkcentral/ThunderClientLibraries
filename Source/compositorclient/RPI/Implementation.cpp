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

// TOOD: glSetViewPort baseed on surface

#include "Module.h"

// TODO: This flag should be included in the preprocessor flags
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

template <class T>
struct _remove_const {
    typedef T type;
};

template <class T>
struct _remove_const <T const> {
    typedef T type;
};

template <class T>
struct _remove_const <T *> {
    typedef T * type;
};

template <class T>
struct _remove_const <T const *>{
    typedef T * type;
};

template <class FROM, class TO, bool ENABLE>
struct _narrowing {
    static_assert (( std::is_arithmetic < FROM > :: value && std::is_arithmetic < TO > :: value ) != false);

    // Not complete, assume zero is minimum for unsigned
    // Common type of signed and unsigned typically is unsigned
    using common_t = typename std::common_type < FROM, TO > :: type;
    static constexpr bool value =   ENABLE
                                    && (
                                        ( std::is_signed < FROM > :: value && std::is_unsigned < TO > :: value )
                                        || static_cast < common_t > ( std::numeric_limits < FROM >::max () ) >= static_cast < common_t > ( std::numeric_limits < TO >::max () )
                                    )
                                    ;
};

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence (T &&) {}

// Show logging in blue to make it distinct from TRACE
#define TRACE_WITHOUT_THIS(level, ...) do {             \
    fprintf (stderr, "\033[1;34m");                     \
    fprintf (stderr, "[%s:%d] : ", __FILE__, __LINE__); \
    fprintf (stderr, ##__VA_ARGS__);                    \
    fprintf (stderr, "\n");                             \
    fprintf (stderr, "\033[0m");                        \
    fflush (stderr);                                    \
} while (0)

namespace WPEFramework {
namespace RPI_INTERNAL {

    class GLResourceMediator {

        private :

// TODO: see modeset for types

            struct _native {
                struct gbm_surface * _surf;
                uint32_t _width;
                uint32_t _height;

                struct prime {
                    int _fd;
                    int _sync_fd;
                    uint32_t _width;
                    uint32_t _height;
                    uint32_t _stride;
// TODO DRM_FORMAT_INVALID and DRM_FORMAT_MOD_NONE narrowing
                    uint32_t _format;
                    uint64_t _modifier;

                    bool operator == (prime const & other) const {
                        bool _ret = _fd == other._fd
                                    && _sync_fd == other._sync_fd
                                    && _width == other._width
                                    && _height == other._height
                                    && _stride == other._stride
                                    && _format == other._format
                                    && _modifier == other._modifier;
                        return _ret;
                    }

                    bool operator < (prime const & other) const {
                        bool _ret =  !( *this == other );
                        return _ret;
                    }

                    bool operator () (prime const & left, prime const & right) const {
                        bool _ret = left < right;
                        return _ret;
                    }

                    bool Lock () {
                        auto init = [] () -> struct flock {
                            struct flock fl;
                            /* void * */ memset( &fl, 0, sizeof ( fl ) );

                            fl.l_type = F_WRLCK;
                            fl.l_whence = SEEK_SET;
                            fl.l_start = 0;
                            fl.l_len = 0;

                            return fl;
                        };

                        static struct flock fl = init ();

                        bool ret = _sync_fd > -1 && fcntl (_sync_fd, F_SETLKW,  &fl) != -1;
                        return ret;
                    }

                    bool Unlock () {
                        auto init = [] () -> struct flock {
                            struct flock fl;
                                /* void * */ memset( &fl, 0, sizeof ( fl ) );

                                fl.l_type = F_UNLCK;
                                fl.l_whence = SEEK_SET;
                                fl.l_start = 0;
                                fl.l_len = 0;

                                return fl;
                            };

                        static struct flock fl = init ();

                        bool ret = _sync_fd > -1 && fcntl (_sync_fd, F_SETLK, &fl) != -1;
                        return ret;
                    }

                } _prime;

                bool operator == (_native const & other) const {
                    bool _ret = _surf == other._surf
                                && _width == other._width
                                && _height == other._height
                                && _prime == other._prime;

                    return _ret;
                }

                bool operator < (_native const & other) const {
                    bool _ret = !( *this == other );
                    return _ret;
                }

                bool operator () (_native const & left, _native const & right) const {
                    bool _ret = left < right;
                    return _ret;
                }
            };

        public :

            using prime_t = decltype (_native::_prime);

            // Putting it here, enables easy life time management
            class EGL {

#define XSTRINGIFY(X) STRINGIFY(X)
#define STRINGIFY(X) #X

#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_FOREVER EGL_FOREVER
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#else
#define _KHRFIX(left, right) left ## right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_SYNC_FENCE EGL_SYNC_FENCE_KHR
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_FOREVER EGL_FOREVER_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
                using EGLAttrib = EGLint;
#endif
                using EGLuint64KHR = khronos_uint64_t;

                class Sync final {

                    public :

                        using dpy_t = EGLDisplay;
                        using sync_t = KHRFIX (EGLSync);

                    private :

                        sync_t _sync;
                        dpy_t _dpy;

                    public :
// TODO: calling Supported is expensive, sync objects are heavily used
                        explicit Sync (dpy_t & dpy) : _dpy {dpy} {
//                            assert (dpy != InvalidDpy ());

                            _sync = ( EGL::Supported (dpy, "EGL_KHR_fence_sync") && dpy != InvalidDpy () ) != false ? KHRFIX (eglCreateSync) (dpy, _EGL_SYNC_FENCE, nullptr) : InvalidSync ();
                        }

                        ~Sync () {

                            if (_sync == InvalidSync ()) {
                                // Error creating sync object or unable to create one
                                glFinish ();
                            }
                            else {
                                glFlush ();

                                EGLint _val = KHRFIX (eglClientWaitSync) (_dpy, _sync, 0 /* no flags */ , _EGL_FOREVER);

                                if (_val != EGL_TRUE) {
                                    // Error
                                }

                                // Consume the (possible) error
                                /* ELGint */ eglGetError ();
                            }
                        }

                        static constexpr dpy_t InvalidDpy () { return EGL_NO_DISPLAY; }
                        static constexpr sync_t InvalidSync () { return _EGL_NO_SYNC; }

                    private :

                        void * operator new (size_t) = delete;
                        void * operator new [] (size_t) = delete;
                        void operator delete (void *) = delete;
                        void operator delete [] (void *) = delete;
                };

                public :

                    using dpy_t = Sync::dpy_t;
                    using ctx_t = EGLContext;
                    using surf_t = EGLSurface;
                    using img_t = KHRFIX (EGLImage);
                    using win_t = EGLNativeWindowType;

                    using width_t = EGLint;
                    using height_t = EGLint;

                    using sync_t = Sync;

                    static constexpr dpy_t InvalidDpy () { return Sync::InvalidDpy (); }
                    static constexpr ctx_t InvalidCtx () { return EGL_NO_CONTEXT; }
                    static constexpr surf_t InvalidSurf () { return EGL_NO_SURFACE; }

                    static_assert (std::is_convertible < decltype (nullptr), win_t > :: value != false);
                    static constexpr win_t InvalidWin () { return nullptr; }

                    static constexpr img_t InvalidImg () { return _EGL_NO_IMAGE; }

                    static img_t CreateImage (dpy_t const & dpy, const ctx_t & ctx, const win_t win, GLResourceMediator::prime_t const & prime) {
                        img_t _ret = InvalidImg ();

                        if (dpy != InvalidDpy () && ctx != InvalidCtx () && win != InvalidImg () && ( Supported (dpy, "EGL_KHR_image") && Supported (dpy, "EGL_KHR_image_base")  && Supported (dpy, "EGL_EXT_image_dma_buf_import") && Supported (dpy, "EGL_EXT_image_dma_buf_import_modifiers") ) != false ) {

                            static_assert ((std::is_same <dpy_t, EGLDisplay> :: value && std::is_same <ctx_t, EGLContext> :: value && std::is_same <img_t, KHRFIX (EGLImage)> :: value ) != false);

                            constexpr char _methodName [] = XSTRINGIFY ( KHRFIX (eglCreateImage) );

                            static KHRFIX (EGLImage) (* _eglCreateImage) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLAttrib const * ) = reinterpret_cast < KHRFIX (EGLImage) (*) (EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, EGLAttrib const * ) > (eglGetProcAddress ( _methodName ));

                            if (_eglCreateImage != nullptr) {
                                using width_t =  decltype (prime._width);
                                using height_t = decltype (prime._height);
                                using stride_t = decltype (prime._stride);
                                using format_t = decltype (prime._format);
                                using modifier_t = decltype (prime._modifier);
                                using fd_t = decltype ( prime._fd);

                                // Narrowing detection

                                // Enable narrowing detection
// TODO:
                                constexpr bool _enable = false;

                                // (Almost) all will fail!
                                if (_narrowing < width_t, EGLAttrib, _enable > :: value != false
                                    && _narrowing < height_t, EGLAttrib, _enable > :: value != false
                                    && _narrowing < stride_t, EGLAttrib, _enable > :: value != false
                                    && _narrowing < format_t, EGLAttrib, _enable > :: value != false
                                    && _narrowing < modifier_t, EGLAttrib, _enable > :: value != false
                                    && _narrowing < fd_t, EGLAttrib, _enable > :: value != false) {
                                    TRACE_WITHOUT_THIS (Trace::Information, (_T ("Possible narrowing detected!")));
                                }


                                // EGL may report differently than GBM / DRM
                                // Platform formats are cross referenced against prime settings at construction time


                                // Query EGL
                                static EGLBoolean (* _eglQueryDmaBufFormatsEXT) (EGLDisplay, EGLint, EGLint *, EGLint *) = reinterpret_cast < EGLBoolean (*) (EGLDisplay, EGLint, EGLint *, EGLint *) > (eglGetProcAddress ("eglQueryDmaBufFormatsEXT"));
                                static EGLBoolean (* _eglQueryDmaBufModifiersEXT) (EGLDisplay,EGLint, EGLint, EGLuint64KHR *, EGLBoolean *, EGLint *) = reinterpret_cast < EGLBoolean (*) (EGLDisplay,EGLint, EGLint, EGLuint64KHR *, EGLBoolean *, EGLint *) > (eglGetProcAddress ("eglQueryDmaBufModifiersEXT"));

                                bool _valid = _eglQueryDmaBufFormatsEXT != nullptr && _eglQueryDmaBufModifiersEXT != nullptr;

                                EGLint _count = 0;

                                _valid = _valid && _eglQueryDmaBufFormatsEXT (dpy, 0, nullptr, &_count) != EGL_FALSE;

                                _valid = _valid && _eglQueryDmaBufFormatsEXT (dpy, 0, nullptr, &_count) != EGL_FALSE;

                                EGLint _formats [_count];

                                _valid = _valid && _eglQueryDmaBufFormatsEXT (dpy, _count, &_formats [0], &_count) != EGL_FALSE;

                                // _format should be listed as supported
                                if (_valid != false) {
                                    std::list <EGLint> _list_e_for (&_formats [0], &_formats [_count]);

                                    auto _it_e_for = std::find (_list_e_for.begin (), _list_e_for.end (), prime._format);

                                    _valid = _it_e_for != _list_e_for.end ();
                                }

                                _valid = _valid && _eglQueryDmaBufModifiersEXT (dpy, prime._format, 0, nullptr, nullptr, &_count) != EGL_FALSE;

                                EGLuint64KHR _modifiers [_count];
                                EGLBoolean _external [_count];

                                // External is required to exclusive use withGL_TEXTURE_EXTERNAL_OES
                                _valid = _valid && _eglQueryDmaBufModifiersEXT (dpy, prime._format, _count, &_modifiers [0], &_external [0], &_count) != FALSE;

                                // _modifier should be listed as supported, and _external should be true

                                if (_valid != false) {
                                    std::list <EGLuint64KHR> _list_e_mod (&_modifiers [0], &_modifiers [_count]);

                                    auto _it_e_mod = std::find (_list_e_mod.begin (), _list_e_mod.end (), static_cast <EGLuint64KHR> (prime._modifier));

                                    _valid = _it_e_mod != _list_e_mod.end ();

                                    // For the compositor not relevant, only relevant for the client
                                    if (_valid != false) {
                                        // For the compositor not relevant
                                        _valid = _external [ std::distance (_list_e_mod.begin (), _it_e_mod) ] != true;
                                    }
                                }

                                if (_valid != false) {
                                    EGLAttrib const _attrs [] = {
                                        EGL_WIDTH, static_cast <EGLAttrib> (prime._width),
                                        EGL_HEIGHT, static_cast <EGLAttrib> (prime._height),
                                        EGL_LINUX_DRM_FOURCC_EXT, static_cast <EGLAttrib> (prime._format),
                                        EGL_DMA_BUF_PLANE0_FD_EXT, static_cast <EGLAttrib> (prime._fd),
// TODO: magic constant
                                        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                                        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast <EGLAttrib> (prime._stride),
                                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, static_cast <EGLAttrib> (static_cast <EGLuint64KHR> (prime._modifier) & 0xFFFFFFFF),
                                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, static_cast <EGLAttrib> (static_cast <EGLuint64KHR> (prime._modifier) >> 32),
//                                        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                        EGL_NONE
                                    };

                                    static_assert (std::is_convertible < decltype (nullptr), EGLClientBuffer > :: value != false);
                                    _ret = _eglCreateImage (dpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, _attrs);
                                }
                            }
                            else {
                                // Error
                                TRACE_WITHOUT_THIS (Trace::Error, _T ("%s is unavailable or invalid parameters."), _methodName);
                            }
                        }
                        else {
                            TRACE_WITHOUT_THIS (Trace::Error, _T ("EGL is not properly initialized."));
                        }

                        return _ret;
                    }

                    static bool DestroyImage (img_t & img, dpy_t const & dpy, ctx_t const & ctx) {
                        bool _ret = img != InvalidImg ();

                        if (dpy != InvalidDpy () && ctx != InvalidCtx () && ( Supported (dpy, "EGL_KHR_image") && Supported (dpy, "EGL_KHR_image_base") ) != false ) {

                            static_assert ((std::is_same <dpy_t, EGLDisplay> :: value && std::is_same <ctx_t, EGLContext> :: value && std::is_same <img_t, KHRFIX (EGLImage)> :: value ) != false);

                            constexpr char _methodName [] = XSTRINGIFY ( KHRFIX (eglDestroyImage) );

                            static EGLBoolean (* _eglDestroyImage) (EGLDisplay, KHRFIX (EGLImage)) = reinterpret_cast < EGLBoolean (*) (EGLDisplay, KHRFIX (EGLImage)) > (eglGetProcAddress ( KHRFIX ("eglDestroyImage") ));

                            if (_eglDestroyImage != nullptr) {
                                _ret = _eglDestroyImage (dpy, img) != EGL_FALSE ? true : false;
                                img = InvalidImg ();
                            }
                            else {
                                // Error
                                TRACE_WITHOUT_THIS (Trace::Error, _T ("%s is unavailable or invalid parameters are provided."), _methodName);
                            }

                        }
                        else {
                            TRACE_WITHOUT_THIS (Trace::Error, (_T ("EGL is not properly initialized.")));
                        }

                        return _ret;
                    }

                    // Although compile / build time may succeed, runtime checks are also mandatory
                    static bool Supported (dpy_t const dpy, std::string const & name) {
                        bool _ret = false;

                        static_assert ((std::is_same <dpy_t, EGLDisplay> :: value) != false);

#ifdef EGL_VERSION_1_5
                        // KHR extentions that have become part of the standard

                        // Sync capability
                        _ret = name.find ("EGL_KHR_fence_sync") != std::string::npos
                               /* CreateImage / DestroyImage */
                               || name.find ("EGL_KHR_image") != std::string::npos
                               || name.find ("EGL_KHR_image_base") != std::string::npos;
#endif

                        if (_ret != true) {
                            static_assert (std::is_same <std::string::value_type, char> :: value != false);
                            char const * _ext = eglQueryString (dpy, EGL_EXTENSIONS);

                            _ret =  _ext != nullptr
                                    && name.size () > 0
                                    && ( std::string (_ext).find (name)
                                         != std::string::npos );
                        }

                        return _ret;
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
            };

            class GLES {
                public :

                    using fbo_t = GLuint;
                    using tex_t = GLuint;

                    static constexpr fbo_t InvalidFbo () { return 0; }
                    static constexpr tex_t InvalidTex () { return 0; }

                    bool ImageAsTarget (EGL::img_t const & img, EGL::width_t width, EGL::height_t height, tex_t & tex, fbo_t & fbo) {
                        bool _ret = glGetError () == GL_NO_ERROR && img != EGL::InvalidImg () && width > 0 && height > 0;

                        // Always
                        constexpr GLuint _tgt = GL_TEXTURE_2D;

                        if (_ret != false) {
                            // Just an arbitrary selected unit
                            glActiveTexture (GL_TEXTURE0);
                            _ret = glGetError () == GL_NO_ERROR;
                        }

                        if (_ret != false) {
                            if (tex != InvalidTex ()) {
                                glDeleteTextures (1, &tex);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            tex = InvalidTex ();

                            if (_ret != false) {
                                glGenTextures (1, &tex);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            if (_ret != false) {
                                glBindTexture (_tgt, tex);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            if (_ret != false) {
                                glTexParameteri (_tgt, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            if (_ret != false) {
                                glTexParameteri (_tgt, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            if (_ret != false) {
                                glTexParameteri (_tgt, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                _ret = glGetError () == GL_NO_ERROR;
                            }

                            if (_ret != false) {
                                glTexParameteri (_tgt, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                _ret = glGetError () == GL_NO_ERROR;
                            }


                            // A valid GL context should exist for GLES::Supported ()
                            EGL::ctx_t _ctx = eglGetCurrentContext ();
                            EGL::dpy_t _dpy = _ctx != EGL::InvalidCtx () ? eglGetCurrentDisplay () : EGL::InvalidDpy ();


                            _ret = _ret && eglGetError () == EGL_SUCCESS && _ctx != EGL::InvalidCtx ();

                            if ( _ret && ( Supported ("GL_OES_EGL_image") && ( EGL::Supported (_dpy, "EGL_KHR_image") || EGL::Supported (_dpy, "EGL_KHR_image_base") ) ) != false) {

                                // Take storage for the texture from the EGLImage; Pixel data becomes undefined
                                static void (* _EGLImageTargetTexture2DOES) (GLenum, GLeglImageOES) = reinterpret_cast < void (*) (GLenum, GLeglImageOES) > (eglGetProcAddress ("glEGLImageTargetTexture2DOES"));

                                if (_ret != false && _EGLImageTargetTexture2DOES != nullptr) {
                                    // Logical const
                                    using no_const_img_t = _remove_const < decltype (img) > :: type;
                                    _EGLImageTargetTexture2DOES (_tgt, reinterpret_cast <GLeglImageOES> ( const_cast < no_const_img_t > (img) ));
                                    _ret = glGetError () == GL_NO_ERROR;
                                }
                                else {
                                    _ret = false;
                                }
                            }

                            if (_ret != false) {
                                if (fbo != InvalidFbo ()) {
                                    glDeleteFramebuffers (1, &fbo);
                                    _ret = glGetError () == GL_NO_ERROR;
                                }

                                if (_ret != false) {
                                    glGenFramebuffers (1, &fbo);
                                    _ret = glGetError () == GL_NO_ERROR;
                                }

                                if (_ret != false) {
                                    glBindFramebuffer (GL_FRAMEBUFFER, fbo);
                                    _ret = glGetError () == GL_NO_ERROR;
                                }

                                if (_ret != false) {
                                    // Bind the created texture as one of the buffers of the frame buffer object
                                    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _tgt, tex, 0 /* level */);
                                    _ret = glGetError () == GL_NO_ERROR;
                                }

                                if (_ret != false) {
                                    _ret = glCheckFramebufferStatus (GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
                                }

                                if (_ret != true) {
                                    glDeleteFramebuffers (1, &fbo);
                                    glDeleteTextures (1, &tex);

                                    fbo = InvalidFbo ();
                                    tex = InvalidTex ();
                                }
                            }
                        }

                        return _ret;
                    }

                    bool Supported (std::string const & name) {
                        bool _ret = false;

                        using string_t = std::string::value_type;
                        using ustring_t = std::make_unsigned < string_t > :: type;

                        // Identical underlying types except for signedness
                        static_assert (std::is_same < ustring_t, GLubyte > :: value != false);

                        string_t const * _ext = reinterpret_cast <string_t const *> ( glGetString (GL_EXTENSIONS) );

                        _ret = _ext != nullptr
                               && name.size () > 0
                               && ( std::string (_ext).find (name)
                                    != std::string::npos );

                        return _ret;
                    }

            };

        public :

            using native_t = struct _native;
            using callback_t = std::function < void () >;

            static_assert ( std::is_convertible < decltype (native_t::_surf), decltype ( EGL::InvalidWin () ) > :: value != false );
// TODO: narrowing
            static constexpr native_t InvalidNative () { return { static_cast < decltype (native_t::_surf) > ( EGL::InvalidWin () ), 0, 0, { -1, -1, 0, 0, 0, DRM_FORMAT_INVALID, DRM_FORMAT_MOD_NONE} }; }

        private :

            // Enable platform specific hooks to be called
            std::map < native_t const, callback_t > _callbacks;

            std::recursive_mutex _resourceLock;

        public :

            static GLResourceMediator & Instance () {
                static GLResourceMediator _instance;
                return _instance;
            }

            // Enable EGL / GLES to call any method on the 'native'
            bool Register (native_t const & native, callback_t callback) {
                std::lock_guard < decltype (_resourceLock) > const _lock (_resourceLock);

                bool _ret = _callbacks.insert ( std::pair < native_t const, callback_t > ( native, callback) ).second;

                return _ret;
            }

            // As the name suggests
            bool Unregister (native_t const & native) {
                std::lock_guard < decltype (_resourceLock) > const _lock (_resourceLock);

                bool _ret = _callbacks.erase ( native ) == 1;

                return _ret;
            }

            // Call the registered callback for an existing association
            bool Visit (native_t const & native) /* const */ {
// TODO: logical const
                std::lock_guard < decltype (_resourceLock) > const _lock (_resourceLock);

                auto _it = _callbacks.find (native);

                bool _ret = _it != _callbacks.end ();

                    if (_ret != false) {
                        _it->second ();
                    }

                return _ret;
            }

        private :

            GLResourceMediator () = default;
            ~GLResourceMediator () = default;

            GLResourceMediator (GLResourceMediator const &) = delete;
            GLResourceMediator & operator= (GLResourceMediator const &) = delete;

    };

}
}

static const char* connectorNameVirtualInput = "/tmp/keyhandler";

namespace WPEFramework {
namespace RPI {

static Core::NodeId Connector()
{
    string connector;
    if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR"), connector) == false) || (connector.empty() == true)) {
        connector = _T("/tmp/compositor");
    }
    return (Core::NodeId(connector.c_str()));
}

class Display : public Compositor::IDisplay {
private:
    Display() = delete;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    Display(const std::string& displayName, Exchange::IComposition::IDisplay * display);

        class DMATransfer {

            private : 

                // Actual socket for communication
                int _transfer;

                struct sockaddr_un _addr;

                bool _valid;

                using valid_t = decltype (_valid);

                using timeout_t = _remove_const < decltype ( Core::infinite ) > :: type;

            public :

                // Sharing handles (file descriptors)
                static constexpr uint8_t MAX_SHARING_FDS = 2;
                using fds_t = std::array <int, MAX_SHARING_FDS>;

                DMATransfer () : _transfer { -1 }, _addr { AF_UNIX, "/tmp/Compositor/DMA" }, _valid { Initialize () } {}
                ~DMATransfer () {
                    /* bool */ Deinitialize ();
                }

                DMATransfer (DMATransfer const &) = delete;
                DMATransfer & operator = (DMATransfer const &) = delete;

                valid_t Valid () const { return _valid; }

                valid_t Receive (std::string & msg, DMATransfer::fds_t & fds) {
                    valid_t _ret = Valid ();

                    if (_ret != true) {
                        TRACE (Trace::Information, (_T ("Unable to receive (DMA) data.")));
                    }
                    else {
                        _ret = _Receive (msg, fds.data (), fds.size ());
                    }

                    return _ret;
                }

                valid_t Send (std::string const & msg, DMATransfer::fds_t const & fds) {
                    valid_t _ret = Valid ();

                    if (_ret != true) {
                        TRACE (Trace::Information, (_T ("Unable to send (DMA) data.")));
                    }
                    else {
                        _ret = _Send (msg, fds.data (), fds.size ());
                    }

                    return _ret;
                }

                valid_t Connect () {
                    valid_t _ret = false;

                    timeout_t _timeout = Core::infinite;

                    _ret = _Connect (_timeout);

                    return _ret;
                }

                valid_t Disconnect () {
                     valid_t _ret = false;

                    timeout_t _timeout = Core::infinite;

                    _ret = _Disconnect (_timeout);

                    return _ret;
                }

            private :

                valid_t  Initialize () {
                    valid_t _ret = false;

                    _transfer = socket (_addr.sun_family /* domain */, SOCK_STREAM /* type */, 0 /* protocol */);
                    _ret = _transfer != -1;

                    return _ret;
                }

                valid_t Deinitialize () {
                    valid_t _ret = false;

                    _ret = Disconnect ();

                    return _ret;
                }

                valid_t _Connect (timeout_t timeout) {
                    silence (timeout);

                    valid_t _ret = _transfer > -1;

                    _ret = _ret && connect (_transfer, reinterpret_cast < struct sockaddr const * > (&_addr), sizeof (_addr)) == 0;

                    decltype (EISCONN) _err = errno;

                    // Already connected is not an error
                    _ret = _ret != false || _err == EISCONN;

                    return _ret;
                }

                valid_t _Disconnect (timeout_t timeout) {
                    silence (timeout);

                    valid_t _ret = _transfer > -1;

                    _ret = _ret && close (_transfer) == 0;

                    _transfer = -1;

                    return _ret;
                }

                valid_t _Send (std::string const & msg, int const * fd, uint8_t count) {
                    using fd_t = _remove_const < std::remove_pointer < decltype (fd) > :: type > :: type;

                    valid_t _ret = false;

                    // Logical const
                    static_assert ((std::is_same <char *, _remove_const  < decltype ( & msg [0] ) > :: type >:: value) != false);
                    char * _buf  = const_cast <char *> ( & msg [0] );

                    size_t const _bufsize = msg.size ();

                    if (_bufsize > 0) {

                        // Scatter array for vector I/O
                        struct iovec _iov;

                        // Starting address
                        _iov.iov_base = reinterpret_cast <void *> (_buf);
                        // Number of bytes to transfer
                        _iov.iov_len = _bufsize;

                        // Actual message
                        struct msghdr _msgh {};

                        // Optional address
                        _msgh.msg_name = nullptr;
                        // Size of address
                        _msgh.msg_namelen = 0;
                        // Scatter array
                        _msgh.msg_iov = &_iov;
                        // Elements in msg_iov
                        _msgh.msg_iovlen = 1;

                        // Ancillary data
                        // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
                        char _control [CMSG_SPACE (sizeof ( fd_t ) * count)];

                        // Only valid file descriptor (s) can be sent via extra payload
                        _ret = true;
                        for (decltype (count) i = 0; i < count && fd != nullptr; i++) {
                            _ret = fd [i] > -1 && _ret;
                        }

                        if (_ret != false) {
                            // Contruct ancillary data to be added to the transfer via the control message

                            // Ancillary data, pointer
                            _msgh.msg_control = _control;

                            // Ancillery data buffer length
                            _msgh.msg_controllen = sizeof ( _control );

                            // Ancillary data should be access via cmsg macros
                            // https://linux.die.net/man/2/recvmsg
                            // https://linux.die.net/man/3/cmsg
                            // https://linux.die.net/man/2/setsockopt
                            // https://www.man7.org/linux/man-pages/man7/unix.7.html

                            // Control message

                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                            struct cmsghdr * _cmsgh = CMSG_FIRSTHDR ( &_msgh );

                            if (_cmsgh != nullptr) {
                                // Originating protocol
                                // To manipulate options at the sockets API level
                                _cmsgh->cmsg_level = SOL_SOCKET;

                                // Protocol specific type
                                // Option at the API level, send or receive a set of open file descriptors from another process
                                _cmsgh->cmsg_type = SCM_RIGHTS;

                                // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignmen, eg byte count of control message including header
                                _cmsgh->cmsg_len = CMSG_LEN (sizeof ( fd_t ) * count);

                                // Initialize the payload
                                /* void */ memcpy (CMSG_DATA (_cmsgh ), fd, sizeof ( fd_t ) * count);

                                _ret = true;
                            }
                            else {
                                // Error
                            }
                        }
                        else {
                            // No extra payload, ie  file descriptor(s), to include
                            _msgh.msg_control = nullptr;
                            _msgh.msg_controllen = 0;

                            _ret = true;
                        }

                        if (_ret != false) {
                            // Configuration succeeded

                            // https://linux.die.net/man/2/sendmsg
                            // https://linux.die.net/man/2/write
                            // Zero flags is equivalent to write

                            ssize_t _size = -1;
                            socklen_t _len = sizeof (_size);

                            // Only send data if the buffer is large enough to contain all data
                            if (getsockopt (_transfer, SOL_SOCKET, SO_SNDBUF, &_size, &_len) == 0) {
                                // Most options use type int, ssize_t was just a placeholder
                                static_assert (sizeof (int) <= sizeof (ssize_t));
                                TRACE (Trace::Information, (_T ("The sending buffer capacity equals %d bytes."), static_cast < int > (_size)));

// TODO: do not send if the sending buffer is too small
                                _size = sendmsg (_transfer, &_msgh, 0);
                            }
                            else {
                                // Unable to determine buffer capacity
                            }

                            _ret = _size != -1;

                            if (_ret != false) {
                                TRACE (Trace::Information, (_T ("Send %d bytes out of %d."), _size, _msgh.msg_iov->iov_len /* just a single element */ + _msgh.msg_controllen));
                            }
                            else {
                                TRACE (Trace::Error, (_T ("Failed to send data.")));
                            }
                        }

                    }
                    else {
                        TRACE (Trace::Error, (_T ("A data message to be send cannot be empty!")));
                    }

                    return _ret;
                }

                valid_t _Receive (std::string & msg, int * fd, uint8_t count) {
                    bool _ret = false;

                    msg.clear ();

                    ssize_t _size = -1;
                    socklen_t _len = sizeof (_size);

                    if (getsockopt (_transfer, SOL_SOCKET, SO_RCVBUF, &_size, &_len) == 0) {
                        TRACE (Trace::Information, (_T ("The receiving buffer maximum capacity equals %d [bytes]."), _size));

                        // Most options use type int, ssize_t was just a placeholder
                        static_assert (sizeof (int) <= sizeof (ssize_t));
                        msg.reserve (static_cast < int > (_size));
                    }
                    else {
                        // Unable to determine buffer capacity
                        TRACE (Trace::Information, (_T ("Unable to determine buffer maximum cpacity. Using %d [bytes] instead."), msg.capacity ()));
                    }

                    size_t const _bufsize = msg.capacity ();

                    if (_bufsize > 0 && count > 0 && fd != nullptr) {
                        using fd_t = std::remove_pointer < decltype (fd) > :: type;

                        for (decltype (count) i = 0; i < count; i++) {
                            fd [i] = -1;
                        }

                        static_assert ((std::is_same <char *, _remove_const  < decltype ( & msg [0] ) > :: type >:: value) != false);
                        char _buf [_bufsize];

                        // Scatter array for vector I/O
                        struct iovec _iov;

                        // Starting address
                        _iov.iov_base = reinterpret_cast <void *> ( &_buf [0] );
                        // Number of bytes to transfer
                        _iov.iov_len = _bufsize;

                        // Actual message
                        struct msghdr _msgh {};

                        // Optional address
                        _msgh.msg_name = nullptr;
                        // Size of address
                        _msgh.msg_namelen = 0;
                        // Elements in msg_iov
                        _msgh.msg_iovlen = 1;
                        // Scatter array
                        _msgh.msg_iov = &_iov;

                        // Ancillary data
                        // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
                        char _control [CMSG_SPACE (sizeof ( fd_t ) * count)];

                        // Ancillary data, pointer
                        _msgh.msg_control = _control;

                        // Ancillary data buffer length
                        _msgh.msg_controllen = sizeof (_control);

                        // No flags set
// TODO: do not receive if the receiving buffer is too small
                        _size = recvmsg (_transfer, &_msgh, 0);

                        _ret = _size > 0;

                        switch (_size) {
                            case -1 :   // Error
                                        {
                                            // TRACE (Trace::Error, (_T ("Error receiving remote (DMA) data.")));
                                            break;
                                        }
                            case 0  :   // Peer has done an orderly shutdown, before any data was received
                                        {
                                            // TRACE (Trace::Error, (_T ("Error receiving remote (DMA) data. Client may have become unavailable.")));
                                            break;
                                        }
                            default :   // Data
                                        {
                                            // Extract the file descriptor information
                                            TRACE (Trace::Information, (_T ("Received %d bytes."), _size));

                                            // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                                            struct cmsghdr * _cmsgh = CMSG_FIRSTHDR ( &_msgh );

                                            // Check for the expected properties the client should have set
                                            if (_cmsgh != nullptr 
                                                && _cmsgh->cmsg_level == SOL_SOCKET 
                                                && _cmsgh->cmsg_type == SCM_RIGHTS) {

                                                // The macro returns a pointer to the data portion of a cmsghdr.
                                                /* void */ memcpy (fd, CMSG_DATA (_cmsgh ), sizeof ( fd_t ) * count);
                                            }
                                            else {
                                                TRACE (Trace::Information, (_T ("No (valid) ancillary data received.")));
                                            }

                                            msg.assign (_buf, _size);
                                        }
                        }
                    }
                    else {
                        // Error
                        TRACE (Trace::Error, (_T ("A receiving data buffer (message) cannot be empty!")));
                    }

                    return _ret;
                }
        };

    class SurfaceImplementation : public Compositor::IDisplay::ISurface {
    public:
        SurfaceImplementation() = delete;
        SurfaceImplementation(const SurfaceImplementation&) = delete;
        SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        SurfaceImplementation(
            Display& display, const std::string& name,
            const uint32_t width, const uint32_t height);
        ~SurfaceImplementation() override;

    public:
        EGLNativeWindowType Native() const override
        {
            static_assert ( (std::is_convertible < decltype (_nativeSurface._surf), EGLNativeWindowType >:: value ) != false);
            return static_cast < EGLNativeWindowType > ( _nativeSurface._surf );
        }
        std::string Name() const override {
            return ( _remoteClient != nullptr ? _remoteClient->Name() : string());
        }
        void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
        {
            assert((_keyboard == nullptr) ^ (keyboard == nullptr));
            _keyboard = keyboard;
        }
        void Pointer(Compositor::IDisplay::IPointer* pointer) override
        {
            assert((_pointer == nullptr) ^ (pointer == nullptr));
            _pointer = pointer;
        }
        void Wheel(Compositor::IDisplay::IWheel* wheel) override
        {
            assert((_wheel == nullptr) ^ (wheel == nullptr));
            _wheel = wheel;
        }
        void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
        {
            assert((_touchpanel == nullptr) ^ (touchpanel == nullptr));
            _touchpanel = touchpanel;
        }
        int32_t Width() const override
        {
            using width_t = decltype (_nativeSurface._width);

            static_assert ( ( std::numeric_limits < width_t > :: is_exact &&
                0 >= std::numeric_limits < int32_t > :: min () &&
                std::numeric_limits < std::make_signed < width_t >::type > :: max () <= std::numeric_limits < int32_t > :: max () ) !=
                false);

            return ( _nativeSurface._surf != nullptr && _nativeSurface._width <= static_cast < width_t > (std::numeric_limits < int32_t >::max ()) ? _nativeSurface._width : 0 );
        }
        int32_t Height() const override
        {
            using height_t = decltype (_nativeSurface._height);

            static_assert ( ( std::numeric_limits < height_t > :: is_exact &&
                0 >= std::numeric_limits < int32_t > :: min () &&
                std::numeric_limits < std::make_signed < height_t >::type > :: max () <= std::numeric_limits < int32_t > :: max () ) !=
                false);

            return ( _nativeSurface._surf != nullptr && _nativeSurface._height <= static_cast < height_t > (std::numeric_limits < int32_t >::max ()) ? _nativeSurface._height : 0 );
        }
        inline void SendKey(
            const uint32_t key,
            const IKeyboard::state action, const uint32_t)
        {
            if (_keyboard != nullptr) {
                _keyboard->Direct(key, action);
            }
        }
        inline void SendWheelMotion(const int16_t x, const int16_t y, const uint32_t)
        {
            if (_wheel != nullptr) {
                _wheel->Direct(x, y);
            }
        }
        inline void SendPointerButton(const uint8_t button, const IPointer::state state, const uint32_t)
        {
            if (_pointer != nullptr) {
                _pointer->Direct(button, state);
            }
        }
        inline void SendPointerPosition(const int16_t x, const int16_t y, const uint32_t)
        {
            if (_pointer != nullptr) {
                _pointer->Direct(x, y);
            }
        }
        inline void SendTouch(const uint8_t index, const ITouchPanel::state state, const uint16_t x, const uint16_t y, const uint32_t)
        {
            if (_touchpanel != nullptr) {
                _touchpanel->Direct(index, state, x, y);
            }
        }

        inline void ScanOut () {
            // Changes of currents cannot be reliably be monitored
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t _dpy = eglGetCurrentDisplay ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t _ctx = eglGetCurrentContext ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t _surf = eglGetCurrentSurface (EGL_DRAW);

            bool _ret = /*_nativeSurface != WPEFramework::RPI_INTERNAL::GLResourceMediator::InvalidNative ()
                        && */_dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                        && _ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                        && _surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ();

            // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
            if ( _ret != false && _remoteRenderer != nullptr && _remoteClient != nullptr) {
                _remoteRenderer->ScanOut ();
            }
            else {
                TRACE (Trace::Error, (_T ("Remote scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?")));
            }
        }

        inline void PreScanOut () {
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t _width = 0;
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t _height = 0;


            // Changes of currents cannot be reliably be monitored
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t _dpy = eglGetCurrentDisplay ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t _ctx = eglGetCurrentContext ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t _surf = eglGetCurrentSurface (EGL_DRAW);


            bool _ret = /*_nativeSurface != WPEFramework::RPI_INTERNAL::GLResourceMediator::InvalidNative ()
                        && */_dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                        && _ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                        && _surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ()
                        && eglQuerySurface (_dpy, _surf, EGL_WIDTH, &_width) != EGL_FALSE
                        && eglQuerySurface (_dpy, _surf, EGL_HEIGHT, &_height) != EGL_FALSE;

            if (_ret != false) {
// TODO: Destroy
                static WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::img_t _img = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::CreateImage (_dpy, _ctx, _nativeSurface._surf, _nativeSurface._prime );

// TODO: Destroy
                static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::tex_t _tex = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidTex ();
// TODO: Destroy
                static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::fbo_t _fbo = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFbo ();

                static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES _gles;

                static_assert (_narrowing < decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::_width), WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t, true > :: value && _narrowing < decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::_height), WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t, true > :: value != false);

                using n_width_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::_width);
                using e_width_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t;

                using n_height_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::_height);
                using e_height_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t;


// TODO:
                constexpr bool _enable = true;

                if ( ( _narrowing < n_width_t, e_width_t, _enable > :: value
                      && _narrowing < n_height_t, e_height_t, _enable > :: value ) != false
                ) {
                    TRACE_WITHOUT_THIS (Trace::Information, (_T ("Possible narrowing detected!")));
                }


                using common_width_t = std::common_type < n_width_t, e_width_t> :: type;
                using common_height_t = std::common_type < n_height_t, e_height_t> :: type;


                bool _ret = _img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImg () && static_cast <common_width_t> (_width) == static_cast <common_width_t> (_nativeSurface._width) && static_cast <common_height_t> (_height) == static_cast <common_height_t> (_nativeSurface._height) && _gles.ImageAsTarget (_img, _width, _height, _tex, _fbo);

                _ret = ( _ret && _img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImg () && _fbo != WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFbo () ) != false;
            }

            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::sync_t _sync (_dpy);

            // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
            if ( _ret != false && _remoteRenderer != nullptr && _remoteClient != nullptr) {
                _remoteRenderer->PreScanOut ();
            }
            else {
                TRACE (Trace::Error, (_T ("Remote pre scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?")));
            }

        }

        inline void PostScanOut () {
            // Changes of currents cannot be reliably be monitored
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t _dpy = eglGetCurrentDisplay ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t _ctx = eglGetCurrentContext ();
            WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t _surf = eglGetCurrentSurface (EGL_DRAW);

            bool _ret = /*_nativeSurface != WPEFramework::RPI_INTERNAL::GLResourceMediator::InvalidNative ()
                        && */_dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                        && _ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                        && _surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ();

            if (_ret != false) {
                WPEFramework::RPI_INTERNAL::GLResourceMediator & _map = WPEFramework::RPI_INTERNAL::GLResourceMediator::Instance ();
                _ret = _map.Visit (_nativeSurface) != false;
            }

            // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
            if ( _ret != false && _remoteRenderer != nullptr && _remoteClient != nullptr) {
                _remoteRenderer->PostScanOut ();
            }
            else {
                TRACE (Trace::Error, (_T ("Remote post scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?")));
            }
        }

        bool SyncPrimitiveStart () {
            bool ret = _nativeSurface._prime.Lock ();
            return ret;
        }

        bool SyncPrimitiveEnd () {
            bool ret = _nativeSurface._prime.Unlock ();
            return ret;
        }

    private:

        Display& _display;

        IKeyboard* _keyboard;
        IWheel* _wheel;
        IPointer* _pointer;
        ITouchPanel* _touchpanel;

        Exchange::IComposition::IClient* _remoteClient;
        Exchange::IComposition::IRender* _remoteRenderer;

        WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t _nativeSurface;

        void Callback () {
//            TRACE_WITHOUT_THIS (Trace::Information, _T ("Callback called"));
        }

    };

    using InputFunction = std::function<void(SurfaceImplementation*)>;

    static void VirtualKeyboardCallback(keyactiontype type, unsigned int code)
    {
        if (type != KEY_COMPLETED) {
            time_t timestamp = time(nullptr);
            const IDisplay::IKeyboard::state state = ((type == KEY_RELEASED) ? IDisplay::IKeyboard::released : 
                                                     ((type == KEY_REPEAT)   ? IDisplay::IKeyboard::repeated : 
                                                                               IDisplay::IKeyboard::pressed));

            InputFunction action = [=](SurfaceImplementation* s) { s->SendKey(code, state, timestamp); };

            Publish(action);
        }
    }

    static void VirtualMouseCallback(mouseactiontype type, unsigned short button, signed short horizontal, signed short vertical)
    {
        static int32_t pointer_x = 0;
        static int32_t pointer_y = 0;

        time_t timestamp = time(nullptr);
        InputFunction action;
        pointer_x = pointer_x + horizontal;
        pointer_y = pointer_y + vertical;

        switch (type)
        {
            case MOUSE_MOTION:
                action = [=](SurfaceImplementation* s) { 
                    int32_t X = std::min(std::max(0, pointer_x), s->Width());
                    int32_t Y = std::min(std::max(0, pointer_y), s->Height());
                    s->SendPointerPosition(X,Y,timestamp); };
                break;
            case MOUSE_SCROLL:
                action = [=](SurfaceImplementation* s) { s->SendWheelMotion(horizontal, vertical, timestamp); };
                break;
            case MOUSE_RELEASED:
            case MOUSE_PRESSED:
                action = [=](SurfaceImplementation* s) { s->SendPointerButton(button, type == MOUSE_RELEASED? IDisplay::IPointer::released : IDisplay::IPointer::pressed, timestamp); };
                break;
        }

        Publish(action);
    }

    static void VirtualTouchScreenCallback(touchactiontype type, unsigned short index, unsigned short x, unsigned short y)
    {
        static uint16_t touch_x = ~0;
        static uint16_t touch_y = ~0;
        static touchactiontype last_type = TOUCH_RELEASED;

        // Get touch position in pixels
        // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
        if ((x != touch_x) || (y != touch_y) || (type != last_type)) {

            last_type = type;
            touch_x = x;
            touch_y = y;

            time_t timestamp = time(nullptr);
            const IDisplay::ITouchPanel::state state = ((type == TOUCH_RELEASED) ? ITouchPanel::released : 
                                                       ((type == TOUCH_PRESSED)  ? ITouchPanel::pressed  : 
                                                                                   ITouchPanel::motion));

            InputFunction action = [=](SurfaceImplementation* s) { 
                const uint16_t mapped_x = (s->Width() * x) >> 16;
                const uint16_t mapped_y = (s->Height() * y) >> 16;
                s->SendTouch(index, state, mapped_x, mapped_y, timestamp); 
            };

            Publish(action);
        }
    }

public:
    typedef std::map<const string, Display*> DisplayMap;

    ~Display() override;

    static Display* Instance(const string& displayName, Exchange::IComposition::IDisplay * display) {
        Display* result(nullptr);

        _displaysMapLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName, display);
            if( result->RemoteDisplay() != nullptr ) {
                _displays.insert(std::pair<const std::string, Display*>(displayName, result));
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

    void AddRef() const override
    {
        Core::InterlockedIncrement(_refCount);
    }

    uint32_t Release() const override
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            _displaysMapLock.Lock();

            DisplayMap::iterator display = _displays.find(_displayName);

            if (display != _displays.end()){
                _displays.erase(display);
            }

            _displaysMapLock.Unlock();

            delete this;

            return (Core::ERROR_CONNECTION_CLOSED);
        }
        return (Core::ERROR_NONE);
    }

    EGLNativeDisplayType Native() const override
    {
        static_assert ( (std::is_convertible < decltype (_nativeDisplay._device), EGLNativeDisplayType > :: value ) != false);
        return static_cast < EGLNativeDisplayType > ( _nativeDisplay._device );
    }
    const std::string& Name() const override
    {
        return (_displayName);
    }
    int Process(const uint32_t data) override;
    int FileDescriptor() const override;
    ISurface* SurfaceByName(const std::string& name) override;
    ISurface* Create( const std::string& name, const uint32_t width, const uint32_t height) override;


    bool PrimeFor (Exchange::IComposition::IClient const & client, WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t & prime) {
        std::lock_guard < decltype (_surfaceLock) > const _lock (_surfaceLock);

        bool _ret = false;
// TODO: narrowing
        prime = { -1, -1, 0, 0, 0, DRM_FORMAT_INVALID, DRM_FORMAT_MOD_NONE};

        DMATransfer::fds_t handles = {prime._fd, prime._sync_fd};

        if (_dma != nullptr) {
            std::string _msg = client.Name ();

            // Announce ourself to indicate the data exchange is a request for (file descriptor) data
            // Then, after receiving the data DMA is no longer needed for this client
            _ret = _dma->Connect () && _dma->Send (_msg, handles) && _dma->Receive (_msg, handles) && _dma->Disconnect ();

            prime._fd = handles [0];
            prime._sync_fd = handles [1];

            // <prefix>:width:height:stride:format
            // Search from end to begin

            using width_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t::_width);
            using height_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t::_height);
            using stride_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t::_height);
            using format_t = decltype ( WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t::_height);
            using modifier_t = decltype (WPEFramework::RPI_INTERNAL::GLResourceMediator::prime_t::_height);

            using common_t = std::common_type <width_t, height_t, stride_t, format_t, modifier_t> :: type;

            auto str2prop = [&_msg] (std::string::size_type & pos) -> common_t {
                constexpr char _spacer [] = ":";

                std::string::size_type _start = _msg.rfind (_spacer, pos);

                static_assert (_narrowing <unsigned long, common_t, true> :: value != false);
                common_t _ret = ( _start != std::string::npos && _start <= _msg.length () != false ) ? std::stoul (_msg.substr (_start + 1, pos) ) : 0;

                // Narrowing detection

// TODO:
                constexpr bool _enable = false;

                if ( _narrowing < decltype (_ret), uint32_t, _enable> :: value != false ) {
                    TRACE_WITHOUT_THIS (Trace::Information, (_T ("Possible narrowing detected!")));
                }

                decltype (ERANGE) _err = errno;

                if (_err == ERANGE) {
                    TRACE_WITHOUT_THIS (Trace::Error, (_T ("Unbale to determine property value.")));
                }

                pos = _start - 1;

                return _ret;
            };

            std::string::size_type _pos = _msg.size () - 1;

// TODO: signed vs unsigned , commmon_type  for return type str2prop
            prime._modifier = static_cast <modifier_t> (str2prop (_pos));
            prime._format = static_cast <format_t> (str2prop (_pos));
            prime._stride = static_cast <stride_t> (str2prop (_pos));
            prime._height = static_cast <height_t> (str2prop (_pos));
            prime._width = static_cast <width_t> (str2prop (_pos));

            TRACE_WITHOUT_THIS (Trace::Information, _T ("Received the following properties via DMA: width %d height %d stride %d format %d modifier %d."), prime._width, prime._height, prime._stride, prime._format, prime._modifier);
        }

        if (_ret != true) {
            TRACE (Trace::Error, (_T ("Unable to receive (DMA) data for %s."), client.Name ().c_str ()));
        }

        return _ret;
    }

private:
    inline Exchange::IComposition::IClient* CreateRemoteSurface(const std::string& name, const uint32_t width, const uint32_t height);
    inline void Register(SurfaceImplementation* surface);
    inline void Unregister(SurfaceImplementation* surface);

    inline static void Publish(InputFunction& action) {
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

    inline void Initialize (Exchange::IComposition::IDisplay * display)
    {
        if (Core::WorkerPool::IsAvailable() == true) {
            // If we are in the same process space as where a WorkerPool is registered (Main Process or
            // hosting process) use, it!
            Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());
            ASSERT(static_cast<Core::IReferenceCounted*>(engine) != nullptr);

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));
            ASSERT(_compositerServerRPCConnection != nullptr);

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        } else {
            // Seems we are not in a process space initiated from the Main framework process or its hosting process.
            // Nothing more to do than to create a workerpool for RPC our selves !
            Core::ProxyType<RPC::InvokeServerType<2,0,8>> engine = Core::ProxyType<RPC::InvokeServerType<2,0,8>>::Create();
            ASSERT(engine != nullptr);

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));
            ASSERT(_compositerServerRPCConnection != nullptr);

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        }

        if (display != nullptr) {
            _remoteDisplay = display;
        }
        else {
            // Connect to the CompositorServer..
            uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);

            if (result != Core::ERROR_NONE) {
                TRACE_L1(_T("Could not open connection to Compositor with node %s. Error: %s"), _compositerServerRPCConnection->Source().RemoteId().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
                _compositerServerRPCConnection.Release();
            }
            else {
                _remoteDisplay = _compositerServerRPCConnection->Aquire<Exchange::IComposition::IDisplay>(2000, _displayName, ~0);

                if (_remoteDisplay == nullptr) {
                    TRACE_L1(_T("Could not create remote display for Display %s!"), Name().c_str());
                }
            }
        }

        _virtualinput = virtualinput_open(_displayName.c_str(), connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE_L1(_T("Initialization of virtual input failed for Display %s!"), Name().c_str());
        }
    }

    inline void Deinitialize()
    {
        if (_virtualinput != nullptr) {
            virtualinput_close(_virtualinput);
            _virtualinput = nullptr;
        }

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            string name = (*index)->Name();

            if ((*index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) { //note, need cast to prevent ambigious call
                TRACE_L1(_T("Compositor Surface [%s] is not properly destructed"), name.c_str());
            }

            index = _surfaces.erase(index);
        }

        if(_remoteDisplay != nullptr) {
            _remoteDisplay->Release();
            _remoteDisplay = nullptr;
        }

        if (_compositerServerRPCConnection.IsValid() == true) {
            _compositerServerRPCConnection.Release();
        }
    }

    inline const Exchange::IComposition::IDisplay* RemoteDisplay() const {
        return _remoteDisplay;
    }

    inline Exchange::IComposition::IDisplay* RemoteDisplay() {
        return _remoteDisplay;
    }

private:

    static DisplayMap _displays; 
    static Core::CriticalSection _displaysMapLock;

    std::string _displayName;
    mutable Core::CriticalSection _adminLock;
    void* _virtualinput;
    std::list<SurfaceImplementation*> _surfaces;
    Core::ProxyType<RPC::CommunicatorClient> _compositerServerRPCConnection;
    mutable uint32_t _refCount;

    Exchange::IComposition::IDisplay* _remoteDisplay;

    struct {
        struct gbm_device * _device;
        int _fd;
    } _nativeDisplay;

    DMATransfer * _dma;
    std::mutex _surfaceLock;
};

Display::DisplayMap Display::_displays;
Core::CriticalSection Display::_displaysMapLock;

Display::SurfaceImplementation::SurfaceImplementation(
    Display& display,
    const std::string& name,
    const uint32_t width, const uint32_t height)
    : _display(display)
    , _keyboard(nullptr)
    , _wheel(nullptr)
    , _pointer(nullptr)
    , _touchpanel(nullptr)
    , _remoteClient(nullptr)
    , _remoteRenderer(nullptr)
    , _nativeSurface { WPEFramework::RPI_INTERNAL::GLResourceMediator::InvalidNative () }
{
// TODO: gbm_surface_Create_with_modifiers
    _nativeSurface._surf = gbm_surface_create (_display.Native (), width, height, RenderDevice::SupportedBufferType (), GBM_BO_USE_RENDERING /* used for rendering */);

    if (_nativeSurface._surf == nullptr) {
        TRACE_L1 (_T ("Failed to create a native (underlying) surface"));
    }
    else {
        _nativeSurface._height = height;
        _nativeSurface._width = width;
    }

    // Implicit AddRef ()
    _remoteClient = _display.CreateRemoteSurface (name, width, height);

    if (_remoteClient != nullptr) {
        // Implicit AddRef ()
        _remoteRenderer = _remoteClient->QueryInterface<Exchange::IComposition::IRender> ();

        if(_remoteRenderer == nullptr) {
            TRACE_L1(_T("Could not aquire remote renderer for surface %s."), name.c_str());
        }
        else {

            if (_display.PrimeFor (*_remoteClient, _nativeSurface._prime) != true || gbm_device_is_format_supported (static_cast <RenderDevice::GBM::dev_t> (_display.Native ()), _nativeSurface._prime._format, GBM_BO_USE_RENDERING) != 1) {
                TRACE (Trace::Error, (_T ( "Failed to setup a share for rendering results!")));

                auto result = _remoteRenderer->Release ();
                ASSERT (result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _remoteRenderer = nullptr;

                /* auto */ result = _remoteClient->Release ();
                ASSERT (result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                _remoteClient = nullptr;
            }
            else {
                assert (gbm_device_get_format_modifier_plane_count (static_cast <RenderDevice::GBM::dev_t>(_display.Native ()), _nativeSurface._prime._format, _nativeSurface._prime._modifier) == 1);

                // Register a callback for EGL updates

                WPEFramework::RPI_INTERNAL::GLResourceMediator & _map = WPEFramework::RPI_INTERNAL::GLResourceMediator::Instance ();
                /* bool */ _map.Register ( _nativeSurface, std::bind ( &WPEFramework::RPI::Display::SurfaceImplementation::Callback, this ) );
            }

        }

    } else {
        TRACE_L1(_T("Could not create remote surface for surface %s."), name.c_str());
    }

    _display.AddRef();
    _display.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation()
{
    WPEFramework::RPI_INTERNAL::GLResourceMediator & _map = WPEFramework::RPI_INTERNAL::GLResourceMediator::Instance ();
    /* bool */ _map.Unregister ( _nativeSurface );

    _display.Unregister(this);

    if(_remoteClient != nullptr) {

        TRACE_L1(_T("Destructing surface %s"), _remoteClient->Name().c_str());

        if(_remoteRenderer != nullptr) {
            auto result = _remoteRenderer->Release ();
            ASSERT (result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _remoteRenderer = nullptr;
        }

        auto result = _remoteClient->Release ();
        ASSERT (result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        _remoteClient = nullptr;

    }

    if (_nativeSurface._surf != nullptr) {
        gbm_surface_destroy (_nativeSurface._surf);
        _nativeSurface._surf = nullptr;
    }

    _nativeSurface = WPEFramework::RPI_INTERNAL::GLResourceMediator::InvalidNative ();

    auto result = _display.Release ();
    ASSERT (result = Core::ERROR_DESTRUCTION_SUCCEEDED);
}

Display::Display (const string& name, Exchange::IComposition::IDisplay * display)
    : _displayName(name)
    , _adminLock()
    , _virtualinput(nullptr)
    , _surfaces()
    , _compositerServerRPCConnection()
    , _refCount(0)
    , _remoteDisplay(nullptr)
    , _nativeDisplay {nullptr, -1}
    , _dma { nullptr }
{
    Initialize (display);

    std::vector < std::string > _nodes;

    RenderDevice::GetNodes (static_cast <uint32_t> (DRM_NODE_RENDER), _nodes);

    for (auto _begin = _nodes.begin (), _it = _begin, _end = _nodes.end (); _it != _end; _it++) {
        decltype (_nativeDisplay._fd) & _fd = _nativeDisplay._fd;
        decltype (_nativeDisplay._device) & _device = _nativeDisplay._device;

        _fd = open(_it->c_str(), O_RDWR);

        if (_fd >= 0) {
            _device = gbm_create_device (_fd);

            if (_device != nullptr) {
                TRACE (Trace::Information, (_T ("Opened RenderDevice: %s"), _it->c_str ()));

                _dma = new DMATransfer ();

                if (_dma == nullptr || _dma->Valid () != true) {
                    TRACE (Trace::Error, (_T ("DMA transfers are not supported.")));

                    delete _dma;
                    _dma = nullptr;
                }
                else {
                    break;
                }
            }

            /* int */ close (_fd);
        }
    }
}

Display::~Display()
{
    if (_dma != nullptr) {
        delete _dma;
    }

    decltype (_nativeDisplay._fd) & _fd = _nativeDisplay._fd;
    decltype (_nativeDisplay._device) & _device = _nativeDisplay._device;

    if (_device != nullptr) {
        gbm_device_destroy (_device);
    }

    if (_fd >= 0) {
        close (_fd);
    }

    _dma = nullptr;
    _device = nullptr;
    _fd = -1;

    Deinitialize();
}

int Display::Process(const uint32_t)
{
    // Signed and at least 45 bits
    using milli_t = int32_t;

    auto RefreshRateFromResolution = [] (Exchange::IComposition::ScreenResolution const resolution) -> milli_t {
        // Assume 'unknown' rate equals 60 Hz
        milli_t _rate = 60;

        switch (resolution) {
            case Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                                                        _rate = 24; break;
            case Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
            case Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
            case Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
                                                                        _rate = 50; break;
            case Exchange::IComposition::ScreenResolution_480i      : // 720x480
            case Exchange::IComposition::ScreenResolution_480p      : // 720x480 progressive
            case Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
            case Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
            case Exchange::IComposition::ScreenResolution_Unknown   :   _rate = 60;
        }

        return _rate;
    };

    constexpr milli_t _milli = 1000;

    static decltype (_milli) _rate = RefreshRateFromResolution ( ( _remoteDisplay != nullptr ? _remoteDisplay->Resolution () : Exchange::IComposition::ScreenResolution_Unknown ) );
    static std::chrono::milliseconds _delay = std::chrono::milliseconds (_milli / _rate);

// TODO: move to scanout ?

    // Delay the (free running) loop
    auto _current_time = std::chrono::steady_clock::now ();

    static decltype (_current_time) _last_access_time = _current_time;

    auto _duration = std::chrono::duration_cast < std::chrono::milliseconds > (_current_time - _last_access_time);

    if (_duration.count () < _delay .count () ) {
        std::this_thread::sleep_for( std::chrono::milliseconds (_delay - _duration) );
    }
    else {
    }

    _last_access_time = _current_time;

    // Scan out all surfaces belonging to this display
// TODO: only scan out the surface that actually has completed or the client should be made aware that all surfaces should be completed at the time of calling
// TODO: This flow introduces artefacts
    for (auto _begin = _surfaces.begin (), _it = _begin, _end = _surfaces.end (); _it != _end; _it++) {

        // THe way the API has been constructed limits the optimal syncing strategy

        (*_it)->PreScanOut ();  // will render

        // At least fails the very first time
        bool  ret = (*_it)->SyncPrimitiveEnd ();
//            assert (ret != false);

        (*_it)->ScanOut ();     // actual scan out (at the remote site)

        ret = (*_it)->SyncPrimitiveStart ();
//            assert (ret != false);

        (*_it)->PostScanOut (); // rendered

    }

    return (0);
}

int Display::FileDescriptor() const
{
    return -1;
}

Compositor::IDisplay::ISurface* Display::SurfaceByName(const std::string& name)
{
    IDisplay::ISurface* result = nullptr;

    _adminLock.Lock();

    std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
    while ( (index != _surfaces.end()) && ((*index)->Name() != name) ) { index++; }

    if (index != _surfaces.end()) {
        result = *index;
        result->AddRef();
    }

    _adminLock.Unlock();

    return result;
}

static uint32_t WidthFromResolution (Exchange::IComposition::ScreenResolution const resolution) {
    // Asumme an invalid width equals 0
    uint32_t _width = 0;

    switch (resolution) {
        case Exchange::IComposition::ScreenResolution_480p      :   // 720x480
                                                                    _width = 720; break;
        case Exchange::IComposition::ScreenResolution_720p      :   // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution_720p50Hz  :   // 1280x720 @ 50 Hz
                                                                    _width = 720; break;
        case Exchange::IComposition::ScreenResolution_1080p24Hz :   // 1920x1080 progressive @ 24 Hz
        case Exchange::IComposition::ScreenResolution_1080i50Hz :   // 1920x1080 interlaced  @ 50 Hz
        case Exchange::IComposition::ScreenResolution_1080p50Hz :   // 1920x1080 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution_1080p60Hz :   // 1920x1080 progressive @ 60 Hz
                                                                    _width = 1920; break;
        case Exchange::IComposition::ScreenResolution_2160p50Hz :   // 4K, 3840x2160 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution_2160p60Hz :   // 4K, 3840x2160 progressive @ 60 Hz
                                                                    _width = 3840; break;
        case Exchange::IComposition::ScreenResolution_480i      :   // Unknown according to the standards (?)
        case Exchange::IComposition::ScreenResolution_Unknown   :
        default                                                 :   _width = 0;
    }

    return _width;
}

static uint32_t HeightFromResolution(const Exchange::IComposition::ScreenResolution resolution) {
    // Asumme an invalid height equals 0
    uint32_t _height = 0;

    switch (resolution) {
        // For descriptions see WidthFromResolution
        case Exchange::IComposition::ScreenResolution_480i      : // 720x480
        case Exchange::IComposition::ScreenResolution_480p      : // 720x480 progressive
                                                                    _height = 480; break;
        case Exchange::IComposition::ScreenResolution_720p      : // 1280x720 progressive
        case Exchange::IComposition::ScreenResolution_720p50Hz  : // 1280x720 progressive @ 50 Hz
                                                                    _height = 720; break;
        case Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
        case Exchange::IComposition::ScreenResolution_1080i50Hz : // 1920x1080 interlaced @ 50 Hz
        case Exchange::IComposition::ScreenResolution_1080p50Hz : // 1920x1080 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution_1080p60Hz : // 1920x1080 progressive @ 60 Hz
                                                                    _height = 1080; break;
        case Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
        case Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                                    _height = 2160; break;
        case Exchange::IComposition::ScreenResolution_Unknown   :
        default                                                 :   _height = 0;
    }

    return _height;
}

Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
{
    uint32_t _realHeight = height;
    uint32_t _realWidth = width;

    if (_remoteDisplay != nullptr) {
        Exchange::IComposition::ScreenResolution _resolution = _remoteDisplay->Resolution ();

        _realHeight = HeightFromResolution (_resolution);
        _realWidth = WidthFromResolution (_resolution);

        if (_realWidth != width || _realHeight != height) {
            TRACE (Trace::Information, (_T ("Requested surface dimensions (%d x %d) differ from true (real) display dimensions (%d x %d). Continuing with the latter!"), width, height, _realWidth, _realHeight));
        }
    }
    else {
        TRACE (Trace::Information, (_T ("No remote display exist. Unable to query its dimensions. Expect the unexpected!")));
    }

    Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, _realWidth, _realHeight));
    Compositor::IDisplay::ISurface* result = &(*retval);
    result->AddRef();

    return result;
}

inline Exchange::IComposition::IClient* Display::CreateRemoteSurface(const std::string& name, const uint32_t width, const uint32_t height) {
    return (_remoteDisplay != nullptr ? _remoteDisplay->CreateClient(name, width, height) : nullptr);
}

inline void Display::Register(Display::SurfaceImplementation* surface)
{
    ASSERT(surface != nullptr);

    _adminLock.Lock();

    std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));
    if (index == _surfaces.end()) {
        _surfaces.push_back(surface);
    }

    _adminLock.Unlock();
}

inline void Display::Unregister(Display::SurfaceImplementation* surface)
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

} // RPI

Compositor::IDisplay* Compositor::IDisplay::Instance (const string& displayName, void * display)
{
    return RPI::Display::Instance(displayName, reinterpret_cast < Exchange::IComposition::IDisplay * > (display));
}
} // WPEFramework
