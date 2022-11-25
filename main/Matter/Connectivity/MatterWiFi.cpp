#include "Globals.h"

#include "Matter.h"
#include "MatterWiFi.h"
#include "WiFi.h"

#include <lib/support/Span.h>

#include "esp_log.h"
#include <vector>

using namespace std;

using MyStatus                  = chip::DeviceLayer::NetworkCommissioning::Status;
using MyScanResponseIterator    = chip::DeviceLayer::NetworkCommissioning::WiFiScanResponseIterator;

void AppScanCallback::OnFinished(MyStatus status, chip::CharSpan debugText, MyScanResponseIterator * NetworksIterator)
{
    ESP_LOGE("BUGAGA","!");

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