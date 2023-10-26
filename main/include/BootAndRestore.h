/*
*    BootAndRestore.h
*    Class to handle device boots and restores
*
*/

#ifndef BOOT_AND_RESTORE
#define BOOT_AND_RESTORE

#include "FreeRTOSWrapper.h"
#include "string.h"

using namespace std;

#define	NVSBootAndRestoreArea 			"BAR"
#define	NVSBootAndRestoreAreaFirmware 	"Firmware"
#define	NVSBootAndRestoreAreaOnAttempts	"OnAttempts"

class BootAndRestoreTask;
class BootAndRestoreTaskDeviceOn;

class BootAndRestore {
	friend BootAndRestoreTask;
	friend BootAndRestoreTaskDeviceOn;

	public:
		enum 	OperationTypeEnum { NONE = 0x0, REBOOT, HARDRESET, HARDRESET_ON_START };

		static 	void	OnDeviceStart();
		static 	void	MarkDeviceStartedWithDelay(uint16_t Delay);

		static 	void 	Reboot(bool IsDelayed = true);
		static 	void 	HardReset(bool IsDelayed = true);
		static 	void 	HardResetFromInit();

	protected:
		static 	bool	IsDeviceFullyStarted;
		static 	void	RunOperation			(OperationTypeEnum, bool IsDelayed = true);
	private:
		static	void 	CheckPartitionTable();

		static	void	Migration				(string OldFirmware, string NewFirmware);

		static 	void 	ExecuteOperationNow		(OperationTypeEnum Operation);
		static 	void 	ExecuteOperationDelayed	(OperationTypeEnum Operation);

		static	void	ExecuteOperationDelayedTask	(void*);
		static	void 	MarkDeviceStartedTask		(void*);

		static 	TaskHandle_t	DelayedOperationTaskHandler;
		static 	TaskHandle_t	MarkDeviceStartedTaskHandler;
};

#endif
