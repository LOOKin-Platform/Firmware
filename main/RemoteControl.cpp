/*
*    MQTT.cpp
*    Works with ESP32 MQTT internal libraries
*
*/

#include <RemoteControl.h>
#include "Globals.h"
#include "API.h"

#include "Matter.h"

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

	ESP_LOGE("MQTT","%s", Username.c_str());
}

void RemoteControl_t::Start() {
	if (Username != "" && Status != CONNECTED)
	{
		ESP_LOGI(Tag, "Started RAM left %lu", esp_get_free_heap_size());

		if (ClientHandle == NULL)
		{
		    static esp_mqtt_client_config_t mqtt_cfg = CreateConfig();

			ConnectionTries = 0;

		    RemoteControl_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

			esp_mqtt_client_register_event(ClientHandle, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, &mqtt_event_handler, NULL);

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

void RemoteControl_t::ChangeSecuredType(bool sIsSecuredFlag) {
	if (!IsCredentialsSet())
		return;
	
	if (Status == UNACTIVE)
		return;

	if (sIsSecuredFlag && IsSecured())
		return;

	if (!sIsSecuredFlag && !IsSecured())
		return;
	
	esp_mqtt_client_config_t Config;

	Config = CreateConfig();

	Config.broker.address.uri 			= (!sIsSecuredFlag) ? Settings.RemoteControl.ServerUnsecure.c_str() : Settings.RemoteControl.Server.c_str();

	IsSecuredFlag = sIsSecuredFlag;

	esp_mqtt_set_config(ClientHandle, &Config);
	esp_mqtt_client_reconnect(ClientHandle);
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

void RemoteControl_t::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)  
{
    esp_mqtt_event_handle_t event 	= (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    string DeviceTopic = Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString();

    uint32_t MessageID = 0;

    // your_context_t *context = event->context;
    switch ((esp_mqtt_event_id_t)event_id) 
	{
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

			if (Topic == DeviceTopic + "/UDP")
			{
				string UDPDatagram(event->data, event->data_len);

				// LOOK.in -> LOOKin
				if (UDPDatagram.size() > 8)
					if (UDPDatagram.find("LOOK.in") == 0)
						UDPDatagram = "LOOKin" + UDPDatagram.substr(7);

				ESP_LOGI("UDPDatagram from /UDP", "%s", UDPDatagram.c_str());

				if (UDPDatagram == WebServer_t::UDPDiscoverBody())
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
					return;

				Response.Body = Converter::ToString<uint16_t>(Response.CodeToInt()) + " " + Response.Body;

				SendMessage(Response.Body,
							Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() +
							"/" + Converter::ToString<uint32_t>(MessageID));
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
}

int RemoteControl_t::SendMessage(string Payload, string Topic, uint8_t QOS, uint8_t Retain) {
	if (Status != CONNECTED)
		return -1;

	if (Topic == "")
		Topic = Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString();

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
	//return esp_mqtt_client_enqueue(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain, false);

}

string RemoteControl_t::StartChunk(uint32_t MessageID, uint8_t QOS, uint8_t Retain) {
    string ChunkID = Converter::ToHexString((rand() % 0xFFFF) + 1, 4);
    SendMessage("200 CHUNK " + ChunkID +  " START", Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString<uint32_t>(MessageID));
    return ChunkID;
}

void RemoteControl_t::SendChunk(string Payload, string ChunkHash, uint16_t ChunkPartID, uint32_t MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " " + Converter::ToString<uint16_t>(ChunkPartID) + " " + Payload, Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString<uint32_t>(MessageID));
}

void RemoteControl_t::EndChunk (string ChunkHash, uint32_t MessageID, uint8_t QOS, uint8_t Retain) {
    SendMessage("200 CHUNK " + ChunkHash +  " END", Settings.RemoteControl.DeviceTopicPrefix + Device.IDToString() + "/" + Converter::ToString<uint32_t>(MessageID));
}


bool RemoteControl_t::IsCredentialsSet() {
	return (Username != "" && Password != "");
}

bool RemoteControl_t::IsSecured() {
	return IsSecuredFlag;
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

	IsSecuredFlag = !(Matter::IsEnabledForDevice() || LocalMQTT.GetIsActive());

	//Config.host			= "mqtt.look-in.club";
	Config.broker.address.uri 			= (!IsSecuredFlag) ? Settings.RemoteControl.ServerUnsecure.c_str() : Settings.RemoteControl.Server.c_str();
	Config.broker.address.port			= 8883;

	//Config.transport 	= MQTT_TRANSPORT_OVER_SSL;

	Config.credentials.username 	= Username.c_str();
    Config.credentials.authentication.password	
									= Password.c_str();
    Config.credentials.client_id	= Username.c_str();

	//! Перейти на MQTT 5
    Config.session.protocol_ver		= MQTT_PROTOCOL_V_3_1_1;

    return Config;
}

esp_mqtt_client_config_t RemoteControl_t::ConfigDefault() {
	esp_mqtt_client_config_t Config;
	::memset(&Config, 0, sizeof(Config));

	Config.broker.address.port						= 0;
	Config.broker.address.transport 				= MQTT_TRANSPORT_UNKNOWN;

	Config.network.reconnect_timeout_ms 			= 10000;
	Config.network.disable_auto_reconnect 			= false;
	Config.network.timeout_ms						= 10000;
	Config.network.refresh_connection_after_ms		= 0;

	Config.session.keepalive						= 60;
	Config.session.disable_keepalive    			= false;
	Config.session.message_retransmit_timeout 		= 0;
	Config.session.disable_clean_session			= false;
	Config.session.protocol_ver 					= MQTT_PROTOCOL_V_3_1;

	Config.buffer.size								= 3072;
	Config.buffer.out_size							= 0; // if 0 then used buffer_size

	Config.broker.verification.skip_cert_common_name_check 
													= true;

    Config.task.stack_size							= 5120;//6144;//8192;//8192;
	Config.task.priority 							= 6;

	return Config;
}

