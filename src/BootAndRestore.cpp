/*
*    BootAndRestore.h
*    Class to handle device boots and restores
*
*/

#include "BootAndRestore.h"
#include "HomeKit.h"
#include "Memory.h"
#include "Settings.h"

bool BootAndRestore::IsDeviceFullyStarted = false;

TaskHandle_t	BootAndRestore::DelayedOperationTaskHandler		= NULL;
TaskHandle_t	BootAndRestore::MarkDeviceStartedTaskHandler	= NULL;

void BootAndRestore::OnDeviceStart() {
	NVS Memory(NVSBootAndRestoreArea);
	FirmwareVersion OldFirmware = FirmwareVersion(Memory.GetUInt32Bit(NVSBootAndRestoreAreaFirmware));

	if (OldFirmware != Settings.Firmware) {
		Memory.SetUInt32Bit(NVSBootAndRestoreAreaFirmware, Settings.Firmware.Version.Uint32Data);
		Migration(OldFirmware.ToString(), Settings.Firmware.ToString());
	}

	if (Log::VerifyLastBoot()) {
		Memory.SetInt8Bit(NVSBootAndRestoreAreaOnAttempts, 0);
	}
	else if (!(Device.PowerMode == DevicePowerMode::BATTERY && Settings.eFuse.Type == Settings.Devices.Remote))
	{
		uint8_t InvalidStartAttempts = Memory.GetInt8Bit(NVSBootAndRestoreAreaOnAttempts);
		ESP_LOGI("InvalidStartAttempts", "%d / %d", InvalidStartAttempts, Settings.BootAndRestore.AttemptsToReset);

		if (InvalidStartAttempts > Settings.BootAndRestore.AttemptsToRevert * 2)
			InvalidStartAttempts = Settings.BootAndRestore.AttemptsToRevert * 2;

		if (InvalidStartAttempts <= Settings.BootAndRestore.AttemptsToReset)
			Memory.SetInt8Bit(NVSBootAndRestoreAreaOnAttempts, InvalidStartAttempts + 1);
		else
			RunOperation(HARDRESET_ON_START, false);
	}

	Log::Add(Log::Events::System::DeviceOn);

	/*
		Log::Add(Log::Events::System::DeviceRollback);
		ESP_LOGE(tag, "Failed start, device rollback started");
		OTA::Rollback();
	 */
}

void BootAndRestore::MarkDeviceStartedWithDelay(uint16_t Delay) {
	if (IsDeviceFullyStarted) return;

	if (MarkDeviceStartedTaskHandler == NULL)
		MarkDeviceStartedTaskHandler = FreeRTOS::StartTask(BootAndRestore::MarkDeviceStartedTask, "MarkDeviceStarted", (void *)(uint32_t)Delay, 3072, 5);
}


void BootAndRestore::Reboot(bool IsDelayed) {
	RunOperation(OperationTypeEnum::REBOOT, IsDelayed);
}

void BootAndRestore::HardReset(bool IsDelayed) {
	RunOperation(OperationTypeEnum::HARDRESET, IsDelayed);
}

void BootAndRestore::HardResetFromInit() {
	RunOperation(OperationTypeEnum::HARDRESET_ON_START, false);
}


void BootAndRestore::RunOperation(OperationTypeEnum Operation, bool IsDelayed) {
	if (IsDelayed)
		ExecuteOperationDelayed(Operation);
	else
		ExecuteOperationNow(Operation);
}

void BootAndRestore::Migration(string OldFirmware, string NewFirmware) {
	if (OldFirmware == "" && NewFirmware == "2.10") {
		// removed NVSLogArray in 2.10 version prior to BLOB
		NVS Memory(NVSLogArea);
		Memory.EraseStartedWith(NVSLogArray);
	}

	// Set sensor mode on for all updated and new devices
	if (OldFirmware < "2.14" || OldFirmware == "")
	{
		NVS Memory(NVSDeviceArea);
		Memory.SetInt8Bit(NVSDeviceEco, 1);
		Memory.Commit();
	}

	// Set sensor mode on for all updated and new devices
	if (OldFirmware < "2.40" && Settings.DeviceGeneration > 1)
	{
		SPIFlash::EraseRange(0x8A0000, 0x450000);
	}
}

void BootAndRestore::ExecuteOperationNow(OperationTypeEnum Operation) {
	if (Operation == HARDRESET) {
		Wireless.StopInterfaces(true);
		Operation = HARDRESET_ON_START;
	}

	switch (Operation) {
		case REBOOT:
			if (HomeKit::IsEnabledForDevice())
				hap_reboot_accessory();
			else
				esp_restart();

			break;
		case HARDRESET_ON_START:
			if (Settings.DeviceGeneration == 1)
				SPIFlash::EraseRange(0x32000, 0xEE000);
			else
			{
				SPIFlash::EraseRange(0x670000, 0x990000);
				/*
				 * idfupdate - replace for this after ESP_PARTITION_TYPE_ANY will be avaliable
				PartitionAPI::ErasePartition("coredump");
				PartitionAPI::ErasePartition("misc");
				PartitionAPI::ErasePartition("dataitems");
				PartitionAPI::ErasePartition("scenarios");
				PartitionAPI::ErasePartition("scache");
				PartitionAPI::ErasePartition("storage");
				*/
			}
			//PartitionAPI::ErasePartition("constants");

			NVS::ClearAll();

			if (HomeKit::IsEnabledForDevice())
				HomeKit::ResetData();

			ExecuteOperationNow(REBOOT);

			break;
		default:
			break;
	}
}

void BootAndRestore::ExecuteOperationDelayed(OperationTypeEnum Operation) {
	if (DelayedOperationTaskHandler == NULL)
		DelayedOperationTaskHandler = FreeRTOS::StartTask(BootAndRestore::ExecuteOperationDelayedTask, "OperationDelayedTask", (void *)(uint32_t)Operation, 3072, 5);
}

void BootAndRestore::ExecuteOperationDelayedTask(void *Data) {
	FreeRTOS::Sleep(Settings.BootAndRestore.ActionsDelay);

	BootAndRestore::OperationTypeEnum Operation = static_cast<BootAndRestore::OperationTypeEnum>((uint32_t)Data);
	BootAndRestore::RunOperation(Operation, false);

	DelayedOperationTaskHandler = NULL;

	FreeRTOS::DeleteTask();
}

void BootAndRestore::MarkDeviceStartedTask(void *Data) {
	FreeRTOS::Sleep((uint32_t)Data);

	Log::Add(Log::Events::System::DeviceStarted);
	BootAndRestore::IsDeviceFullyStarted = true;

	MarkDeviceStartedTaskHandler = NULL;
	FreeRTOS::DeleteTask();
}
