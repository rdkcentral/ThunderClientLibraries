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

#define __GBM__

#include <EGL/egl.h>


#include <algorithm>

#include <core/core.h>
#include <com/com.h>
#include <interfaces/IComposition.h>
#include <virtualinput/virtualinput.h>
#include "../Client.h"

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
    
    Display(const std::string& displayName);

    class SurfaceImplementation : public Compositor::IDisplay::ISurface {
    public:
        SurfaceImplementation() = delete;
        SurfaceImplementation(const SurfaceImplementation&) = delete;
        SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        SurfaceImplementation(
            Display& compositor, const std::string& name,
            const uint32_t width, const uint32_t height);
        ~SurfaceImplementation() override;

    public:
        EGLNativeWindowType Native() const override
        {
            return ( _remoteClient != nullptr ? reinterpret_cast<EGLNativeWindowType>(_remoteClient->Native()) : reinterpret_cast<EGLNativeWindowType>(-1)); // huppel, what is invalidf handle code?
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
            // huppel is this correct? Check...
            int32_t width = 0;
            if(_remoteClient != nullptr) {
                Exchange::IComposition::Rectangle rect = _remoteClient->Geometry(); 
                width = rect.width;
            }
            return width; 
        }
        int32_t Height() const override
        {
            // huppel is this correct? Check...
            int32_t width = 0;
            if(_remoteClient != nullptr) {
                Exchange::IComposition::Rectangle rect = _remoteClient->Geometry(); 
                width = rect.height;
            }
            return width; 
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

        inline void ScanOut() {
            if( _remoteRenderer != nullptr ) {
                _remoteRenderer->ScanOut();
            }
        }

    private:
        Display& _display;

        IKeyboard* _keyboard;
        IWheel* _wheel;
        IPointer* _pointer;
        ITouchPanel* _touchpanel;

        Exchange::IComposition::IClient* _remoteClient;
        Exchange::IComposition::IRender* _remoteRenderer;
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

    static Display* Instance(const string& displayName){
        Display* result(nullptr);

        _displaysMapLock.Lock();

        DisplayMap::iterator index(_displays.find(displayName));

        if (index == _displays.end()) {
            result = new Display(displayName);
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
        return (_remoteDisplay != nullptr ? reinterpret_cast<EGLNativeDisplayType>(_remoteDisplay->Native()) : static_cast<EGLNativeDisplayType>(0)); //huppel -1 or is tehre an error value?
    }
    const std::string& Name() const override
    {
        return (_displayName);
    }
    int Process(const uint32_t data) override;
    int FileDescriptor() const override;
    ISurface* SurfaceByName(const std::string& name) override;
    ISurface* Create( const std::string& name, const uint32_t width, const uint32_t height) override;

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

    inline void Initialize()
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
{
    _remoteClient = _display.CreateRemoteSurface(name, width, height);

    if (_remoteClient != nullptr) {
        _remoteRenderer = _remoteClient->QueryInterface<Exchange::IComposition::IRender>();

        if(_remoteRenderer == nullptr) {
            TRACE_L1(_T("Could not aquire remote renderer for surface %s."), name.c_str());
        }

    } else {
        TRACE_L1(_T("Could not create remote surface for surface %s."), name.c_str());
    }

    _display.AddRef();
    _display.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation()
{
    _display.Unregister(this);

    if(_remoteClient != nullptr) {

        TRACE_L1(_T("Destructing surface %s"), _remoteClient->Name().c_str());

        if(_remoteRenderer != nullptr) {
            _remoteRenderer->Release();
        }

        _remoteClient->Release();
    }

    _display.Release();
}

Display::Display(const string& name)
    : _displayName(name)
    , _adminLock()
    , _virtualinput(nullptr)
    , _surfaces()
    , _compositerServerRPCConnection()
    , _refCount(0)
    , _remoteDisplay(nullptr)
{
    Initialize();
}

Display::~Display()
{
    Deinitialize();
}

int Display::Process(const uint32_t)
{
    _adminLock.Lock();

    for( auto& surface : _surfaces ) {
        surface->ScanOut();
    }
    _adminLock.Unlock();

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

Compositor::IDisplay::ISurface* Display::Create(const std::string& name, const uint32_t width, const uint32_t height)
{
    Core::ProxyType<SurfaceImplementation> retval = (Core::ProxyType<SurfaceImplementation>::Create(*this, name, width, height));
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

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    return RPI::Display::Instance(displayName);
}
} // WPEFramework
