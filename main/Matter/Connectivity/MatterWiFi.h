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

#include <lib/support/Span.h>

using MyStatus                  = chip::DeviceLayer::NetworkCommissioning::Status;
using MyScanResponseIterator    = chip::DeviceLayer::NetworkCommissioning::WiFiScanResponseIterator;

class AppScanCallback : public chip::DeviceLayer::NetworkCommissioning::WiFiDriver::ScanCallback 
//chip::DeviceLayer::NetworkCommissioning::Internal::WirelessDriver::WiFiDriver::ScanCallback
{
    public:
        void OnFinished(MyStatus status, chip::CharSpan debugText, MyScanResponseIterator * NetworksIterator) override;

/*
    static chip::DeviceLayer::NetworkCommissioning::WiFiDriver::ScanCallback & GetInstance()
    {
        static AppScanCallback instance;
        return (chip::DeviceLayer::NetworkCommissioning::WiFiDriver::ScanCallback)instance;
    }
*/
};

#endif