/*
*    BootAndRestore.h
*    Class to handle device boots and restores
*
*/

#include "BootAndRestore.h"
#include "Matter.h"
#include "Memory.h"
#include "Settings.h"

bool BootAndRestore::IsDeviceFullyStarted = false;

TaskHandle_t	BootAndRestore::DelayedOperationTaskHandler		= NULL;
TaskHandle_t	BootAndRestore::MarkDeviceStartedTaskHandler	= NULL;

static char Tag[] = "BootAndRestore";

void BootAndRestore::OnDeviceStart() {
	NVS Memory(NVSBootAndRestoreArea);
	FirmwareVersion OldFirmware = FirmwareVersion(Memory.GetUInt32Bit(NVSBootAndRestoreAreaFirmware));

	CheckPartitionTable();

	if (OldFirmware != Settings.Firmware) {
		Memory.SetUInt32Bit(NVSBootAndRestoreAreaFirmware, Settings.Firmware.Version.Uint32Data);
		Memory.Commit();
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

void BootAndRestore::CheckPartitionTable() {
	const esp_partition_t *Partition = ::esp_partition_find_first((esp_partition_type_t)0x82, ESP_PARTITION_SUBTYPE_ANY, "scenarios");
    if (Partition != NULL) {
    	if (Partition->subtype != 0x06)
    	{
        	ESP_LOGI("BootAndRestore", "Old partition found");

        	extern const uint8_t partitions_4mb_v2_start[] asm("_binary_partitions_4mb_v2_start");
        	extern const uint8_t partitions_4mb_v2_end[]   asm("_binary_partitions_4mb_v2_end");
        	extern const uint8_t partitions_16mb_v2_start[] asm("_binary_partitions_16mb_v2_start");
        	extern const uint8_t partitions_16mb_v2_end[]   asm("_binary_partitions_16mb_v2_end");

            const uint32_t PartitionSize4mb 	= (partitions_4mb_v2_end - partitions_4mb_v2_start);
            const uint32_t PartitionSize16mb 	= (partitions_16mb_v2_end - partitions_16mb_v2_start);

            SPIFlash::EraseRange(0x8000, 0xC00);

            esp_err_t err;

            if (Settings.DeviceGeneration < 2)
            	{ err = spi_flash_write(0x8000, partitions_4mb_v2_start, PartitionSize4mb);}
            else
            	{ err = spi_flash_write(0x8000, partitions_16mb_v2_start, PartitionSize16mb); }

        	if (err == ESP_OK)
            	Reboot(false);
        	else
        		ESP_LOGE("Partition write failed", "%s", Converter::ErrorToString(err));
    	}
    }

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

	// Set eco mode on for all updated and new devices
	if (OldFirmware < "2.41" && OldFirmware != "")
	{
		NVS Memory(NVSDeviceArea);
		Memory.SetInt8Bit(NVSDeviceEco, 0);
		Memory.Commit();
	}

	// set autoupdate to true
	if (OldFirmware < "2.43" && OldFirmware != "")
	{
		NVS Memory(NVSDeviceArea);
		Memory.SetInt8Bit(NVSDeviceAutoUpdate, 2);
		Memory.Commit();
	}

	// Erase firmware while update to 2.40
	if (OldFirmware < "2.40" && Settings.DeviceGeneration > 1) {
		SPIFlash::EraseRange(0x8A0000, 0x450000);
	}

	if (OldFirmware < "2.41" && OldFirmware != "") {
		NVS Memory(NVSLogArea);
		if (!Memory.IsKeyExists(NVSBrightness)) {

			uint8_t LEDBrightness = 30;
			if (Settings.eFuse.Revision >= 3 && Settings.eFuse.Model > 1 && Settings.eFuse.Type == Settings.Devices.Remote)
				LEDBrightness = 7;

			Memory.SetInt8Bit(NVSBrightness, LEDBrightness);
			Memory.Commit();
		}
	}

}

void BootAndRestore::ExecuteOperationNow(OperationTypeEnum Operation) {
	if (Operation == HARDRESET) {
		Wireless.StopInterfaces(true);
		Operation = HARDRESET_ON_START;

		FreeRTOS::Sleep(1000);
	}

	switch (Operation) {
		case REBOOT:
			if (HomeKit::IsEnabledForDevice())
				hap_reboot_accessory();
			else
				esp_restart();

			break;
		case HARDRESET_ON_START:
		{
			Log::Add(Log::Events::System::HardResetIntied);

			ESP_LOGI(Tag, "Hardreset step1: Clear flash memory");

			PartitionAPI::ErasePartition("coredump",0x01);
			PartitionAPI::ErasePartition("misc", 0x80);
			PartitionAPI::ErasePartition("dataitems", 0x81);
			PartitionAPI::ErasePartition("scenarios", 0x82);
			PartitionAPI::ErasePartition("scache", 0x83);
			PartitionAPI::ErasePartition("storage", 0x84);
			//PartitionAPI::ErasePartition("constants", 0x85);

			ESP_LOGI(Tag, "Hardreset step2: Clear NVS memory");

			NVS MemoryBefore(NVSDeviceArea);

			uint32_t 	DeviceID 		= (MemoryBefore.IsKeyExists(NVSDeviceID)) 		? MemoryBefore.GetUInt32Bit(NVSDeviceID) 		: Settings.Memory.Empty32Bit;
			uint16_t	DeviceType 		= (MemoryBefore.IsKeyExists(NVSDeviceType)) 	? MemoryBefore.GetUInt16Bit(NVSDeviceType) 		: Settings.Memory.Empty16Bit;
			uint8_t		DeviceModel		= (MemoryBefore.IsKeyExists(NVSDeviceModel)) 	? MemoryBefore.GetInt8Bit(NVSDeviceModel) 		: Settings.Memory.Empty8Bit;
			uint16_t	DeviceRevision	= (MemoryBefore.IsKeyExists(NVSDeviceRevision)) ? MemoryBefore.GetUInt16Bit(NVSDeviceRevision) 	: Settings.Memory.Empty16Bit;

			NVS::ClearAll();

			NVS::Init();

			NVS MemoryAfter(NVSDeviceArea);

			ESP_LOGI(Tag, "Hardreset step3: Restore efuse NVS values");

			if (DeviceID 			!= Settings.Memory.Empty32Bit) 		MemoryAfter.SetUInt32Bit	(NVSDeviceID, DeviceID);
			if (DeviceType 			!= Settings.Memory.Empty16Bit) 		MemoryAfter.SetUInt16Bit	(NVSDeviceType, DeviceType);
			if (DeviceModel 		!= Settings.Memory.Empty8Bit) 		MemoryAfter.SetInt8Bit	(NVSDeviceModel, DeviceModel);
			if (DeviceRevision 		!= Settings.Memory.Empty16Bit) 		MemoryAfter.SetUInt16Bit	(NVSDeviceRevision, DeviceRevision);

			MemoryAfter.Commit();

			if (HomeKit::IsEnabledForDevice())
				HomeKit::ResetData();

			ExecuteOperationNow(REBOOT);

			break;
		}
		default:
			break;
	}
}

void BootAndRestore::ExecuteOperationDelayed(OperationTypeEnum Operation) {
	if (DelayedOperationTaskHandler == NULL)
		DelayedOperationTaskHandler = FreeRTOS::StartTask(BootAndRestore::ExecuteOperationDelayedTask, "OperationDelayedTask", (void *)(uint32_t)Operation, 3072, 9);
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
