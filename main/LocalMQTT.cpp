/*
*    MQTT.cpp
*    Works with ESP32 MQTT internal libraries
*
*/

#include <LocalMQTT.h>
#include "Globals.h"
#include "API.h"

static char TAG[] = "LocalMQTT";

string						LocalMQTT_t::ServerURI		= "";
string						LocalMQTT_t::Username		= "";
string						LocalMQTT_t::Password		= "";
string						LocalMQTT_t::ClientID		= "";
bool						LocalMQTT_t::IsActive		= true;

LocalMQTT_t::StatusEnum 	LocalMQTT_t::CurrentStatus	= LocalMQTT_t::StatusEnum::UNACTIVE;
uint8_t						LocalMQTT_t::ConnectionTries= 0;

esp_mqtt_client_handle_t 	LocalMQTT_t::ClientHandle = NULL;


void LocalMQTT_t::Init() {
	LoadCredentials();
}

void LocalMQTT_t::LoadCredentials() {
	NVS Memory(NVSNetworkArea);
	string Credentials = Memory.GetString(NVSLocalMQTTCredetials);

	JSON JSONObject(Credentials);
	ServerURI 	= JSONObject.GetItem(NVSCredentialsServerURI);
	Username	= JSONObject.GetItem(NVSCredentialsUsername);
	Password	= JSONObject.GetItem(NVSCredentialsPassword);
	ClientID	= JSONObject.GetItem(NVSCredentialsClientID);
	IsActive	= (JSONObject.GetItem(NVSCredentialsIsActive) == "0") ? false : true;
}

void LocalMQTT_t::SetCredentials(bool sIsActive, string sServerURI, string sUsername, string sPassword, string sClientID) {
	JSON JSONObject;

	JSONObject.SetItems(
		vector<pair<string,string>>({
			make_pair(NVSCredentialsServerURI	, sServerURI),
			make_pair(NVSCredentialsUsername	, sUsername),
			make_pair(NVSCredentialsPassword	, sPassword),
			make_pair(NVSCredentialsClientID	, sClientID),
			make_pair(NVSCredentialsIsActive	, (sIsActive) ? "1" : "0")
		}));

	NVS Memory(NVSNetworkArea);
	Memory.SetString(NVSLocalMQTTCredetials, JSONObject.ToString());
	Memory.Commit();

	Stop();

	ServerURI 	= sServerURI;
	Username 	= sUsername;
	Password 	= sPassword;
	ClientID 	= sClientID;
	IsActive	= sIsActive;

	if (IsActive) {
		FreeRTOS::Sleep(1500);
		Start();
	}

	return;
}

void LocalMQTT_t::SetIsActive(bool IsActive) {
	SetCredentials(IsActive, ServerURI, Username, Password, ClientID);
}

string LocalMQTT_t::GetCredentialsJSON() {
	JSON JSONObject;
		JSONObject.SetItems(
				vector<pair<string,string>>({
					make_pair("Status"			, GetStatusString()),
					make_pair("ServerURI"		, ServerURI),
					make_pair("Username"		, Username),
					make_pair("ClientID"		, ClientID),
					make_pair("IsActive"		, (IsActive) ? "true" : "false"),
					make_pair("IsPasswordSet"	, (Password.size() > 0) ? "true" : "false"),
		}));

	return JSONObject.ToString();
}



void LocalMQTT_t::Start() {
	if (ServerURI != "" && CurrentStatus != CONNECTED && IsActive) {
		SetStatus(CONNECTING);

	    static esp_mqtt_client_config_t mqtt_cfg = CreateConfig();

		ConnectionTries = 0;

	    LocalMQTT_t::ClientHandle = esp_mqtt_client_init(&mqtt_cfg);

		esp_mqtt_client_register_event(ClientHandle, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	    if (ClientHandle != NULL)
			esp_mqtt_client_start(LocalMQTT_t::ClientHandle);
	    else
			ESP_LOGE(TAG, "esp_mqtt_client_init failed");
	}
}

void LocalMQTT_t::Stop() {
	if (ClientHandle && CurrentStatus != UNACTIVE) {
		SetStatus(UNACTIVE);

		if (ClientHandle != NULL)
			::esp_mqtt_client_destroy(ClientHandle);

		ClientHandle	= NULL;

	    ESP_LOGI(TAG,"MQTT Stopped");
	}
}

void LocalMQTT_t::Reconnect() {
	if (CurrentStatus == CONNECTED  && ClientHandle != NULL) {
		Stop();
		FreeRTOS::Sleep(2000);
		Start();
		return;
	}

	if (CurrentStatus != UNACTIVE && ClientHandle != NULL) {
		::esp_mqtt_client_reconnect(ClientHandle);
		return;
	}

	Start();
}

void IRAM_ATTR LocalMQTT_t::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t 	event 	= (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t 	client	= event->client;

    string DeviceTopic = Settings.LocalMQTT.TopicPrefix + Device.IDToString();

    // your_context_t *context = event->context;
    switch ((esp_mqtt_event_id_t)event_id) 
	{
        case MQTT_EVENT_CONNECTED:
        	SetStatus(CONNECTED);

			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			ConnectionTries = 0;

			::esp_mqtt_client_subscribe(client, string(DeviceTopic + "/#").c_str() , Settings.LocalMQTT.DefaultQOS);
			::esp_mqtt_client_subscribe(client, string(Settings.LocalMQTT.TopicPrefix + "broadcast").c_str(), Settings.LocalMQTT.DefaultQOS);

			SendMessage(WebServer.UDPAliveBody(), Settings.LocalMQTT.TopicPrefix + "broadcast");

			RemoteControl.ChangeSecuredType(false);
			break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

            if (CurrentStatus == CONNECTED && ClientHandle != NULL)
            	SetStatus(ERROR);

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
			string Data(event->data, event->data_len);

			if (Topic == Settings.LocalMQTT.TopicPrefix + "broadcast")
			{
				if (string(event->data, event->data_len) == WebServer_t::UDPDiscoverBody())
					SendMessage(WebServer.UDPAliveBody(), Settings.LocalMQTT.TopicPrefix + "broadcast");
			}

			if (Topic.size() > (DeviceTopic.size() + 1))
				Topic = Topic.substr(DeviceTopic.size()+1);
			else
				return;

			ESP_LOGE(TAG, "Topic: %s", Topic.c_str());

			vector<string> TopicParts = Converter::StringToVector(Topic, "/");

			if (TopicParts.size() < 3)
				return;

			if (TopicParts[0] == "commands")
			{
				Command_t* Command = Command_t::GetCommandByName(TopicParts[1]);

				if (Command == NULL)
					return;

				Command->Execute(Command->GetEventCode(TopicParts[2]), Data.c_str());
			}
			break;
		}
        case MQTT_EVENT_ERROR:
        	SetStatus(ERROR);
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR Error type: %d Connect return code: %d", event->error_handle->error_type, event->error_handle->connect_return_code);
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

int LocalMQTT_t::SendMessage(string Payload, string Topic, uint8_t QOS, uint8_t Retain) {
	if (CurrentStatus != CONNECTED)
		return -1;

	bool IsTopicFull = false;

	if (Topic.size() >= Settings.LocalMQTT.TopicPrefix.size())
		if (Topic.rfind(Settings.LocalMQTT.TopicPrefix, 0) == 0)
			IsTopicFull = true;


	if (!IsTopicFull)
		Topic = Settings.LocalMQTT.TopicPrefix + Device.IDToString() + Topic;

	return ::esp_mqtt_client_publish(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain);
	//return ::esp_mqtt_client_enqueue(ClientHandle, Topic.c_str(), Payload.c_str(), Payload.length(), QOS, Retain, false);

}

bool LocalMQTT_t::IsCredentialsSet() {
	return (Username != "" && Password != "");
}

bool LocalMQTT_t::GetIsActive() {
	return IsActive;
}

void LocalMQTT_t::SetStatus(StatusEnum Status) {
	if (CurrentStatus == Status)
		return;

	CurrentStatus = Status;

	if ((uint8_t)Status > 1)
		Wireless.SendBroadcastUpdated("MQTT", Converter::ToHexString((uint8_t)Status,1));
}

LocalMQTT_t::StatusEnum LocalMQTT_t::GetStatus() {
	return CurrentStatus;
}

string LocalMQTT_t::GetStatusString() {
	switch (CurrentStatus) {
		case UNACTIVE	: return "Unactive";
		case CONNECTING	: return "Connecting";
		case CONNECTED	: return "Connected";
		case ERROR		: return "Error";
	}

	return "Undefined";
}

esp_mqtt_client_config_t LocalMQTT_t::CreateConfig() {
	esp_mqtt_client_config_t Config = ConfigDefault();

	Config.broker.address.uri 		= (ServerURI != "") ? ServerURI.c_str() : NULL;

	Config.credentials.username 	= (Username != "") ? Username.c_str() : NULL;
    Config.credentials.authentication.password	
									= (Password != "") ? Password.c_str() : NULL;
    Config.credentials.client_id	= (ClientID != "") ? ClientID.c_str() : NULL;

	//! Перейти на MQTT 5
    Config.session.protocol_ver		= MQTT_PROTOCOL_V_3_1_1;

    return Config;
}

esp_mqtt_client_config_t LocalMQTT_t::ConfigDefault() {
	esp_mqtt_client_config_t Config;
	::memset(&Config, 0, sizeof(Config));

	Config.broker.address.port						= 0;
	Config.broker.address.transport 				= MQTT_TRANSPORT_UNKNOWN;

	Config.network.reconnect_timeout_ms 			= 15000;
	Config.network.disable_auto_reconnect 			= false;
	Config.network.timeout_ms						= 5000;
	Config.network.refresh_connection_after_ms		= 0;

	Config.session.keepalive						= 60;
	Config.session.disable_keepalive    			= false;
	Config.session.message_retransmit_timeout 		= 0;
	Config.session.disable_clean_session			= false;
	Config.session.protocol_ver 					= MQTT_PROTOCOL_V_3_1;

	Config.credentials.authentication.use_secure_element		
													= false;

	Config.buffer.size								= 4096;
	Config.buffer.out_size							= 0; // if 0 then used buffer_size

	Config.broker.verification.use_global_ca_store 	= false;
	Config.broker.verification.alpn_protos			= NULL;
	Config.broker.verification.skip_cert_common_name_check 
													= true;

    Config.task.stack_size							= 4096;
	Config.task.priority 							= 5;

	return Config;
}

