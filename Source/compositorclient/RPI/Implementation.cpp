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

#include "Module.h"

#ifdef VC6
#define __GBM__
#endif

#include <EGL/egl.h>

#ifdef VC6
#include "ModeSet.h"
#else
#include <EGL/eglext.h>
#include <bcm_host.h>
#endif

#include <algorithm>

#include <core/core.h>
#include <com/com.h>
#include <interfaces/IComposition.h>
#include <virtualinput/virtualinput.h>
#include "../Client.h"

static const char* connectorNameVirtualInput = "/tmp/keyhandler";

namespace {

#ifdef VC6

using namespace WPEFramework;

class Platform {
private:
    class Surface : public ModeSet::ICallback {
    public:
        Surface() = delete;
        Surface(const Surface&) = delete;
        Surface& operator= (const Surface&) = delete;
     
        Surface(ModeSet& modeSet, const EGLSurface surface) 
            : _modeSet(modeSet)
            , _surface(reinterpret_cast<struct gbm_surface*>(surface))
            , _id(~0)
            , _counter(0) { 
        }
        ~Surface()  {
            if (_id != static_cast<uint32_t>(~0)) {       
                _modeSet.DropSurfaceFromOutput(_id);
            }
        }

    public:
        void ScanOut() {
            // See if we have this surface attached to the FramBuffer (or whatever :-) 
            if ( (_id != static_cast<uint32_t>(~0)) || ((_id = _modeSet.AddSurfaceToOutput(_surface)) != static_cast<uint32_t>(~0)) ) {
                _modeSet.ScanOutRenderTarget (_surface, _id);
            }
        }

    private:
        void PageFlip(unsigned int frame, unsigned int sec, unsigned int usec) override {
            if (_counter == 0) {
                fprintf(stderr, "Succefully flipped the page.\n");
            }
            _counter = ((_counter + 1) & 0x7F);
        }
        void VBlank(unsigned int frame, unsigned int sec, unsigned int usec) override {
            // fprintf(stderr, "VBlank\n");
        }

    private:
	ModeSet& _modeSet;
        struct gbm_surface* _surface;
        uint32_t _id;
        uint32_t _counter;
    };

    using SurfaceMap = std::unordered_map<EGLSurface, Surface>;

public:
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    Platform(const string&) : _platform()
    {
    }
    ~Platform() = default;

public:
    EGLNativeDisplayType Display() const 
    {
        EGLNativeDisplayType result (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));

        const struct gbm_device* pointer = _platform.UnderlyingHandle();

        if(pointer != nullptr) {
            result = reinterpret_cast<EGLNativeDisplayType>(const_cast<struct gbm_device*>(pointer));
        }
        else {
            TRACE_L1(_T("The native display (id) might be invalid / unsupported. Using the EGL default display instead!"));
        }

        return (result);
    }
    uint32_t Width() const
    {
        return (_platform.Width());
    }
    uint32_t Height() const
    {
        return (_platform.Height());
    }
    EGLSurface CreateSurface (const EGLNativeDisplayType& display, const uint32_t width, const uint32_t height) 
    {
        // A Native surface that acts as a native window
        EGLSurface result = reinterpret_cast<EGLSurface>(_platform.CreateRenderTarget(width, height));

        if (result == 0) {
            TRACE_L1(_T("The native window (handle) might be invalid / unsupported. Expect undefined behavior!"));
        }
        else {
            _surfaces.emplace(std::piecewise_construct,
              std::forward_as_tuple(result),
              std::forward_as_tuple(_platform, result));
        }

        return (result);
    }
    void DestroySurface(const EGLSurface& surface) 
    {
        _platform.DestroyRenderTarget(reinterpret_cast<struct gbm_surface*>(surface));
    }
    void Opacity(const EGLSurface&, const uint8_t) 
    {
        TRACE_L1(_T("Currently not supported"));
    }
    void Geometry (const EGLSurface&, const Exchange::IComposition::Rectangle&) 
    {
        TRACE_L1(_T("Currently not supported"));
    }
    void ZOrder(const EGLSurface&, const uint16_t)
    {
        TRACE_L1(_T("Currently not supported"));
    }
    void ScanOut () {
        for (std::pair<EGLSurface const, Surface>& entry : _surfaces) {
            entry.second.ScanOut();
        }
    }
    int Descriptor() const {
        return (_platform.Descriptor());
    }

private:
    ModeSet _platform;
    SurfaceMap _surfaces;
};

#else

class Platform {
private:
    struct Surface {
        EGL_DISPMANX_WINDOW_T surface;
        VC_RECT_T rectangle;
        uint16_t layer;
        uint8_t opacity;
    };

public:
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    Platform(const string&)
    {
        bcm_host_init();
    }
    ~Platform()
    {
        bcm_host_deinit();
    }

public:
    EGLNativeDisplayType Display() const 
    {
        EGLNativeDisplayType result (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));

        return (result);
    }
    uint32_t Width() const
    {
        uint32_t width, height;
        graphics_get_display_size(0, &width, &height);
        return (width);
    }
    uint32_t Height() const
    {
        uint32_t width, height;
        graphics_get_display_size(0, &width, &height);
        return (height);
    }
    EGLSurface CreateSurface (const EGLNativeWindowType&, const uint32_t, const uint32_t) 
    {
        EGLSurface result;

        uint32_t displayWidth  = Width();
        uint32_t displayHeight = Height();
        Surface* surface = new Surface;

        VC_RECT_T srcRect;

        vc_dispmanx_rect_set(&(surface->rectangle), 0, 0, displayWidth, displayHeight);
        vc_dispmanx_rect_set(&srcRect, 0, 0, displayWidth << 16, displayHeight << 16);
        surface->layer = 0;
        surface->opacity = 255;

        VC_DISPMANX_ALPHA_T alpha = {
            static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_MIX),
            surface->opacity,
            255
        };

        DISPMANX_DISPLAY_HANDLE_T dispmanDisplay = vc_dispmanx_display_open(0);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate  = vc_dispmanx_update_start(0);
        DISPMANX_ELEMENT_HANDLE_T dispmanElement = vc_dispmanx_element_add(
            dispmanUpdate,
            dispmanDisplay,
            surface->layer, /* Z order layer, new one is always on top */
            &(surface->rectangle),
            0 /*src*/,
            &srcRect,
            DISPMANX_PROTECTION_NONE,
            &alpha /*alpha*/,
            0 /*clamp*/,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);

        surface->surface.element = dispmanElement;
        surface->surface.width   = displayWidth;
        surface->surface.height  = displayHeight;
        result                   = static_cast<EGLSurface>(surface);

        return (result);
    }

    void DestroySurface(const EGLSurface& surface) 
    {
        Surface* object = reinterpret_cast<Surface*>(surface);
        // DISPMANX_DISPLAY_HANDLE_T dispmanDisplay = vc_dispmanx_display_open(0);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate  = vc_dispmanx_update_start(0);
        vc_dispmanx_element_remove(dispmanUpdate, object->surface.element);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
        // vc_dispmanx_display_close(dispmanDisplay);
        delete object;
    }
    
    void Opacity(const EGLSurface& surface, const uint16_t opacity) 
    {
        VC_RECT_T srcRect;
        Surface* object = reinterpret_cast<Surface*>(surface);

        vc_dispmanx_rect_set(&srcRect, 0, 0, (Width() << 16), (Height() << 16));
        object->opacity = opacity;

        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        vc_dispmanx_element_change_attributes(dispmanUpdate,
            object->surface.element,
            (1 << 1),
            object->layer,
            object->opacity,
            &object->rectangle,
            &srcRect,
            0,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }

    void Geometry (const EGLSurface& surface, const WPEFramework::Exchange::IComposition::Rectangle& rectangle) 
    {
        VC_RECT_T srcRect;
        Surface* object = reinterpret_cast<Surface*>(surface);

        vc_dispmanx_rect_set(&srcRect, 0, 0, (Width() << 16), (Height() << 16));
        vc_dispmanx_rect_set(&(object->rectangle), rectangle.x, rectangle.y, rectangle.width, rectangle.height);

        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        vc_dispmanx_element_change_attributes(dispmanUpdate,
            object->surface.element,
            (1 << 2),
            object->layer,
            object->opacity,
            &object->rectangle,
            &srcRect,
            0,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }

    void ZOrder(const EGLSurface& surface, const uint16_t layer)
    {
        // RPI is unique: layer #0 actually means "deepest", so we need to convert.
        const uint16_t actualLayer = std::numeric_limits<uint16_t>::max() - layer;
        Surface* object = reinterpret_cast<Surface*>(surface);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        object->layer = layer;
        vc_dispmanx_element_change_layer(dispmanUpdate, object->surface.element, actualLayer);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }
    int Descriptor() const {
        return (-1);
    }
    void ScanOut () {
    }

};

#endif

}

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
    
    Display(const std::string& displayName);

    class EXTERNAL CompositorClient {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        CompositorClient(const CompositorClient& a_Copy) = delete;
        CompositorClient& operator=(const CompositorClient& a_RHS) = delete;

    public:
        CompositorClient(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        CompositorClient(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~CompositorClient() = default;

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        string _text;
    };

    class SurfaceImplementation : public Compositor::IDisplay::ISurface {
    private:
        class RemoteAccess : public Exchange::IComposition::IClient {
        public:
            RemoteAccess() = delete;
            RemoteAccess(const RemoteAccess&) = delete;
            RemoteAccess& operator= (const RemoteAccess&) = delete;

            RemoteAccess(EGLSurface& surface, const string& name, const uint32_t width, const uint32_t height)
                : _name(name)
                , _opacity(Exchange::IComposition::maxOpacity)
                , _layer(0)
                , _nativeSurface(surface)
                , _destination( { 0, 0, width, height } ) 
            {
            }
            ~RemoteAccess() override = default;

        public:
            inline const EGLSurface& Surface() const
            {
                return (_nativeSurface);
            }
            inline int32_t Width() const
            {
                return _destination.width;
            }
            inline int32_t Height() const
            {
                return _destination.height;
            }
            string Name() const override
            {
                return _name;
            }
            void Opacity(const uint32_t value) override;
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override;
            Exchange::IComposition::Rectangle Geometry() const override;
            uint32_t ZOrder(const uint16_t zorder) override;
            uint32_t ZOrder() const override;

            BEGIN_INTERFACE_MAP(RemoteAccess)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP
 
        private:
            const std::string _name;

            uint32_t _opacity;
            uint32_t _layer;

            EGLSurface _nativeSurface;

            Exchange::IComposition::Rectangle _destination;
        };

    public:
        SurfaceImplementation() = delete;
        SurfaceImplementation(const SurfaceImplementation&) = delete;
        SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        SurfaceImplementation(
            Display& compositor, const std::string& name,
            const uint32_t width, const uint32_t height);
        ~SurfaceImplementation() override;

    public:
        std::string Name() const override {
            return (_remoteAccess->Name());
        }
        inline EGLNativeWindowType Native() const
        {
            return (reinterpret_cast<EGLNativeWindowType>(_remoteAccess->Surface()));
        }
        inline int32_t Width() const
        {
            return (_remoteAccess->Width());
        }
        inline int32_t Height() const
        {
            return (_remoteAccess->Height());
        }
        inline void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
        {
            assert((_keyboard == nullptr) ^ (keyboard == nullptr));
            _keyboard = keyboard;
        }
        inline void Wheel(Compositor::IDisplay::IWheel* wheel) override
        {
            assert((_wheel == nullptr) ^ (wheel == nullptr));
            _wheel = wheel;
        }
        inline void Pointer(Compositor::IDisplay::IPointer* pointer) override
        {
            assert((_pointer == nullptr) ^ (pointer == nullptr));
            _pointer = pointer;
        }
        inline void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
        {
            assert((_touchpanel == nullptr) ^ (touchpanel == nullptr));
            _touchpanel = touchpanel;
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

    private:
        Display& _display;

        IKeyboard* _keyboard;
        IWheel* _wheel;
        IPointer* _pointer;
        ITouchPanel* _touchpanel;

        RemoteAccess* _remoteAccess;
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

    static Display& Instance(const string& displayName){
        Display* result(nullptr);

        _displaysMapLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName);
            _displays.insert(std::pair<const std::string, Display*>(displayName, result));
        } else {
            result = index->second;
        }
        result->AddRef();
        _displaysMapLock.Unlock();

        assert(result != nullptr);

        return (*result);
    } 

    void AddRef() const override
    {
        if (Core::InterlockedIncrement(_refCount) == 1) {
            const_cast<Display*>(this)->Initialize();
        }
        return;
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

            const_cast<Display*>(this)->Deinitialize();

            return (Core::ERROR_CONNECTION_CLOSED);
        }
        return (Core::ERROR_NONE);
    }
    EGLNativeDisplayType Native() const override
    {
        return (_platform.Display());
    }
    const std::string& Name() const final override
    {
        return (_displayName);
    }
    int Process(const uint32_t data) override;
    int FileDescriptor() const override;
    ISurface* SurfaceByName(const std::string& name) override;
    ISurface* Create(
        const std::string& name,
        const uint32_t width, const uint32_t height) override;

    inline uint32_t DisplaySizeWidth() const
    {
        return _platform.Width();
    }

    inline uint32_t DisplaySizeHeight() const
    {
        return _platform.Height();
    }

private:
    inline void Register(SurfaceImplementation* surface);
    inline void Unregister(SurfaceImplementation* surface);
    inline void OfferClientInterface(Exchange::IComposition::IClient* client);
    inline void RevokeClientInterface(Exchange::IComposition::IClient* client);

    inline static void Publish (InputFunction& action) {
        if (action != nullptr) {
            _displaysMapLock.Lock();
            for (std::pair<const string, Display*>& entry : _displays) {
                std::for_each(begin(entry.second->_surfaces), end(entry.second->_surfaces), action);
            }
            _displaysMapLock.Unlock();
        }
    }

    inline void Initialize()
    {
        _adminLock.Lock();

        if (Core::WorkerPool::IsAvailable() == true) {
            // If we are in the same process space as where a WorkerPool is registered (Main Process or
            // hosting ptocess) use, it!
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

        uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not open connection to Compositor with node %s. Error: %s"), _compositerServerRPCConnection->Source().RemoteId().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
            _compositerServerRPCConnection.Release();
        }

        _virtualinput = virtualinput_open(_displayName.c_str(), connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE_L1(_T("Initialization of virtual input failed for Display %s!"), Name().c_str());
        }

        _adminLock.Unlock();
    }

    inline void Deinitialize()
    {
        _adminLock.Lock();

        if (_virtualinput != nullptr) {
            virtualinput_close(_virtualinput);
        }

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            string name = (*index)->Name();

            if ((*index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) { //note, need cast to prevent ambigious call
                TRACE_L1(_T("Compositor Surface [%s] is not properly destructed"), name.c_str());
            }

            index = _surfaces.erase(index);
        }
        if (_compositerServerRPCConnection.IsValid() == true) {
            _compositerServerRPCConnection.Release();
        }

        _adminLock.Unlock();
    }

    static DisplayMap _displays; 
    static Core::CriticalSection _displaysMapLock;

    Platform _platform;
    std::string _displayName;
    mutable Core::CriticalSection _adminLock;
    void* _virtualinput;
    std::list<SurfaceImplementation*> _surfaces;
    Core::ProxyType<RPC::CommunicatorClient> _compositerServerRPCConnection;

    mutable uint32_t _refCount;
    std::map <std::string, IDisplay::ISurface*> _surfaceMap;
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
{
    uint32_t realWidth(width);
    uint32_t realHeight(height);

    _display.AddRef();

    // To support scanout the underlying buffer should be large enough to support the selected mode
    // A buffer of smaller dimensions than the display will fail. A larger one is possible but will
    // probably fail in the current setup. Currently, it is best to give both equal dimensions

    if ((width != _display.DisplaySizeWidth()) || (height != _display.DisplaySizeHeight())) {
        TRACE_L1(_T("Requested surface dimensions [%d, %d] might not be honered. Rendering might fail!"), width, height);

        // Truncating
        if (realWidth  != _display.DisplaySizeWidth())  { realWidth  = _display.DisplaySizeWidth();  }
        if (realHeight != _display.DisplaySizeHeight()) { realHeight = _display.DisplaySizeHeight(); }
    }

    EGLSurface nativeSurface = _display._platform.CreateSurface(_display.Native(), realWidth, realHeight);

    _display.Register(this);

    _remoteAccess = Core::Service<RemoteAccess>::Create<RemoteAccess>(nativeSurface, name, realWidth, realHeight);

    _display.OfferClientInterface(_remoteAccess);
}

Display::SurfaceImplementation::~SurfaceImplementation()
{
    TRACE_L1(_T("Destructing client named: %s"), _remoteAccess->Name().c_str());

    _display.Unregister(this);

    _display._platform.DestroySurface(_remoteAccess->Surface());

    _display.RevokeClientInterface(_remoteAccess);

    _remoteAccess->Release();

    _display.Release();
}

void Display::SurfaceImplementation::RemoteAccess::Opacity(
    const uint32_t value)
{

    _opacity = (value > Exchange::IComposition::maxOpacity) ? Exchange::IComposition::maxOpacity : value;

    // _display._platform.Opacity(_nativeSurface, _opacity);
}


uint32_t Display::SurfaceImplementation::RemoteAccess::Geometry(const Exchange::IComposition::Rectangle& rectangle)
{
    _destination = rectangle;

    // _display._platform.Geometry(_nativeSurface, _destination);
    return (Core::ERROR_NONE);
}

Exchange::IComposition::Rectangle Display::SurfaceImplementation::RemoteAccess::Geometry() const 
{
    return (_destination);
}

uint32_t Display::SurfaceImplementation::RemoteAccess::ZOrder(const uint16_t zorder)
{
    _layer = zorder;

    // _display._platform.ZOrder(_nativeSurface, _layer);

    return (Core::ERROR_NONE);
}

uint32_t Display::SurfaceImplementation::RemoteAccess::ZOrder() const {
    return (_layer);
}

Display::Display(const string& name)
    : _platform(name)
    , _displayName(name)
    , _adminLock()
    , _virtualinput(nullptr)
    , _compositerServerRPCConnection()
    , _refCount(0)
{
}

Display::~Display()
{
}

int Display::Process(const uint32_t data)
{
#ifdef VC6
    _platform.ScanOut ();
#endif

    return (0);
}

int Display::FileDescriptor() const
{
    return (_platform.Descriptor());
}

Compositor::IDisplay::ISurface* Display::SurfaceByName(const std::string& name)
{
    IDisplay::ISurface* _ret = nullptr;

    if (_surfaceMap.size () > 0) {
        auto _it =  _surfaceMap.find (name);

        if (_it != _surfaceMap.end ()) {
            _ret = _it->second;
        }
    }

    assert (_ret != nullptr);

    return _ret;
}

Compositor::IDisplay::ISurface* Display::Create(
    const std::string& name, const uint32_t width, const uint32_t height)
{
    Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, width, height));
    Compositor::IDisplay::ISurface* result = &(*retval);
    result->AddRef();

    __attribute__ ((unused)) bool _ret = _surfaceMap.insert (std::pair <std::string, IDisplay::ISurface*> (name, result)).second;

    assert (_ret != false);

    return result;
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

void Display::OfferClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    if (_compositerServerRPCConnection.IsValid()) {
        uint32_t result = _compositerServerRPCConnection->Offer(client);

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not offer IClient interface with callsign %s to Compositor. Error: %s"), client->Name().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
        }
    } else {
#if defined(COMPOSITORSERVERPLUGIN)
        SYSLOG(Logging::Fatal, (_T("The CompositorServer plugin is included in the build, but not able to reach!")));
        ASSERT(false && "The CompositorServer plugin is included in the build, but not able to reach!");
#endif
    }
}

void Display::RevokeClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    if (_compositerServerRPCConnection.IsValid()) {
        uint32_t result = _compositerServerRPCConnection->Revoke(client);

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not revoke IClient interface with callsign %s to Compositor. Error: %s"), client->Name().c_str(), Core::NumberType<uint32_t>(result).Text().c_str());
        }
    }else {
#if defined(COMPOSITORSERVERPLUGIN)
        SYSLOG(Logging::Fatal, (_T("The CompositorServer plugin is included in the build, but not able to reach!")));
        ASSERT(false && "The CompositorServer plugin is included in the build, but not able to reach!");
#endif
    }
}
} // RPI

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    return (&(RPI::Display::Instance(displayName)));
}
} // WPEFramework
