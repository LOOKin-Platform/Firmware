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

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <lwip/nd6.h>
#include <lwip/netif.h>

#include "ConnectivityManagerWiFi.h"
#include "WiFi.h"

extern WiFi_t WiFi;

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::TLV;
using chip::DeviceLayer::Internal::ESP32Utils;

using namespace ::chip::DeviceLayer;

/*
ConnectivityManager::WiFiStationMode ConnectivityManagerWiFi::_GetWiFiStationMode(void)
{
    if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled)
    {
        wifi_mode_t curWiFiMode;
        mWiFiStationMode =
            (esp_wifi_get_mode(&curWiFiMode) == ESP_OK && (curWiFiMode == WIFI_MODE_APSTA || curWiFiMode == WIFI_MODE_STA))
            ? kWiFiStationMode_Enabled
            : kWiFiStationMode_Disabled;
    }
    return mWiFiStationMode;
}
*/

/*
CHIP_ERROR ConnectivityManagerWiFi::_SetWiFiStationMode(ConnectivityManager::WiFiStationMode val)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrExit(val != kWiFiStationMode_NotSupported, err = CHIP_ERROR_INVALID_ARGUMENT);

    if (val != kWiFiStationMode_ApplicationControlled)
    {
        bool autoConnect = (val == kWiFiStationMode_Enabled);
        err              = Internal::ESP32Utils::SetAPMode(autoConnect);
        SuccessOrExit(err);

        DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
    }

    if (mWiFiStationMode != val)
    {
        ChipLogProgress(DeviceLayer, "WiFi station mode change: %s -> %s", WiFiStationModeToStr(mWiFiStationMode),
                        WiFiStationModeToStr(val));
    }

    mWiFiStationMode = val;

exit:
    return err;
}
*/

bool ConnectivityManagerWiFi::_IsWiFiStationProvisioned(void)
{
    return ESP32Utils::IsStationProvisioned();
}

/*
void ConnectivityManagerWiFi::_ClearWiFiStationProvision(void)
{
    if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled)
    {
        wifi_config_t stationConfig;

        memset(&stationConfig, 0, sizeof(stationConfig));
        esp_wifi_set_config(WIFI_IF_STA, &stationConfig);

        DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP
        DeviceLayer::SystemLayer().ScheduleWork(DriveAPState, NULL);
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP
    }
}
*/
#define WIFI_BAND_2_4GHZ 2400
#define WIFI_BAND_5_0GHZ 5000

static uint16_t Map2400MHz(const uint8_t inChannel)
{
    uint16_t frequency = 0;

    if (inChannel >= 1 && inChannel <= 13)
    {
        // Cast is OK because we definitely fit in 16 bits.
        frequency = static_cast<uint16_t>(2412 + ((inChannel - 1) * 5));
    }
    else if (inChannel == 14)
    {
        frequency = 2484;
    }

    return frequency;
}

static uint16_t Map5000MHz(const uint8_t inChannel)
{
    uint16_t frequency = 0;

    switch (inChannel)
    {

    case 183:
        frequency = 4915;
        break;
    case 184:
        frequency = 4920;
        break;
    case 185:
        frequency = 4925;
        break;
    case 187:
        frequency = 4935;
        break;
    case 188:
        frequency = 4940;
        break;
    case 189:
        frequency = 4945;
        break;
    case 192:
        frequency = 4960;
        break;
    case 196:
        frequency = 4980;
        break;
    case 7:
        frequency = 5035;
        break;
    case 8:
        frequency = 5040;
        break;
    case 9:
        frequency = 5045;
        break;
    case 11:
        frequency = 5055;
        break;
    case 12:
        frequency = 5060;
        break;
    case 16:
        frequency = 5080;
        break;
    case 34:
        frequency = 5170;
        break;
    case 36:
        frequency = 5180;
        break;
    case 38:
        frequency = 5190;
        break;
    case 40:
        frequency = 5200;
        break;
    case 42:
        frequency = 5210;
        break;
    case 44:
        frequency = 5220;
        break;
    case 46:
        frequency = 5230;
        break;
    case 48:
        frequency = 5240;
        break;
    case 52:
        frequency = 5260;
        break;
    case 56:
        frequency = 5280;
        break;
    case 60:
        frequency = 5300;
        break;
    case 64:
        frequency = 5320;
        break;
    case 100:
        frequency = 5500;
        break;
    case 104:
        frequency = 5520;
        break;
    case 108:
        frequency = 5540;
        break;
    case 112:
        frequency = 5560;
        break;
    case 116:
        frequency = 5580;
        break;
    case 120:
        frequency = 5600;
        break;
    case 124:
        frequency = 5620;
        break;
    case 128:
        frequency = 5640;
        break;
    case 132:
        frequency = 5660;
        break;
    case 136:
        frequency = 5680;
        break;
    case 140:
        frequency = 5700;
        break;
    case 149:
        frequency = 5745;
        break;
    case 153:
        frequency = 5765;
        break;
    case 157:
        frequency = 5785;
        break;
    case 161:
        frequency = 5805;
        break;
    case 165:
        frequency = 5825;
        break;
    }

    return frequency;
}

static uint16_t MapFrequency(const uint16_t inBand, const uint8_t inChannel)
{
    uint16_t frequency = 0;

    if (inBand == WIFI_BAND_2_4GHZ)
    {
        frequency = Map2400MHz(inChannel);
    }
    else if (inBand == WIFI_BAND_5_0GHZ)
    {
        frequency = Map5000MHz(inChannel);
    }

    return frequency;
}

/*
CHIP_ERROR ConnectivityManagerWiFi::InitWiFi()
{
    mLastStationConnectFailTime   = System::Clock::kZero;
    mWiFiStationMode              = kWiFiStationMode_Disabled;
    mWiFiStationState             = kWiFiStationState_NotConnected;
    mWiFiStationReconnectInterval = System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL);

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP
    mLastAPDemandTime  = System::Clock::kZero;
    mWiFiAPMode        = kWiFiAPMode_Disabled;
    mWiFiAPState       = kWiFiAPState_NotActive;
    mWiFiAPIdleTimeout = System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_WIFI_AP_IDLE_TIMEOUT);
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP

    mFlags.SetRaw(0);

    // TODO Initialize the Chip Addressing and Routing Module.

    // Ensure that ESP station mode is enabled.
    ReturnErrorOnFailure(Internal::ESP32Utils::EnableStationMode());

    // If there is no persistent station provision...
    if (!IsWiFiStationProvisioned())
    {
        // If the code has been compiled with a default WiFi station provision, configure that now.
        if (CONFIG_DEFAULT_WIFI_SSID[0] != 0)
        {
            ChipLogProgress(DeviceLayer, "Setting default WiFi station configuration (SSID: %s)", CONFIG_DEFAULT_WIFI_SSID);

            // Set a default station configuration.
            wifi_config_t wifiConfig;
            memset(&wifiConfig, 0, sizeof(wifiConfig));
            memcpy(wifiConfig.sta.ssid, CONFIG_DEFAULT_WIFI_SSID,
                   std::min(sizeof(wifiConfig.sta.ssid), strlen(CONFIG_DEFAULT_WIFI_SSID)));
            memcpy(wifiConfig.sta.password, CONFIG_DEFAULT_WIFI_PASSWORD,
                   std::min(sizeof(wifiConfig.sta.password), strlen(CONFIG_DEFAULT_WIFI_PASSWORD)));
            wifiConfig.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
            wifiConfig.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
            esp_err_t err              = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
            if (err != ESP_OK)
            {
                ChipLogError(DeviceLayer, "esp_wifi_set_config() failed: %s", esp_err_to_name(err));
            }

            // Enable WiFi station mode.
            ReturnErrorOnFailure(SetWiFiStationMode(kWiFiStationMode_Enabled));
        }

        // Otherwise, ensure WiFi station mode is disabled.
        else
        {
            ReturnErrorOnFailure(SetWiFiStationMode(kWiFiStationMode_Disabled));
        }
    }

    // Force AP mode off for now.
    ReturnErrorOnFailure(Internal::ESP32Utils::SetAPMode(false));

    // Queue work items to bootstrap the AP and station state machines once the Chip event loop is running.
    ReturnErrorOnFailure(DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL));

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP
    ReturnErrorOnFailure(DeviceLayer::SystemLayer().ScheduleWork(DriveAPState, NULL));
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_AP

    return CHIP_NO_ERROR;
}
*/

/*
void ConnectivityManagerWiFi::_OnWiFiStationProvisionChange()
{
    // Schedule a call to the DriveStationState method to adjust the station state as needed.
    DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
}
*/
/*
void ConnectivityManagerWiFi::DriveStationState()
{
    bool stationConnected;

    // Refresh the current station mode.  Specifically, this reads the ESP auto_connect flag,
    // which determine whether the WiFi station mode is kWiFiStationMode_Enabled or
    // kWiFiStationMode_Disabled.
    GetWiFiStationMode();

    // If the station interface is NOT under application control...
    if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled)
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
        if (mWiFiStationState == kWiFiStationState_NotConnected || mWiFiStationState == kWiFiStationState_Connecting_Succeeded)
        {
            ChangeWiFiStationState(kWiFiStationState_Connected);
            ChipLogProgress(DeviceLayer, "WiFi station interface connected");
            mLastStationConnectFailTime = System::Clock::kZero;
            OnStationConnected();
        }

        // If the WiFi station interface is no longer enabled, or no longer provisioned,
        // disconnect the station from the AP, unless the WiFi station mode is currently
        // under application control.
        if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled &&
            (mWiFiStationMode != kWiFiStationMode_Enabled || !IsWiFiStationProvisioned()))
        {
            ChipLogProgress(DeviceLayer, "Disconnecting WiFi station interface");
            esp_err_t err = esp_wifi_disconnect();
            if (err != ESP_OK)
            {
                ChipLogError(DeviceLayer, "esp_wifi_disconnect() failed: %s", esp_err_to_name(err));
                return;
            }

            ChangeWiFiStationState(kWiFiStationState_Disconnecting);
        }
    }

    // Otherwise the station interface is NOT connected to an AP, so...
    else
    {
        System::Clock::Timestamp now = System::SystemClock().GetMonotonicTimestamp();

        // Advance the station state to NotConnected if it was previously Connected or Disconnecting,
        // or if a previous initiated connect attempt failed.
        if (mWiFiStationState == kWiFiStationState_Connected || mWiFiStationState == kWiFiStationState_Disconnecting ||
            mWiFiStationState == kWiFiStationState_Connecting_Failed)
        {
            WiFiStationState prevState = mWiFiStationState;
            ChangeWiFiStationState(kWiFiStationState_NotConnected);
            if (prevState != kWiFiStationState_Connecting_Failed)
            {
                ChipLogProgress(DeviceLayer, "WiFi station interface disconnected");
                mLastStationConnectFailTime = System::Clock::kZero;
                OnStationDisconnected();
            }
            else
            {
                mLastStationConnectFailTime = now;
            }
        }

        // If the WiFi station interface is now enabled and provisioned (and by implication,
        // not presently under application control), AND the system is not in the process of
        // scanning, then...
        if (mWiFiStationMode == kWiFiStationMode_Enabled && IsWiFiStationProvisioned())
        {
            // Initiate a connection to the AP if we haven't done so before, or if enough
            // time has passed since the last attempt.
            if (mLastStationConnectFailTime == System::Clock::kZero ||
                now >= mLastStationConnectFailTime + mWiFiStationReconnectInterval)
            {
                ChipLogProgress(DeviceLayer, "Attempting to connect WiFi station interface");
                esp_err_t err = esp_wifi_connect();
                if (err != ESP_OK)
                {
                    ChipLogError(DeviceLayer, "esp_wifi_connect() failed: %s", esp_err_to_name(err));
                    return;
                }

                ChangeWiFiStationState(kWiFiStationState_Connecting);
            }

            // Otherwise arrange another connection attempt at a suitable point in the future.
            else
            {
                System::Clock::Timeout timeToNextConnect = (mLastStationConnectFailTime + mWiFiStationReconnectInterval) - now;

                ChipLogProgress(DeviceLayer, "Next WiFi station reconnect in %" PRIu32 " ms",
                                System::Clock::Milliseconds32(timeToNextConnect).count());

                ReturnOnFailure(DeviceLayer::SystemLayer().StartTimer(timeToNextConnect, DriveStationState, NULL));
            }
        }
    }

    ChipLogProgress(DeviceLayer, "Done driving station state, nothing else to do...");
    // Kick-off any pending network scan that might have been deferred due to the activity
    // of the WiFi station.
}
*/
void ConnectivityManagerWiFi::OnStationConnected()
{
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

/*
void ConnectivityManagerWiFi::ChangeWiFiStationState(WiFiStationState newState)
{
    if (mWiFiStationState != newState)
    {
        ChipLogProgress(DeviceLayer, "WiFi station state change: %s -> %s", WiFiStationStateToStr(mWiFiStationState),
                        WiFiStationStateToStr(newState));
        mWiFiStationState = newState;
        SystemLayer().ScheduleLambda([]() { NetworkCommissioning::ESPWiFiDriver::GetInstance().OnNetworkStatusChange(); });
    }
}
*/

/*
void ConnectivityManagerWiFi::DriveStationState(::chip::System::Layer * aLayer, void * aAppState)
{
    sInstance.DriveStationState();
}
*/

void ConnectivityManagerWiFi::UpdateInternetConnectivityState(void)
{
    bool haveIPv4Conn      = false;
    bool haveIPv6Conn      = false;
    const bool hadIPv4Conn = FlagHaveIPv4Conn;
    const bool hadIPv6Conn = FlagHaveIPv6Conn;
    IPAddress addr;

    // If the WiFi station is currently in the connected state...
    if (WiFi.GetConnectionStatus() == ESP_OK)
    {
        // Get the LwIP netif for the WiFi station interface.
        struct netif * netif = ESP32Utils::GetStationNetif(); //!WiFi.GetNetIf();//! 

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

void ConnectivityManagerWiFi::OnStationIPv4AddressAvailable(const ip_event_got_ip_t & got_ip) {
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

void ConnectivityManagerWiFi::OnStationIPAddressLost(void)
{
    ChipLogProgress(DeviceLayer, "IPv4 address lost on WiFi station interface");

    UpdateInternetConnectivityState();

    ChipDeviceEvent event;
    event.Type                           = DeviceEventType::kInterfaceIpAddressChanged;
    event.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV4_Lost;
    PlatformMgr().PostEventOrDie(&event);
}

void ConnectivityManagerWiFi::OnStationIPv6AddressAvailable(const ip_event_got_ip6_t & got_ip)
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