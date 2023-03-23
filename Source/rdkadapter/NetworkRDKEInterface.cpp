#include <cstring>
#include <sstream>
#include <vector>

#include "sky/log.h"
#include "NetworkRdkInterface.h"
#include "NetworkController.h"
#include "NetworkUtils.h"


using namespace std;
using namespace WPEFramework;

static NetworkRdkInterface::Notification _notifcation;


NetworkRdkInterface::NetworkRdkInterface()
: m_instanceLock()
, _adapter(RDKAdapter::IRDKAdapter::Instance())
{
    Initialise();
}

NetworkRdkInterface::~NetworkRdkInterface()
{

}

void NetworkRdkInterface::Initialise()
{
  
}

void NetworkRdkInterface::RegisterRdkNetworkEventListener()
{
    _adapter.Register(&_notifcation);
}

int NetworkRdkInterface::EnableInterface( std::string interfaceName, bool enableInterface )
{
   
}

int NetworkRdkInterface::StartAutoConnect()
{
   
}

int NetworkRdkInterface::CancelWPSPairing()
{
   
}

int NetworkRdkInterface::ConnectToAccessPoint( std::string ssid, std::string security, std::string password )
{
  
}

int NetworkRdkInterface::Disconnect()
{
  
}

int NetworkRdkInterface::ClearSSID()
{
   
}

int NetworkRdkInterface::GetInterfaces( std::string &jsonResponseInterfaces )
{
   
}

int NetworkRdkInterface::GetStbIpAddress( std::string &ipAddress )
{
   
}

int NetworkRdkInterface::GetGatewayIpAddress( std::string &gatewayIpAddress )
{
 

}

int NetworkRdkInterface::SetSignalThresholdChangeEnabled()
{

}

int NetworkRdkInterface::GetConnectedSSID( NetworkWifiConnectedSSID &connectedSsid )
{

}

int NetworkRdkInterface::StartWifiScan()
{
  
}

int NetworkRdkInterface::StopWifiScan()
{
 
}

int NetworkRdkInterface::SetDefaultInterface( const std::string &interfaceName, bool isPersist )
{
 
}

std::string NetworkRdkInterface::GetNetworkStatusConnectStatus( const WiFiStatusCode connectState )
{
 
}

std::string NetworkRdkInterface::AStoRDKInterfaceMapping( const std::string &asInterface )
{
  
}

int NetworkRdkInterface::SetManualIpAddress( const std::string &ifname, const ManualIpAddressParameters &manualIpAddressParameters )
{
   
}

int NetworkRdkInterface::GetManualIpAddress( const std::string &ifname, ManualIpAddressParameters &manualIpAddressParameters )
{
  
}

int NetworkRdkInterface::SetWPSLed( WPSLedState ledState )
{
  
}

int NetworkRdkInterface::EchoPing( const std::string &endpoint, const uint32_t packets, uint32_t &packetsReceived )
{
 
}

int NetworkRdkInterface::SetConnectivityTestEndpoints( const std::string &endpointIpAddress )
{
  
}

int NetworkRdkInterface::IsConnectedToInternet( bool &isConnected )
{

}

int NetworkRdkInterface::GetPublicIPAddress( std::string& publicIPAddress )
{

}

int NetworkRdkInterface::GetWakeupReason( std::string& wakeupReason ) const
{

}

int NetworkRdkInterface::InitiateWPSPINPairing()
{

}

void NetworkRdkInterface::OnWifiSignalThresholdChangedHandler( const Core::JSON::String& parameters )
{

}

void NetworkRdkInterface::OnAvailableSSIDsHandler( const Core::JSON::String& parameters )
{

}

void NetworkRdkInterface::OnWIFIStateChangedHandler( const Core::JSON::String& parameters )
{

}

void NetworkRdkInterface::OnWifiErrorHandler( const Core::JSON::String& parameters )
{

}

void NetworkRdkInterface::Notification::InterfaceUpdate(const string& interfacename) 
{
    auto networkControllerInstance = NetworkController::getInstance();
    if ( networkControllerInstance )
    {
        bool available = true;
        _adapter.InterfaceAvailable(interfacename, available);
        std::string availabledata ="{ \"interface\" : \"";
        availabledata += interfacename;
        availabledata += "\", \"enabled\" : ";
        availabledata += available ? "true }" : "false }";

        networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_INTERFACE_STATUS_CHANGED, availabledata );

        std::string primaryaddress;
        _adapter.InterfaceAddress(interfacename, primaryaddress);
        if(primaryaddress.empty() != false) {
            std::string ipdata ="{ \"interface\" : \"";
            primaryaddress += interfacename;
            primaryaddress += "\", \"ip4Address\" : \"";
            primaryaddress += primaryaddress;
            primaryaddress += "\", \"status\" : \"ACQUIRED\"}";
            networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_IP_ADDRESS_CHANGED, primaryaddress );
        }
    }
}

void NetworkRdkInterface::ConnectedUpdate(const bool connected) override {
    auto networkControllerInstance = NetworkController::getInstance();
    if ( networkControllerInstance )
    {
        std::vector<std::string> interfaces;
        _adapter.Interfaces(interfaces);
        for( auto& s : interfaces) {
            bool available = false;
            _adapter.InterfaceAvailable(s, available);
            if(available == true) {
                std::string connecteddata ="{ \"interface\" : \"";
                connecteddata += s;
                connecteddata += "\", \"status\" : ";
                connecteddata += connected ? "\"CONNECTED\" }" : "\"DISCONNECTED\" }";
                networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_CONNECTION_STATUS_CHANGED, connecteddata );
            }
        }
    }
}

//doen we niet

        networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_WIFI_SIGNAL_THRESHOLD_CHANGED, data );

