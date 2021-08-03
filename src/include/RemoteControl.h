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

		RemoteControl_t					(string Username = "", string Password = "");

		void SetCredentials				(string Username = "", string Password = "");
		void ChangeOrSetCredentialsBLE	(string Username = "", string Password = "");

		void Init();

		void Start();
		void Stop();
		void Reconnect(uint16_t Delay = 1000);

		string GetStatusString();

		string GetClientID();

		static esp_err_t 	mqtt_event_handler(esp_mqtt_event_handle_t event);

		static int 			SendMessage (string Payload, string Topic = "",
				uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);

		static string		StartChunk 	(int MessageID = 0, uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);
		static void 		SendChunk 	(string Payload, string ChunkHash, uint16_t ChunkPartID, int MessageID = 0,uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);
		static void 		EndChunk 	(string ChunkHash, int MessageID = 0,uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);

		static bool 		IsCredentialsSet();
		static Status_t 	GetStatus();

	private:
		static string		Username;
		static string		Password;

		static uint8_t		ErrorCounter;

		static esp_mqtt_client_handle_t ClientHandle;

		static Status_t		Status;
		static uint8_t		ConnectionTries;

		esp_mqtt_client_config_t CreateConfig();
		esp_mqtt_client_config_t ConfigDefault();
};


#endif
