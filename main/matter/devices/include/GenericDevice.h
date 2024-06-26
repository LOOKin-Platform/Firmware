#ifndef MATTER_DEVICES_GENERIC
#define MATTER_DEVICES_GENERIC

// These are the bridged devices
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>
#include <platform/CHIPDeviceLayer.h>
#include <lib/support/CHIPMemString.h>
#include <app/reporting/reporting.h>

#include <functional>
#include <typeinfo>
#include <string>

#include "esp_log.h"

using namespace std;
using namespace chip;
using namespace chip::Platform;

#define ZCL_ON_OFF_CLUSTER_REVISION (4u)

class MatterGenericDevice {
    public:
        enum DeviceTypeEnum {
            Undefined   = 0x0,
            Light,
            Thermostat,
            Temperature,
            Humidity,
            MediaPlayer 
        };

        static const int        kDeviceNameSize     = 32;
        static const int        kDeviceLocationSize = 32;

        bool      IsBridgedDevice                   = false;

        string    BridgedUUID                       = "";
        uint8_t   BridgedMisc                       = 0x0;

        chip::DataVersion       dataVersions[8]     = {0};

        uint16_t DynamicIndex                       = 0;

        enum Changed_t
        {
            kChanged_Reachable = 0x01,
            kChanged_State     = 0x02,
            kChanged_Location  = 0x04,
            kChanged_Name      = 0x08,
            kChanged_Last      = kChanged_Name,
        } Changed;

        MatterGenericDevice(string szDeviceName, string szLocation);

        bool IsReachable() const;
        void SetReachable(bool aReachable);

        void SetName(const char * szDeviceName);
        void SetLocation(const char * szLocation);

        inline void             SetEndpointID(chip::EndpointId id)  { mEndpointId = id; };
        inline chip::EndpointId GetEndpointID()                     { return mEndpointId; };
        inline char *           GetName()                           { return mName; };
        inline char *           GetLocation()                       { return mLocation; };
        inline DeviceTypeEnum   GetTypeName()                       { return DeviceType; };

        virtual Protocols::InteractionModel::Status   HandleReadAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Buffer, uint16_t maxReadLength)   { return Protocols::InteractionModel::Status::Success;   }
        virtual Protocols::InteractionModel::Status   HandleWriteAttribute(chip::ClusterId ClusterID, chip::AttributeId AttributeID, uint8_t * Value)                           { return Protocols::InteractionModel::Status::Success;   }

        virtual bool            GetOnOff()                          { ESP_LOGE("GetOnOff Generic", "Invoked"); return false;}
        virtual void            SetOnOff(bool Value)                { ESP_LOGE("SetOnOff Generic", "Invoked"); }

    protected:
        DeviceTypeEnum DeviceType = Undefined;

        char mLocation[kDeviceLocationSize];
        char mName[kDeviceNameSize];

        static void HandleDeviceStatusChanged(MatterGenericDevice * dev, MatterGenericDevice::Changed_t itemChangedMask);
        static void ScheduleReportingCallback(MatterGenericDevice * dev, chip::ClusterId cluster, chip::AttributeId attribute);

    /// <summary>
    /// Send IR Command to device
    /// </summary>
    /// <param name="operand"> string command to execute </param>
        void        MatterSendIRCommand(string operand);

    private:
        const char *Tag = "MatterDevice";

        bool mReachable;
        chip::EndpointId mEndpointId;

        virtual void HandleDeviceChange(MatterGenericDevice * device, MatterGenericDevice::Changed_t changeMask) = 0;

        static void CallReportingCallback(intptr_t closure);
};

#endif