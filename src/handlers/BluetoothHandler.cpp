#ifndef BLUETOOTH_HANDLER
#define BLUETOOTH_HANDLER

class BluetoothPeriodicHandler {
	public:
		static void Pool();

	private:
		static bool		IsTimeSynced;
		static uint64_t BluetoothCountdown;		// used to find period in which turn on bluetooth to periodic info sending
		static uint64_t BluetoothStartedTime; 	// used to determine that bluetooth is on
		static uint32_t BatteryUptime;			// used to determine uptime on first start

};

bool 		BluetoothPeriodicHandler::IsTimeSynced 			= false;
uint64_t 	BluetoothPeriodicHandler::BluetoothCountdown 	= 0;
uint64_t 	BluetoothPeriodicHandler::BluetoothStartedTime 	= 0;
uint32_t 	BluetoothPeriodicHandler::BatteryUptime 		= 0;


void IRAM_ATTR BluetoothPeriodicHandler::Pool() {
	#if defined(CONFIG_BT_ENABLED)

	if (!Device.Type.IsBattery())
		return;

	if (BatteryUptime == 0)
		BatteryUptime = Settings.WiFi.BatteryUptime;

	if (Time::Uptime() < 5 && BluetoothCountdown == 0)
		BluetoothCountdown = Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first;

	if (!Time::IsUptime(Time::Unixtime()) && IsTimeSynced == false) {
		IsTimeSynced = true;

		uint32_t Expired = Time::Unixtime() % 3600;

		BluetoothCountdown = Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first -
							Expired % (Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first
							+ Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second);
	}

	if (Time::Unixtime() % 3600 <= Settings.Pooling.Interval)
		BluetoothCountdown = Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first;

	BluetoothCountdown -= Settings.Pooling.Interval;

	if (BluetoothCountdown <=0) {
		if (!BLE::IsRunning()) {
			ESP_LOGI("BLEHandler", "fired");

			BluetoothStartedTime = Time::UptimeU();
			BluetoothCountdown = Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].first
							+ Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second;

			BLEServer.StartAdvertising("0" +
					Converter::ToHexString(Settings.Wireless.IntervalID,1) +
					Converter::ToHexString(Automation.CurrentVersion(),4) +
					Converter::ToHexString(Storage.CurrentVersion(),3) +
					((Time::Offset == 0) ? "0" : "1") +
					Converter::ToHexString(Settings.eFuse.Type, 2) +
					(Device.Type.IsBattery() ? "0" : "1"));
		}
	}

	if (BLE::IsRunning()) {
		if (BluetoothStartedTime == 0)
			BluetoothStartedTime = Time::UptimeU();

		if (BatteryUptime > 0) {
			if (Time::UptimeU() >= BluetoothStartedTime + BatteryUptime * 1000) {
				ESP_LOGI("BluetoothHandler", "[1] Bluetooth uptime for battery device expired. Stopping bluetooth");
				BluetoothStartedTime = 0;
				BatteryUptime = 0;
				BLEServer.StopAdvertising();
			}
		}
		else {
			if (Time::UptimeU() >= BluetoothStartedTime + Settings.Wireless.AliveIntervals[Settings.Wireless.IntervalID].second * 1000000) {
				ESP_LOGI("BluetoothHandler", "[2] Bluetooth uptime for battery device expired. Stopping bluetooth");
				BluetoothStartedTime = 0;
				BatteryUptime = 0;
				BLEServer.StopAdvertising();
			}
		}
	}

	#endif /* Bluetooth enabled */
}

#endif
