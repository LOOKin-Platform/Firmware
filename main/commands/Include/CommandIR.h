/*
*    CommandIR.h
*    CommandIR_t implementation
*
*/
#ifndef COMMANDS_IR
#define COMMANDS_IR

#include <RMT.h>
#include "Sensors.h"
#include "SensorIR.h"
#include "CommandBLE.h"

#include "Data.h"
#include "DataRemote.h"

#define CommandIRArea 			"RMT"
#define CommandIRTXQueuePrefix 	"tx_"
#define	CommandIRTXPause		"pause"

#define	NVSCommandsIRArea 		"CommandIR"
#define NVSSeqRepeatCounter 	"IRRepeatCounter"

extern DataEndpoint_t *Data;

static rmt_channel_t TXChannel = RMT_CHANNEL_2;

static uint16_t					IRACFrequency			= 38000;
static string 					ACReadPart				= "";


static string 					ProntoHexBlockedBuffer 	= "";
static esp_timer_handle_t 		ProntoHexBlockedTimer 	= NULL;

static QueueHandle_t 			CommandIRTXQueue;
static TaskHandle_t 			CommandIRTXHandle		= NULL;

static uint64_t					CommandsIRLastSignalTime= 0;

struct CommandIRTXQueueData {
	string      		NVSItem;
	uint32_t			Frequency;
	vector<gpio_num_t>	TXGPIO;
	rmt_channel_t		TXChannel;
};

static map<uint32_t, CommandIRTXQueueData>
								CommandIRTXDataMap		= map<uint32_t, CommandIRTXQueueData>();

static string			CommandIRLastSignalCRC = "";

static vector<int32_t> 	ACCode = vector<int32_t>();

class CommandIR_t : public Command_t {
	private:
		uint8_t 		SequenceRepeatCounter = 5;

	public:
		struct LastSignal_t {
			uint8_t 	Protocol 		= 0;
			uint32_t 	Data			= 0;
			uint16_t 	Misc			= 0;
			uint16_t	Frequency		= 0;
		};

		uint32_t 		SendCounter 	= 0;

		CommandIR_t();

		LastSignal_t LastSignal;

		void 	InitSettings() override;
		string 	GetSettings() override;
		void 	SetSettings(WebServer_t::Response &Result, Query_t &Query) override;

		bool 	Execute(uint8_t EventCode, const char* StringOperand) override;
		static void ProntoHexBlockedCallback(void *Param);
		
		bool TXSendHelper(IRLib &Signal,uint32_t Operand, uint16_t Misc);


		static void BLESendToQueue(string Signal);
		static void TXSendAddPause(uint32_t PauseLength = 500);

		static bool TXSend(vector<int32_t> &TXItemData, uint16_t Frequency = 38000, bool ShouldFree = true);

		static void IncrementCounter();

		static bool SendToCommandIRQueue(uint32_t &HashID);

		static void CommandIRTXTask(void *TaskData);

		void 		TriggerStateChange(IRLib &Signal);

		static void ACReadStarted(const char *IP);
		static bool ACReadBody(char* Data, int DataLen, const char *IP);
		static void ACReadFinished(const char *IP);
		static void ACReadAborted(const char *IP);
};

#endif
