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

void BootAndRestore::OnDeviceStart() {
	NVS Memory(NVSBootAndRestoreArea);
	FirmwareVersion OldFirmware = FirmwareVersion(Memory.GetUInt32Bit(NVSBootAndRestoreAreaFirmware));

	if (OldFirmware != Settings.Firmware) {
		Memory.SetUInt32Bit(NVSBootAndRestoreAreaFirmware, Settings.Firmware.Version.Uint32Data);
		Migration(OldFirmware.ToString(), Settings.Firmware.ToString());
	}

	if (Log::VerifyLastBoot()) {
		ESP_LOGE("Log::VerifyLastBoot", "true");
		Memory.SetInt8Bit(NVSBootAndRestoreAreaOnAttempts, 0);
	}
	else
	{
		ESP_LOGE("Log::VerifyLastBoot", "false");

		uint8_t InvalidStartAttempts = Memory.GetInt8Bit(NVSBootAndRestoreAreaOnAttempts);
		ESP_LOGE("InvalidStartAttempts", "%d", InvalidStartAttempts);
		ESP_LOGE("AttemptsToReset", "%d", Settings.BootAndRestore.AttemptsToReset);

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

	BootAndRestoreTaskDeviceOn* pTask = new BootAndRestoreTaskDeviceOn();
	pTask->SetStackSize(2048);
	pTask->Run((void *)(uint32_t)Delay);
}


void BootAndRestore::Reboot(bool IsDelayed) {
	RunOperation(OperationTypeEnum::REBOOT, IsDelayed);
}

void BootAndRestore::HardReset(bool IsDelayed) {
	RunOperation(OperationTypeEnum::HARDRESET, IsDelayed);
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
				PartitionAPI::ErasePartition("coredump");
				PartitionAPI::ErasePartition("misc");
				PartitionAPI::ErasePartition("dataitems");
				PartitionAPI::ErasePartition("scenarios");
				PartitionAPI::ErasePartition("scache");
				PartitionAPI::ErasePartition("storage");
			}
			//PartitionAPI::ErasePartition("constants");

			NVS::ClearAll();

			ExecuteOperationNow(REBOOT);

			break;
		default:
			break;
	}
}

void BootAndRestore::ExecuteOperationDelayed(OperationTypeEnum Operation) {
	BootAndRestoreTask* pTask = new BootAndRestoreTask();
	pTask->SetStackSize(3072);
	pTask->Run((void *)(uint32_t)Operation);
}

void BootAndRestoreTask::Run(void *Data) {
	BootAndRestore::OperationTypeEnum Operation = static_cast<BootAndRestore::OperationTypeEnum>((uint32_t)Data);

	FreeRTOS::Sleep(Settings.BootAndRestore.ActionsDelay);
	BootAndRestore::RunOperation(Operation, false);

    vTaskDelete(NULL);
}

void BootAndRestoreTaskDeviceOn::Run(void *Data) {
	FreeRTOS::Sleep((uint32_t)Data);
	Log::Add(Log::Events::System::DeviceStarted);
	BootAndRestore::IsDeviceFullyStarted = true;
    vTaskDelete(NULL);
}
