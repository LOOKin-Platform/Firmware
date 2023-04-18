/*
*    CommandIR.h
*    CommandBLE_t implementation
*
*/
#ifndef COMMANDS_BLE
#define COMMANDS_BLE

#include "Sensors.h"
#include "Commands.h"
#include "BLEServerGeneric.h"

#include "Data.h"

static string 	        CommandBLELastKBDSignalSended 	= "";
static uint64_t	        CommandBLELastTimeSended		= 0;

#define	NVSCommandsBLEArea 		"CommandBLE"
#define NVSBLESeqRepeatCounter 	"BLERepeatCount"
#define NVSBLESkipPairingCode 	"BLECodeSkiped"

class CommandBLE_t : public Command_t {
	private:
		uint8_t 		SequenceRepeatCounter 	= 5;
		bool			IsBLEPairingCodeSkiped	= false;

	public:
		uint32_t		SendCounter 		= 0;

		CommandBLE_t();

		void    InitSettings() override;
        string  GetSettings() override;
        void    SetSettings(WebServer_t::Response &Result, Query_t &Query) override;

		bool GetIsBLEPairingCodeSkiped() const;

		bool Execute(uint8_t EventCode, const char* StringOperand) override;

		/*
		void TriggerStateChange(IRLib &Signal) {
			if (Settings.eFuse.Type == Settings.Devices.Remote)
				((DataRemote_t*)Data)->SetExternalStatusByIRCommand(Signal);
		}
		*/

		// AC Codeset callbacks
};

#endif
