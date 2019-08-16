/*
*    MQTT.cpp
*    Works with ESP32 MQTT internal libraries
*
*/

#include "MQTT.h"
#include "Globals.h"
#include "API.h"

static char TAG[] = "MQTT";

string				MQTT_t::Username		= "";
string				MQTT_t::Password		= "";
MQTT_t::Status_t 	MQTT_t::Status			= MQTT_t::Status_t::UNACTIVE;
uint8_t				MQTT_t::ConnectionTries	= 0;

esp_mqtt_client_handle_t MQTT_t::ClientHandle = NULL;
TaskHandle_t MQTT_t::TaskHandle = NULL;

MQTT_t::MQTT_t(string Username, string Password) {
	SetCredentials	(Username, Password);
}

void MQTT_t::Init() {
	NVS Memory(NVSMQTTArea);

	Username = Memory.GetString(NVSMQTTClientID);
	Password = Memory.GetString(NVSMQTTClientSecret);

	MQTT.SetCredentials("8qjY2WinkdepHKl1uzV7tNBOfvb4UFAM", "pElOGx6LWCPuUDFiYKcdJXswz04rkaqA");
}

void MQTT_t::Start() {
	if (TaskHandle == NULL)
		TaskHandle = FreeRTOS::StartTask(MQTTTask, "MQTTTask", nullptr, 6144);
}

void MQTT_t::Stop() {
	ESP_LOGI(TAG,"1");
	if (ClientHandle != NULL) {
		ESP_LOGI(TAG,"2");

		::esp_mqtt_client_destroy(ClientHandle);
		ESP_LOGI(TAG,"3");

		ConnectionTries = 0;
		ESP_LOGI(TAG,"4");

		if (TaskHandle != NULL)
			FreeRTOS::DeleteTask(TaskHandle);

		ESP_LOGI(TAG,"5");
		ClientHandle 	= NULL;
		TaskHandle 		= NULL;

		if (Status == CONNECTED)
			Status = UNACTIVE;
		ESP_LOGI(TAG,"6");
	    ESP_LOGI(TAG,"MQTT Stopped");
	}
}

void MQTT_t::SetCredentials(string Username, string Password) {
	this->Username 		= Username;
	this->Password 		= Password;

	NVS Memory(NVSMQTTArea);

	Memory.SetString(NVSMQTTClientID, Username);
	Memory.SetString(NVSMQTTClientSecret, Password);
}


void MQTT_t::MQTTTask(void *TaskData) {
    esp_mqtt_client_config_t mqtt_cfg;
    mqtt_cfg.uri 			= Settings.MQTT.ServerHost.c_str();
    mqtt_cfg.port			= Settings.MQTT.ServerPort;
    mqtt_cfg.lwt_topic		= "";
    mqtt_cfg.event_handle 	= mqtt_event_handler;
    mqtt_cfg.username		= Username.c_str();
    mqtt_cfg.password		= Password.c_str();

    MQTT_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

    if (ClientHandle != NULL)
		esp_mqtt_client_start(MQTT_t::ClientHandle);
    else
		ESP_LOGE(TAG, "esp_mqtt_client_init failed");

    ESP_LOGI(TAG,"MQTT Started");

	while(1) {
		FreeRTOS::Sleep(1000);
	}
}

esp_err_t MQTT_t::mqtt_event_handler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;
    string DeviceTopic = Settings.MQTT.DeviceTopicPrefix + Device.IDToString();

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
		{
			Status = CONNECTED;

			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			ConnectionTries = 0;

			msg_id = esp_mqtt_client_subscribe(client, DeviceTopic.c_str(), Settings.MQTT.DefaultQOS);
			msg_id = esp_mqtt_client_subscribe(client, string(DeviceTopic + "/UDP").c_str(), Settings.MQTT.DefaultQOS);

			msg_id = SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			break;
		}

        case MQTT_EVENT_DISCONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        	ConnectionTries++;

        	if (ConnectionTries >= Settings.MQTT.MaxConnectionTries) {
                ESP_LOGE(TAG, "Can't Connect to MQTT server");

            	Status = Status_t::ERROR;
        		MQTT.Stop();
        	}

            break;
        }

        case MQTT_EVENT_SUBSCRIBED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

            int msg_id = event->msg_id;

            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        }

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
		{
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");

			string Topic	(event->topic, event->topic_len);
			string Payload	(event->data, event->data_len);

			ESP_LOGI(TAG, "Payload: %s, msg_id: %d, msg_id: %d", Payload.c_str(), event->msg_id, msg_id);

			if (Topic == DeviceTopic + "/UDP")
			{
				if (Payload == WebServer_t::UDPDiscoverBody())
					SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			}

			if (Topic == DeviceTopic)
			{
				WebServer_t::Response Response;
				Query_t Query(Payload);

				API::Handle(Response, Query);

				Response.Body = Converter::ToString(Response.CodeToInt()) + " " + Response.Body;

				SendMessage(Response.Body,
							Settings.MQTT.DeviceTopicPrefix + Device.IDToString() +
							"/" + Converter::ToString(event->msg_id));

				ESP_LOGI(TAG, "HTTP");
			}
			break;
		}
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

int MQTT_t::SendMessage(string Payload, string Topic, uint8_t QOS, uint8_t Retain) {
	if (Topic == "")
		Topic = Settings.MQTT.DeviceTopicPrefix + Device.IDToString();

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
}
