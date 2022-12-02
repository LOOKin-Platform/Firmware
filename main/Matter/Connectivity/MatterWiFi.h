#ifndef MATTER_CONNECTIVITY_WIFI
#define MATTER_CONNECTIVITY_WIFI

#include <app-common/zap-generated/cluster-objects.h>
#include <app/AttributeAccessInterface.h>
#include <app/CommandHandlerInterface.h>
#include <app/data-model/Nullable.h>
#include <lib/support/ThreadOperationalDataset.h>
#include <lib/support/Variant.h>
#include <platform/NetworkCommissioning.h>
#include <platform/PlatformManager.h>
#include <platform/internal/DeviceNetworkInfo.h>
#include <platform/ConnectivityManager.h>
#include <lib/support/BitFlags.h>

#include <lib/support/Span.h>

using namespace ::chip::DeviceLayer;

using MyStatus                  = chip::DeviceLayer::NetworkCommissioning::Status;
using MyScanResponseIterator    = chip::DeviceLayer::NetworkCommissioning::WiFiScanResponseIterator;

class MatterWiFi : public chip::DeviceLayer::NetworkCommissioning::WiFiDriver::ScanCallback 
//chip::DeviceLayer::NetworkCommissioning::Internal::WirelessDriver::WiFiDriver::ScanCallback
{
    public:
        void                OnFinished(MyStatus status, chip::CharSpan debugText, MyScanResponseIterator * NetworksIterator) override;
        
        static void         OnScanDone();

        static void         OnSTAStart();
        static void         OnSTAStop();
        static void         OnSTAConnected();
        static void         OnSTADisconnected();

        static void         OnAPStart();
        static void         OnAPStop();
        static void         OnAPSTADisconnected();

        static void         STAGotIP(const ip_event_got_ip_t & got_ip_event);
        static void         STAGotIPv6(const ip_event_got_ip6_t & got_ip_event);
        static void         STALostIP();

    private:
        static inline ConnectivityManager::WiFiStationMode     mWiFiStationMode = ConnectivityManager::kWiFiStationMode_NotSupported;
        static inline ConnectivityManager::WiFiStationState    mWiFiStationState= ConnectivityManager::kWiFiStationState_NotConnected;

        static inline ConnectivityManager::WiFiAPMode          mWiFiAPMode      = ConnectivityManager::kWiFiAPMode_NotSupported;
        static inline ConnectivityManager::WiFiAPState         mWiFiAPState     = ConnectivityManager::kWiFiAPState_NotActive ;

        static inline bool FlagHaveIPv4Conn = false;
        static inline bool FlagHaveIPv6Conn = false;

        static void         OnStationIPv4AddressAvailable(const ip_event_got_ip_t & got_ip);
        static void         OnIPv6AddressAvailable(const ip_event_got_ip6_t & got_ip);
        static void         OnStationDisconnected();

        static void         DriveStationState();
        static void         OnStationConnected();
        static void         OnStationIPv4AddressLost(void);

        static void         ChangeWiFiStationState(ConnectivityManager::WiFiStationState newState);
        static void         ChangeWiFiAPState(ConnectivityManager::WiFiAPState newState);

        static void         DriveAPState();
        static void         DriveAPState(::chip::System::Layer * aLayer, void * aAppState);

        static void         _MaintainOnDemandWiFiAP(void);
        static void         _SetWiFiAPMode(ConnectivityManager::WiFiAPMode val);

        static void         UpdateInternetConnectivityState(void);



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
        static void         DriveStationState(::chip::System::Layer * aLayer, void * aAppState);

        //static void             ChangeWiFiStationState(ConnectivityManager::WiFiStationState newState);
};

#endif