//
// Created by Priyanka on 08/10/19.
//

#ifndef NETWORK_RDK_INTERFACE_H
#define NETWORK_RDK_INTERFACE_H



#include <string>
#include <vector>
#include <memory>
#include <mutex>

#define _SYS_SYSLOG_H 1 // Prevent inclusion of syslog to conflict with our logging system
#include "INetworkRdkInterface.h"
#include "ASModule.h"
#include "core/core.h"
#include <rdkadapter.h>
#include "NetworkStatus.h"

using namespace WPEFramework;

class NetworkInterface;

class NetworkRdkInterface : public INetworkRdkInterface
{
public:
    NetworkRdkInterface(const NetworkRdkInterface&) = delete;
    NetworkRdkInterface& operator=(const NetworkRdkInterface&) = delete;

    NetworkRdkInterface();
    ~NetworkRdkInterface();

    void Initialise() override;

    void RegisterRdkNetworkEventListener() override;

    /**
     * @brief Enable interface
     *
     * @param[in] interfaceName       Interface name that needs to be enabled/disabled
     * @param[in] enableInterface     True if interface needs to be enabled, false otherwise
     *
     * @retval 0 on success, -1 on failure
     */
    int EnableInterface( std::string interfaceName, bool enableInterface ) override;

    /**
     * @brief Start Wifi scan
     *
     * @retval 0 on success, -1 on failure
     */
    int StartWifiScan() override;

    /**
     * @brief Stop Wifi scan
     *
     * @retval 0 on success, -1 on failure
     */
    int StopWifiScan() override;

    /**
     * @brief Start WPS Wifi pairing
     *
     * @retval 0 on success, -1 on failure
     */
    int StartAutoConnect() override;

    /**
     * @brief Stop WPS Wifi pairing
     *
     * @retval 0 on success, -1 on failure
     */
    int CancelWPSPairing() override;

    /**
     * @brief Connect to a particular SSID
     *
     * @param[in] ssid       : SSID of access point to connect to
     * @param[in] security   : Security mode of the access point
     * @param[in] password   : Password required to connect to access point
     *
     * @retval 0 on success, -1 on failure
     */
    int ConnectToAccessPoint( std::string ssid, std::string security, std::string password ) override;

    /**
     * @brief Disconnects from access point if connected
     *
     * @retval 0 on success, -1 on failure
     */
    int Disconnect() override;

    /**
     * @brief Clears the saved SSID and BSSID
     *
     * @retval 0 on success, -1 on failure
     */
    int ClearSSID() override;

    /**
     * @brief Returns a list of interfaces supported by this device including their state
     *
     * @param[out] jsonResponseInterfaces : JSON RPC response containing interface list
     *
     * @retval 0 on success, -1 on failure
     */
    int GetInterfaces( std::string &jsonResponseInterfaces ) override;

    /**
     * @brief Gets STB IP address
     *
     * @param[out] ipAddress : STB IP address
     *
     * @retval 0 on success, -1 on failure
     */
    int GetStbIpAddress( std::string &ipAddress ) override;

    /**
     * @brief Gets Gateway IP address
     *
     * @param[out] ipAddress : Gateway IP address
     *
     * @retval 0 on success, -1 on failure
     */
    int GetGatewayIpAddress( std::string &ipAddress ) override;

    /**
     * @brief  Enable signal threshold change notification. When enabled, allows signalThresholdChange events to be fired
     *
     * @retval 0 on success, -1 on failure
     */
    int SetSignalThresholdChangeEnabled() override;

    /**
     * @brief Gets the connected SSID information
     *
     * @param[out] jsonResponseConnectedSsid  : JSON RPC response with connected SSID details
     *
     * @retval 0 on success, -1 on failure
     */
    int GetConnectedSSID( NetworkWifiConnectedSSID &connectedSsid ) override;

    /**
     * @brief Sets the default interface
     *
     * @param[in] interfaceName  : Name of the interface that will be set to default
     * @param[in] isPersist      : True if this interface will be the default interface currently AND on the next reboot.
     *                             False when this interface will only be the default during this session.
     *
     * @retval 0 on success, -1 on failure
     */
    int SetDefaultInterface( const std::string &interfaceName, bool isPersist ) override;

    /**
     * @brief Map RDK connect status to AS connect status
     *
     * @param[in] connectState  : RDK connect status
     *
     * @retval AS connect status string
     */
    std::string GetNetworkStatusConnectStatus( const WiFiStatusCode connectState ) override;

    /**
     * @brief Set IP address manually
     *
     * @param[in] ifname                     : Name of the interface
     * @param[in] manualIpAddressParameters  : Manual IP address parameters
     *
     * @retval 0 on success, any other value otherwise
     */
    int SetManualIpAddress( const std::string &ifname, const ManualIpAddressParameters &manualIpAddressParameters ) override;

    /**
     * @brief Get IP address manually
     *
     * @param[in]  ifname                     : Name of the interface
     * @param[out] manualIpAddressParameters  : Manual IP address parameters
     *
     * @retval 0 on success, any other value otherwise
     */
    int GetManualIpAddress( const std::string &ifname, ManualIpAddressParameters &manualIpAddressParameters ) override;

    /**
     * @brief Sets the front panel LED to a WPS LED state
     *
     * @param[in] ledState  :   State to put the front panel LED to
     *
     * @retval 0 on success, -1 on failure
     */
    int SetWPSLed( WPSLedState ledState ) override;

    /**
     * @brief Pings the given endpoint
     *
     * @param[in]  endpoint         :   the host name or IP address to ping
     * @param[in]  packets          :   the number of packets to send
     * @param[out] packetsReceived  :   the number of packets received
     *
     * @retval 0 on success, -1 on failure
     */
    int EchoPing( const std::string &endpoint, const uint32_t packets, uint32_t &packetsReceived ) override;

    /**
     * @brief Sets the RDK connectivity test endpoint to a given IP address
     *
     * Two endpoints are set:
     * <endpointIpAddress>:53,
     * <endpointIpAddress>:80.
     *
     * @param[in]  endpointIpAddress:   the IP address of the endpoint to set
     *
     * @retval 0 on success, -1 on failure
     */
    int SetConnectivityTestEndpoints( const std::string &endpointIpAddress ) override;

    /**
     * @brief Gets the public IP address of the device via a getPlatformConfiguration query
     *
     * To comply with GDPR privacy guidelines, the output address shall never be logged unobfuscated.
     *
     * @param[out]  publicIpv4Address   : public IP address of the device
     *
     * @retval 0 on success, -1 on failure
     */
    int GetPublicIPAddress( std::string& publicIPAddress ) override;

    /**
     * @brief Gets the reason of the last wake-up from deep sleep
     *
     * @param[out]  wakeupReason   : last wake-up reason
     *
     * @retval 0 on success, -1 on failure
     */
    int GetWakeupReason( std::string& wakeupReason ) const;

    /**
     * @brief Checks Internet connection based on the endpoints set by SetConnectivityTestEndpoints
     *
     * @param[out]  isConnected     : True if the device is connected to the Internet,
     *                                false otherwise
     *
     * @retval 0 on success, -1 on failure
     */
    int IsConnectedToInternet( bool &isConnected ) override;

    /**
     * @brief Starts Auto WPS PIN pairing
     *
     * @retval 0 on success, -1 on failure
     */
    int InitiateWPSPINPairing() override;

private:
    std::mutex m_instanceLock;
    RDKAdapter::IRDKAdapter& _adapter;

    class Notification : public RDKAdapter::IRDKAdapter::INotification {
    public:
        ~Notification() override = default;

        void InterfaceUpdate(const string& interfacename) override;
        void ConnectedUpdate(const bool connected) override;

    }

    static void OnWifiSignalThresholdChangedHandler( const Core::JSON::String& parameters );

    static void OnDefaultInterfaceChangedHandler(const string& interfacename, bool connected);

    static void OnAvailableSSIDsHandler( const Core::JSON::String& parameters );

    static void OnWIFIStateChangedHandler( const Core::JSON::String& parameters );

    static void OnWifiErrorHandler( const Core::JSON::String& parameters );

    static std::string AStoRDKInterfaceMapping( const std::string &asInterface );
};

#endif //NETWORK_RDK_INTERFACE_H
