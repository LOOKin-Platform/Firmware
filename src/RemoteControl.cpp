/*
*    MQTT.cpp
*    Works with ESP32 MQTT internal libraries
*
*/

#include <RemoteControl.h>
#include "Globals.h"
#include "API.h"

#include "HomeKit.h"


static char Tag[] = "RemoteControl";

string						RemoteControl_t::Username		= "";
string						RemoteControl_t::Password		= "";
RemoteControl_t::Status_t 	RemoteControl_t::Status			= RemoteControl_t::Status_t::UNACTIVE;
uint8_t						RemoteControl_t::ConnectionTries= 0;

esp_mqtt_client_handle_t 	RemoteControl_t::ClientHandle 	= NULL;

RemoteControl_t::RemoteControl_t(string Username, string Password) {
	SetCredentials	(Username, Password);
}

void RemoteControl_t::Init() {
	NVS Memory(NVSMQTTArea);

	Username = Memory.GetString(NVSMQTTClientID);
	Password = Memory.GetString(NVSMQTTClientSecret);
}

void RemoteControl_t::Start() {
	if (Username != "" && Status != CONNECTED)
	{
		ESP_LOGI(Tag, "Started RAM left %d", esp_get_free_heap_size());

		if (ClientHandle == NULL)
		{
		    static esp_mqtt_client_config_t mqtt_cfg = CreateConfig();

			ConnectionTries = 0;

		    RemoteControl_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

		    ESP_LOGI(Tag, "RC MQTT inited");
		}

		esp_err_t err = esp_mqtt_client_start(RemoteControl_t::ClientHandle);
	    ESP_LOGI(Tag, "%s", (err == ESP_OK) ? "Started" : "Start error");
	}
}

void RemoteControl_t::Stop() {
	Status = UNACTIVE;

	if (ClientHandle != NULL) {
		::esp_mqtt_client_stop(ClientHandle);
		::esp_mqtt_client_destroy(ClientHandle);
	}

	ClientHandle = NULL;

	ESP_LOGI(Tag, "Stopped");
}

void RemoteControl_t::Reconnect(uint16_t Delay) {
	if (Status == UNACTIVE && ClientHandle == NULL)
		Start();

	if (Status != UNACTIVE && ClientHandle != NULL) {
		::esp_mqtt_client_reconnect(ClientHandle);
		return;
	}
}


string RemoteControl_t::GetClientID() {
	return Username;
}

void RemoteControl_t::SetCredentials(string Username, string Password) {
	this->Username 		= Username;
	this->Password 		= Password;

	NVS Memory(NVSMQTTArea);
	Memory.SetString(NVSMQTTClientID	, Username);
	Memory.SetString(NVSMQTTClientSecret, Password);
	Memory.Commit();
}

void RemoteControl_t::ChangeOrSetCredentialsBLE(string Username, string Password) {
	if (Username == this->Username && Password == this->Password && Status == CONNECTED) {
		SendMessage(WebServer.UDPAliveBody(), Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/UDP");
		return;
	}

	SetCredentials(Username, Password);

	if (!WiFi.IsConnectedSTA())
		return;

	switch (Status)
	{
		case UNACTIVE	: ESP_LOGE("Status", "Unactive"); break;
		case STARTED	: ESP_LOGE("Status", "STARTED"); break;
		case CONNECTED	: ESP_LOGE("Status", "Connected"); break;
		case ERROR		: ESP_LOGE("Status", "Error");  break;
	}

	if (Status != UNACTIVE && ClientHandle != NULL) {
		static esp_mqtt_client_config_t Config = CreateConfig();
		esp_mqtt_set_config(ClientHandle, &Config);
		esp_mqtt_client_reconnect(ClientHandle);
	}
	else
	{
		Stop();

		FreeRTOS::Sleep(1000);

		Start();
	}
}



esp_err_t RemoteControl_t::mqtt_event_handler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;

    string DeviceTopic = Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString();

    int MessageID = 0;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
			Status = CONNECTED;
			ESP_LOGI(Tag, "MQTT_EVENT_CONNECTED");
			ConnectionTries = 0;

			::esp_mqtt_client_subscribe(client, DeviceTopic.c_str(), Settings.RemoteControl.DefaultQOS);
			::esp_mqtt_client_subscribe(client, string(DeviceTopic + "/UDP").c_str(), Settings.RemoteControl.DefaultQOS);

			SendMessage(WebServer.UDPAliveBody(), DeviceTopic + "/UDP");
			break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(Tag, "MQTT_EVENT_DISCONNECTED");

            if (Status == CONNECTED && ClientHandle != NULL)
            	Status = ERROR;

            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(Tag, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(Tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(Tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
		{
	    	if (event->msg_id > 0)
	    	{
	    		MessageID = event->msg_id;
	    	}

			string Topic(event->topic, event->topic_len);

			if (event->topic_len > 8)
				if (Topic.find("LOOK.in") == 0)
					Topic = "LOOKin" + Topic.substr(8);

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
				Query.MQTTMessageID 	= MessageID;

				ESP_LOGE("MQTT DAta", "%s", event->data);

				API::Handle(Response, Query);

				Query.Cleanup();

				if (Response.ResponseCode == WebServer_t::Response::CODE::IGNORE)
					return ESP_OK;

				Response.Body = Converter::ToString(Response.CodeToInt()) + " " + Response.Body;

				SendMessage(Response.Body,
							Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() +
							"/" + Converter::ToString(MessageID));
			}
			break;
		}
        case MQTT_EVENT_ERROR:
            ESP_LOGI(Tag, "MQTT_EVENT_ERROR: %d", event->event_id);

            break;

        default:
            ESP_LOGI(Tag, "Other event id:%d", event->event_id);

            break;
    }

    return ESP_OK;
}

int RemoteControl_t::SendMessage(string Payload, string Topic, uint8_t QOS, uint8_t Retain) {
	if (Status != CONNECTED)
		return -1;

	if (Topic == "")
		Topic = Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString();

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
	//return esp_mqtt_client_enqueue(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain, false);

}

string RemoteControl_t::StartChunk(int MessageID, uint8_t QOS, uint8_t Retain) {
    string ChunkID = Converter::ToHexString((rand() % 0xFFFF) + 1, 4);
    SendMessage("200 CHUNK " + ChunkID +  " START", Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
    return ChunkID;
}

void RemoteControl_t::SendChunk(string Payload, string ChunkHash, uint16_t ChunkPartID, int MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " " + Converter::ToString(ChunkPartID) + " " + Payload, Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
}

void RemoteControl_t::EndChunk (string ChunkHash, int MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " END", Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString(MessageID));
}


bool RemoteControl_t::IsCredentialsSet() {
	return (Username != "" && Password != "");
}

string RemoteControl_t::GetStatusString() {
	switch (RemoteControl_t::GetStatus())
	{
		case RemoteControl_t::UNACTIVE	: return "Unactive"	; break;
		case RemoteControl_t::STARTED	: return "Started"	; break;
		case RemoteControl_t::CONNECTED	: return "Connected"; break;
		case RemoteControl_t::ERROR		: return "Error"	; break;

		default:
			return "";
	}
}

RemoteControl_t::Status_t RemoteControl_t::GetStatus() {
	return Status;
}

esp_mqtt_client_config_t RemoteControl_t::CreateConfig() {
	esp_mqtt_client_config_t Config = ConfigDefault();

	//Config.host			= "mqtt.look-in.club";
	Config.uri 			= (!HomeKit::IsExperimentalMode()) ? Settings.RemoteControl.Server.c_str() : Settings.RemoteControl.ServerUnsecure.c_str();
	Config.port			= 8883;

	Config.event_handle = mqtt_event_handler;
	//Config.transport 	= MQTT_TRANSPORT_OVER_SSL;

	Config.username		= Username.c_str();
    Config.password		= Password.c_str();
    Config.client_id	= Username.c_str();

    Config.protocol_ver	= MQTT_PROTOCOL_V_3_1_1;

    return Config;
}

esp_mqtt_client_config_t RemoteControl_t::ConfigDefault() {
	esp_mqtt_client_config_t Config;

	Config.host					= NULL;
	Config.uri					= NULL;
	Config.path					= NULL;
	Config.port					= 0;
	Config.keepalive			= 60;
	Config.disable_keepalive	= false;
	Config.reconnect_timeout_ms	= 10000;
	Config.disable_auto_reconnect
								= false;
	Config.network_timeout_ms	= 10000;
	Config.use_secure_element	= false;
	Config.ds_data				= NULL;

	Config.event_handle 		= NULL;
	Config.event_loop_handle	= NULL;

	Config.username				= NULL;
	Config.password				= NULL;
	Config.client_id 			= NULL;
	Config.lwt_topic			= NULL;
	Config.lwt_msg				= NULL;
	Config.lwt_msg_len 			= 0;
	Config.lwt_qos				= 0;
	Config.lwt_retain			= 0;

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

    Config.task_stack			= 4096;//8192;//8192;
	Config.buffer_size			= 3072;
	Config.out_buffer_size		= 0; // if 0 then used buffer_size
	Config.task_prio			= 6;

	Config.protocol_ver 		= MQTT_PROTOCOL_V_3_1;

	Config.user_context			= NULL;
	Config.use_global_ca_store 	= false;

	Config.alpn_protos			= NULL;
	Config.message_retransmit_timeout = 0;
	Config.transport			= MQTT_TRANSPORT_UNKNOWN;
	Config.crt_bundle_attach	= NULL;

	Config.skip_cert_common_name_check = true;

	return Config;
}

