/*
*    SensorIR.cpp
*    SensorIR_t implementation
*
*/

#ifndef SENSORS_IR_H
#define SENSORS_IR_H

#include <IRLib.h>
#include <ISR.h>
#include "HTTPClient.h"

#include "Sensors.h"
#include "Data.h"
#include "DataRemote.h"

#include "esp_timer.h"

class SensorIR_t : public Sensor_t {
    private:
        static inline vector<int32_t> 		        SensorIRCurrentMessage 			= {};
        static inline uint8_t 				        SensorIRID 						= 0x87;

        static inline IRAM_ATTR esp_timer_handle_t  SignalReceivedTimer 	        = NULL;

        static inline string 				        SensorIRACCheckBuffer 			= "";
        static inline char 				            SensorIRSignalCRCBuffer[96] 	= "\0";

    public:
		IRLib 		LastSignal;
		uint32_t	SignalDetectedTime     			= 0;

		SensorIR_t();

		bool HasPrimaryValue() override;
		bool ShouldUpdateInMainLoop() override;

		void Update() override;

		string FormatValue(string Key = "Primary");

		bool CheckOperand(uint8_t SceneEventCode, uint8_t SceneEventOperand) override;

		string SummaryJSON() override;

		static void MessageStart();
		static bool MessageBody(int16_t Bit);
		static void MessageEnd();

		static void SignalReceivedCallback(void *Param);

		// AC check signal callbacks
		static void ACCheckStarted(const char *IP);
		static bool ACCheckBody(char Data[], int DataLen, const char *IP);
		static void ACCheckFinished(const char *IP);
        static void ACReadAborted(const char *IP);

		string              StorageEncode(map<string,string> Data) override;
		map<string,string>  StorageDecode(string DataString) override;
};

#endif