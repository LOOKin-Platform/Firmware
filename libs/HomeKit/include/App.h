// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// The most basic HomeKit example: an accessory that represents a light bulb that
// only supports switching the light on and off. Actions are exposed as individual
// functions below.
//
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

class HomeKitApp {
	public:
		// Global accessory configuration
		typedef struct {
			struct {
				bool lightBulbOn;
			} state;
			HAPAccessoryServerRef* server;
			HAPPlatformKeyValueStoreRef keyValueStore;
		} AccessoryConfiguration;

		/**
		 * Identify routine. Used to locate the accessory.
		 */
		static HAP_RESULT_USE_CHECK
		HAPError IdentifyAccessory( HAPAccessoryServerRef* server, const HAPAccessoryIdentifyRequest* request, void* _Nullable context);

		/**
		 * Handle read request to the 'On' characteristic of the Light Bulb service.
		 */
		static HAP_RESULT_USE_CHECK
		HAPError HandleLightBulbOnRead(HAPAccessoryServerRef* server, const HAPBoolCharacteristicReadRequest* request, bool* value, void* _Nullable context);

		/**
		 * Handle write request to the 'On' characteristic of the Light Bulb service.
		 */
		static HAP_RESULT_USE_CHECK
		HAPError HandleLightBulbOnWrite(HAPAccessoryServerRef* server, const HAPBoolCharacteristicWriteRequest* request, bool value, void* _Nullable context);

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

		static vector<HAPAccessory> Accessories;

	private:
		static HAPAccessory				BridgeAccessory;
		static AccessoryConfiguration	BridgeAccessoryConfiguration;
};

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif
