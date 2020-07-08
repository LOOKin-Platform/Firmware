// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// An example that implements the light bulb HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// This implementation is platform-independent.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The definitions for the HomeKit attribute database.
//
//   4. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   5. The initialization of the accessory state.
//
//   6. Callbacks that notify the server in case their associated value has changed.

#include "App.h"

/**
 * Domain used in the key value store for application data.
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * Key used in the key value store to store the configuration state.
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

HomeKitApp::AccessoryConfiguration HomeKitApp::BridgeAccessoryConfiguration = {0};

/**
 * HomeKit accessory that provides the Light Bulb service.
 *
 * Note: Not constant to enable BCT Manual Name Change.
 */

HAPAccessory HomeKitApp::BridgeAccessory =
	{
		.aid 				= 1,
        .category 			= kHAPAccessoryCategory_Bridges,
		.name 				= "Test Remote",
		.manufacturer 		= "LOOK.in",
		.model 				= "Remote",
		.serialNumber 		= "test",
		.firmwareVersion 	= "1",
		.hardwareVersion 	= "1",
		.services = (const HAPService* const[])
		{
			&accessoryInformationService,
			&hapProtocolInformationService,
			&pairingService,
//			&lightBulbService,
			NULL
		},
		.callbacks = { .identify = IdentifyAccessory }
	};

vector<HAPAccessory> HomeKitApp::Accessories = vector<HAPAccessory>();

/**
 * Load the accessory state from persistent memory.
 */
void HomeKitApp::LoadAccessoryState() {
    HAPPrecondition(BridgeAccessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            BridgeAccessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &BridgeAccessoryConfiguration.state,
            sizeof BridgeAccessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof BridgeAccessoryConfiguration.state) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&BridgeAccessoryConfiguration.state, sizeof BridgeAccessoryConfiguration.state);
    }
}

/**
 * Save the accessory state to persistent memory.
 */
void HomeKitApp::SaveAccessoryState() {
    HAPPrecondition(BridgeAccessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            BridgeAccessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &BridgeAccessoryConfiguration.state,
            sizeof BridgeAccessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}


HAP_RESULT_USE_CHECK HAPError HomeKitApp::IdentifyAccessory(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HomeKitApp::HandleLightBulbOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = BridgeAccessoryConfiguration.state.lightBulbOn;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HomeKitApp::HandleLightBulbOnWrite(HAPAccessoryServerRef* server, const HAPBoolCharacteristicWriteRequest* request,  bool value, void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");

	printf("Write!\n");

    if (BridgeAccessoryConfiguration.state.lightBulbOn != value) {
        BridgeAccessoryConfiguration.state.lightBulbOn = value;

        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}


void HomeKitApp::AccessoryNotification( const HAPAccessory* accessory, const HAPService* service, const HAPCharacteristic* characteristic, void* ctx) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(BridgeAccessoryConfiguration.server, characteristic, service, accessory);
}

void HomeKitApp::Create(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&BridgeAccessoryConfiguration, sizeof BridgeAccessoryConfiguration);
    BridgeAccessoryConfiguration.server = server;
    BridgeAccessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void HomeKitApp::Release() {

}

void HomeKitApp::AppAccessoryServerStart() {

	/*
	const HAPAccessory LightAccessory =
		{
			.aid = 1,
	        .category = kHAPAccessoryCategory_Lighting,
			.name = "Test Light Bulb",
			.manufacturer = "Acme",
			.model = "LightBulb1,1",
			.serialNumber = "099DB48E9E28",
			.firmwareVersion = "1",
			.hardwareVersion = "1",
			.services = (const HAPService* const[])
			{
				&accessoryInformationService,
				&hapProtocolInformationService,
				&pairingService,
				&lightBulbService,
				NULL
			},
			.callbacks = { .identify = IdentifyAccessory }
		};

	Accessories.push_back(LightAccessory);

	const HAPAccessory* AccessoryArray = &Accessories[0];
	 */



	static const HAPAccessory TestDevice = {
		.aid = 10,
		.category = kHAPAccessoryCategory_BridgedAccessory,
		.name = "qwerty",
		.manufacturer = "Acme",
		.model = "LightBulb1,1",
		.serialNumber = "099DB48E9E28",
		.firmwareVersion = "1",
		.hardwareVersion = "1",
		.services = (const HAPService* const[])
		{
			&accessoryInformationService,
			&hapProtocolInformationService,
			&pairingService,
			&lightBulbService,
			NULL
		},
		.callbacks = { .identify = IdentifyAccessory }
	};

	static const HAPAccessory TestDevice2 = {
		.aid = 11,
		.category = kHAPAccessoryCategory_BridgedAccessory,
		.name = "Qwerty2",
		.manufacturer = "Acme",
		.model = "LightBulb1,1",
		.serialNumber = "099DB48E9E28",
		.firmwareVersion = "1",
		.hardwareVersion = "1",
		.services = (const HAPService* const[])
		{
			&accessoryInformationService,
			&hapProtocolInformationService,
			&pairingService,
			&lightBulbService,
			NULL
		},
		.callbacks = { .identify = IdentifyAccessory }
	};

	static const HAPAccessory* const TestArray[] = { &TestDevice, &TestDevice2, 0 };// = { TestDevice};

	HAPAccessoryServerStartBridge(	BridgeAccessoryConfiguration.server,
									&BridgeAccessory,
									&TestArray[0],
									true);
    //HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

//----------------------------------------------------------------------------------------------------------------------

void HomeKitApp::AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!context);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

const HAPAccessory* HomeKitApp::AppGetAccessoryInfo() {
    return &BridgeAccessory;
}

void HomeKitApp::Initialize(HAPAccessoryServerOptions* hapAccessoryServerOptions, HAPPlatform* hapPlatform, HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks) {
    /*no-op*/
}

void HomeKitApp::Deinitialize() {
    /*no-op*/
}
