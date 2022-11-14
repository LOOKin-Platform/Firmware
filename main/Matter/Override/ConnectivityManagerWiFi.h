#ifndef MATTER_OVERRIDES_WIFI_HANDLER
#define MATTER_OVERRIDES_WIFI_HANDLER

#include <platform/ConnectivityManager.h>
#include <lib/support/BitFlags.h>

using namespace ::chip::DeviceLayer;

class ConnectivityManagerWiFi 
{
    public:
        static void OnStationIPv4AddressAvailable(const ip_event_got_ip_t & got_ip);
        static void OnStationIPv6AddressAvailable(const ip_event_got_ip6_t & got_ip);
        static void OnStationIPAddressLost(void);
        static void OnStationConnected();

    private:
        static inline bool FlagHaveIPv4Conn = false;
        static inline bool FlagHaveIPv6Conn = false;

        // static ConnectivityManager::WiFiStationMode  
        //                         _GetWiFiStationMode(void);
        static bool             _IsWiFiStationEnabled(void);
        //static CHIP_ERROR       _SetWiFiStationMode(ConnectivityManager::WiFiStationMode val);
        static bool             _IsWiFiStationProvisioned(void);
        //static void             _ClearWiFiStationProvision(void);
        static uint16_t         Map2400MHz(const uint8_t inChannel);
        static uint16_t         Map5000MHz(const uint8_t inChannel);
        static uint16_t         MapFrequency(const uint16_t inBand, const uint8_t inChannel);
        //static CHIP_ERROR       InitWiFi();
        //static void             _OnWiFiStationProvisionChange();
        //static void             DriveStationState();
        //static void             ChangeWiFiStationState(ConnectivityManager::WiFiStationState newState);
        //static void             DriveStationState(::chip::System::Layer * aLayer, void * aAppState);
        static void             UpdateInternetConnectivityState(void);
};

#endif