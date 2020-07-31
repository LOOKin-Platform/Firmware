// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// This header file is platform-independent.

#ifndef HOMEKITAPP_H
#define HOMEKITAPP_H

#include "DB.h"
#include "HAP.h"

#include <esp_log.h>
#include <stdio.h>

#include <vector>

using namespace std;

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define MAX_BRIDGED_ACCESSORIES 10

class HomeKitApp {
	public:
		// Global accessory configuration
		typedef struct {
			struct {
				bool 	lightBulbOn;
				uint8_t FanActive;
			} state;
			HAPAccessoryServerRef* server;
			HAPPlatformKeyValueStoreRef keyValueStore;
		} AccessoryConfiguration;

		/**
		 * Identify routine. Used to locate the accessory.
		 */
		static HAP_RESULT_USE_CHECK
		HAPError IdentifyAccessory( HAPAccessoryServerRef* server, const HAPAccessoryIdentifyRequest* request, void* _Nullable context);


		static bool HandlePowerAction(uint16_t AccessoryID, bool Value);


		static HAP_RESULT_USE_CHECK HAPError HandleOnRead		(HAPAccessoryServerRef* server, const HAPBoolCharacteristicReadRequest* request, bool* value, void* _Nullable context);
		static HAP_RESULT_USE_CHECK HAPError HandleOnWrite		(HAPAccessoryServerRef* server, const HAPBoolCharacteristicWriteRequest* request, bool value, void* _Nullable context);

		static HAP_RESULT_USE_CHECK HAPError HandleActiveRead	(HAPAccessoryServerRef* server, const HAPUInt8CharacteristicReadRequest* request, uint8_t* value, void* _Nullable context);
		static HAP_RESULT_USE_CHECK HAPError HandleActiveWrite	(HAPAccessoryServerRef* server, const HAPUInt8CharacteristicWriteRequest* request, uint8_t value , void* _Nullable context);


		static void AccessoryNotification(const HAPAccessory*, const HAPService*, const HAPCharacteristic*, void* ctx);

		/**
		 * Initialize the application.
		 */
		static void Create(HAPAccessoryServerRef*, HAPPlatformKeyValueStoreRef);

		/**
		 * Deinitialize the application.
		 */
		static void Release();

		/**
		 * Start the accessory server for the app.
		 */
		static void AppAccessoryServerStart();
		static void AppAccessoryServerStop();

		/**
		 * Handle the updated state of the Accessory Server.
		 */
		static void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context);
		static void AccessoryServerHandleSessionAccept(HAPAccessoryServerRef* server, HAPSessionRef* session, void* _Nullable context);
		static void AccessoryServerHandleSessionInvalidate(HAPAccessoryServerRef* server, HAPSessionRef* session, void* _Nullable context);

		/**
		 * Restore platform specific factory settings.
		 */
		static void RestorePlatformFactorySettings(void);

		/**
		 * Returns pointer to accessory information
		 */
		static const HAPAccessory* AppGetAccessoryInfo();

		static void LoadAccessoryState();
		static void SaveAccessoryState();

		static void Initialize(HAPAccessoryServerOptions*, HAPPlatform*, HAPAccessoryServerCallbacks*);
		static void Deinitialize();

	private:
		static char AccessoryName[65];
		static char AccessoryModel[65];
		static char AccessorySerialNumber[9];
		static char AccessoryFirmwareVersion[8];
		static char AccessoryHardwareVersion[4];


		static HAPAccessory				BridgeAccessory;
		static AccessoryConfiguration	BridgeAccessoryConfiguration;
		static const HAPAccessory* 		BridgedAccessories[MAX_BRIDGED_ACCESSORIES];
		static uint8_t 					BridgedAccessoriesCount;
};

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif
