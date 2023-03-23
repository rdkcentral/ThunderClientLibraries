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

#include "rdkadapter.h"
#include <interfaces/IRDKAdapter.h>
#include <interfaces/INetworkControl.h>
#include <interfaces/IWifiControl.h>

 #include <list>
 #include <algorithm>

#define PRINT(format, ...) fprintf(stderr, _T("rdkadapter: ") format _T("\n"), ##__VA_ARGS__)

using namespace WPEFramework;

namespace {

// in case we need to override Operational for one or the other lket's wrap them...

class AdapterLink : protected RPC::SmartInterfaceType<Exchange::IRDKAdapter> {
private:
using SmartInterface = RPC::SmartInterfaceType<Exchange::IRDKAdapter>;

public:
    AdapterLink(const AdapterLink&) = delete;
    AdapterLink(AdapterLink&&) = delete;
    AdapterLink& operator=(const AdapterLink&) = delete;
    AdapterLink& operator=(AdapterLink&) = delete;

    AdapterLink() 
    : SmartInterface()
    {
        if (SmartInterface::Open(RPC::CommunicationTimeOut, SmartInterface::Connector(), _T("RDKAdapter")) != Core::ERROR_NONE) {
            PRINT(_T("Failed to open the smart interface!"));
        }
    }
    ~AdapterLink() override
    {
        SmartInterface::Close(Core::infinite);
    }

    const Exchange::IRDKAdapter* AdapterInterface() const 
    {
        return SmartInterface::Interface();
    }

    Exchange::IRDKAdapter* AdapterInterface()  
    {
        return SmartInterface::Interface();
    }
};

class NetworkLink : protected RPC::SmartInterfaceType<Exchange::INetworkControl> {
private:
using SmartInterface = RPC::SmartInterfaceType<Exchange::INetworkControl>;

public:
    NetworkLink(const NetworkLink&) = delete;
    NetworkLink(NetworkLink&&) = delete;
    NetworkLink& operator=(const NetworkLink&) = delete;
    NetworkLink& operator=(NetworkLink&) = delete;

    NetworkLink() 
    : SmartInterface()
    {
        if (SmartInterface::Open(RPC::CommunicationTimeOut, SmartInterface::Connector(), _T("NetworkControl")) != Core::ERROR_NONE) {
            PRINT(_T("Failed to open the smart interface!"));
        }
    }
    ~NetworkLink() override
    {
        SmartInterface::Close(Core::infinite);
    }

    const Exchange::INetworkControl* NetworkInterface() const 
    {
        return SmartInterface::Interface();
    }

    Exchange::INetworkControl* NetworkInterface()  
    {
        return SmartInterface::Interface();
    }
};

class WifiLink : protected RPC::SmartInterfaceType<Exchange::IWifiControl> {
private:
using SmartInterface = RPC::SmartInterfaceType<Exchange::IWifiControl>;

public:
    WifiLink(const WifiLink&) = delete;
    WifiLink(NetworkLink&&) = delete;
    WifiLink& operator=(const WifiLink&) = delete;
    WifiLink& operator=(WifiLink&) = delete;

    WifiLink() 
    : SmartInterface()
    {
        if (SmartInterface::Open(RPC::CommunicationTimeOut, SmartInterface::Connector(), _T("WifiControl")) != Core::ERROR_NONE) {
            PRINT(_T("Failed to open the smart interface!"));
        }
    }
    ~WifiLink() override
    {
        SmartInterface::Close(Core::infinite);
    }

    const Exchange::IWifiControl* WifiInterface() const 
    {
        return SmartInterface::Interface();
    }

    Exchange::IWifiControl* WifiInterface()  
    {
        return SmartInterface::Interface();
    }
};

class RDKAdapterImp : public RDKAdapter::IRDKAdapter {
private:
    class Notification : public Exchange::INetworkControl::INotification
                       , public Exchange::IRDKAdapter::INotification {
    public:
        explicit Notification(RDKAdapterImp& parent) : Exchange::INetworkControl::INotification(), _parent(parent) {}
        ~Notification() override = default;

        void Update(const string& interfaceName) override {
            _parent.InterfaceUpdate(interfaceName);
        }

        void ConnectionUpdate(const bool connected) override {
            _parent.ConnectionUpdate(connected);
        }


    public:
        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::INetworkControl::INotification)
        INTERFACE_ENTRY(Exchange::IRDKAdapter::INotification)
        END_INTERFACE_MAP

    private:
        RDKAdapterImp& _parent;
    };


public:
    RDKAdapterImp(const RDKAdapterImp&) = delete;
    RDKAdapterImp(RDKAdapterImp&&) = delete;
    RDKAdapterImp& operator=(const RDKAdapterImp&) = delete;
    RDKAdapterImp& operator=(RDKAdapterImp&) = delete;
    
    RDKAdapterImp() 
    : RDKAdapter::IRDKAdapter()
    , _adminLock()
    , _adapterlink()
    , _networklink()
    , _wifilink()
    , _sink(*this)
    {
        Exchange::INetworkControl* network = _networklink.NetworkInterface();
        if(network != nullptr) {
            network->Register(&_sink);
        }
    }

    ~RDKAdapterImp() override
    {
        Exchange::INetworkControl* network = _networklink.NetworkInterface();
        if(network != nullptr) {
            network->Unregister(&_sink);
        }
    }

    static RDKAdapterImp& Instance(){
        static RDKAdapterImp adapter;
        return adapter;
    } 

    uint32_t Register(IRDKAdapter::INotification* sink) override {
        _adminLock.Lock();

        ASSERT(std::find(_listeners.begin(), _listeners.end(), sink) == _listeners.end());

        _listeners.emplace_back(sink);

        _adminLock.Unlock();

        //they might to receive an initial update anyway:
        bool connected = false;
        Connected(connected);
        sink->ConnectedUpdate(connected);

        return Core::ERROR_NONE;
    }

    uint32_t Unregister(IRDKAdapter::INotification* sink) override {
        _adminLock.Lock();

        auto item = std::find(_listeners.begin(), _listeners.end(), sink);

        ASSERT(item != _listeners.end());

        _listeners.erase(item);

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    virtual uint32_t Interfaces(std::vector<std::string>& interfaces) const override {
        uint32_t result = Core::ERROR_RPC_CALL_FAILED;
        interfaces.clear();

        const Exchange::INetworkControl* network = _networklink.NetworkInterface();
        Exchange::INetworkControl::IStringIterator* it = nullptr;
        if(network != nullptr) {
            if ((result = network->Interfaces(it)) == Core::ERROR_NONE) {
                it->Reset(0);
                string i;
                while(it->Next(i) == true) {
                    interfaces.emplace_back(i);
                }

                it->Release();
            }
            network->Release();
        }
        return result;
    }


    uint32_t InterfaceAvailable(const string& interfacename, bool& available) const override {
        uint32_t result = Core::ERROR_RPC_CALL_FAILED;
        available = false;

        const Exchange::INetworkControl* network = _networklink.NetworkInterface();
        Exchange::INetworkControl::StatusType status = Exchange::INetworkControl::StatusType::UNAVAILABLE;
        if(network != nullptr) {
            if ((result = network->Status(interfacename, status)) == Core::ERROR_NONE) {
                available = status == Exchange::INetworkControl::StatusType::AVAILABLE;
            }
            network->Release();
        }
        return result;
    }

    uint32_t InterfaceUp(const std::string& interfacename, bool& up) const override {
        uint32_t result = Core::ERROR_RPC_CALL_FAILED;
        up = false;

        const Exchange::INetworkControl* network = _networklink.NetworkInterface();
        if(network != nullptr) {
            result = network->Up(interfacename, up);
            network->Release();
        }
        return result;
    }

    uint32_t InterfaceAddress(const std::string& interfacename, string& primaryaddress) const {
        uint32_t result = Core::ERROR_RPC_CALL_FAILED;
        
        const Exchange::INetworkControl* network = _networklink.NetworkInterface();
        Exchange::INetworkControl::INetworkInfoIterator* it;
        if(network != nullptr) {
            if ((result = network->Network(interfacename, it)) == Core::ERROR_NONE) {
                if(it->IsValid() == true) {
                    primaryaddress = it->Current().address;
                }
                it->Release();
            }
            network->Release();
        }
        return result;
    }

    uint32_t Connected(bool& connected) const override {
        uint32_t result = Core::ERROR_RPC_CALL_FAILED;
        
        const Exchange::IRDKAdapter* adapter = _adapterlink.AdapterInterface();
        if(adapter != nullptr) {
            result = adapter->Connected(connected);
            adapter->Release();
        }
        return result;
    }

private:
    void InterfaceUpdate(const string& interfaceName) {
        _adminLock.Lock();

        for (auto l : _listeners) {
            l->InterfaceUpdate(interfaceName);
        }

        _adminLock.Unlock();
    }

    void ConnectionUpdate(const bool connected) {
        _adminLock.Lock();

        for (auto l : _listeners) {
            l->ConnectedUpdate(connected);
        }

        _adminLock.Unlock();
    }


private:
    using NotificationContainer = std::list<RDKAdapter::IRDKAdapter::INotification*>;

    mutable Core::CriticalSection _adminLock;
    AdapterLink _adapterlink;
    NetworkLink _networklink;
    WifiLink _wifilink;
    NotificationContainer _listeners;
    Core::Sink<RDKAdapterImp::Notification> _sink;
};

}

namespace WPEFramework {

RDKAdapter::IRDKAdapter& RDKAdapter::IRDKAdapter::Instance()
{
    return RDKAdapterImp::Instance();
}

}
