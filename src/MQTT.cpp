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
}

void MQTT_t::Start() {
	if (Username != "" && Status != CONNECTED) {
		ESP_LOGI("MQTT Started","RAM left %d", esp_get_free_heap_size());

	    esp_mqtt_client_config_t mqtt_cfg = CreateConfig();

		ConnectionTries = 0;

	    MQTT_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

	    if (ClientHandle != NULL)
			esp_mqtt_client_start(MQTT_t::ClientHandle);
	    else
			ESP_LOGE(TAG, "esp_mqtt_client_init failed");

	    ESP_LOGI(TAG,"MQTT Started");
	}
}

void MQTT_t::Stop() {
	if (ClientHandle && Status != UNACTIVE) {
		Status = UNACTIVE;

		::esp_mqtt_client_destroy(ClientHandle);

		ClientHandle	= NULL;

	    ESP_LOGI(TAG,"MQTT Stopped");
	}
}

void MQTT_t::Reconnect() {
	if (Status == CONNECTED  && ClientHandle != nullptr) {
		Stop();
		FreeRTOS::Sleep(1000);
		Start();
		return;
	}

	if (Status != UNACTIVE && ClientHandle != nullptr) {
		::esp_mqtt_client_reconnect(ClientHandle);
		return;
	}

	Start();
}

string MQTT_t::GetClientID() {
	return Username;
}

void MQTT_t::SetCredentials(string Username, string Password) {
	this->Username 		= Username;
	this->Password 		= Password;

	NVS Memory(NVSMQTTArea);
	Memory.SetString(NVSMQTTClientID	, Username);
	Memory.SetString(NVSMQTTClientSecret, Password);
}

void MQTT_t::ChangeOrSetCredentialsBLE(string Username, string Password) {
	if (Username == this->Username && Password == this->Password && Status == CONNECTED) {
		SendMessage(WebServer.UDPAliveBody(), Settings.MQTT.DeviceTopicPrefix + Device.IDToString() + "/UDP");
		return;
	}

	SetCredentials(Username, Password);

	if (!WiFi.IsConnectedSTA())
		return;

	switch (Status)
	{
		case UNACTIVE	: ESP_LOGE("Status", "Unactive"); break;
		case CONNECTED	: ESP_LOGE("Status", "Connected"); break;
		case ERROR		: ESP_LOGE("Status", "Error");  break;
	}

	if (Status == UNACTIVE && ClientHandle == nullptr)
		Start();
	else if (ClientHandle != nullptr)
	{
		esp_mqtt_client_config_t Config = CreateConfig();
		::esp_mqtt_set_config(ClientHandle, &Config);
		::esp_mqtt_client_reconnect(ClientHandle);
		ESP_LOGE("MQTT", "reconnected");
	}
}

esp_err_t IRAM_ATTR MQTT_t::mqtt_event_handler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;

    string DeviceTopic = Settings.MQTT.DeviceTopicPrefix + Device.IDToString();

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
			Status = CONNECTED;

			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			ConnectionTries = 0;

			::esp_mqtt_client_subscribe(client, DeviceTopic.c_str(), Settings.MQTT.DefaultQOS);
			::esp_mqtt_client_subscribe(client, string(DeviceTopic + "/UDP").c_str(), Settings.MQTT.DefaultQOS);

			SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

            if (Status == CONNECTED && ClientHandle != NULL)
            	Status = ERROR;

            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
		{
			string Topic(event->topic, event->topic_len);

			if (Topic == DeviceTopic + "/UDP")
			{
				if (string(event->data, event->data_len) == WebServer_t::UDPDiscoverBody())
					SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			}

			if (Topic == DeviceTopic)
			{
				WebServer_t::Response Response;
				event->data[event->data_len] = '\0';
				Query_t Query(event->data);
				Query.Transport 		= WebServer.QueryTransportType::MQTT;
				Query.MQTTMessageID 	= event->msg_id;

				API::Handle(Response, Query);

				if (Response.ResponseCode == WebServer_t::Response::CODE::IGNORE)
					return ESP_OK;

				Response.Body = Converter::ToString(Response.CodeToInt()) + " " + Response.Body;

				SendMessage(Response.Body,
							Settings.MQTT.DeviceTopicPrefix + Device.IDToString() +
							"/" + Converter::ToString(event->msg_id));
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
	ESP_LOGD("Payload", "%s", Payload.c_str());
	ESP_LOGD("Topic", "%s", Topic.c_str());

	if (Status != CONNECTED)
		return -1;

	if (Topic == "")
		Topic = Settings.MQTT.DeviceTopicPrefix + Device.IDToString();

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
}

string MQTT_t::StartChunk(int MessageID, uint8_t QOS, uint8_t Retain) {
    string ChunkID = Converter::ToHexString((rand() % 0xFFFF) + 1, 4);
    SendMessage("200 CHUNK " + ChunkID +  " START", Settings.MQTT.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
    return ChunkID;
}

void MQTT_t::SendChunk(string Payload, string ChunkHash, uint16_t ChunkPartID, int MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " " + Converter::ToString(ChunkPartID) + " " + Payload, Settings.MQTT.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
}

void MQTT_t::EndChunk (string ChunkHash, int MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " END", Settings.MQTT.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
}


bool MQTT_t::IsCredentialsSet() {
	return (Username != "" && Password != "");
}

MQTT_t::Status_t MQTT_t::GetStatus() {
	return Status;
}

esp_mqtt_client_config_t MQTT_t::CreateConfig() {
	esp_mqtt_client_config_t Config = ConfigDefault();

	Config.uri 			= Settings.MQTT.Server.c_str();
	Config.port			= 8883;

	Config.event_handle = mqtt_event_handler;
	Config.transport 	= MQTT_TRANSPORT_OVER_SSL;

	Config.username		= Username.c_str();
    Config.password		= Password.c_str();
    Config.client_id	= Username.c_str();

    Config.protocol_ver	= MQTT_PROTOCOL_V_3_1_1;

    return Config;
}

esp_mqtt_client_config_t MQTT_t::ConfigDefault() {
	esp_mqtt_client_config_t Config;

	Config.host					= NULL;
	Config.uri					= NULL;
	Config.port					= 0;
	Config.keepalive			= 120;
	Config.disable_auto_reconnect
								= false;
//	Config.use_secure_element	= false;

	Config.client_id 			= NULL;
	Config.lwt_topic			= NULL;
	Config.lwt_msg				= NULL;
	Config.lwt_msg_len 			= 0;

	Config.cert_pem				= NULL;
	Config.client_cert_pem 		= NULL;
	Config.client_key_pem		= NULL;
	Config.cert_len				= 0;
	Config.client_cert_len		= 0;
	Config.client_key_len		= 0;
	Config.psk_hint_key			= NULL;
	Config.clientkey_password 	= NULL;
	Config.clientkey_password_len = 0;

	Config.disable_clean_session= false;
	Config.refresh_connection_after_ms
								= 0;

    Config.task_stack			= 6144;

	Config.buffer_size			= 2560;
	Config.out_buffer_size		= 0; // if 0 then used buffer_size
	Config.task_stack			= 0;
	Config.task_prio			= 0;

	Config.protocol_ver 		= MQTT_PROTOCOL_V_3_1;

	Config.user_context			= NULL;
	Config.use_global_ca_store 	= false;

	Config.alpn_protos			= NULL;

	return Config;
}

