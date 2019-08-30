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

MQTT_t::MQTT_t(string Username, string Password) {
	SetCredentials	(Username, Password);
}

void MQTT_t::Init() {
	NVS Memory(NVSMQTTArea);

	Username = Memory.GetString(NVSMQTTClientID);
	Password = Memory.GetString(NVSMQTTClientSecret);

	//MQTT.SetCredentials("QgWyNcr8wHuLK39UVbZA2CkItzhnX45F", "2PSO1F7YCNL6rGs9WUygezhM0vfVRuBo");
}

void MQTT_t::Start() {
	if (Username != "") {
		ESP_LOGI("MQTT Started","RAM left %d", esp_get_free_heap_size());

	    esp_mqtt_client_config_t mqtt_cfg = ConfigDefault();
	    mqtt_cfg.uri 			= "mqtts://mqtt.look-in.club:8883";//Settings.MQTT.ServerHost.c_str();
	    mqtt_cfg.event_handle 	= mqtt_event_handler;
	    mqtt_cfg.transport 		= MQTT_TRANSPORT_OVER_SSL;

	    mqtt_cfg.username		= "QgWyNcr8wHuLK39UVbZA2CkItzhnX45F";//Username.c_str();
	    mqtt_cfg.password		= "2PSO1F7YCNL6rGs9WUygezhM0vfVRuBo";
	    mqtt_cfg.client_id		= "QgWyNcr8wHuLK39UVbZA2CkItzhnX45F";

	    MQTT_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

	    if (ClientHandle != NULL)
			esp_mqtt_client_start(MQTT_t::ClientHandle);
	    else
			ESP_LOGE(TAG, "esp_mqtt_client_init failed");
	}
}

void MQTT_t::Stop() {
	if (ClientHandle != NULL) {

		::esp_mqtt_client_destroy(ClientHandle);
		ConnectionTries = 0;

		ClientHandle 	= NULL;

		if (Status == CONNECTED)
			Status = UNACTIVE;

	    ESP_LOGI(TAG,"MQTT Stopped");
	}
}

void MQTT_t::SetCredentials(string Username, string Password) {
	this->Username 		= Username;
	this->Password 		= Password;

	NVS Memory(NVSMQTTArea);
}

esp_err_t IRAM_ATTR MQTT_t::mqtt_event_handler(esp_mqtt_event_handle_t event) {
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

			//msg_id = SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			break;
		}

        case MQTT_EVENT_DISCONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

            if (Status == CONNECTED && ClientHandle != NULL) {
            	Status = ERROR;
				esp_mqtt_client_start(MQTT_t::ClientHandle);
            }

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

            ESP_LOGI(TAG, "subscribed successful to , msg_id=%d", msg_id);
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

			ESP_LOGI(TAG, "Payload: %s, msg_id: %d", Payload.c_str(), event->msg_id);

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
	if (Status != CONNECTED)
		return -1;

	if (Topic == "")
		Topic = Settings.MQTT.DeviceTopicPrefix + Device.IDToString();

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
}

esp_mqtt_client_config_t MQTT_t::ConfigDefault() {
	esp_mqtt_client_config_t Config;

	Config.host				= NULL;
	Config.uri				= NULL;
	Config.port				= 0;
	Config.keepalive		= 120;
	Config.disable_auto_reconnect = true;

	Config.client_id 		= NULL;
	Config.lwt_topic		= NULL;
	Config.lwt_msg			= NULL;

	Config.cert_pem			= NULL;
	Config.client_cert_pem 	= NULL;
	Config.client_key_pem	= NULL;

	Config.disable_clean_session = false;
	Config.refresh_connection_after_ms = 0;

	Config.buffer_size		= 0;
	Config.task_stack		= 0;
	Config.task_prio		= 0;

	Config.user_context		= NULL;

	return Config;
}
