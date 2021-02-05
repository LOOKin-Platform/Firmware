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

#define	NVSLocalMQTTArea		"LocalMQTT"
#define	NVSLocalMQTTLogin		"Login"
#define	NVSLocalMQTTPassword	"Password"
#define	NVSLocalMQTTServerIP	"ServerIP"
#define	NVSLocalMQTTServerPort	"ServerPort"

class MQTT_t {
	public:
		enum Status_t { UNACTIVE, CONNECTED, ERROR };

		MQTT_t							(string Username = "", string Password = "");

		void SetCredentials				(string Username = "", string Password = "");

		void Start();
		void Stop();
		void Reconnect();

		static esp_err_t 	mqtt_event_handler(esp_mqtt_event_handle_t event);

		static int 			SendMessage (string Payload, string Topic = "",
				uint8_t QOS = Settings.MQTT.DefaultQOS, uint8_t Retain = Settings.MQTT.DefaultRetain);

		static bool 		IsCredentialsSet();
		static Status_t 	GetStatus();

	private:
		static string		Username;
		static string		Password;
		static string		ClientID;

		static string		ServerIP;
		static uint32_t		ServerPort;

		static esp_mqtt_client_handle_t ClientHandle;

		static Status_t		Status;
		static uint8_t		ConnectionTries;

		esp_mqtt_client_config_t CreateConfig();
		esp_mqtt_client_config_t ConfigDefault();
};


#endif
