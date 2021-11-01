#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include "NimBLECharacteristic.h"
#include "NimBLEHIDDevice.h"
#define BLEDevice                  NimBLEDevice
#define BLEServerCallbacks         NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice               NimBLEHIDDevice
#define BLECharacteristic          NimBLECharacteristic
#define BLEAdvertising             NimBLEAdvertising

const uint8_t KEY_LEFT_CTRL = 0x80;
const uint8_t KEY_LEFT_SHIFT = 0x81;
const uint8_t KEY_LEFT_ALT = 0x82;
const uint8_t KEY_LEFT_GUI = 0x83;
const uint8_t KEY_RIGHT_CTRL = 0x84;
const uint8_t KEY_RIGHT_SHIFT = 0x85;
const uint8_t KEY_RIGHT_ALT = 0x86;
const uint8_t KEY_RIGHT_GUI = 0x87;

const uint8_t KEY_UP_ARROW = 0xDA;
const uint8_t KEY_DOWN_ARROW = 0xD9;
const uint8_t KEY_LEFT_ARROW = 0xD8;
const uint8_t KEY_RIGHT_ARROW = 0xD7;
const uint8_t KEY_BACKSPACE = 0xB2;
const uint8_t KEY_TAB = 0xB3;
const uint8_t KEY_RETURN = 0xB0;
const uint8_t KEY_ESC = 0xB1;
const uint8_t KEY_INSERT = 0xD1;
const uint8_t KEY_DELETE = 0xD4;
const uint8_t KEY_PAGE_UP = 0xD3;
const uint8_t KEY_PAGE_DOWN = 0xD6;
const uint8_t KEY_HOME = 0xD2;
const uint8_t KEY_END = 0xD5;
const uint8_t KEY_CAPS_LOCK = 0xC1;
const uint8_t KEY_F1 = 0xC2;
const uint8_t KEY_F2 = 0xC3;
const uint8_t KEY_F3 = 0xC4;
const uint8_t KEY_F4 = 0xC5;
const uint8_t KEY_F5 = 0xC6;
const uint8_t KEY_F6 = 0xC7;
const uint8_t KEY_F7 = 0xC8;
const uint8_t KEY_F8 = 0xC9;
const uint8_t KEY_F9 = 0xCA;
const uint8_t KEY_F10 = 0xCB;
const uint8_t KEY_F11 = 0xCC;
const uint8_t KEY_F12 = 0xCD;
const uint8_t KEY_F13 = 0xF0;
const uint8_t KEY_F14 = 0xF1;
const uint8_t KEY_F15 = 0xF2;
const uint8_t KEY_F16 = 0xF3;
const uint8_t KEY_F17 = 0xF4;
const uint8_t KEY_F18 = 0xF5;
const uint8_t KEY_F19 = 0xF6;
const uint8_t KEY_F20 = 0xF7;
const uint8_t KEY_F21 = 0xF8;
const uint8_t KEY_F22 = 0xF9;
const uint8_t KEY_F23 = 0xFA;
const uint8_t KEY_F24 = 0xFB;

typedef uint8_t MediaKeyReport[2];

#include <string>
using namespace std;

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

enum BLEServerModeEnum { OFF = 0, BASIC, HID };

class BLEServer_t : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
	private:
		BLEHIDDevice* 		HIDDevice;
		BLECharacteristic*	inputKeyboard;
		BLECharacteristic*	outputKeyboard;
		BLECharacteristic*	inputMediaKeys;
		BLEAdvertising*		advertising;
		KeyReport			_keyReport;
		MediaKeyReport		_mediaKeyReport;
		std::string			deviceName;
		std::string			deviceManufacturer;
		uint8_t				batteryLevel;
		bool				connected = false;
		uint32_t			_delay_ms = 7;
		bool				isRunning	= false;

		BLEServerModeEnum	CurrentMode = OFF;

		void 				delay_ms(uint64_t ms);

		uint16_t vid       = 0x05ac;
		uint16_t pid       = 0x820a;
		uint16_t version   = 0x0210;

		int8_t 				GetRSSIForConnection(uint16_t ConnectionHandle);

		NimBLEServer* 			pServer						= NULL;

		NimBLECharacteristic*	ManufactorerCharacteristic 	= NULL;
		NimBLECharacteristic* 	DeviceTypeCharacteristic 	= NULL;
		NimBLECharacteristic* 	DeviceIDCharacteristic 		= NULL;
		NimBLECharacteristic* 	FirmwareVersionCharacteristic = NULL;
		NimBLECharacteristic* 	DeviceModelCharacteristic 	= NULL;
		NimBLECharacteristic* 	WiFiSetupCharacteristic 	= NULL;
		NimBLECharacteristic* 	RCSetupCharacteristic 		= NULL;

		bool		IsHIDEnabledForDevice();

		void		Init();

	public:
		void		CheckHIDMode();
		void		ForceHIDMode(BLEServerModeEnum Mode);

		BLEServer_t(std::string deviceName = "ESP32 Keyboard", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);

		void 		StartAdvertising();
		void		StartAdvertisingAsHID();
		void 		StartAdvertisingAsGenericDevice();
		void 		StopAdvertising();

		void 		SendReport(KeyReport* keys);
		void 		SendReport(MediaKeyReport* keys);

		size_t 		Press(uint8_t k);
		size_t 		Press(const MediaKeyReport k);

		size_t 		Release(uint8_t k);
		size_t 		Release(const MediaKeyReport k);

		size_t 		Write(uint8_t c);
		size_t 		Write(const MediaKeyReport c);
		size_t 		Write(const uint8_t *buffer, size_t size);

		void 		ReleaseAll(void);
		bool 		isConnected();
		void 		setBatteryLevel(uint8_t level);
		void 		SetName(std::string deviceName);
		void 		SetDelay(uint32_t ms);

		void 		set_vendor_id(uint16_t vid);
		void 		set_product_id(uint16_t pid);
		void 		set_version(uint16_t version);

		bool		IsRunning();

		const MediaKeyReport KEY_MEDIA_NEXT_TRACK 			= {1	, 0};
		const MediaKeyReport KEY_MEDIA_PREVIOUS_TRACK 		= {2	, 0};
		const MediaKeyReport KEY_MEDIA_STOP 				= {4	, 0};
		const MediaKeyReport KEY_MEDIA_PLAY_PAUSE 			= {8	, 0};
		const MediaKeyReport KEY_MEDIA_MUTE 				= {16	, 0};
		const MediaKeyReport KEY_MEDIA_VOLUME_UP 			= {32	, 0};
		const MediaKeyReport KEY_MEDIA_VOLUME_DOWN 			= {64	, 0};
		const MediaKeyReport KEY_MEDIA_WWW_HOME 			= {128	, 0};
		const MediaKeyReport KEY_MEDIA_LOCAL_MACHINE_BROWSER= {0	, 1}; // Opens "My Computer" on Windows
		const MediaKeyReport KEY_MEDIA_CALCULATOR 			= {0	, 2};
		const MediaKeyReport KEY_MEDIA_WWW_BOOKMARKS 		= {0	, 4};
		const MediaKeyReport KEY_MEDIA_WWW_SEARCH 			= {0	, 8};
		const MediaKeyReport KEY_MEDIA_WWW_STOP 			= {0	, 16};
		const MediaKeyReport KEY_MEDIA_WWW_BACK 			= {0	, 32};
		const MediaKeyReport KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION = {0, 64}; // Media Selection
		const MediaKeyReport KEY_MEDIA_EMAIL_READER 		= {0	, 128};

	protected:
		virtual void onStarted(NimBLEServer *pServer) { };
		virtual void onConnect(NimBLEServer* pServer) override;
		virtual void onDisconnect(NimBLEServer* pServer) override;

		virtual void onWrite(BLECharacteristic* me, ble_gap_conn_desc* desc) override;
};



#endif // BLE_SERVER_H
