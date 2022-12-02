#include "Globals.h"

#include "Matter.h"
#include "MatterWiFi.h"
#include "WiFi.h"

#include <lib/support/Span.h>

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/CommissionableDataProvider.h>
#include <platform/ConnectivityManager.h>

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/ESP32/ESP32Utils.h>
#include <platform/ESP32/NetworkCommissioningDriver.h>
#include <platform/internal/BLEManager.h>

#include <esp_log.h>
#include <lwip/nd6.h>

#include <vector>

using namespace std;
using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::TLV;
using chip::DeviceLayer::Internal::ESP32Utils;

using MyStatus                  = chip::DeviceLayer::NetworkCommissioning::Status;
using MyScanResponseIterator    = chip::DeviceLayer::NetworkCommissioning::WiFiScanResponseIterator;

void MatterWiFi::OnFinished(MyStatus status, chip::CharSpan debugText, MyScanResponseIterator * NetworksIterator)
{
    if (NetworksIterator != nullptr)
    {
        ESP_LOGE("Found AP count","%d", NetworksIterator->Count());

        chip::DeviceLayer::NetworkCommissioning::WiFiScanResponse ScannedItem;

        vector<WiFiScannedAPListItem> ResultToSave = vector<WiFiScannedAPListItem>();

        while (NetworksIterator->Next(ScannedItem))
        {
            WiFiScannedAPListItem Record;
            Record.SSID     = string((char *)ScannedItem.ssid, ScannedItem.ssidLen);

            memcpy(Record.BSSID, ScannedItem.bssid, 6);

            //Record.SecurityMode    = ScannedItem.security;
            Record.Channel  = ScannedItem.channel;
            Record.Band     = (ScannedItem.wiFiBand == chip::DeviceLayer::NetworkCommissioning::WiFiBand::k2g4) ? NetworkWiFiBandEnum::K2G4 : K5;
            Record.RSSI     = ScannedItem.rssi;

            ResultToSave.push_back(Record);
        }

        Network.ImportScannedSSIDList(ResultToSave);
    }
    else
        ESP_LOGE("No AP found"," :(");

	ESP_LOGE("WiFiScan", "MScanFinished GIVE");
    Matter::m_scanFinished.Give();
}

void MatterWiFi::OnScanDone() {
    return;
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_SCAN_DONE");
    NetworkCommissioning::ESPWiFiDriver::GetInstance().OnScanWiFiNetworkDone();
}

void MatterWiFi::OnSTAStart() {
    return;
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_START");
    DriveStationState();
}

void MatterWiFi::OnSTAStop() {
    return;
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_STOP");
    DriveStationState();
}

void MatterWiFi::OnSTAConnected() {
    return;
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_CONNECTED");
    if (mWiFiStationState == ConnectivityManager::kWiFiStationState_Connecting)
    {
        ChangeWiFiStationState(ConnectivityManager::kWiFiStationState_Connecting_Succeeded);
    }
    
    DriveStationState();
}

void MatterWiFi::OnSTADisconnected() {
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_DISCONNECTED");
    //!NetworkCommissioning::ESPWiFiDriver::GetInstance().SetLastDisconnectReason(event);
    if (mWiFiStationState == ConnectivityManager::kWiFiStationState_Connecting)
    {
        ChangeWiFiStationState(ConnectivityManager::kWiFiStationState_Connecting_Failed);
    }
    DriveStationState();
}

void MatterWiFi::OnAPStart() {
    return;

    ChipLogProgress(DeviceLayer, "WIFI_EVENT_AP_START");
    ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_Active);
    DriveAPState();
}

void MatterWiFi::OnAPStop() {
    return;

    ChipLogProgress(DeviceLayer, "WIFI_EVENT_AP_STOP");
    ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_NotActive);
    DriveAPState();
}

void MatterWiFi::OnAPSTADisconnected() {
    ChipLogProgress(DeviceLayer, "WIFI_EVENT_AP_STACONNECTED");
    //!(ConnectivityManager::ConnectivityMgr()).MaintainOnDemandWiFiAP();
}

void MatterWiFi::STAGotIP(const ip_event_got_ip_t & got_ip_event) {
    ChipLogProgress(DeviceLayer, "IP_EVENT_STA_GOT_IP");
    OnStationIPv4AddressAvailable(got_ip_event);
}

void MatterWiFi::STAGotIPv6(const ip_event_got_ip6_t & got_ip_event) {
    ChipLogProgress(DeviceLayer, "IP_EVENT_GOT_IP6");
    OnIPv6AddressAvailable(got_ip_event);
}

void MatterWiFi::STALostIP() {
    ChipLogProgress(DeviceLayer, "IP_EVENT_STA_LOST_IP");
    OnStationIPv4AddressLost();
}

void MatterWiFi::ChangeWiFiStationState(ConnectivityManager::WiFiStationState newState)
{
    if (mWiFiStationState != newState)
    {
        //!ChipLogProgress(DeviceLayer, "WiFi station state change: %s -> %s", ConnectivityManager::WiFiStationStateToStr(mWiFiStationState),
        //!                ConnectivityManager::WiFiStationStateToStr(newState));
        mWiFiStationState = newState;
        SystemLayer().ScheduleLambda([]() { NetworkCommissioning::ESPWiFiDriver::GetInstance().OnNetworkStatusChange(); });
    }
}

void MatterWiFi::ChangeWiFiAPState(ConnectivityManager::WiFiAPState newState)
{
    if (mWiFiAPState != newState)
    {
        //!ChipLogProgress(DeviceLayer, "WiFi AP state change: %s -> %s", ConnectivityManager::WiFiAPStateToStr(mWiFiAPState), ConnectivityManager::WiFiAPStateToStr(newState));
        mWiFiAPState = newState;
    }
}

void MatterWiFi::DriveAPState()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    ConnectivityManager::WiFiAPState targetState;
    bool espAPModeEnabled = true;

    // Adjust the Connectivity Manager's AP state to match the state in the WiFi layer.
    if (espAPModeEnabled && (mWiFiAPState == ConnectivityManager::kWiFiAPState_NotActive || mWiFiAPState == ConnectivityManager::kWiFiAPState_Deactivating))
    {
        ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_Activating);
    }
    if (!espAPModeEnabled && (mWiFiAPState == ConnectivityManager::kWiFiAPState_Active || mWiFiAPState == ConnectivityManager::kWiFiAPState_Activating))
    {
        ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_Deactivating);
    }

    // If the AP interface is not under application control...
    if (mWiFiAPMode != ConnectivityManager::kWiFiAPMode_ApplicationControlled)
    {
        // Determine the target (desired) state for AP interface...

        // The target state is 'NotActive' if the application has expressly disabled the AP interface.
        if (mWiFiAPMode == ConnectivityManager::kWiFiAPMode_Disabled)
        {
            targetState = ConnectivityManager::kWiFiAPState_NotActive;
        }

        // The target state is 'Active' if the application has expressly enabled the AP interface.
        else if (mWiFiAPMode == ConnectivityManager::kWiFiAPMode_Enabled)
        {
            targetState = ConnectivityManager::kWiFiAPState_Active;
        }

        // The target state is 'Active' if the AP mode is one of the 'On demand' modes and there
        // has been demand for the AP within the idle timeout period.
        else if (mWiFiAPMode == ConnectivityManager::kWiFiAPMode_OnDemand || mWiFiAPMode == ConnectivityManager::kWiFiAPMode_OnDemand_NoStationProvision)
        {
            System::Clock::Timestamp now = System::SystemClock().GetMonotonicTimestamp();

            targetState = ConnectivityManager::kWiFiAPState_Active;
        }

        // Otherwise the target state is 'NotActive'.
        else
        {
            targetState = ConnectivityManager::kWiFiAPState_NotActive;
        }

        // If the current AP state does not match the target state...
        if (mWiFiAPState != targetState)
        {
            // If the target state is 'Active' and the current state is NOT 'Activating', enable
            // and configure the AP interface, and then enter the 'Activating' state.  Eventually
            // a SYSTEM_EVENT_AP_START event will be received from the ESP WiFi layer which will
            // cause the state to transition to 'Active'.
            if (targetState == ConnectivityManager::kWiFiAPState_Active)
            {
                if (mWiFiAPState != ConnectivityManager::kWiFiAPState_Activating)
                {
                    err = ESP32Utils::SetAPMode(true);
                    ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_Activating);
                }
            }

            // Otherwise, if the target state is 'NotActive' and the current state is not 'Deactivating',
            // disable the AP interface and enter the 'Deactivating' state.  Later a SYSTEM_EVENT_AP_STOP
            // event will move the AP state to 'NotActive'.
            else
            {
                if (mWiFiAPState != ConnectivityManager::kWiFiAPState_Deactivating)
                {
                    err = ESP32Utils::SetAPMode(false);
                    SuccessOrExit(err);

                    ChangeWiFiAPState(ConnectivityManager::kWiFiAPState_Deactivating);
                }
            }
        }
    }

    // If AP is active, but the interface doesn't have an IPv6 link-local
    // address, assign one now.
    if (mWiFiAPState == ConnectivityManager::kWiFiAPState_Active && ESP32Utils::IsInterfaceUp("WIFI_AP_DEF") &&
        !ESP32Utils::HasIPv6LinkLocalAddress("WIFI_AP_DEF"))
    {
        esp_err_t error = esp_netif_create_ip6_linklocal(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));
        if (error != ESP_OK)
        {
            ChipLogError(DeviceLayer, "esp_netif_create_ip6_linklocal() failed for WIFI_AP_DEF interface: %s",
                         esp_err_to_name(error));
            goto exit;
        }
    }

exit:
    if (err != CHIP_NO_ERROR && mWiFiAPMode != ConnectivityManager::kWiFiAPMode_ApplicationControlled)
    {
        //!ConnectivityManager::SetWiFiAPMode(ConnectivityManager::kWiFiAPMode_Disabled);
        ESP32Utils::SetAPMode(false);
    }
}


void MatterWiFi::DriveStationState()
{
    bool stationConnected;

    // Refresh the current station mode.  Specifically, this reads the ESP auto_connect flag,
    // which determine whether the WiFi station mode is kWiFiStationMode_Enabled or
    // kWiFiStationMode_Disabled.
    //!GetWiFiStationMode();

    // If the station interface is NOT under application control...
    if (mWiFiStationMode != ConnectivityManager::kWiFiStationMode_ApplicationControlled)
    {
        // Ensure that the ESP WiFi layer is started.
        ReturnOnFailure(ESP32Utils::StartWiFiLayer());

        // Ensure that station mode is enabled in the ESP WiFi layer.
        ReturnOnFailure(ESP32Utils::EnableStationMode());
    }

    // Determine if the ESP WiFi layer thinks the station interface is currently connected.
    ReturnOnFailure(ESP32Utils::IsStationConnected(stationConnected));

    // If the station interface is currently connected ...
    if (stationConnected)
    {
        // Advance the station state to Connected if it was previously NotConnected or
        // a previously initiated connect attempt succeeded.
        if (mWiFiStationState == ConnectivityManager::kWiFiStationState_NotConnected || mWiFiStationState == ConnectivityManager::kWiFiStationState_Connecting_Succeeded)
        {
            ChangeWiFiStationState(ConnectivityManager::kWiFiStationState_Connected);
            ChipLogProgress(DeviceLayer, "WiFi station interface connected");
            OnStationConnected();
        }
    }

    // Otherwise the station interface is NOT connected to an AP, so...
    else
    {
        System::Clock::Timestamp now = System::SystemClock().GetMonotonicTimestamp();

        // Advance the station state to NotConnected if it was previously Connected or Disconnecting,
        // or if a previous initiated connect attempt failed.
        if (mWiFiStationState == ConnectivityManager::kWiFiStationState_Connected || mWiFiStationState == ConnectivityManager::kWiFiStationState_Disconnecting ||
            mWiFiStationState == ConnectivityManager::kWiFiStationState_Connecting_Failed)
        {
            ConnectivityManager::WiFiStationState prevState = mWiFiStationState;
            ChangeWiFiStationState(ConnectivityManager::kWiFiStationState_NotConnected);
            if (prevState != ConnectivityManager::kWiFiStationState_Connecting_Failed)
            {
                ChipLogProgress(DeviceLayer, "WiFi station interface disconnected");
                OnStationDisconnected();
            }
        }
    }

    ChipLogProgress(DeviceLayer, "Done driving station state, nothing else to do...");
    // Kick-off any pending network scan that might have been deferred due to the activity
    // of the WiFi station.
}

void MatterWiFi::OnStationConnected()
{
    ESP_LOGE("!","MatterWiFi::OnStationConnected");

    // Assign an IPv6 link local address to the station interface.
    esp_err_t err = esp_netif_create_ip6_linklocal(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"));
    if (err != ESP_OK)
    {
        ChipLogError(DeviceLayer, "esp_netif_create_ip6_linklocal() failed for WIFI_STA_DEF interface: %s", esp_err_to_name(err));
    }
    NetworkCommissioning::ESPWiFiDriver::GetInstance().OnConnectWiFiNetwork();
    // TODO Invoke WARM to perform actions that occur when the WiFi station interface comes up.

    // Alert other components of the new state.
    ChipDeviceEvent event;
    event.Type                          = DeviceEventType::kWiFiConnectivityChange;
    event.WiFiConnectivityChange.Result = kConnectivity_Established;
    PlatformMgr().PostEventOrDie(&event);
    WiFiDiagnosticsDelegate * delegate = GetDiagnosticDataProvider().GetWiFiDiagnosticsDelegate();

    if (delegate)
    {
        delegate->OnConnectionStatusChanged(
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::WiFiConnectionStatus::kConnected));
    }

    UpdateInternetConnectivityState();
}


void MatterWiFi::OnStationIPv4AddressAvailable(const ip_event_got_ip_t & got_ip)
{
#if CHIP_PROGRESS_LOGGING
    {
        ChipLogProgress(DeviceLayer, "IPv4 address %s on WiFi station interface: " IPSTR "/" IPSTR " gateway " IPSTR,
                        (got_ip.ip_changed) ? "changed" : "ready", IP2STR(&got_ip.ip_info.ip), IP2STR(&got_ip.ip_info.netmask),
                        IP2STR(&got_ip.ip_info.gw));
    }
#endif // CHIP_PROGRESS_LOGGING

    UpdateInternetConnectivityState();

    ChipDeviceEvent event;
    event.Type                           = DeviceEventType::kInterfaceIpAddressChanged;
    event.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV4_Assigned;
    PlatformMgr().PostEventOrDie(&event);
}

void MatterWiFi::OnStationIPv4AddressLost(void)
{
    ChipLogProgress(DeviceLayer, "IPv4 address lost on WiFi station interface");

    UpdateInternetConnectivityState();

    ChipDeviceEvent event;
    event.Type                           = DeviceEventType::kInterfaceIpAddressChanged;
    event.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV4_Lost;
    PlatformMgr().PostEventOrDie(&event);
}

void MatterWiFi::OnIPv6AddressAvailable(const ip_event_got_ip6_t & got_ip)
{
#if CHIP_PROGRESS_LOGGING
    {
        ChipLogProgress(DeviceLayer, "IPv6 addr available. Ready on %s interface: " IPV6STR, esp_netif_get_ifkey(got_ip.esp_netif),
                        IPV62STR(got_ip.ip6_info.ip));
    }
#endif // CHIP_PROGRESS_LOGGING

    UpdateInternetConnectivityState();

    ChipDeviceEvent event;
    event.Type                           = DeviceEventType::kInterfaceIpAddressChanged;
    event.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV6_Assigned;
    PlatformMgr().PostEventOrDie(&event);
}

void MatterWiFi::OnStationDisconnected()
{
    // TODO Invoke WARM to perform actions that occur when the WiFi station interface goes down.

    // Alert other components of the new state.
    ChipDeviceEvent event;
    event.Type                          = DeviceEventType::kWiFiConnectivityChange;
    event.WiFiConnectivityChange.Result = kConnectivity_Lost;
    PlatformMgr().PostEventOrDie(&event);
    WiFiDiagnosticsDelegate * delegate = GetDiagnosticDataProvider().GetWiFiDiagnosticsDelegate();
    uint16_t reason                    = NetworkCommissioning::ESPWiFiDriver::GetInstance().GetLastDisconnectReason();
    uint8_t associationFailureCause =
        chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCause::kUnknown);

    switch (reason)
    {
    case WIFI_REASON_ASSOC_TOOMANY:
    case WIFI_REASON_NOT_ASSOCED:
    case WIFI_REASON_ASSOC_NOT_AUTHED:
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_GROUP_CIPHER_INVALID:
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
    case WIFI_REASON_AKMP_INVALID:
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCause::kAssociationFailed);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, reason);
        }
        break;
    case WIFI_REASON_NOT_AUTHED:
    case WIFI_REASON_MIC_FAILURE:
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
    case WIFI_REASON_INVALID_RSN_IE_CAP:
    case WIFI_REASON_INVALID_PMKID:
    case WIFI_REASON_802_1X_AUTH_FAILED:
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCause::kAuthenticationFailed);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, reason);
        }
        break;
    case WIFI_REASON_NO_AP_FOUND:
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCause::kSsidNotFound);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, reason);
        }
    case WIFI_REASON_BEACON_TIMEOUT:
    case WIFI_REASON_AUTH_EXPIRE:
    case WIFI_REASON_AUTH_LEAVE:
    case WIFI_REASON_ASSOC_LEAVE:
    case WIFI_REASON_ASSOC_EXPIRE:
        break;

    default:
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, reason);
        }
        break;
    }

    if (delegate)
    {
        delegate->OnDisconnectionDetected(reason);
        delegate->OnConnectionStatusChanged(
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::WiFiConnectionStatus::kNotConnected));
    }

    UpdateInternetConnectivityState();
}

void MatterWiFi::UpdateInternetConnectivityState(void)
{
    bool haveIPv4Conn      = false;
    bool haveIPv6Conn      = false;
    const bool hadIPv4Conn = FlagHaveIPv4Conn;
    const bool hadIPv6Conn = FlagHaveIPv6Conn;
    IPAddress addr;

    // If the WiFi station is currently in the connected state...
    if (mWiFiStationState == ConnectivityManager::kWiFiStationState_Connected)
    {
        // Get the LwIP netif for the WiFi station interface.
        struct netif * netif = ESP32Utils::GetStationNetif();

        // If the WiFi station interface is up...
        if (netif != NULL && netif_is_up(netif) && netif_is_link_up(netif))
        {
            // Check if a DNS server is currently configured.  If so...
            ip_addr_t dnsServerAddr = *dns_getserver(0);
            if (!ip_addr_isany_val(dnsServerAddr))
            {
                // If the station interface has been assigned an IPv4 address, and has
                // an IPv4 gateway, then presume that the device has IPv4 Internet
                // connectivity.
                if (!ip4_addr_isany_val(*netif_ip4_addr(netif)) && !ip4_addr_isany_val(*netif_ip4_gw(netif)))
                {
                    haveIPv4Conn = true;

                    esp_netif_ip_info_t ipInfo;
                    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ipInfo) == ESP_OK)
                    {
                        char addrStr[INET_ADDRSTRLEN];
                        // ToDo: change the code to using IPv6 address
                        esp_ip4addr_ntoa(&ipInfo.ip, addrStr, sizeof(addrStr));
                        IPAddress::FromString(addrStr, addr);
                    }
                }

                // Search among the IPv6 addresses assigned to the interface for a Global Unicast
                // address (2000::/3) that is in the valid state.  If such an address is found...
                for (uint8_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
                {
                    if (ip6_addr_isglobal(netif_ip6_addr(netif, i)) && ip6_addr_isvalid(netif_ip6_addr_state(netif, i)))
                    {
                        // Determine if there is a default IPv6 router that is currently reachable
                        // via the station interface.  If so, presume for now that the device has
                        // IPv6 connectivity.
                        struct netif * found_if = nd6_find_route(IP6_ADDR_ANY6);
                        if (found_if && netif->num == found_if->num)
                        {
                            haveIPv6Conn = true;
                        }
                    }
                }
            }
        }
    }

    // If the internet connectivity state has changed...
    if (haveIPv4Conn != hadIPv4Conn || haveIPv6Conn != hadIPv6Conn)
    {
        // Update the current state.
        FlagHaveIPv4Conn = haveIPv4Conn;
        FlagHaveIPv6Conn = haveIPv6Conn;
        
        // Alert other components of the state change.
        ChipDeviceEvent event;
        event.Type                                 = DeviceEventType::kInternetConnectivityChange;
        event.InternetConnectivityChange.IPv4      = GetConnectivityChange(hadIPv4Conn, haveIPv4Conn);
        event.InternetConnectivityChange.IPv6      = GetConnectivityChange(hadIPv6Conn, haveIPv6Conn);
        event.InternetConnectivityChange.ipAddress = addr;

        PlatformMgr().PostEventOrDie(&event);

        if (haveIPv4Conn != hadIPv4Conn)
        {
            ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv4", (haveIPv4Conn) ? "ESTABLISHED" : "LOST");
        }

        if (haveIPv6Conn != hadIPv6Conn)
        {
            ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv6", (haveIPv6Conn) ? "ESTABLISHED" : "LOST");
        }
    }
}
