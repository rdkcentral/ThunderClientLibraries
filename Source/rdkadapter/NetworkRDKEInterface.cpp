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
    // don't think it is needed anyway...
    uint32_t result =_adapter.InterfaceUp(interfaceName, enableInterface);
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::StartAutoConnect()
{
    return -1; // not supported for now
}

int NetworkRdkInterface::CancelWPSPairing()
{
    return -1; // not supported for now
}

int NetworkRdkInterface::ConnectToAccessPoint( std::string ssid, std::string security, std::string password )
{
    uint32_t result = Core::ERROR_RPC_CALL_FAILED;

    SsidSecurity securityMode;
    if ( NETWORK_UTILS::GetWirelessSecurityModeFromASSecurityString( security, securityMode ) == 0 ) {
        result =_adapter.Connect(ssid, securityMode, password);
    }
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::Disconnect()
{
    uint32_t result =_adapter.Disconnect();
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::ClearSSID()
{
    return -1; // not supported for now
}

int NetworkRdkInterface::GetInterfaces( std::string &jsonResponseInterfaces )
{
    std::vector<std::string> interfaces;
    uint32_t result = _adapter.Interfaces(interfaces);
    if(result == Core::ERROR_NONE) {
        jsonResponseInterfaces = "{\"interfaces\":[";
        bool added = false;
        for( auto i = interfaces.begin(); i != interfaces.end(); ++i  ) {
            if(added == true) {
                jsonResponseInterfaces += ", ";
            }
            bool up = false;
            result = _adapter.InterfaceUp(*it, up);
            if(result == Core::ERROR_NONE) {
                added = true;
                jsonResponseInterfaces += "{\"interface\":\"";
                jsonResponseInterfaces += *it;
                jsonResponseInterfaces += "\",\"macAddress\":\"\", \"enabled\":"; //maccaddress not available for now
                jsonResponseInterfaces += up ? "true, " : "false, ";
                jsonResponseInterfaces += "\"connected\":true}"; // no connect info for now
            }
        }
        jsonResponseInterfaces += "]}";
    }
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::GetStbIpAddress( std::string &ipAddress )
{
    uint32_t result =_adapter.PublicIpAddress(ipAddress);
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::GetGatewayIpAddress( std::string &gatewayIpAddress )
{
    // not supported for now
    return -1;
}

int NetworkRdkInterface::SetSignalThresholdChangeEnabled()
{
    // not supported for now
    return -1;
}

int NetworkRdkInterface::GetConnectedSSID( NetworkWifiConnectedSSID &connectedSsid )
{
    string ssid;
    uint32_t result =_adapter.WifiConnected(ssid);
    // mhmm cannot see how it should behave if not connected. let's assume we return empty values...
    if(result == Core::ERROR_NONE) {
        connectedSsid.ssid = ssid;
        RDKAdapter::IRDKAdapter::WifiNetworkInfo info;
        result =_adapter.WifiInfo(ssid,info);
        if(result == Core::ERROR_NONE) {
            connectedSsid.bssid = info.bssid;
            connectedSsid.wirelessSecurity = info.security;
            connectedSsid.frequency = info.frequency;
            connectedSsid.sigstr = info.signal;
        }
    }
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::StartWifiScan()
{
    uint32_t result =_adapter.WifiScan();
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::StopWifiScan()
{
    // not supported for now, but let's pretent we do :)
    return 0;
}

int NetworkRdkInterface::SetDefaultInterface( const std::string &interfaceName, bool isPersist )
{
    // not supported for now
    return -1;
}

std::string NetworkRdkInterface::GetNetworkStatusConnectStatus( const WiFiStatusCode connectState )
{
    // not supported for now
    return "";
}

int NetworkRdkInterface::SetManualIpAddress( const std::string &ifname, const ManualIpAddressParameters &manualIpAddressParameters )
{
    RDKAdapter::IRDKAdapter::NetworkInfo info;
    info.mode == manualIpAddressParameters.autoConfig == true? RDKAdapter::IRDKAdapter::ModeType::DYNAMIC : RDKAdapter::IRDKAdapter::ModeType::STATIC;
    info.address = manualIpAddressParameters.ipAdrr;
    info.mask = manualIpAddressParameters.netMask;
    info.defaultGateway = manualIpAddressParameters.gateway;
    manualIpAddressParameters.primaryDns = info.dns.size() > 0 ? 
    info.dns.emplace_back(manualIpAddressParameters.primaryDns);
    info.dns.emplace_back(manualIpAddressParameters.secondaryDns);
    uint32_t result =_adapter.InterfaceSetting(ifname, info);
    return result == Core::ERROR_NONE : 0 : -1;  
}

int NetworkRdkInterface::GetManualIpAddress( const std::string &ifname, ManualIpAddressParameters &manualIpAddressParameters )
{
    RDKAdapter::IRDKAdapter::NetworkInfo info;
    uint32_t result =_adapter.InterfaceSetting(ifname, info);
    if(result == Core::ERROR_NONE) {
        manualIpAddressParameters.autoConfig = info.mode == RDKAdapter::IRDKAdapter::ModeType::DYNAMIC;
        manualIpAddressParameters.ipAdrr = info.address;
        manualIpAddressParameters.netMask = info.mask;
        manualIpAddressParameters.gateway = info.defaultGateway;
        manualIpAddressParameters.primaryDns = info.dns.size() > 0 ? info.dns[0] : "";
        manualIpAddressParameters.secondaryDns = info.dns.size() > 1 ? info.dns[1] : "";
    }
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::SetWPSLed( WPSLedState ledState )
{
    return -1;
}

int NetworkRdkInterface::EchoPing( const std::string &endpoint, const uint32_t packets, uint32_t &packetsReceived )
{
    return -1;
}

int NetworkRdkInterface::SetConnectivityTestEndpoints( const std::string &endpointIpAddress )
{
    return -1;
}

int NetworkRdkInterface::IsConnectedToInternet( bool &isConnected )
{
    uint32_t result =_adapter.Connected(isConnected);
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::GetPublicIPAddress( std::string& publicIPAddress )
{
    uint32_t result =_adapter.PublicIpAddress(publicIPAddress);
    return result == Core::ERROR_NONE : 0 : -1;
}

int NetworkRdkInterface::GetWakeupReason( std::string& wakeupReason ) const
{
    return -1;
}

int NetworkRdkInterface::InitiateWPSPINPairing()
{
    return -1;
}

void NetworkRdkInterface::Notification::InterfaceUpdate(const std::string& interfacename) 
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

void NetworkRdkInterface::ConnectedUpdate(const bool connected)  {
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

void NetworkRdkInterface::Notification::SSIDSUpdate() {
    auto networkControllerInstance = NetworkController::getInstance();
    if ( networkControllerInstance )
    {
        std::vector<std::string> ssids;
        _adapter.SSIDS(ssids);
        if(ssids.size() > 0) {
            std::string ssiddata ="{ \"ssids\" : [";
            for( auto it = ssids.begin(); it != ssids.end(); ++it ) {
                if(it != ssids.begin()) {
                    ssiddata += ", ";
                }
                ssiddata += '\"';
                ssiddata += *it;
                ssiddata += '\"';
            }
            ssiddata += "]}";
            // note the json also has a "moreData", let's assume it is not mandatory
            networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_SSID_AVAILABLE, ssiddata );
        }
    }
}

void NetworkRdkInterface::Notification::WifiConnectionChange(const std::string& ssid) {
    auto networkControllerInstance = NetworkController::getInstance();
    if ( networkControllerInstance )
    {
        std::string data ="{ \"state\" : ";
        data += ssid.empty() == false ? '5' : '2'; // 5 is connected, 2 is disconnected...
        data += ", \"isLNF\" : false}";

        networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_WIFI_STATE_CHANGED, data );
    }
}

//not supported, and I gues will work without:

//        networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_WIFI_SIGNAL_THRESHOLD_CHANGED, data );
//        networkControllerInstance->RdkEventsListener( NETWORK_EVENT_TYPE_RDK_WIFI_ERROR, data );


//not supported, think we might need it but skipped for now
