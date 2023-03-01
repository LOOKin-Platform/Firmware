/*
*    MQTT.h
*    Works with ESP32 MQTT internal libraries
*
*/

#ifndef REMOTECONTROL_H_
#define REMOTECONTROL_H_

using namespace std;

#include "mqtt_client.h"

#include "FreeRTOSWrapper.h"

#include <string>

#include "Query.h"
#include "Settings.h"

#define	NVSMQTTArea				"MQTT"
#define	NVSMQTTClientID			"ClientID"
#define	NVSMQTTClientSecret		"ClientSecret"

class RemoteControl_t {
	public:
		enum Status_t { UNACTIVE, STARTED, CONNECTED, ERROR};

		RemoteControl_t		(string Username = "", string Password = "");

		void 				SetCredentials				(string Username = "", string Password = "");
		void 				ChangeOrSetCredentialsBLE	(string Username = "", string Password = "");

		void 				Init();

		void 				Start();
		void 				Stop();
		void 				Reconnect(uint16_t Delay = 1000);

		void 				ChangeSecuredType(bool IsSecured);

		string 				GetStatusString();

		string 				GetClientID();

		static void 		mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

		static int 			SendMessage (string Payload, string Topic = "",
				uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);

		static string		StartChunk 	(uint32_t MessageID = 0, uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);
		static void 		SendChunk 	(string Payload, string ChunkHash, uint16_t ChunkPartID, uint32_t MessageID = 0,uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);
		static void 		EndChunk 	(string ChunkHash, uint32_t MessageID = 0,uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);

		static bool 		IsCredentialsSet();
		static bool			IsSecured();
		static Status_t 	GetStatus();

	private:
		static string		Username;
		static string		Password;

		static esp_mqtt_client_handle_t ClientHandle;

		static Status_t		Status;
		static uint8_t		ConnectionTries;

		static inline bool	IsSecuredFlag = true;

		esp_mqtt_client_config_t CreateConfig();
		esp_mqtt_client_config_t ConfigDefault();
};


#endif
