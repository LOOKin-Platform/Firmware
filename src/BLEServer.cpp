#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>

#include "HIDTypes.h"
#include <driver/adc.h>

#include "BLEServer.h"

#include "Device.h"
#include "WiFi.h"
#include "Network.h"
#include "RemoteControl.h"

extern Device_t 		Device;
extern WiFi_t			WiFi;
extern Network_t		Network;
extern RemoteControl_t	RemoteControl;


#include "esp_log.h"
static const char* Tag = "BLEDevice";

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),		0x01,			// USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),				0x06,			// USAGE (Keyboard)
  COLLECTION(1),		0x01,			// COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  REPORT_ID(1),			KEYBOARD_ID,	//   REPORT_ID (1)
  USAGE_PAGE(1),		0x07,			//   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),		0xE0,			//   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),		0xE7,			//   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1),	0x00,			//   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),	0x01,			//   Logical Maximum (1)
  REPORT_SIZE(1),		0x01,			//   REPORT_SIZE (1)
  REPORT_COUNT(1),		0x08,			//   REPORT_COUNT (8)
  INPUT(1),				0x02,			//   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),		0x01,			//   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),		0x08,			//   REPORT_SIZE (8)
  INPUT(1),				0x01,			//   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),		0x05,			//   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),		0x01,			//   REPORT_SIZE (1)
  USAGE_PAGE(1),		0x08,			//   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),		0x01,			//   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),		0x05,			//   USAGE_MAXIMUM (0x05) ; Kana
  OUTPUT(1),			0x02,			//   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),		0x01,			//   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),		0x03,			//   REPORT_SIZE (3)
  OUTPUT(1),			0x01,			//   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),		0x06,			//   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),		0x08,			//   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1),	0x00,			//   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1),	0x65,			//   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),		0x07,			//   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),		0x00,			//   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),		0x65,			//   USAGE_MAXIMUM (0x65)
  INPUT(1),				0x00,			//   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 	// END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),		0x0C,			// USAGE_PAGE (Consumer)
  USAGE(1),				0x01,			// USAGE (Consumer Control)
  COLLECTION(1),		0x01,			// COLLECTION (Application)
  REPORT_ID(1),			MEDIA_KEYS_ID,	//   REPORT_ID (3)
  USAGE_PAGE(1),		0x0C,			//   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1),	0x00,			//   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),	0x01,			//   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),		0x01,			//   REPORT_SIZE (1)
  REPORT_COUNT(1),		0x10,			//   REPORT_COUNT (16)
  USAGE(1),				0xB5,			//   USAGE (Scan Next Track)     ; bit 0: 1
  USAGE(1),				0xB6,			//   USAGE (Scan Previous Track) ; bit 1: 2
  USAGE(1),				0xB7,			//   USAGE (Stop)                ; bit 2: 4
  USAGE(1),				0xCD,			//   USAGE (Play/Pause)          ; bit 3: 8
  USAGE(1),				0xE2,			//   USAGE (Mute)                ; bit 4: 16
  USAGE(1),				0xE9,			//   USAGE (Volume Increment)    ; bit 5: 32
  USAGE(1),				0xEA,			//   USAGE (Volume Decrement)    ; bit 6: 64
  USAGE(2),				0x23, 0x02,		//   Usage (WWW Home)            ; bit 7: 128
  USAGE(2),				0x94, 0x01,		//   Usage (My Computer) ; bit 0: 1
  USAGE(2),				0x92, 0x01,		//   Usage (Calculator)  ; bit 1: 2
  USAGE(2),				0x2A, 0x02,		//   Usage (WWW fav)     ; bit 2: 4
  USAGE(2),				0x21, 0x02,		//   Usage (WWW search)  ; bit 3: 8
  USAGE(2),				0x26, 0x02,		//   Usage (WWW stop)    ; bit 4: 16
  USAGE(2),				0x24, 0x02,		//   Usage (WWW back)    ; bit 5: 32
  USAGE(2),				0x83, 0x01,		//   Usage (Media sel)   ; bit 6: 64
  USAGE(2),				0x8A, 0x01,		//   Usage (Mail)        ; bit 7: 128
  INPUT(1),				0x02,			//   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0)						// END_COLLECTION
};

bool BLEServer_t::IsHIDEnabledForDevice() {
	if (Settings.eFuse.Type == Settings.Devices.Remote)
		return true;

	return false;
}

void BLEServer_t::CheckHIDMode() {
	if (!IsHIDEnabledForDevice()) return;

	/*
	if (WiFi.IsConnectedSTA() && CurrentMode == BASIC) {
		ForceMode(HID);
		return;
	}

	if (!WiFi.IsConnectedSTA() && CurrentMode == HID) {
		ForceMode(BASIC);
		return;
	}
	*/
}

void BLEServer_t::ForceHIDMode(BLEServerModeEnum Mode) {
	if (!IsHIDEnabledForDevice()) return;

	if (CurrentMode == Mode) return;

	if (Mode == BASIC && pServer->getConnectedCount())
	{
		for (auto& Connection : pServer->getPeerDevices())
			pServer->disconnect(Connection);
	}

	ble_gatts_svc_set_visibility(HIDDevice->hidService()->getHandle(), (Mode == BASIC) ? 0 : 1);
	CurrentMode = Mode;
}



BLEServer_t::BLEServer_t(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
    : HIDDevice(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) {}

int8_t BLEServer_t::GetRSSIForConnection(uint16_t ConnectionHandle) {
	int8_t RSSI = 0;

	if (::ble_gap_conn_rssi(ConnectionHandle, &RSSI) == 0)
		return RSSI;
	else
		return -128;
}

void BLEServer_t::Init() {
	BLEDevice::init(Settings.Bluetooth.DeviceNamePrefix + Device.IDToString());

	if (pServer == NULL) {
		pServer = NimBLEDevice::createServer();
		pServer->setCallbacks(this);
	}

	if (ManufactorerCharacteristic == NULL) {
		ManufactorerCharacteristic = new NimBLECharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ);
		ManufactorerCharacteristic->setValue("LOOKin");
	}

	if (DeviceTypeCharacteristic == NULL) {
		DeviceTypeCharacteristic = new NimBLECharacteristic((uint16_t)0x2A24, NIMBLE_PROPERTY::READ);
		DeviceTypeCharacteristic->setValue(Device.Type.ToHexString());
	}

	if (DeviceIDCharacteristic == NULL) {
		DeviceIDCharacteristic = new NimBLECharacteristic((uint16_t)0x2A25, NIMBLE_PROPERTY::READ);
		DeviceIDCharacteristic->setValue(Converter::ToHexString(Settings.eFuse.DeviceID,8));
	}

	if (FirmwareVersionCharacteristic == NULL) {
		FirmwareVersionCharacteristic = new NimBLECharacteristic((uint16_t)0x2A26, NIMBLE_PROPERTY::READ);
		FirmwareVersionCharacteristic->setValue(Settings.Firmware.ToString());
	}

	if (DeviceModelCharacteristic == NULL) {
		DeviceModelCharacteristic = new NimBLECharacteristic((uint16_t)0x2A27, NIMBLE_PROPERTY::READ);
		DeviceModelCharacteristic->setValue(Converter::ToHexString(Settings.eFuse.Model,2));
	}

	if (WiFiSetupCharacteristic == NULL) {
		WiFiSetupCharacteristic = new NimBLECharacteristic((uint16_t)0x4000, NIMBLE_PROPERTY::WRITE);
		WiFiSetupCharacteristic->setCallbacks(this);
	}

	if (RCSetupCharacteristic == NULL) {
		RCSetupCharacteristic = new NimBLECharacteristic((uint16_t)0x5000, NIMBLE_PROPERTY::WRITE);
		RCSetupCharacteristic->setCallbacks(this);
	}
}

void BLEServer_t::StartAdvertising() {
	if (IsHIDEnabledForDevice())
		StartAdvertisingAsHID();
	else
		StartAdvertisingAsGenericDevice();
}


void BLEServer_t::StartAdvertisingAsHID()
{
	Init();

	HIDDevice 		= new BLEHIDDevice(pServer);
	inputKeyboard 	= HIDDevice->inputReport(KEYBOARD_ID);  // <-- input REPORTID from report map
	outputKeyboard 	= HIDDevice->outputReport(KEYBOARD_ID);
	inputMediaKeys 	= HIDDevice->inputReport(MEDIA_KEYS_ID);

	outputKeyboard->setCallbacks(this);

	HIDDevice->manufacturer()->setValue("LOOKin");
	HIDDevice->pnp(0x02, vid, pid, version);
	HIDDevice->hidInfo(0x00, 0x01);

	HIDDevice->deviceInfo()->addCharacteristic(DeviceTypeCharacteristic);
	HIDDevice->deviceInfo()->addCharacteristic(DeviceIDCharacteristic);
	HIDDevice->deviceInfo()->addCharacteristic(FirmwareVersionCharacteristic);
	HIDDevice->deviceInfo()->addCharacteristic(DeviceModelCharacteristic);
	HIDDevice->deviceInfo()->addCharacteristic(WiFiSetupCharacteristic);
	HIDDevice->deviceInfo()->addCharacteristic(RCSetupCharacteristic);

	BLEDevice::setSecurityAuth(true, true, true);

	HIDDevice->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
	HIDDevice->startServices();

	onStarted(pServer);

	advertising = pServer->getAdvertising();
	advertising->setAppearance(HID_KEYBOARD);
	advertising->addServiceUUID(HIDDevice->hidService()->getUUID());
	advertising->setScanResponse(false);

	advertising->setMaxInterval(0);
	advertising->setMinInterval(0);

	advertising->start();

	HIDDevice->setBatteryLevel(batteryLevel);

	CurrentMode = HID;

	ForceHIDMode(BASIC);
	ESP_LOGD(Tag, "Advertising started!");

	isRunning = true;
}

void BLEServer_t::StartAdvertisingAsGenericDevice()
{
	Init();

	BLEService *pService = pServer->createService(NimBLEUUID((uint16_t) 0x180A));

	pService->addCharacteristic(DeviceTypeCharacteristic);
	pService->addCharacteristic(DeviceIDCharacteristic);
	pService->addCharacteristic(FirmwareVersionCharacteristic);
	pService->addCharacteristic(DeviceModelCharacteristic);
	pService->addCharacteristic(WiFiSetupCharacteristic);
	pService->addCharacteristic(RCSetupCharacteristic);

	pService->start();

	// BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID((uint16_t)0x180A);
	pAdvertising->setScanResponse(true);

	pAdvertising->setMaxInterval(0);
	pAdvertising->setMinInterval(0);


	BLEDevice::startAdvertising();

	CurrentMode = BASIC;

	ESP_LOGD(Tag, "Advertising started!");

	isRunning = true;
}

void BLEServer_t::StopAdvertising()
{
	isRunning = false;
}

bool BLEServer_t::isConnected() {
  return this->connected;
}

bool BLEServer_t::IsRunning() {
	return isRunning;
}

void BLEServer_t::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (HIDDevice != 0)
    this->HIDDevice->setBatteryLevel(this->batteryLevel);
}

//must be called before begin in order to set the name
void BLEServer_t::SetName(std::string deviceName) {
  this->deviceName = deviceName;
}

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 *
 * @param ms Time in milliseconds
 */
void BLEServer_t::SetDelay(uint32_t ms) {
  this->_delay_ms = ms;
}

void BLEServer_t::set_vendor_id(uint16_t vid) {
	this->vid = vid;
}

void BLEServer_t::set_product_id(uint16_t pid) {
	this->pid = pid;
}

void BLEServer_t::set_version(uint16_t version) {
	this->version = version;
}

void BLEServer_t::SendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();

    // vTaskDelay(delayTicks);
    this->delay_ms(_delay_ms);
  }
}

void BLEServer_t::SendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputMediaKeys->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->inputMediaKeys->notify();

    //vTaskDelay(delayTicks);
    this->delay_ms(_delay_ms);
  }
}

#define SHIFT 0x80

const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US
	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // {
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};

uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BLEServer_t::Press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			ESP_LOGE(Tag, "!k");
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			ESP_LOGE(Tag, "i==6");
			return 0;
		}
	}
	SendReport(&_keyReport);
	return 1;
}

size_t BLEServer_t::Press(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

    mediaKeyReport_16 |= k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	SendReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BLEServer_t::Release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	SendReport(&_keyReport);
	return 1;
}

size_t BLEServer_t::Release(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
    mediaKeyReport_16 &= ~k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	SendReport(&_mediaKeyReport);
	return 1;
}

void BLEServer_t::ReleaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
	SendReport(&_keyReport);
}

size_t BLEServer_t::Write(uint8_t c)
{
	uint8_t p = Press(c);  // Keydown
	Release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BLEServer_t::Write(const MediaKeyReport c)
{
	uint16_t p = Press(c);  // Keydown
	Release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BLEServer_t::Write(const uint8_t *buffer, size_t size) {
	size_t n = 0;
	while (size--)
	{
		if (*buffer != '\r')
		{
			if (Write(*buffer))
			{
			  n++;
			}
			else
			{
			  break;
			}
		}
		buffer++;
	}
	return n;
}

void BLEServer_t::onConnect(NimBLEServer* pServer) {
	ESP_LOGE(Tag, "ISconnected");

	BLEDevice::startAdvertising();

	this->connected = true;
}

void BLEServer_t::onDisconnect(NimBLEServer* pServer) {
	this->connected = false;

	//BLEDevice::startAdvertising();
}

void BLEServer_t::onWrite(BLECharacteristic* me, ble_gap_conn_desc* desc) {
	if (((me->getUUID().toString() == "0x4000") || (me->getUUID().toString() == "0x5000")) && GetRSSIForConnection(desc->conn_handle) < Settings.Bluetooth.RSSILimit)
	{
		ESP_LOGE(Tag ,"RSSI so small: %d", GetRSSIForConnection(desc->conn_handle));
	}

	if (me->getUUID().toString() == "0x4000") // GATTDeviceWiFiCallback
	{
		string WiFiData = me->getValue();

		if (WiFiData.length() > 0)
		{
			JSON JSONItem(WiFiData);

			map<string,string> Params;

			ESP_LOGE(Tag, "Value %s", WiFiData.c_str());

			if (JSONItem.GetType() == JSON::RootType::Object)
				Params = JSONItem.GetItems();

			if (!(Params.count("s") && Params.count("p")))
			{
				vector<string> Parts = Converter::StringToVector(WiFiData," ");

				if (Parts.size() > 0) Params["s"] = Parts[0].c_str();
				if (Parts.size() > 1) Params["p"] = Parts[1].c_str();
			}

			if (!(Params.count("s") && Params.count("p")))
			{
				ESP_LOGE(Tag, "WiFi characteristics error input format");
			}
			else
			{
				ESP_LOGD(Tag, "WiFi Data received. SSID: %s, Password: %s", Params["s"].c_str(), Params["p"].c_str());
				Network.AddWiFiNetwork(Params["s"], Params["p"]);

				//If device in AP mode or can't connect as Station - try to connect with new data
				if ((WiFi.GetMode() == WIFI_MODE_STA_STR && WiFi.GetConnectionStatus() == UINT8_MAX) || (WiFi.GetMode() == WIFI_MODE_AP_STR))
					Network.WiFiConnect(Params["s"], true);
			}
		}
	}

	if (me->getUUID().toString() == "0x5000") // GATTDeviceMQTTCallback
	{

		//return os_mbuf_append(ctxt->om, Result.c_str(), Result.size()) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

		string MQTTData = me->getValue();

		ESP_LOGE(Tag, "%s", MQTTData.c_str());
		if (MQTTData.length() > 0)
		{
			vector<string> Parts = Converter::StringToVector(MQTTData," ");

			if (Parts.size() != 2) {
				ESP_LOGE(Tag, "MQTT credentials error input format");
			}
			else
			{
				ESP_LOGD(Tag, "MQTT credentials data received. ClientID: %s, ClientSecret: %s", Parts[0].c_str(), Parts[1].c_str());
				RemoteControl.ChangeOrSetCredentialsBLE(Parts[0], Parts[1]);

				FreeRTOS::Sleep(500);
				ForceHIDMode(HID);
			}
		}

	    return;
	}

	/*
	 *
	 *
	//me->getValue()

	uint8_t* value = (uint8_t*)(me->getValue().c_str());
	(void)value;

	ESP_LOGI(Tag, "special keys: %d", *value);
	 *
	 */
}

void BLEServer_t::delay_ms(uint64_t ms) {
  uint64_t m = esp_timer_get_time();
  if(ms){
    uint64_t e = (m + (ms * 1000));
    if(m > e){ //overflow
        while(esp_timer_get_time() > e) { }
    }
    while(esp_timer_get_time() < e) {}
  }
}

