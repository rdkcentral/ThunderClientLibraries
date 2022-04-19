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

#include "../Module.h"

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

template < typename T >
struct remove_const {
    typedef T type;
};

template < typename T >
struct remove_const < T const > {
    typedef T type;
};

template < typename T >
struct remove_const < T *> {
    typedef T * type;
};

template < typename T >
struct remove_const < T const * > {
    typedef T * type;
};

template < typename FROM , typename TO , bool ENABLE >
struct narrowing {
    static_assert ( ( std::is_arithmetic < FROM > :: value && std::is_arithmetic < TO > :: value ) != false );

    // Not complete, assume zero is minimum for unsigned
    // Common type of signed and unsigned typically is unsigned
    using common_t = typename std::common_type < FROM , TO > :: type;
    static constexpr bool value =      ENABLE
                                    && (    (    std::is_signed < FROM > :: value
                                              && std::is_unsigned < TO > :: value
                                            )
                                         || (    std::is_same < FROM , TO > :: value
                                              && static_cast < common_t > ( std::numeric_limits < FROM > :: max () ) > static_cast < common_t > ( std::numeric_limits < TO > :: max () )
                                            )
                                       )
                                    ;
};

template < typename TYPE , intmax_t VAL >
struct in_signed_range {
    static_assert ( (    std::is_integral < TYPE > :: value
                      && std::is_signed < TYPE > :: value
                    )
                    != false
                  );
    using common_t = typename std::common_type < TYPE , decltype ( VAL ) > :: type;
    static constexpr bool value =    static_cast < common_t > ( VAL ) >= static_cast < common_t > ( std::numeric_limits < TYPE > :: min () )
                                  && static_cast < common_t > ( VAL ) <= static_cast < common_t > ( std::numeric_limits < TYPE > :: max () );
};

template < typename TYPE , uintmax_t VAL >
struct in_unsigned_range {
    static_assert ( (    std::is_integral < TYPE > :: value
                      && std::is_unsigned < TYPE > :: value
                    )
                    != false
                  );
    using common_t = typename std::common_type < TYPE , decltype ( VAL ) > :: type;
    static constexpr bool value =    static_cast < common_t > ( VAL ) >= static_cast < common_t > ( std::numeric_limits < TYPE > :: min () )
                                  && static_cast < common_t > ( VAL ) <= static_cast < common_t > ( std::numeric_limits < TYPE > :: max () );
};

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template < typename T >
void silence ( T && ) {}

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


            class Native final {
            public :



                class Prime final {
                public :

                    using fd_t = RenderDevice::DRM::fd_t;
                    using width_t = RenderDevice::GBM::width_t;
                    using height_t = RenderDevice::GBM::height_t;
                    using stride_t = RenderDevice::GBM::stride_t;
                    using format_t = RenderDevice::GBM::frmt_t;
                    using modifier_t = RenderDevice::GBM::modifier_t;

                    using valid_t = bool;

                    Prime ( Prime const & ) = delete;
                    Prime & operator= ( Prime const & ) = delete;

                    Prime ();
                    explicit Prime ( fd_t const & , fd_t const & , modifier_t const & , format_t const & , stride_t const & , height_t const & , width_t const & );
                    ~Prime ();

                    explicit Prime ( Prime && );

                    Prime & operator= ( Prime && );

                    bool operator== ( Prime const & ) const;
                    bool operator< ( Prime const &  ) const;
                    bool operator() ( Prime const & , Prime const & ) const;

                    fd_t const & Fd () const { return _fd ; }
                    fd_t const & SyncFd () const { return _sync_fd ; }

                    modifier_t const & Modifier () const { return _modifier ; }
                    format_t const & Format () const  { return _format ; };
                    stride_t const & Stride () const { return _stride ; }

                    height_t const & Height () const { return _height ; }
                    width_t const & Width () const { return _width ; }

                    bool Lock ();
                    bool Unlock ();

                    static constexpr fd_t InvalidFd () { return static_cast < fd_t > ( RenderDevice::DRM::InvalidFd () ) ; }
                    static constexpr modifier_t InvalidModifier () { return static_cast < modifier_t > ( RenderDevice::GBM::InvalidModifier () ) ; }
                    static constexpr format_t InvalidFormat () { return static_cast < format_t > ( RenderDevice::GBM::InvalidFrmt () ) ; }
                    static constexpr stride_t InvalidStride () { return static_cast < stride_t > ( RenderDevice::GBM::InvalidStride () ) ; }
                    static constexpr width_t InvalidWidth () { return static_cast < width_t > ( RenderDevice::GBM::InvalidWidth () ) ; }
                    static constexpr height_t InvalidHeight () { return static_cast < height_t > ( RenderDevice::GBM::InvalidHeight () ) ; } 

                    valid_t Valid () const;

                private :

                    fd_t _fd;
                    fd_t _sync_fd;

                    modifier_t _modifier;
                    format_t _format;
                    stride_t _stride;

                    height_t _height;
                    width_t _width;
                };



                using surf_t = RenderDevice::GBM::surf_t;
                using prime_t = Native::Prime;

                using width_t = RenderDevice::DRM::width_t;
                using height_t = RenderDevice::DRM::height_t;

                using valid_t = bool;

                Native () = delete;
                Native ( Native const & ) = delete;
                Native & operator= ( Native const & ) = delete;

                explicit Native ( surf_t const , width_t const , height_t const );
                // A way to update prime
                explicit Native ( prime_t && , surf_t const , width_t const , height_t const );
// TODO: clean up
                ~Native () = default;

                explicit Native ( Native && );
                Native & operator= ( Native && );

                bool operator == ( Native const & other ) const;
                bool operator < ( Native const & other ) const;
                bool operator () ( Native const & , Native const &) const;

                prime_t const & Prime () const { return _prime; }
                surf_t const & Surface () const { return _surf; }

                width_t const & Width () const { return _width; }
                height_t const & Height () const { return _height; }

                static constexpr surf_t InvalidSurface () { return static_cast < surf_t > ( RenderDevice::GBM::InvalidSurf () ) ; }
                static constexpr width_t InvalidWidth () { return static_cast < width_t >  ( RenderDevice::GBM::InvalidWidth () ) ; }
                static constexpr height_t InvalidHeight () { return static_cast < height_t > ( RenderDevice::GBM::InvalidHeight () ) ; }

                valid_t Valid ();

            private :

                prime_t _prime;

                surf_t _surf;
                width_t _width;
                height_t _height;
            };

        public :

            class EGL {
            private :
#ifdef EGL_VERSION_1_5
#define KHRFIX(name) name
#define _EGL_NO_SYNC EGL_NO_SYNC
#define _EGL_NO_IMAGE EGL_NO_IMAGE
#else
#define _KHRFIX(left, right) left ## right
#define KHRFIX(name) _KHRFIX(name, KHR)
#define _EGL_NO_SYNC EGL_NO_SYNC_KHR
#define _EGL_NO_IMAGE EGL_NO_IMAGE_KHR
                using EGLAttrib = EGLint;
#endif
                using EGLuint64KHR = khronos_uint64_t;



                class Sync final {
                public :

                    using dpy_t = EGLDisplay;

                    Sync () = delete;
                    Sync ( Sync const & ) = delete;
                    Sync ( Sync && ) = delete;

                    Sync & operator= ( Sync const & ) = delete;
                    Sync & operator= ( Sync && ) = delete;

                    void * operator new ( size_t ) = delete;
                    void * operator new [] ( size_t ) = delete;
                    void operator delete ( void * ) = delete;
                    void operator delete [] ( void * ) = delete;

                    explicit Sync ( dpy_t & );
                    ~Sync ();

                private :

                    using sync_t = KHRFIX ( EGLSync );

                    static_assert ( std::is_convertible < decltype ( _EGL_NO_SYNC ) , sync_t > :: value != false );

                    static constexpr dpy_t InvalidDpy () { return static_cast < dpy_t > ( EGL::InvalidDpy () ) ; }
                    static constexpr sync_t InvalidSync () { return static_cast < sync_t > ( _EGL_NO_SYNC ) ; }

                    sync_t _sync;
                    dpy_t _dpy;
                };



            public :

                EGL () = delete;
                ~EGL () = delete;

                using dpy_t = Sync::dpy_t;
                using ctx_t = EGLContext;
                using surf_t = EGLSurface;
                using img_t = KHRFIX ( EGLImage );
                using win_t = EGLNativeWindowType;

                using width_t = EGLint;
                using height_t = EGLint;

                using sync_t = Sync;

                static_assert ( std::is_convertible < decltype ( EGL_NO_DISPLAY ) , dpy_t > :: value != false );
                static_assert ( std::is_convertible < decltype ( EGL_NO_CONTEXT ) , ctx_t > :: value != false );
                static_assert ( std::is_convertible < decltype ( EGL_NO_SURFACE ) , surf_t > :: value != false );

                static constexpr dpy_t InvalidDpy () { return static_cast < dpy_t > ( EGL_NO_DISPLAY ) ; }
                static constexpr ctx_t InvalidCtx () { return static_cast < ctx_t > ( EGL_NO_CONTEXT ) ; }
                static constexpr surf_t InvalidSurf () { return static_cast < surf_t > ( EGL_NO_SURFACE ); }

                static_assert ( std::is_convertible < decltype ( nullptr ) , win_t > :: value != false );
                static constexpr win_t InvalidWin () { return static_cast < win_t > ( nullptr ) ; }

                static_assert ( std::is_convertible < decltype ( _EGL_NO_IMAGE ) , dpy_t > :: value != false );
                static constexpr img_t InvalidImg () { return static_cast < img_t > ( _EGL_NO_IMAGE ) ; }

                static img_t CreateImage ( dpy_t const & , ctx_t const & , win_t const , Native::prime_t const & );
                static bool DestroyImage ( img_t & , dpy_t const & , ctx_t const & );

                // Although compile / build time may succeed, runtime checks are also mandatory
                static bool Supported ( dpy_t const , std::string const & );

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

                GLES () = default;
                ~GLES () = default;

                using fbo_t = GLuint;
                using tex_t = GLuint;

                // Probably signed to unsigned
                static_assert ( std::is_integral < fbo_t > :: value != false );
                static constexpr fbo_t InvalidFbo () { return static_cast < fbo_t > ( 0 ); }
                static_assert ( std::is_integral < tex_t > :: value != false );
                static constexpr tex_t InvalidTex () { return static_cast < tex_t > ( 0 ); }

                bool ImageAsTarget ( EGL::img_t const & , EGL::width_t const, EGL::height_t const , tex_t & , fbo_t & );

                bool Supported ( std::string const & );
            };

        public :

            using native_t = Native;

            static_assert ( std::is_convertible < native_t::surf_t, decltype ( EGL::InvalidWin () ) > :: value != false );

            GLResourceMediator ( GLResourceMediator const & ) = delete;
            GLResourceMediator ( GLResourceMediator && ) = delete;

            GLResourceMediator & operator= ( GLResourceMediator const & ) = delete;
            GLResourceMediator & operator= ( GLResourceMediator && ) = delete;

            static GLResourceMediator & Instance ();

        private :

            GLResourceMediator () = default;
            ~GLResourceMediator () = default;
    };

}
}



static const char* connectorNameVirtualInput = "/tmp/keyhandler";



namespace WPEFramework {
namespace RPI {

    static Core::NodeId Connector ();

    class Display : public Compositor::IDisplay {
    public:

        typedef std::map < const string , Display * > DisplayMap;

        Display () = delete;
        Display ( Display const & ) = delete;
        Display & operator= ( Display const & ) = delete;

        ~Display () override;

        static Display * Instance ( string const & , Exchange::IComposition::IDisplay * );

        void AddRef() const override;
        uint32_t Release() const override;

        EGLNativeDisplayType Native () const override;

        std::string const & Name () const override;

        int Process ( uint32_t const ) override;

        int FileDescriptor () const override;

        ISurface * SurfaceByName ( std::string const & ) override;

        ISurface * Create ( std::string const & , uint32_t const , uint32_t const ) override;

        bool PrimeFor ( Exchange::IComposition::IClient const & , WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t & );

    private :

        class SurfaceImplementation;

        using InputFunction = std::function < void ( SurfaceImplementation * ) >;

        static void VirtualKeyboardCallback ( keyactiontype , unsigned int const );
        static void VirtualMouseCallback ( mouseactiontype , unsigned short const , signed short const , signed short const );
        static void VirtualTouchScreenCallback ( touchactiontype , unsigned short const , unsigned short const , unsigned short const );



        class DMATransfer final {
        public :
            using valid_t = bool;

            // Sharing handles (file descriptors)
            static constexpr uint8_t const MAX_SHARING_FDS = 2;
            using fds_t = std::array < int , MAX_SHARING_FDS >;

            DMATransfer ( DMATransfer const & ) = delete;
            DMATransfer & operator= ( DMATransfer const & ) = delete;


            DMATransfer ();
            ~DMATransfer ();

            valid_t Valid () const;

            valid_t Receive ( std::string & , fds_t & fds );
            valid_t Send ( std::string const & , fds_t const & );

            valid_t Connect ();
            valid_t Disconnect ();

        private :

            using timeout_t = remove_const < decltype ( Core::infinite ) > :: type;

            using sock_t = int;
            using fd_t = int;

            static constexpr sock_t InvalidSocket () { return static_cast < sock_t > ( -1 ) ; };
            static constexpr fd_t InvalidFd () { return static_cast < fd_t > ( -1 ) ; };

            valid_t Initialize ();
            valid_t Deinitialize ();

            valid_t Connect ( timeout_t );
            valid_t Disconnect ( timeout_t );

            valid_t Send ( std::string const & , int const * , uint8_t const );
            valid_t Receive ( std::string & , int * , uint8_t const );

            // Actual socket for communication
            sock_t _transfer;

            struct sockaddr_un _addr;

            valid_t _valid;
        };



        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        public:

            SurfaceImplementation () = delete;
            SurfaceImplementation ( SurfaceImplementation const & ) = delete;
            SurfaceImplementation & operator= ( SurfaceImplementation const & ) = delete;

            SurfaceImplementation ( Display & display , std::string const & name , uint32_t const width , uint32_t const height );
            ~SurfaceImplementation () override;

            EGLNativeWindowType Native () const override;

            std::string Name () const override;

            void Keyboard ( Compositor::IDisplay::IKeyboard * ) override;
            void Pointer ( Compositor::IDisplay::IPointer * ) override;
            void Wheel ( Compositor::IDisplay::IWheel * ) override;
            void TouchPanel ( Compositor::IDisplay::ITouchPanel * ) override;

            int32_t Width() const override;
            int32_t Height() const override;

            inline void SendKey ( uint32_t const , IKeyboard::state const , uint32_t const );
            inline void SendWheelMotion ( int16_t const , int16_t const , uint32_t const );
            inline void SendPointerButton ( uint8_t const , IPointer::state const , uint32_t const );
            inline void SendPointerPosition ( int16_t const , int16_t const , uint32_t const );
            inline void SendTouch ( uint8_t const , ITouchPanel::state const , uint16_t const , uint16_t const , uint32_t const );

            inline void ScanOut ();
            inline void PreScanOut ();
            inline void PostScanOut ();

            bool SyncPrimitiveStart ();
            bool SyncPrimitiveEnd ();


        private :

            Display & _display;

            IKeyboard * _keyboard;
            IWheel * _wheel;
            IPointer * _pointer;
            ITouchPanel * _touchpanel;

            Exchange::IComposition::IClient * _remoteClient;
            Exchange::IComposition::IRender * _remoteRenderer;

            WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t _nativeSurface;
        };


        Display ( std::string const & , WPEFramework::Exchange::IComposition::IDisplay * );

        inline Exchange::IComposition::IClient * CreateRemoteSurface ( std::string const & , uint32_t const , uint32_t const );

        inline void Register ( SurfaceImplementation * surface );
        inline void Unregister ( SurfaceImplementation * surface );

        inline static void Publish ( InputFunction & action );

        inline void Initialize ( WPEFramework::Exchange::IComposition::IDisplay * display );
        inline void Deinitialize ();

        inline Exchange::IComposition::IDisplay const * RemoteDisplay () const;
        inline Exchange::IComposition::IDisplay * RemoteDisplay ();

    private :

        static DisplayMap _displays;
        static Core::CriticalSection _displaysMapLock;

        std::string _displayName;
        mutable Core::CriticalSection _adminLock;
        void * _virtualinput;
        std::list < SurfaceImplementation * > _surfaces;
        Core::ProxyType < RPC::CommunicatorClient > _compositerServerRPCConnection;
        mutable uint32_t _refCount;

        Exchange::IComposition::IDisplay * _remoteDisplay;

        struct {
            struct gbm_device * _device;
            int _fd;
        } _nativeDisplay;

        DMATransfer * _dma;
        std::mutex _surfaceLock;
    };

    Display::DisplayMap Display::_displays;
    Core::CriticalSection Display::_displaysMapLock;

    static uint32_t WidthFromResolution ( WPEFramework::Exchange::IComposition::ScreenResolution const );
    static uint32_t HeightFromResolution ( WPEFramework::Exchange::IComposition::ScreenResolution const );

} // RPI
} // WPEFramework



    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Prime ()
        : _fd { InvalidFd () }
        , _sync_fd { InvalidFd () }
        , _modifier { InvalidModifier () }
        , _format { InvalidFormat () }
        , _stride { InvalidStride () }
        , _height { InvalidHeight () }
        , _width { InvalidWidth () } {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Prime ( Prime::fd_t const & fd , Prime::fd_t const & sync_fd , Prime::modifier_t const & modifier , Prime::format_t const & format , Prime::stride_t const & stride , Prime::height_t const & height , Prime::width_t const & width )
        : _fd { fd }
        , _sync_fd { sync_fd }
        , _modifier { modifier }
        , _format { format }
        , _stride { stride }
        , _height { height }
        , _width { width } {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::~Prime () {
        /* bool */ Unlock ();
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Prime ( Prime && other ) {
        if ( this != & other ) {
            _fd = other . _fd;
            _sync_fd = other . _sync_fd;

            _modifier = other . _modifier;
            _format = other . _format;
            _stride = other . _stride;

            _height = other . _height;
            _width = other . _width;

            other . _fd = InvalidFd ();
            other . _sync_fd = InvalidFd ();

            other . _modifier = InvalidModifier ();
            other . _format = InvalidFormat ();
            other . _stride = InvalidStride ();

            other . _height = InvalidHeight ();
            other . _width = InvalidWidth ();
        }
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::prime_t & WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::operator= ( Prime && other ) {
        if ( this != & other ) {
            _fd = other . _fd;
            _sync_fd = other . _sync_fd;

            _modifier = other . _modifier;
            _format = other . _format;
            _stride = other . _stride;

            _height = other . _height;
            _width = other . _width;

            other . _fd = InvalidFd ();
            other . _sync_fd = InvalidFd ();

            other . _modifier = InvalidModifier ();
            other . _format = InvalidFormat ();
            other . _stride = InvalidStride ();

            other . _height = InvalidHeight ();
            other . _width = InvalidWidth ();
        }

        return * this;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::operator== ( Prime const & other ) const {
        bool ret =    _fd == other . _fd
                   && _sync_fd == other . _sync_fd
                   && _width == other . _width
                   && _height == other . _height
                   && _stride == other . _stride
                   && _format == other . _format
                   && _modifier == other . _modifier;
        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::operator< ( Prime const & other ) const {
        bool ret =  ! ( * this == other );
        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::operator() ( Prime const & left , Prime const & right ) const {
        bool ret = left < right;
        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Lock () {
        auto init = [] () -> struct flock {
            struct flock fl;

            /* void * */ memset ( & fl , 0 , sizeof ( fl ) );

            fl . l_type = F_WRLCK;
            fl . l_whence = SEEK_SET;
            fl . l_start = 0;
            fl . l_len = 0;

            return fl;
        };

        static struct flock fl = init ();

        bool ret =    _sync_fd != InvalidFd ()
                   && fcntl ( _sync_fd , F_SETLKW ,  & fl ) != -1
                   ;

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Unlock () {
        auto init = [] () -> struct flock {
            struct flock fl;

            /* void * */ memset ( & fl , 0 , sizeof ( fl ) );

            fl . l_type = F_UNLCK;
            fl . l_whence = SEEK_SET;
            fl . l_start = 0;
            fl . l_len = 0;

            return fl;
        };

        static struct flock fl = init ();

        bool ret =    _sync_fd != InvalidFd ()
                   && fcntl ( _sync_fd , F_SETLK , & fl ) != -1
                   ;

        return ret;
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::valid_t WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::Valid () const {
        valid_t ret =    _fd != InvalidFd ()
                      && _sync_fd != InvalidFd ()
                      && _modifier != InvalidModifier ()
                      && _format != InvalidFormat ()
                      && _stride != InvalidStride ()
                      && _height != InvalidHeight ()
                      && _width != InvalidWidth ();

        return ret;
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Native ( surf_t surf , width_t width , height_t height)
        :  _surf { surf }
        , _width { width }
        , _height { height } {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Native ( prime_t && prime , surf_t surf , width_t width , height_t height )
        : _prime { std::move ( prime ) }
        , _surf { surf }
        , _width { width }
        , _height { height } {
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Native ( Native && other ) {
        if ( this != & other ) {
            _prime = std::move ( other . _prime );

            _surf  = other . _surf;
            _width = other . _width;
            _height = other . _height;

            other . _surf = InvalidSurface ();
            other . _width = InvalidWidth ();
            other . _height = InvalidHeight ();
        }
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t & WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::operator= ( Native && other ) {
        if ( this != & other ) {
            _prime = std::move ( other . _prime );

            _surf  = other . _surf;
            _width = other . _width;
            _height = other . _height;

            other . _surf = InvalidSurface ();
            other . _width = InvalidWidth ();
            other . _height = InvalidHeight ();
        }

        return * this;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::operator == ( Native const & other ) const {
        bool ret =    _surf == other . _surf
                   && _width == other . _width
                   && _height == other . _height
                   && _prime == other . _prime;

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::operator < ( Native const & other ) const {
        bool ret = ! ( * this == other );
        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::operator () ( Native const & left , Native const & right ) const {
        bool ret = left < right;
        return ret;
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::valid_t  WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Valid () {
        valid_t ret =    _prime . Valid ()
                      && _surf != InvalidSurface ()
                      && _width != InvalidWidth ()
                      && _height != InvalidHeight ();

        return ret;
    }

// Copied from previous macros
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



    WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Sync::Sync ( Sync::dpy_t & dpy ) : _dpy { dpy } {
        static bool supported = EGL::Supported ( _dpy , "EGL_KHR_fence_sync" ) != false ;

        assert ( supported != false );
        assert ( dpy != InvalidDpy () );

        _sync = ( supported && _dpy != InvalidDpy () ) != false ? KHRFIX ( eglCreateSync ) ( _dpy , _EGL_SYNC_FENCE , nullptr ) : InvalidSync ();
    }

    WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Sync::~Sync () {
        if ( _sync == InvalidSync () ) {
            // Error : unable to create sync object
            glFinish ();
        }
        else {
            glFlush ();

            EGLint val = static_cast < EGLint > ( KHRFIX ( eglClientWaitSync ) ( _dpy , _sync , _EGL_SYNC_FLUSH_COMMANDS_BIT , _EGL_FOREVER ) );

            if ( val == static_cast < EGLint > ( EGL_FALSE ) || val != static_cast < EGLint > ( _EGL_CONDITION_SATISFIED ) ) {
                EGLAttrib status;

                bool ret = KHRFIX ( eglGetSyncAttrib ) ( _dpy , _sync , _EGL_SYNC_STATUS , & status ) != EGL_FALSE;

                ret =    ret
                      && ( status == _EGL_SIGNALED )
                      ;

                // Assert on error
                if ( ret != true ) {
                    TRACE ( Trace::Error , ( _T ( "EGL: synchronization primitive" ) ) );
                    assert ( false );
                }
            }

            /* EGLBoolean */ val = static_cast < EGLint > ( KHRFIX ( eglDestroySync ) ( _dpy , _sync ) );

            if ( val != EGL_TRUE ) {
                // Error
            }

            // Consume the (possible) error
            /* ELGint */ glGetError ();
            /* ELGint */ eglGetError ();
        }
    }


    /* static */ WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::img_t WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::CreateImage ( dpy_t const & dpy , ctx_t const & ctx , win_t const win , Native::prime_t const & prime ) {
        img_t ret = InvalidImg ();

        if (    dpy != InvalidDpy ()
             && ctx != InvalidCtx ()
             && win != InvalidImg ()
             && (    Supported ( dpy , "EGL_KHR_image" )
                  && Supported ( dpy , "EGL_KHR_image_base" )
                  && Supported ( dpy , "EGL_EXT_image_dma_buf_import" )
                  && Supported ( dpy , "EGL_EXT_image_dma_buf_import_modifiers" )
                ) != false
            ) {

            constexpr char const methodName [] = XSTRINGIFY ( KHRFIX ( eglCreateImage ) );

            // Probably never fires
            static_assert ( (    std::is_same < dpy_t , EGLDisplay > :: value
                              && std::is_same < ctx_t , EGLContext > :: value
                              && std::is_same < img_t , KHRFIX ( EGLImage ) > :: value
                            ) != false
                          );

            static KHRFIX ( EGLImage ) ( * peglCreateImage ) ( EGLDisplay , EGLContext , EGLenum , EGLClientBuffer , EGLAttrib const * ) = reinterpret_cast < KHRFIX ( EGLImage ) ( * ) ( EGLDisplay , EGLContext , EGLenum , EGLClientBuffer , EGLAttrib const * ) > ( eglGetProcAddress ( methodName ) );

            if ( peglCreateImage != nullptr ) {
                using width_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::width_t;
                using height_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::height_t;
                using stride_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::stride_t;
                using format_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::format_t;
                using modifier_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::modifier_t;
                using fd_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::Native::Prime::fd_t;

                // Disable / enable narrowing detection
// TODO:
                constexpr bool const enable = false;

                // (Almost) all will fail!
                if (    narrowing < width_t , EGLAttrib , enable > :: value != false
                     || narrowing < height_t , EGLAttrib , enable > :: value != false
                     || narrowing < stride_t , EGLAttrib , enable > :: value != false
                     || narrowing < format_t , EGLAttrib , enable > :: value != false
                     || narrowing < modifier_t , EGLAttrib , enable > :: value != false
                     || narrowing < fd_t , EGLAttrib , enable > :: value != false
                   ) {
                    TRACE_WITHOUT_THIS ( Trace::Information , ( _T ( "Possible narrowing detected!" ) ) );
                }


                // EGL may report differently than GBM / DRM
                // Platform formats are cross referenced against prime settings at construction time


                // Query EGL
                static EGLBoolean ( * peglQueryDmaBufFormatsEXT ) ( EGLDisplay , EGLint , EGLint * , EGLint * ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , EGLint , EGLint * , EGLint * ) > ( eglGetProcAddress ( "eglQueryDmaBufFormatsEXT" ) );
                static EGLBoolean ( * peglQueryDmaBufModifiersEXT ) ( EGLDisplay , EGLint , EGLint , EGLuint64KHR * , EGLBoolean * , EGLint * ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , EGLint , EGLint , EGLuint64KHR * , EGLBoolean * , EGLint * ) > ( eglGetProcAddress ( "eglQueryDmaBufModifiersEXT" ) );

                EGLint count = 0;

                bool valid =    peglQueryDmaBufFormatsEXT != nullptr
                             && peglQueryDmaBufModifiersEXT != nullptr
                             && peglQueryDmaBufFormatsEXT ( dpy , 0 , nullptr , & count ) != EGL_FALSE;

                EGLint formats [ count ];

                valid =    valid
                        && peglQueryDmaBufFormatsEXT ( dpy , count , & formats [ 0 ] , & count ) != EGL_FALSE;


                // Format should be listed as supported
                if ( valid != false ) {
                    std::list < EGLint > list_e_format ( & formats [ 0 ] , & formats [ count ] );

                    auto it_e_format = std::find ( list_e_format . begin () , list_e_format . end () , prime . Format () );

                    valid = it_e_format != list_e_format . end ();
                }


                valid =    valid
                        && peglQueryDmaBufModifiersEXT ( dpy , prime . Format () , 0 , nullptr , nullptr , & count ) != EGL_FALSE;


                EGLuint64KHR modifiers [ count ];
                EGLBoolean external [ count ];


                // External is required to exclusive use withGL_TEXTURE_EXTERNAL_OES
                valid =    valid
                        && peglQueryDmaBufModifiersEXT ( dpy , prime .Format () , count , & modifiers [ 0 ] , & external [ 0 ] , & count ) != EGL_FALSE;


                // Modifier should be listed as supported, and _external should be true

                if ( valid != false ) {
                    std::list < EGLuint64KHR > list_e_modifier ( & modifiers [ 0 ] , & modifiers [ count ] );

                    static_assert ( narrowing < std::remove_reference < decltype ( prime . Modifier () ) > :: type , EGLuint64KHR , true > :: value != true );

                    auto it_e_modifier = std::find ( list_e_modifier . begin () , list_e_modifier . end () , static_cast < EGLuint64KHR > ( prime . Modifier () ) );

                    valid = it_e_modifier != list_e_modifier . end ();

                    // For the compositor not relevant, only relevant for the client
                    if ( valid != false ) {
                        valid = external [ std::distance ( list_e_modifier . begin () , it_e_modifier ) ] != true;
                    }
                }


                if ( valid != false ) {

                    static_assert ( std::is_integral < EGLAttrib > :: value != false );
                    static_assert ( std::is_signed < EGLAttrib > :: value != false );

                    static_assert ( in_signed_range < EGLAttrib , EGL_WIDTH > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_HEIGHT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_LINUX_DRM_FOURCC_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_FD_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_OFFSET_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_PITCH_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT > :: value != false );
                    static_assert ( in_signed_range < EGLAttrib , EGL_NONE > :: value != false );

                    // 64 bit hence the shift 32 and mask 0xFFFFFFF, each half equals 32 bitsF
                    static_assert ( sizeof ( EGLuint64KHR ) == static_cast < size_t > ( 8 ) );

                    EGLAttrib const attrs [] = {
                        EGL_WIDTH , static_cast < EGLAttrib > ( prime . Width ()  ) ,
                        EGL_HEIGHT , static_cast < EGLAttrib > ( prime . Height () ) ,
                        EGL_LINUX_DRM_FOURCC_EXT , static_cast < EGLAttrib > ( prime . Format () ) ,
                        EGL_DMA_BUF_PLANE0_FD_EXT , static_cast < EGLAttrib > ( prime . Fd () ) ,
// TODO: magic constant
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT , 0 ,
                        EGL_DMA_BUF_PLANE0_PITCH_EXT , static_cast < EGLAttrib > ( prime . Stride () ) ,
                        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT , static_cast < EGLAttrib > ( static_cast < EGLuint64KHR > ( prime . Modifier () ) & 0xFFFFFFFF ) ,
                        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT , static_cast < EGLAttrib > ( static_cast < EGLuint64KHR > ( prime . Modifier () ) >> 32 ) ,
//                        EGL_IMAGE_PRESERVED_KHR , EGL_TRUE,
                        EGL_NONE
                    };

                    static_assert ( std::is_convertible < decltype ( nullptr ) , EGLClientBuffer > :: value != false );
                    ret = peglCreateImage ( dpy , EGL_NO_CONTEXT , EGL_LINUX_DMA_BUF_EXT , nullptr , attrs );
                }
            }
            else {
                // Error
                TRACE_WITHOUT_THIS ( Trace::Error , _T ( "%s is unavailable or invalid parameters." ) , methodName );
            }
        }
        else {
            TRACE_WITHOUT_THIS ( Trace::Error , _T ( "EGL is not properly initialized." ) );
        }

        return ret;
    }

    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::DestroyImage ( img_t & img , dpy_t const & dpy , ctx_t const & ctx ) {
        bool ret = img != InvalidImg ();

        if (    dpy != InvalidDpy ()
             && ctx != InvalidCtx ()
             && (   Supported ( dpy , "EGL_KHR_image" )
                  && Supported ( dpy , "EGL_KHR_image_base" )
                ) != false
           ) {

            static_assert ( (    std::is_same < dpy_t , EGLDisplay > :: value
                              && std::is_same < ctx_t , EGLContext > :: value
                              && std::is_same < img_t , KHRFIX ( EGLImage ) > :: value
                            ) != false
                          );

            constexpr char methodName [] = XSTRINGIFY ( KHRFIX ( eglDestroyImage ) );

            static EGLBoolean ( * peglDestroyImage ) ( EGLDisplay , KHRFIX ( EGLImage ) ) = reinterpret_cast < EGLBoolean ( * ) ( EGLDisplay , KHRFIX ( EGLImage ) ) > ( eglGetProcAddress ( KHRFIX ( "eglDestroyImage" ) ) );

            if ( peglDestroyImage != nullptr ) {
                ret = peglDestroyImage ( dpy , img ) != EGL_FALSE ? true : false;
                img = InvalidImg ();
            }
            else {
                // Error
                TRACE_WITHOUT_THIS ( Trace::Error , _T ( "%s is unavailable or invalid parameters are provided." ) , methodName );
            }

        }
        else {
            TRACE_WITHOUT_THIS ( Trace::Error , ( _T ( "EGL is not properly initialized." ) ) );
        }

        return ret;
    }

    // Although compile / build time may succeed, runtime checks are also mandatory
    /* static */ bool WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::Supported ( dpy_t const dpy , std::string const & name ) {
        bool ret = false;

        static_assert ( ( std::is_same < dpy_t , EGLDisplay > :: value ) != false );

#ifdef EGL_VERSION_1_5
        // KHR extentions that have become part of the standard

        // Sync capability
        ret =    name . find ( "EGL_KHR_fence_sync" ) != std::string::npos
                 /* CreateImage / DestroyImage */
              || name . find ( "EGL_KHR_image" ) != std::string::npos
              || name . find ( "EGL_KHR_image_base" ) != std::string::npos
              ;
#endif

        if ( ret != true ) {
            static_assert ( std::is_same < std::string::value_type , char > :: value != false );
            char const * ext = eglQueryString ( dpy , EGL_EXTENSIONS );

            ret =    ext != nullptr
                  && name .size () > 0
                  && ( std::string ( ext ) . find ( name ) != std::string::npos );
        }

        return ret;
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



    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::ImageAsTarget ( EGL::img_t const & img , EGL::width_t const width , EGL::height_t const height , tex_t & tex , fbo_t & fbo ) {
        bool ret =    glGetError () == GL_NO_ERROR
                   && img != EGL::InvalidImg ()
                   && width > 0
                   && height > 0
                   ;

        static_assert ( std::is_integral < EGL::width_t > :: value != false );
        static_assert ( std::is_integral < EGL::height_t > :: value != false );

        // Always
        static_assert ( std::is_integral < decltype ( GL_TEXTURE_2D ) > :: value != false );
        // Possibly signed to unsigned
        static_assert (    narrowing < decltype ( GL_TEXTURE_2D ) , GLuint , true > :: value != true 
                        || (    GL_TEXTURE_2D >= static_cast < decltype ( GL_TEXTURE_2D ) > ( 0 )
                             && in_unsigned_range < GLuint , GL_TEXTURE_2D > :: value != false
                           )
                      );
        constexpr GLuint const tgt = GL_TEXTURE_2D;

        if ( ret != false ) {
            // Just an arbitrary selected unit
            glActiveTexture ( GL_TEXTURE0 );
            ret = glGetError () == GL_NO_ERROR;
        }

        if ( ret != false ) {
            if ( tex != InvalidTex () ) {
                glDeleteTextures ( 1 , & tex );
                ret = glGetError () == GL_NO_ERROR;
            }

            tex = InvalidTex ();

            if ( ret != false ) {
                glGenTextures ( 1 , & tex );
                ret = glGetError () == GL_NO_ERROR;
            }

            if ( ret != false ) {
                glBindTexture ( tgt , tex );
                ret = glGetError () == GL_NO_ERROR;
            }

            if ( ret != false ) {
                glTexParameteri ( tgt , GL_TEXTURE_WRAP_S , GL_CLAMP_TO_EDGE );
                ret = glGetError () == GL_NO_ERROR;
            }

            if ( ret != false ) {
                glTexParameteri ( tgt , GL_TEXTURE_WRAP_T , GL_CLAMP_TO_EDGE );
                ret = glGetError () == GL_NO_ERROR;
            }

            if ( ret != false ) {
                glTexParameteri ( tgt , GL_TEXTURE_MIN_FILTER , GL_LINEAR );
                ret = glGetError () == GL_NO_ERROR;
            }

            if ( ret != false ) {
                glTexParameteri ( tgt , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
                ret = glGetError () == GL_NO_ERROR;
            }


            // A valid GL context should exist for GLES::Supported ()
            EGL::ctx_t ctx = eglGetCurrentContext ();
            EGL::dpy_t dpy = ctx != EGL::InvalidCtx () ? eglGetCurrentDisplay () : EGL::InvalidDpy ();


            ret =    ret
                  && eglGetError () == EGL_SUCCESS
                  && ctx != EGL::InvalidCtx ();

            if (    ret
                 && (    Supported ( "GL_OES_EGL_image" )
                      && (   EGL::Supported ( dpy , "EGL_KHR_image" )
                          || EGL::Supported ( dpy , "EGL_KHR_image_base" )
                         )
                    ) != false
               ) {


                // Take storage for the texture from the EGLImage; Pixel data becomes undefined
                static void ( * pEGLImageTargetTexture2DOES ) ( GLenum , GLeglImageOES ) = reinterpret_cast < void ( * ) ( GLenum , GLeglImageOES ) > ( eglGetProcAddress ( "glEGLImageTargetTexture2DOES" ) );

                if (    ret != false
                     && pEGLImageTargetTexture2DOES != nullptr ) {
                    // Logical const
                    using no_const_img_t = remove_const < decltype ( img ) > :: type;
                    pEGLImageTargetTexture2DOES ( tgt , reinterpret_cast < GLeglImageOES > ( const_cast < no_const_img_t > ( img ) ) );
                    ret = glGetError () == GL_NO_ERROR;
                }
                else {
                    ret = false;
                }
            }

            if ( ret != false ) {
                if ( fbo != InvalidFbo () ) {
                    glDeleteFramebuffers ( 1 , & fbo );
                    ret = glGetError () == GL_NO_ERROR;
                }

                if ( ret != false ) {
                    glGenFramebuffers ( 1 , & fbo );
                    ret = glGetError () == GL_NO_ERROR;
                }

                if ( ret != false ) {
                    glBindFramebuffer ( GL_FRAMEBUFFER , fbo );
                    ret = glGetError () == GL_NO_ERROR;
                }

                if ( ret != false ) {
                    // Bind the created texture as one of the buffers of the frame buffer object
                    glFramebufferTexture2D ( GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , tgt , tex , 0 /* level */ );
                    ret = glGetError () == GL_NO_ERROR;
                }

                if ( ret != false ) {
                    ret = glCheckFramebufferStatus ( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE;
                }

                if ( ret != true ) {
                    glDeleteFramebuffers ( 1 , & fbo );
                    glDeleteTextures ( 1 , & tex );

                    fbo = InvalidFbo ();
                    tex = InvalidTex ();
                }
            }
        }

        return ret;
    }

    bool WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::Supported ( std::string const & name ) {
        bool ret = false;

        using string_t = std::string::value_type;
        using ustring_t = std::make_unsigned < string_t > :: type;

        // Identical underlying types except for signedness
        static_assert ( std::is_same < ustring_t , GLubyte > :: value != false );

        string_t const * ext = reinterpret_cast < string_t const * > ( glGetString ( GL_EXTENSIONS ) );

        ret =    ext != nullptr
              && name . size () > 0
              && ( std::string ( ext ) . find ( name ) != std::string::npos );

        return ret;
    }

    /* static */ WPEFramework::RPI_INTERNAL::GLResourceMediator & WPEFramework::RPI_INTERNAL::GLResourceMediator::Instance () {
        static GLResourceMediator instance;
        return instance;
    }



    /* static */ WPEFramework::Core::NodeId WPEFramework::RPI::Connector () {
        string connector;
        if (    ( Core::SystemInfo::GetEnvironment ( _T ( "COMPOSITOR" ) , connector ) == false )
             || ( connector.empty () == true)
           ) {
            connector = _T ( "/tmp/compositor" );
        }

        return ( Core::NodeId ( connector . c_str () ) );
    }



    WPEFramework::RPI::Display::~Display () {
        if ( _dma != nullptr ) {
            delete _dma;
        }

        decltype ( _nativeDisplay . _fd ) & fd = _nativeDisplay . _fd;
        decltype ( _nativeDisplay . _device ) & device = _nativeDisplay . _device;

        if ( device != RenderDevice::GBM::InvalidDev () ) {
            gbm_device_destroy ( device );
        }

        if ( fd != RenderDevice::GBM::InvalidFd () ) {
            close ( fd );
        }

        _dma = nullptr;
        device = RenderDevice::GBM::InvalidDev ();
        fd = RenderDevice::GBM::InvalidFd ();

        Deinitialize();
    }

    /* static */ WPEFramework::RPI::Display * WPEFramework::RPI::Display::Instance ( string const & displayName , Exchange::IComposition::IDisplay * display ) {
        Display * result ( nullptr );

        _displaysMapLock . Lock ();

        DisplayMap::iterator index ( _displays . find ( displayName ) );

        if ( index == _displays . end () ) {
            result = new Display ( displayName , display );
            if (    result != nullptr
                 && result -> RemoteDisplay () != nullptr
               ) {

                _displays . insert ( std::pair < const std::string , Display * > ( displayName , result ) );

                // If successful inserted
                result -> AddRef ();
            } else {
                delete result;
                result = nullptr;
            }
        } else {
            result = index -> second;
            result -> AddRef ();
        }
        _displaysMapLock . Unlock ();

        return result;
    }

    void WPEFramework::RPI::Display::AddRef () const {
        Core::InterlockedIncrement ( _refCount );
    }

    uint32_t WPEFramework::RPI::Display::Release () const {
        if ( Core::InterlockedDecrement ( _refCount ) == 0 ) {
            _displaysMapLock . Lock ();

            DisplayMap::iterator display = _displays . find ( _displayName );

            if ( display != _displays . end () ){
                _displays . erase ( display );
            }

            _displaysMapLock . Unlock ();

            delete this;

            // Posibly signed to unsiged
            static_assert ( std::is_enum < decltype ( Core::ERROR_CONNECTION_CLOSED ) > :: value != false );
            static_assert (    narrowing < std::underlying_type < decltype ( Core::ERROR_CONNECTION_CLOSED ) > :: type , uint32_t , true > :: value != true
                            || (    Core::ERROR_CONNECTION_CLOSED >= static_cast < std::underlying_type < decltype ( Core::ERROR_CONNECTION_CLOSED ) > :: type > ( 0 )
                                 && in_unsigned_range < uint32_t , Core::ERROR_CONNECTION_CLOSED > :: value != false
                               )
                          );

            return static_cast < uint32_t > ( Core::ERROR_CONNECTION_CLOSED );
        }

        // Posibly signed to unsiged
        static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
        static_assert (    narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , uint32_t , true > :: value != true
                        || (    Core::ERROR_NONE >= static_cast < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type > ( 0 )
                             && in_unsigned_range < uint32_t , Core::ERROR_NONE > :: value != false
                           )
                      );

        return static_cast < uint32_t > ( Core::ERROR_NONE );
    }

    EGLNativeDisplayType WPEFramework::RPI::Display::Native () const {
        static_assert ( (std::is_convertible < decltype ( _nativeDisplay . _device ) , EGLNativeDisplayType > :: value ) != false );
        return static_cast < EGLNativeDisplayType > ( _nativeDisplay . _device );
    }

    const std::string & WPEFramework::RPI::Display::Name () const {
        return ( _displayName );
    }

    int WPEFramework::RPI::Display::Process ( uint32_t const ) {
        // Signed and at least 45 bits
        using milli_t = int32_t;

        auto RefreshRateFromResolution = [] ( Exchange::IComposition::ScreenResolution const resolution ) -> milli_t {
            // Assume 'unknown' rate equals 60 Hz
            milli_t rate = 60;

            switch (resolution) {
                case Exchange::IComposition::ScreenResolution_1080p24Hz : // 1920x1080 progressive @ 24 Hz
                                                                          rate = 24; break;
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

        constexpr milli_t milli = 1000;

        static decltype ( milli ) rate = RefreshRateFromResolution ( ( _remoteDisplay != nullptr ? _remoteDisplay -> Resolution () : Exchange::IComposition::ScreenResolution_Unknown ) );
        static std::chrono::milliseconds delay = std::chrono::milliseconds ( milli / rate);

        // Delay the (free running) loop
        auto current_time = std::chrono::steady_clock::now ();

        static decltype ( current_time ) last_access_time = current_time;

        auto duration = std::chrono::duration_cast < std::chrono::milliseconds > ( current_time - last_access_time );

        if ( duration . count () < delay . count () ) {
            // skip frame, the client is creating frames at a too high rate
            std::this_thread::sleep_for ( std::chrono::milliseconds ( delay - duration ) );
        }

        // Scan out all surfaces belonging to this display
// TODO: only scan out the surface that actually has completed or the client should be made aware that all surfaces should be completed at the time of calling
// TODO: This flow introduces artefacts
        last_access_time = current_time;

        for ( auto begin = _surfaces . begin () , it = begin , end = _surfaces .end ( ) ; it !=  end ; it++ ) {
            // THe way the API has been constructed limits the optimal syncing strategy

            ( * it ) -> PreScanOut ();  // will render

            // At least fails the very first time
            bool ret = ( * it ) -> SyncPrimitiveEnd ();
            silence ( ret );
//            assert ( ret != false );

            ( * it ) -> ScanOut (); // actual scan out (at the remote site)

            ret = ( * it ) -> SyncPrimitiveStart ();
//            assert ( ret != false );

            ( * it ) -> PostScanOut (); // rendered
        }

        return ( 0 );
    }

    int WPEFramework::RPI::Display::FileDescriptor () const {
        return -1;
    }

    WPEFramework::Compositor::IDisplay::ISurface * WPEFramework::RPI::Display::SurfaceByName ( std::string const & name ) {
        IDisplay::ISurface * result = nullptr;

        _adminLock . Lock ();

        std::list < SurfaceImplementation * > :: iterator index ( _surfaces . begin () );
        while ( ( index != _surfaces . end () ) && ( ( * index ) -> Name () != name) ) { index ++ ; }

        if (index != _surfaces . end( )) {
            result = * index;
            result -> AddRef ();
        }

        _adminLock . Unlock ();

        return result;
    }

    WPEFramework::Compositor::IDisplay::ISurface * WPEFramework::RPI::Display::Create ( std::string const & name , uint32_t const width , uint32_t const height ) {
        uint32_t realHeight = height;
        uint32_t realWidth = width;

        if ( _remoteDisplay != nullptr ) {
            Exchange::IComposition::ScreenResolution resolution = _remoteDisplay -> Resolution ();

            realHeight = HeightFromResolution ( resolution );
            realWidth = WidthFromResolution ( resolution );

            if (    realWidth != width
                 || realHeight != height
               ) {
                static_assert ( narrowing < decltype ( width ) , int , true > :: value != true );
                static_assert ( narrowing < decltype ( height ) , int , true > :: value != true );
                static_assert ( narrowing < decltype ( realWidth ) , int , true > :: value != true );
                static_assert ( narrowing < decltype ( realHeight ) , int , true > :: value != true );

                TRACE ( Trace::Information , ( _T ( "Requested surface dimensions (%d x %d) differ from true (real) display dimensions (%d x %d). Continuing with the latter!" ) , width , height , realWidth , realHeight ) );
            }
        }
        else {
            TRACE ( Trace::Information , ( _T ( "No remote display exist. Unable to query its dimensions. Expect the unexpected!" ) ) );
        }

        Core::ProxyType < SurfaceImplementation > retval = ( Core::ProxyType < SurfaceImplementation > :: Create ( * this , name , realWidth , realHeight ) );
        Compositor::IDisplay::ISurface * result = & ( * retval );
        result -> AddRef ();

        return result;
    }



    bool WPEFramework::RPI::Display::PrimeFor ( Exchange::IComposition::IClient const & client , WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t & prime ) {
        std::lock_guard < decltype ( _surfaceLock ) > const lock ( _surfaceLock );

        bool ret = false;

        // This does not update our prime since values are const by reference
        DMATransfer::fds_t handles = { prime . Fd () , prime . SyncFd () };

        if ( _dma != nullptr ) {
            std::string msg = client . Name ();

            // Announce ourself to indicate the data exchange is a request for (file descriptor) data
            // Then, after receiving the data DMA is no longer needed for this client
            ret =    _dma -> Connect ()
                  && _dma -> Send ( msg , handles )
                  && _dma -> Receive ( msg , handles )
                  && _dma -> Disconnect ();

            // <prefix>:width:height:stride:format
            // Search from end to begin

            using width_t    = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::width_t;
            using height_t   = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::height_t;
            using stride_t   = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::stride_t;
            using format_t   = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::format_t;
            using modifier_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::modifier_t;

            using prime_t    = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t;
            using fd_t       = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t::fd_t;

            using common_t   = std::common_type < width_t , height_t , stride_t , format_t , modifier_t > :: type;

            auto str2prop = [ & msg ] ( std::string::size_type & pos ) -> common_t {
                constexpr char spacer [] = ":";

                std::string::size_type start = msg . rfind ( spacer , pos );

                static_assert ( narrowing  < unsigned long , common_t , true > :: value != true );
                common_t ret = ( ( start != std::string::npos && start <= msg . length () ) != false ) ? std::stoul ( msg . substr ( start + 1 , pos ) ) : 0;

                // Narrowing detection
                constexpr bool enable = false;

                if ( narrowing < decltype ( ret ) , uint32_t , enable > :: value != false ) {
                    TRACE_WITHOUT_THIS ( Trace::Information , ( _T ( "Possible narrowing detected!" ) ) );
                }

                decltype ( ERANGE ) err = errno;

                if ( err == ERANGE ) {
                    TRACE_WITHOUT_THIS ( Trace::Error , ( _T ( "Unbale to determine property value." ) ) );
                }

                pos = start - 1;

                return ret;
            };

            std::string::size_type pos = msg . size () - 1;


// TODO: signed vs unsigned , commmon_type  for return type str2prop
            prime_t tmp_prime (
                  static_cast < fd_t > ( handles [ 0 ] )
                , static_cast < fd_t > ( handles [ 1 ] )
                , static_cast < modifier_t > (str2prop ( pos ) )
                , static_cast < format_t > (str2prop ( pos ) )
                , static_cast < stride_t > (str2prop ( pos ) )
                , static_cast < height_t > (str2prop ( pos ) )
                , static_cast < width_t > ( str2prop ( pos ) )
            );

            prime = std::move ( tmp_prime );

            static_assert ( narrowing < std::remove_reference < decltype ( prime . Width () ) > :: type  , int , true > :: value != true );
            static_assert ( narrowing < std::remove_reference < decltype ( prime . Height () ) > :: type  , int , true > :: value != true );
            static_assert ( narrowing < std::remove_reference < decltype ( prime . Stride () ) > :: type  , int , true > :: value != true );
            static_assert ( narrowing < std::remove_reference < decltype ( prime . Format () ) > :: type  , int , true > :: value != true );
            static_assert ( narrowing < std::remove_reference < decltype ( prime . Modifier () ) > :: type  , int , true > :: value != true );

            TRACE_WITHOUT_THIS ( Trace::Information , _T ( "Received the following properties via DMA: width %d height %d stride %d format %d modifier %ld." ) , prime . Width () , prime . Height () , prime . Stride () , prime . Format () , prime . Modifier () );
        }

        if ( ret != true ) {
            TRACE ( Trace::Error , ( _T  ( "Unable to receive (DMA) data for %s." ) , client . Name () . c_str () ) );
        }

        return ret;
    }

    /* static */ void WPEFramework::RPI::Display::VirtualKeyboardCallback ( keyactiontype type , unsigned int code ) {
        if ( type != KEY_COMPLETED ) {
            time_t timestamp = time ( nullptr );
            const IDisplay::IKeyboard::state state = ( ( type == KEY_RELEASED ) ? IDisplay::IKeyboard::released
                                                                                : ( ( type == KEY_REPEAT ) ? IDisplay::IKeyboard::repeated
                                                                                                           : IDisplay::IKeyboard::pressed
                                                                                  )
                                                     );

            InputFunction action = [ = ] ( SurfaceImplementation * s ) {
                s -> SendKey ( code , state , timestamp ) ;
            };

            Publish ( action );
        }
    }

    /* static */ void WPEFramework::RPI::Display::VirtualMouseCallback ( mouseactiontype type , unsigned short button , signed short horizontal , signed short vertical ) {
        static int32_t pointer_x = 0;
        static int32_t pointer_y = 0;

        time_t timestamp = time ( nullptr );
        InputFunction action;
        pointer_x = pointer_x + horizontal;
        pointer_y = pointer_y + vertical;

        switch ( type ) {
            case MOUSE_MOTION   :
                            action = [ = ] ( SurfaceImplementation * s ) {
                                int32_t X = std::min ( std::max ( 0 , pointer_x ) , s -> Width () );
                                int32_t Y = std::min ( std::max ( 0 , pointer_y ) , s -> Height () );
                                s -> SendPointerPosition ( X , Y , timestamp ) ;
                            };
                            break;
            case MOUSE_SCROLL   :
                            action = [ = ] ( SurfaceImplementation * s ) {
                                s -> SendWheelMotion ( horizontal , vertical , timestamp );
                            };
                            break;
            case MOUSE_RELEASED :
            case MOUSE_PRESSED  :
                            action = [ = ] ( SurfaceImplementation * s ) {
                                s -> SendPointerButton ( button , type == MOUSE_RELEASED ? IDisplay::IPointer::released
                                                                                         : IDisplay::IPointer::pressed
                                                                , timestamp 
                                                       );
                            };
                break;
            default : assert ( false);
        }

        Publish ( action );
    }

    /* static */ void WPEFramework::RPI::Display::VirtualTouchScreenCallback ( touchactiontype type , unsigned short index , unsigned short x , unsigned short y ) {
        static uint16_t touch_x = ~0;
        static uint16_t touch_y = ~0;
        static touchactiontype last_type = TOUCH_RELEASED;

        // Get touch position in pixels
        // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
        if (      ( x != touch_x )
               || ( y != touch_y )
               || ( type != last_type )
           ) {

            last_type = type;
            touch_x = x;
            touch_y = y;

            time_t timestamp = time ( nullptr );
            const IDisplay::ITouchPanel::state state = ( ( type == TOUCH_RELEASED ) ? ITouchPanel::released
                                                                                    : ( ( type == TOUCH_PRESSED ) ? ITouchPanel::pressed
                                                                                                                  : ITouchPanel::motion
                                                                                      )
                                                       );

            InputFunction action = [ = ] ( SurfaceImplementation * s ) {
                const uint16_t mapped_x = ( s->Width () * x ) >> 16;
                const uint16_t mapped_y = ( s->Height () * y ) >> 16;
                s -> SendTouch ( index , state , mapped_x , mapped_y , timestamp );
            };

            Publish ( action );
        }
    }



    WPEFramework::RPI::Display::DMATransfer::DMATransfer ()
        : _transfer { InvalidSocket () }
        , _addr { AF_UNIX , "/tmp/Compositor/DMA" }
        , _valid { Initialize () } {
    }

    WPEFramework::RPI::Display::DMATransfer::~DMATransfer () {
        /* valid_t */ Deinitialize ();
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Valid () const {
        return _valid;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Receive ( std::string & msg , DMATransfer::fds_t & fds ) {
        valid_t ret = Valid ();

        if ( ret != true ) {
            TRACE ( Trace::Information , ( _T ( "Unable to receive (DMA) data." ) ) );
        }
        else {
            ret = Receive ( msg , fds . data () , fds . size () );
        }

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Send ( std::string const & msg , fds_t const & fds ) {
        valid_t ret = Valid ();

        if ( ret != true ) {
            TRACE ( Trace::Information , ( _T ( "Unable to send (DMA) data." ) ) );
        }
        else {
            ret = Send ( msg , fds . data () , fds . size () );
        }

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Connect () {
        valid_t ret = false;

        timeout_t timeout = Core::infinite;

        ret = Connect ( timeout );

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Disconnect () {
        valid_t ret = false;

        timeout_t timeout = Core::infinite;

        ret = Disconnect ( timeout );

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Initialize () {
        valid_t ret = false;

        _transfer = socket ( _addr . sun_family /* domain */ , SOCK_STREAM /* type */ , 0 /* protocol */ );

        ret = _transfer != InvalidSocket ();

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Deinitialize () {
        valid_t ret = false;

        ret = Disconnect ();

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Connect ( timeout_t const timeout ) {
        silence ( timeout );

        valid_t ret = _transfer != InvalidSocket ();

        ret =    ret
              && connect ( _transfer , reinterpret_cast < struct sockaddr const * > ( & _addr ) , sizeof ( _addr ) ) == 0;

        decltype ( EISCONN ) err = errno;

        // Already connected is not an error
        ret = ret != false || err == EISCONN;

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Disconnect ( timeout_t const timeout ) {
        silence ( timeout );

        valid_t ret = _transfer != InvalidSocket ();

        ret =    ret
              && close ( _transfer ) == 0;

        _transfer = InvalidSocket ();

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Send ( std::string const & msg , fd_t const * fd , uint8_t count ) {
        valid_t ret = false;

        // Logical const
        static_assert ( ( std::is_same < char * , remove_const  < decltype ( & msg  [ 0 ] ) > :: type > :: value ) != false );
        char * buf  = const_cast < char * > ( & msg [ 0 ] );

        size_t const bufsize = msg .size ();

        if ( bufsize > 0 ) {

            // Scatter array for vector I/O
            struct iovec iov;

            // Starting address
            iov . iov_base = reinterpret_cast < void * > ( buf );
            // Number of bytes to transfer
            iov . iov_len = bufsize;

            // Actual message
            struct msghdr msgh {};

            // Optional address
            msgh . msg_name = nullptr;
            // Size of address
            msgh . msg_namelen = 0;
            // Scatter array
            msgh . msg_iov = & iov;
            // Elements in msg_iov
            msgh . msg_iovlen = 1;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control [ CMSG_SPACE ( sizeof ( fd_t ) * count ) ];

            // Only valid file descriptor (s) can be sent via extra payload
            ret = true;
            for ( decltype ( count ) i = 0 ; i < count && fd != nullptr ; i++ ) {
                ret = fd [ i ] != InvalidFd () && ret;
            }

            if ( ret != false ) {
                // Contruct ancillary data to be added to the transfer via the control message

                // Ancillary data, pointer
                msgh . msg_control = control;

                // Ancillery data buffer length
                msgh . msg_controllen = sizeof ( control );

                // Ancillary data should be access via cmsg macros
                // https://linux.die.net/man/2/recvmsg
                // https://linux.die.net/man/3/cmsg
                // https://linux.die.net/man/2/setsockopt
                // https://www.man7.org/linux/man-pages/man7/unix.7.html

                // Control message

                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                struct cmsghdr * cmsgh = CMSG_FIRSTHDR ( & msgh );

                if ( cmsgh != nullptr ) {
                    // Originating protocol
                    // To manipulate options at the sockets API level
                    cmsgh -> cmsg_level = SOL_SOCKET;

                    // Protocol specific type
                    // Option at the API level, send or receive a set of open file descriptors from another process
                    cmsgh -> cmsg_type = SCM_RIGHTS;

                    // The value to store in the cmsg_len member of the cmsghdr structure, taking into account any necessary alignmen, eg byte count of control message including header
                    cmsgh -> cmsg_len = CMSG_LEN ( sizeof ( fd_t ) * count );

                    // Initialize the payload
                    /* void */ memcpy ( CMSG_DATA ( cmsgh ) , fd , sizeof ( fd_t ) * count );

                    ret = true;
                }
                else {
                    // Error
                }
            }
            else {
                // No extra payload, ie  file descriptor(s), to include
                msgh . msg_control = nullptr;
                msgh . msg_controllen = 0;

                ret = true;
            }

            if ( ret != false ) {
                // Configuration succeeded

                // https://linux.die.net/man/2/sendmsg
                // https://linux.die.net/man/2/write
                // Zero flags is equivalent to write

                ssize_t size = -1;
                socklen_t len = sizeof ( size );

                // Only send data if the buffer is large enough to contain all data
                if ( getsockopt ( _transfer , SOL_SOCKET, SO_SNDBUF , & size , & len ) == 0 ) {
                    // Most options use type int, ssize_t was just a placeholder
                    static_assert ( narrowing < decltype ( size ) , int , true > :: value != true );
                    TRACE ( Trace::Information , ( _T ( "The sending buffer capacity equals %d bytes." ) , static_cast < int > ( size ) ) );

// TODO: do not send if the sending buffer is too small
                    size = sendmsg ( _transfer , & msgh , 0 );
                }
                else {
                    // Unable to determine buffer capacity
                }

                ret = size != -1;

                if ( ret != false ) {
                    static_assert ( narrowing < decltype ( size ) , int , true > :: value != true );
                    static_assert ( narrowing < decltype ( msgh . msg_iov -> iov_len + msgh . msg_controllen ) , int , true > :: value != true );
                    TRACE ( Trace::Information , ( _T ( "Send %d bytes out of %d." ) , size , msgh . msg_iov -> iov_len /* just a single element */ + msgh . msg_controllen ) );
                }
                else {
                    TRACE ( Trace::Error , ( _T ( "Failed to send data." ) ) );
                }
            }

        }
        else {
            TRACE ( Trace::Error , ( _T ( "A data message to be send cannot be empty!" ) ) );
        }

        return ret;
    }

    WPEFramework::RPI::Display::DMATransfer::valid_t WPEFramework::RPI::Display::DMATransfer::Receive ( std::string & msg , fd_t * fd , uint8_t const count ) {
        valid_t ret = false;

        msg . clear ();

        ssize_t size = -1;
        socklen_t len = sizeof ( size );

        static_assert ( narrowing < decltype ( len ) , decltype ( size ) , true > :: value != true );

        if ( getsockopt ( _transfer , SOL_SOCKET , SO_RCVBUF , & size , & len ) == 0 ) {
            static_assert ( narrowing < decltype ( size ) , int , true > :: value != true );

            TRACE ( Trace::Information , ( _T ( "The receiving buffer maximum capacity equals %d [bytes]." ) , size ) );

            // Most options use type int, ssize_t was just a placeholder
            static_assert ( narrowing < int , decltype ( size ) , true > :: value != true );
            msg . reserve ( static_cast < int > ( size ) );
        }
        else {
            // Unable to determine buffer capacity
            static_assert ( narrowing < decltype ( msg . capacity () ) , int , true > :: value != true );
            TRACE ( Trace::Information , ( _T ( "Unable to determine buffer maximum cpacity. Using %d [bytes] instead." ) , msg . capacity () ) );
        }

        static_assert ( narrowing < decltype ( msg . capacity () ) , size_t , true > :: value != true );
        size_t const bufsize = msg . capacity ();

        if (    bufsize > 0
             && count > 0
             && fd != nullptr
           ) {

            for ( remove_const < decltype ( count ) > :: type i = 0 ; i < count ; i++ ) {
                fd [ i ] = InvalidFd ();
            }

            static_assert ( ( std::is_same < char * , remove_const  < decltype ( & msg [ 0 ] ) > :: type > :: value ) != false );
            char buf [ bufsize ];

            // Scatter array for vector I/O
            struct iovec iov;

            // Starting address
            iov . iov_base = reinterpret_cast < void * > ( & buf [ 0 ] ) ;
            // Number of bytes to transfer
            iov . iov_len = bufsize;

            // Actual message
            struct msghdr msgh {};

            // Optional address
            msgh . msg_name = nullptr;
            // Size of address
            msgh . msg_namelen = 0;
            // Elements in msg_iov
            msgh  .msg_iovlen = 1;
            // Scatter array
            msgh . msg_iov = & iov;

            // Ancillary data
            // The macro returns the number of bytes an ancillary element with payload of the passed in data length, eg size of ancillary data to be sent
            char control [ CMSG_SPACE ( sizeof ( fd_t ) * count ) ];

            // Ancillary data, pointer
            msgh . msg_control = control;

            // Ancillary data buffer length
            msgh . msg_controllen = sizeof ( control );

            // No flags set
// TODO: do not receive if the receiving buffer is too small
            size = recvmsg ( _transfer , & msgh , 0 );

            ret = size > 0;

            switch ( size ) {
                case -1 :   // Error
                            {
                                // TRACE ( Trace::Error , ( _T ( "Error receiving remote (DMA) data. " ) ) );
                                break;
                            }
                case 0  :   // Peer has done an orderly shutdown, before any data was received
                            {
                                // TRACE ( Trace::Error , ( _T ( "Error receiving remote (DMA) data. Client may have become unavailable." ) ) );
                                break;
                            }
                default :   // Data
                            {
                                // Extract the file descriptor information
                                static_assert ( narrowing < decltype ( size ) , int , true > :: value != true );
                                TRACE ( Trace::Information , ( _T ( "Received %d bytes." ) , size ) );

                                // Pointer to the first cmsghdr in the ancillary data buffer associated with the passed msgh
                                struct cmsghdr * cmsgh = CMSG_FIRSTHDR ( & msgh );

                                // Check for the expected properties the client should have set
                                if (    cmsgh != nullptr
                                     && cmsgh -> cmsg_level == SOL_SOCKET
                                     && cmsgh -> cmsg_type == SCM_RIGHTS
                                ) {

                                    // The macro returns a pointer to the data portion of a cmsghdr.
                                    /* void */ memcpy ( fd , CMSG_DATA ( cmsgh ) , sizeof ( fd_t ) * count );
                                }
                                else {
                                    TRACE ( Trace::Information , ( _T ( "No (valid) ancillary data received." ) ) );
                                }

                                msg . assign ( buf , size );
                            }
            }
        }
        else {
            // Error
            TRACE ( Trace::Error , ( _T  ( "A receiving data buffer (message) cannot be empty!" ) ) );
        }

        return ret;
    }

    WPEFramework::RPI::Display::SurfaceImplementation::SurfaceImplementation ( Display & display , std::string const & name , uint32_t const width , uint32_t const height )
        : _display { display }
        , _keyboard { nullptr }
        , _wheel { nullptr }
        , _pointer { nullptr }
        , _touchpanel { nullptr }
        , _remoteClient { nullptr }
        , _remoteRenderer { nullptr }
        , _nativeSurface { WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidSurface () , WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidWidth () , WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidHeight () } {

        _display . AddRef ();

// TODO: gbm_surface_Create_with_modifiers
        WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::surf_t surf = gbm_surface_create ( _display . Native () , width , height , RenderDevice::SupportedBufferType () , GBM_BO_USE_RENDERING /* used for rendering */ );

        if ( surf == WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidSurface () ) {
            TRACE_L1  ( _T ( "Failed to create a native (underlying) surface" ) );
        }
        else {
            _remoteClient = _display . CreateRemoteSurface ( name , width , height );
        }

        if ( _remoteClient != nullptr ) {
            _remoteRenderer = _remoteClient -> QueryInterface < Exchange::IComposition::IRender > ();

            if ( _remoteRenderer == nullptr ) {
                TRACE_L1(_T ( "Could not aquire remote renderer for surface %s." ) , name . c_str () );
            }
            else {

                WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t prime;

                if (    _display . PrimeFor ( * _remoteClient , prime ) != true
                     || gbm_device_is_format_supported ( static_cast < RenderDevice::GBM::dev_t > ( _display . Native () ) , prime . Format () , GBM_BO_USE_RENDERING ) != 1
                   ) {

                    TRACE ( Trace::Error , ( _T ( "Failed to setup a share for rendering results!" ) ) );

                    auto result = _remoteRenderer -> Release ();
                    ASSERT ( result == Core::ERROR_DESTRUCTION_SUCCEEDED );
                    _remoteRenderer = nullptr;

                    /* auto */ result = _remoteClient -> Release ();
                    ASSERT ( result == Core::ERROR_DESTRUCTION_SUCCEEDED );
                    _remoteClient = nullptr;
                }
                else {
                    assert ( gbm_device_get_format_modifier_plane_count ( static_cast < RenderDevice::GBM::dev_t > ( _display . Native () ) , prime . Format () , prime . Modifier () ) == 1 );

                    WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t native ( std::move ( prime ), surf , width, height );

                    _nativeSurface = std::move ( native );
                }

            }
        }
        else {
            TRACE_L1(_T ( "Could not create remote surface for surface %s." ) , name . c_str () );
        }

        _display . Register ( this );
    }

    WPEFramework::RPI::Display::SurfaceImplementation::~SurfaceImplementation () {

        if ( _keyboard != nullptr ) {
            _keyboard -> Release ();
        }

        if ( _wheel != nullptr ) {
            _wheel -> Release ();
        }

        if ( _pointer != nullptr ) {
            _pointer -> Release ();
        }

        if ( _touchpanel != nullptr ) {
            _touchpanel -> Release ();
        }

        if ( _remoteClient != nullptr ) {

            TRACE_L1(_T ( "Destructing surface %s" ) , _remoteClient -> Name() . c_str () );

            _display . RemoteDisplay () -> InvalidateClient ( _remoteClient -> Name () );

            if ( _remoteRenderer != nullptr ) {
                auto result = _remoteRenderer -> Release ();
                silence ( result );
//                ASSERT ( result == Core::ERROR_DESTRUCTION_SUCCEEDED );
                _remoteRenderer = nullptr;
            }

            auto result = _remoteClient -> Release ();
            silence ( result );
//            ASSERT ( result == Core::ERROR_DESTRUCTION_SUCCEEDED);
            _remoteClient = nullptr;

        }

        _display . Unregister ( this );

        if ( _nativeSurface . Surface () != WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidSurface () ) {
            gbm_surface_destroy ( _nativeSurface  . Surface () );
        }

        auto result = _display . Release ();
        silence ( result );
        ASSERT ( result = Core::ERROR_DESTRUCTION_SUCCEEDED );
    }

    EGLNativeWindowType WPEFramework::RPI::Display::SurfaceImplementation::Native () const {
        static_assert ( (std::is_convertible < WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::surf_t , EGLNativeWindowType > :: value ) != false );
        return static_cast < EGLNativeWindowType > ( _nativeSurface . Surface () );
    }

    std::string WPEFramework::RPI::Display::SurfaceImplementation::Name () const {
        return ( _remoteClient != nullptr ? _remoteClient -> Name() : string () );
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Keyboard (WPEFramework::Compositor::IDisplay::IKeyboard * keyboard ) {
        assert ( ( _keyboard == nullptr ) ^ ( keyboard == nullptr ) );
        _keyboard = keyboard;
        _keyboard -> AddRef ();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Pointer ( WPEFramework::Compositor::IDisplay::IPointer * pointer ) {
        assert ( ( _pointer == nullptr ) ^ ( pointer == nullptr ) );
        _pointer = pointer;
        _pointer -> AddRef ();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::Wheel ( WPEFramework::Compositor::IDisplay::IWheel * wheel ) {
        assert ( ( _wheel == nullptr ) ^ ( wheel == nullptr ) );
        _wheel = wheel;
        _wheel -> AddRef ();
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::TouchPanel ( WPEFramework::Compositor::IDisplay::ITouchPanel * touchpanel ) {
        assert ( ( _touchpanel == nullptr ) ^ ( touchpanel == nullptr ) );
        _touchpanel = touchpanel;
        _touchpanel -> AddRef ();
    }

    int32_t WPEFramework::RPI::Display::SurfaceImplementation::Width () const {
        using width_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::width_t;

        static_assert ( narrowing < width_t , int32_t, true > :: value != true );

        return (    _nativeSurface . Surface () != WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidSurface ()
                 && _nativeSurface . Width () <= static_cast < width_t > ( std::numeric_limits < int32_t > :: max () ) ? _nativeSurface . Width () : 0 );
    }

    int32_t WPEFramework::RPI::Display::SurfaceImplementation::Height () const {
        using height_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::height_t;

        static_assert ( narrowing < height_t , int32_t, true > :: value != true );

        return (    _nativeSurface . Surface () != WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::InvalidSurface ()
                 && _nativeSurface . Height () <= static_cast < height_t > ( std::numeric_limits < int32_t > :: max ()) ? _nativeSurface . Height () : 0 );
    }


    void WPEFramework::RPI::Display::SurfaceImplementation::SendKey ( uint32_t const key , IKeyboard::state const action , uint32_t const ) {
        if ( _keyboard != nullptr ) {
            _keyboard -> Direct ( key , action );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendWheelMotion ( int16_t const x , int16_t const y , uint32_t const ) {
        if ( _wheel != nullptr ) {
            _wheel -> Direct( x , y );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendPointerButton ( uint8_t const button , IPointer::state const state , uint32_t const ) {
        if ( _pointer != nullptr ) {
            _pointer -> Direct ( button , state );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendPointerPosition ( int16_t const x , int16_t const y , uint32_t const ) {
        if ( _pointer != nullptr ) {
            _pointer -> Direct ( x , y );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::SendTouch ( uint8_t const index , ITouchPanel::state const state , uint16_t const x , uint16_t const y , uint32_t const ) {
        if ( _touchpanel != nullptr ) {
            _touchpanel -> Direct ( index , state , x , y );
        }
    }


    void WPEFramework::RPI::Display::SurfaceImplementation::ScanOut () {
        // Changes of currents cannot be reliably be monitored
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t dpy = eglGetCurrentDisplay ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t ctx = eglGetCurrentContext ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t surf = eglGetCurrentSurface ( EGL_DRAW );

        bool status =    _nativeSurface . Valid () 
                      && dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                      && ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                      && surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ();

        // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
        if (    status != false
             && _remoteRenderer != nullptr
             && _remoteClient != nullptr
           ) {
            _remoteRenderer -> ScanOut ();
        }
        else {
            TRACE ( Trace::Error , ( _T ( "Remote scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?" ) ) );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::PreScanOut () {
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t width = 0;
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t height = 0;


        // Changes of currents cannot be reliably be monitored
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t dpy = eglGetCurrentDisplay ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t ctx = eglGetCurrentContext ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t surf = eglGetCurrentSurface ( EGL_DRAW );

        bool status =    _nativeSurface . Valid ()
                      && dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                      && ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                      && surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ()
                      && eglQuerySurface ( dpy , surf , EGL_WIDTH , & width ) != EGL_FALSE
                      && eglQuerySurface ( dpy , surf , EGL_HEIGHT , & height ) != EGL_FALSE;

        if ( status != false ) {
// TODO: Destroy
            static WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::img_t img = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::CreateImage ( dpy , ctx , _nativeSurface . Surface () , _nativeSurface . Prime () );

// TODO: Destroy
            static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::tex_t tex = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidTex ();
// TODO: Destroy
            static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::fbo_t fbo = WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFbo ();

            static WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES gles;

            static_assert (    narrowing < WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::width_t , WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t , true > :: value != true
                            && narrowing < WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::height_t , WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t , true > :: value != true
                          );

            using n_width_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::width_t;
            using e_width_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::width_t;

            using n_height_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::height_t;
            using e_height_t = WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::height_t;


// TODO:
            constexpr bool const enable = false;

            if ( (    narrowing < n_width_t, e_width_t , enable > :: value != false
                   || narrowing < n_height_t, e_height_t , enable > :: value != false
                 ) != false ) {
                TRACE_WITHOUT_THIS ( Trace::Information , ( _T ( "Possible narrowing detected!" ) ) );
            }


            using common_width_t = std::common_type < n_width_t , e_width_t > :: type;
            using common_height_t = std::common_type < n_height_t , e_height_t > :: type;


            bool status =    img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImg ()
                          && static_cast < common_width_t > ( width ) == static_cast < common_width_t > ( _nativeSurface . Width () )
                          && static_cast < common_height_t > ( height ) == static_cast < common_height_t > ( _nativeSurface . Height () )
                          && gles . ImageAsTarget ( img , width , height , tex , fbo )
                          ;

            status = (    status
                       && img != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidImg ()
                       && fbo != WPEFramework::RPI_INTERNAL::GLResourceMediator::GLES::InvalidFbo ()
                     ) != false;
        }

        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::sync_t sync ( dpy );

        // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
        if (    status != false
             && _remoteRenderer != nullptr
             && _remoteClient != nullptr
           ) {
            _remoteRenderer -> PreScanOut ();
        }
        else {
            TRACE ( Trace::Error , ( _T ( "Remote pre scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?" ) ) );
        }
    }

    void WPEFramework::RPI::Display::SurfaceImplementation::PostScanOut () {
        // Changes of currents cannot be reliably be monitored
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::dpy_t dpy = eglGetCurrentDisplay ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::ctx_t ctx = eglGetCurrentContext ();
        WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::surf_t surf = eglGetCurrentSurface ( EGL_DRAW );

        bool status =    _nativeSurface . Valid ()
                      && dpy != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidDpy ()
                      && ctx != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidCtx ()
                      && surf != WPEFramework::RPI_INTERNAL::GLResourceMediator::EGL::InvalidSurf ()
                      ;

        // A remote ClientSurface has been created and the IRender interface is supported so the compositor is able to support scan out for this client
        if (    status != false
             && _remoteRenderer != nullptr
             && _remoteClient != nullptr
           ) {
            _remoteRenderer -> PostScanOut ();
        }
        else {
            TRACE ( Trace::Error , ( _T ( "Remote post scan out is not (yet) supported. Has a remote surface been created? Is the IRender interface available?" ) ) );
        }
    }

    bool WPEFramework::RPI::Display::SurfaceImplementation::SyncPrimitiveStart () {
        bool ret = const_cast < WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t & > ( _nativeSurface . Prime () ) . Lock ();
        return ret;
    }

    bool WPEFramework::RPI::Display::SurfaceImplementation::SyncPrimitiveEnd () {
        bool ret = const_cast < WPEFramework::RPI_INTERNAL::GLResourceMediator::native_t::prime_t & > ( _nativeSurface . Prime () ) . Unlock ();
        return ret;
    }



    WPEFramework::RPI::Display::Display ( std::string const & displayName , WPEFramework::Exchange::IComposition::IDisplay * display )
        : _displayName { displayName }
        , _adminLock ()
        , _virtualinput { nullptr }
        , _surfaces ()
        , _compositerServerRPCConnection ()
        , _refCount { 0 }
        , _remoteDisplay { nullptr }
        , _nativeDisplay { RenderDevice::GBM::InvalidDev () , RenderDevice::GBM::InvalidFd () }
        , _dma { nullptr } {

        Initialize ( display );

        std::vector < std::string > nodes;

        RenderDevice::GetNodes ( static_cast < uint32_t > ( DRM_NODE_RENDER ) , nodes );

        for ( auto begin = nodes . begin () , it = begin , end = nodes . end () ; it != end ; it++ ) {
            decltype ( _nativeDisplay . _fd ) & fd = _nativeDisplay . _fd;
            decltype ( _nativeDisplay . _device ) & device = _nativeDisplay . _device;

            fd = open ( it -> c_str () , O_RDWR );

// TODO : invalidFd

            if ( fd != RenderDevice::GBM::InvalidFd () ) {
                device = gbm_create_device ( fd );

                if ( device != RenderDevice::GBM::InvalidDev () ) {
                    TRACE ( Trace::Information , ( _T ( "Opened RenderDevice: %s" ) , it -> c_str () ) );

                    _dma = new DMATransfer ();

                    if (    _dma == nullptr
                         || _dma -> Valid () != true
                       ) {

                        TRACE ( Trace::Error , ( _T ( "DMA transfers are not supported." ) ) );

                        delete _dma;
                        _dma = nullptr;
                    }
                    else {
                        break;
                    }
                }

                /* int */ close ( fd );

                fd = RenderDevice::GBM::InvalidFd ();
            }
        }
    }

    WPEFramework::Exchange::IComposition::IClient * WPEFramework::RPI::Display::CreateRemoteSurface ( std::string const & name , uint32_t const width , uint32_t const height ) {
        return ( _remoteDisplay != nullptr ? _remoteDisplay -> CreateClient ( name , width , height ) : nullptr );
    }

    void WPEFramework::RPI::Display::Register ( Display::SurfaceImplementation * surface ) {
        ASSERT ( surface != nullptr );

        _adminLock . Lock ();

        std::list < SurfaceImplementation * > :: iterator index ( std::find ( _surfaces . begin () , _surfaces . end () , surface ) );

        if ( index == _surfaces . end () ) {
            _surfaces . push_back ( surface );
        }

        _adminLock . Unlock ();
    }

    void WPEFramework::RPI::Display::Unregister ( Display::SurfaceImplementation * surface ) {
        ASSERT ( surface != nullptr );

        _adminLock . Lock ();

        auto index ( std::find ( _surfaces . begin () , _surfaces . end () , surface ) );

        ASSERT ( index != _surfaces . end () );

        if ( index != _surfaces . end () ) {
            _surfaces . erase ( index );
        }

        _adminLock . Unlock ();
    }

    /* static */ void WPEFramework::RPI::Display::Publish ( InputFunction & action ) {
        if ( action != nullptr ) {
            _displaysMapLock . Lock ();

            for ( std::pair < string const , Display * > & entry : _displays ) {
                entry . second -> _adminLock . Lock ();

                std::for_each ( begin ( entry . second -> _surfaces ) , end (entry . second -> _surfaces ) , action );

                entry . second -> _adminLock . Unlock ();
            }

            _displaysMapLock . Unlock ();
        }
    }

    void WPEFramework::RPI::Display::Initialize ( WPEFramework::Exchange::IComposition::IDisplay * display ) {
        if ( WPEFramework::Core::WorkerPool::IsAvailable () == true ) {
            // If we are in the same process space as where a WorkerPool is registered (Main Process or
            // hosting process) use, it!
            Core::ProxyType < RPC::InvokeServer > engine = Core::ProxyType < RPC::InvokeServer > :: Create ( & Core::WorkerPool::Instance () );

            _compositerServerRPCConnection = Core::ProxyType < RPC::CommunicatorClient > :: Create ( Connector () , Core::ProxyType < Core::IIPCServer > ( engine ) );

            engine -> Announcements ( _compositerServerRPCConnection -> Announcement () );
        }
        else {
            // Seems we are not in a process space initiated from the Main framework process or its hosting process.
            // Nothing more to do than to create a workerpool for RPC our selves !
            Core::ProxyType < RPC::InvokeServerType < 2 , 0 , 8 > > engine = Core::ProxyType < RPC::InvokeServerType < 2 , 0 , 8 > > :: Create ();

            _compositerServerRPCConnection = Core::ProxyType < RPC::CommunicatorClient > :: Create ( Connector () , Core::ProxyType<Core::IIPCServer > ( engine ) );

            engine -> Announcements ( _compositerServerRPCConnection -> Announcement () );
        }

        if ( display != nullptr ) {
            _remoteDisplay = display;
            _remoteDisplay -> AddRef ();
        }
        else {
            // Connect to the CompositorServer..
            uint32_t result = _compositerServerRPCConnection -> Open ( RPC::CommunicationTimeOut );

            // Posibly signed to unsiged
            static_assert ( std::is_enum < decltype ( Core::ERROR_NONE ) > :: value != false );
            static_assert (    narrowing < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type , uint32_t , true > :: value != true
                            || (    Core::ERROR_NONE >= static_cast < std::underlying_type < decltype ( Core::ERROR_NONE ) > :: type > ( 0 )
                                 && in_unsigned_range < uint32_t , Core::ERROR_NONE > :: value != false
                               )
                         );

            if ( result != static_cast < decltype ( result ) > ( Core::ERROR_NONE ) ) {
                TRACE_L1 ( _T ( "Could not open connection to Compositor with node %s. Error: %s" ) , _compositerServerRPCConnection -> Source () . RemoteId () . c_str () , Core::NumberType < uint32_t > ( result ) . Text () . c_str () );
                _compositerServerRPCConnection . Release ();
            }
            else {
                _remoteDisplay = _compositerServerRPCConnection -> Aquire < Exchange::IComposition::IDisplay > ( 2000 , _displayName , ~0 );

                if ( _remoteDisplay == nullptr ) {
                    TRACE_L1 ( _T ( "Could not create remote display for Display %s!" ) , Name () . c_str () );
                }
            }
        }

        _virtualinput = virtualinput_open ( _displayName . c_str () , connectorNameVirtualInput , VirtualKeyboardCallback , VirtualMouseCallback , VirtualTouchScreenCallback );

        if ( _virtualinput == nullptr ) {
            TRACE_L1 ( _T ( "Initialization of virtual input failed for Display %s!" ) , Name () . c_str () );
        }
    }

    void WPEFramework::RPI::Display::Deinitialize () {
        if ( _virtualinput != nullptr ) {
            virtualinput_close ( _virtualinput );
            _virtualinput = nullptr;
        }

        std::list < SurfaceImplementation * > :: iterator index ( _surfaces . begin () );
        while ( index != _surfaces . end () ) {
            string name = ( * index ) -> Name ();

            if ( ( * index ) -> Release () != Core::ERROR_DESTRUCTION_SUCCEEDED ) { //note, need cast to prevent ambigious call
                TRACE_L1 ( _T (  "Compositor Surface [%s] is not properly destructed" ) , name . c_str () );
            }

            index = _surfaces . erase ( index );
        }

        if ( _remoteDisplay != nullptr ) {
            _remoteDisplay -> Release ();
            _remoteDisplay = nullptr;
        }

        if ( _compositerServerRPCConnection . IsValid () == true ) {
            _compositerServerRPCConnection -> Close ( RPC::CommunicationTimeOut );
            _compositerServerRPCConnection . Release ();
        }
    }

    WPEFramework::Exchange::IComposition::IDisplay const * WPEFramework::RPI::Display::RemoteDisplay () const {
        return _remoteDisplay;
    }

    WPEFramework::Exchange::IComposition::IDisplay * WPEFramework::RPI::Display::RemoteDisplay () {
        return _remoteDisplay;
    }



    /* static */ uint32_t WPEFramework::RPI::WidthFromResolution ( WPEFramework::Exchange::IComposition::ScreenResolution const resolution ) {
        // Asumme an invalid width equals 0
        uint32_t width = 0;

        switch ( resolution ) {
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
            case WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
            case WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                                                    width = 3840; break;
            case WPEFramework::Exchange::IComposition::ScreenResolution_480i      : // Unknown according to the standards (?)
            case WPEFramework::Exchange::IComposition::ScreenResolution_Unknown   :
            default                                                               : width = 0;
        }

        return width;
    }

    /* static */ uint32_t WPEFramework::RPI::HeightFromResolution ( WPEFramework::Exchange::IComposition::ScreenResolution const resolution ) {
        // Asumme an invalid height equals 0
        uint32_t height = 0;

        switch ( resolution ) {
            // For descriptions see WidthFromResolution
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
            case WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz : // 4K, 3840x2160 progressive @ 50 Hz
            case WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz : // 4K, 3840x2160 progressive @ 60 Hz
                                                                                    height = 2160; break;
            case WPEFramework::Exchange::IComposition::ScreenResolution_Unknown   :
            default                                                               : height = 0;
        }

        return height;
    }



    WPEFramework::Compositor::IDisplay * WPEFramework::Compositor::IDisplay::Instance ( string const & displayName , void * display ) {
        return WPEFramework::RPI::Display::Instance ( displayName , reinterpret_cast < WPEFramework::Exchange::IComposition::IDisplay * > ( display ) ) ;
    }

