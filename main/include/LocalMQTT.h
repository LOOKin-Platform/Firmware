/*
*    MQTT.h
*    Works with ESP32 MQTT internal libraries
*
*/

#ifndef MQTT_H_
#define MQTT_H_

using namespace std;

#include "mqtt_client.h"

#include "FreeRTOSWrapper.h"

#include <string>

#include "Query.h"
#include "Settings.h"

#define	NVSLocalMQTTCredetials	"LocalMQTT"

#define NVSCredentialsServerURI	"ServerURI"
#define NVSCredentialsUsername	"Username"
#define NVSCredentialsPassword	"Password"
#define NVSCredentialsClientID	"ClientID"
#define NVSCredentialsIsActive	"IsActive"


class LocalMQTT_t {
	public:
		enum 				StatusEnum { UNACTIVE, CONNECTING, CONNECTED, ERROR };

		void 				Init();

		void 				LoadCredentials();
		void 				SetCredentials			(bool, string, string, string, string);
		void 				SetIsActive				(bool);
		string				GetCredentialsJSON();

		void 				Start();
		void 				Stop();
		void 				Reconnect();

		static void 		mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

		static int 			SendMessage (string Payload, string Topic = "",
				uint8_t QOS = Settings.RemoteControl.DefaultQOS, uint8_t Retain = Settings.RemoteControl.DefaultRetain);

		static bool 		IsCredentialsSet();
		static bool 		GetIsActive();

		static void			SetStatus(StatusEnum);
		static StatusEnum 	GetStatus();
		static string		GetStatusString();

	private:
		static bool			IsActive;
		static string		ServerURI;
		static string		Username;
		static string		Password;
		static string		ClientID;

		static esp_mqtt_client_handle_t ClientHandle;

		static StatusEnum	CurrentStatus;
		static uint8_t		ConnectionTries;

		esp_mqtt_client_config_t CreateConfig();
		esp_mqtt_client_config_t ConfigDefault();
};


#endif
