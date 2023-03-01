/*
*    HTTPClient.h
*    Class to make HTTP Queries
*
*/

#include "HTTPClient.h"
#include "Settings.h"

static char tag[] = "HTTPClient";

QueueHandle_t 	HTTPClient::Queue 			= FreeRTOS::Queue::Create(Settings.HTTPClient.QueueSize, sizeof(uint32_t));
QueueHandle_t	HTTPClient::SystemQueue		= FreeRTOS::Queue::Create(Settings.HTTPClient.SystemQueueSize, sizeof(uint32_t));
uint8_t 		HTTPClient::ThreadsCounter 	= 0;
string 			HTTPClient::UserAgent 		= "";
map<uint32_t, HTTPClient::HTTPClientData_t> HTTPClient::QueryData = map<uint32_t, HTTPClient::HTTPClientData_t>();

/**
 * @brief Add query to the request queue.
 *
 * @param [in] Hostname e. g. look-in.club
 * @param [in] Port Server port, e. g. 80
 * @param [in] ContentURI Path to the content in the server
 * @param [in] Type Query type, e. g. QueryType::POST
 * @param [in] IP Server IP. If empty firmware will try to resolve hostname and fill that field
 * @param [in] ToFront If query is primary it will be better to send it to the front of queue
 * @param [in] ReadStartedCallback Callback function invoked while class started to read data from server
 * @param [in] ReadBodyCallback Callback function invoked while server data reading process
 * @param [in] ReadFinishedCallback Callback function invoked when reading process is over
 * @param [in] AbortedCallback Callback function invoked when reading failed
 * @param [in] POSTData Post data for sending to server
 */

void HTTPClient::Query(string URL, QueryType Type, bool ToFront, bool IsSystem,
        ReadStarted ReadStartedCallback, ReadBody ReadBodyCallback, ReadFinished ReadFinishedCallback, Aborted AbortedCallback, string POSTData) {

	HTTPClientData_t QueryData;

	QueryData.URL 					= URL;
	QueryData.POSTData 				= POSTData;

	QueryData.Method  				= Type;
	QueryData.BufferSize 			= 1024;

	QueryData.ReadStartedCallback   = ReadStartedCallback;
	QueryData.ReadBodyCallback      = ReadBodyCallback;
	QueryData.ReadFinishedCallback  = ReadFinishedCallback;
	QueryData.AbortedCallback       = AbortedCallback;

	Query(QueryData, ToFront, IsSystem);
}

/**
 * @brief Add query to the request queue.
 *
 * @param [in] Query HTTPClientData_t struct with query info
 * @param [in] ToFront If query is primary it will be better to send it to the front of queue
 */

void HTTPClient::Query(HTTPClientData_t Query, bool ToFront, bool IsSystem) {
	if( Queue == 0 ) {
		ESP_LOGE(tag, "Failed to create queue");
		return;
	}

	CheckUserAgent();

	static uint32_t HashID = esp_random();
	while (QueryData.count(HashID) > 0)
		HashID = esp_random();

	QueryData[HashID] = Query;

	FreeRTOS::Queue::Send((IsSystem) ? SystemQueue : Queue, &HashID, ToFront, (TickType_t) Settings.HTTPClient.BlockTicks);

	if (ThreadsCounter <= 0) {
		ThreadsCounter = 1;
		FreeRTOS::StartTask(HTTPClientTask, "HTTPClientTask", (void *)(uint32_t)ThreadsCounter, Settings.HTTPClient.SystemThreadStackSize);
		ESP_LOGE("HTTPClientTask","System task started");
	}

	if (!IsSystem && FreeRTOS::Queue::Count(Queue) >= Settings.HTTPClient.NewThreadStep && ThreadsCounter < Settings.HTTPClient.ThreadsMax) {
		ThreadsCounter++;
		FreeRTOS::StartTask(HTTPClient::HTTPClientTask, "HTTPClientTask", (void *)(uint32_t)ThreadsCounter, Settings.HTTPClient.ThreadStackSize);
		ESP_LOGE("HTTPClientTask","Not system task started");
	}
}

esp_err_t HTTPClient::QueryHandler(esp_http_client_event_t *event)
{
	uint32_t HashID = (uint32_t)event->user_data;

	HTTPClientData_t ClientData = QueryData[HashID];

    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
        	Failed(ClientData);
            break;
        case HTTP_EVENT_ON_CONNECTED:
			if (ClientData.ReadStartedCallback != NULL)
				ClientData.ReadStartedCallback(ClientData.URL.c_str());
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
			if (ClientData.ReadBodyCallback != NULL)
	            //if (!esp_http_client_is_chunked_response(event->client))
					if (!ClientData.ReadBodyCallback((char*)event->data, event->data_len, ClientData.URL.c_str()))
						HTTPClient::Failed(ClientData);
            break;
        case HTTP_EVENT_ON_FINISH:
			if (ClientData.ReadFinishedCallback != NULL)
				ClientData.ReadFinishedCallback(ClientData.URL.c_str());

			break;

        case HTTP_EVENT_DISCONNECTED:
            break;

		case HTTP_EVENT_REDIRECT:
			break;
    }

    return ESP_OK;
}

/**
 * @brief The task of processing the HTTP requests in queue
 *
 * @param [in] TaskData Data - passed in task. Used for task identifier (number)
 */

void HTTPClient::HTTPClientTask(void *TaskData) {
	ESP_LOGD(tag, "Task %lu created", (uint32_t)TaskData);

	uint32_t HashID;
	uint32_t ThreadNumber = (uint32_t)TaskData;

	portBASE_TYPE IsItemReceived = false;

	do
	{
		IsItemReceived = false;

		if (ThreadNumber == 1 && SystemQueue != 0)
			IsItemReceived = FreeRTOS::Queue::Receive(HTTPClient::SystemQueue, &HashID, (TickType_t) Settings.HTTPClient.BlockTicks);

		if (!IsItemReceived && Queue != 0)
			IsItemReceived = FreeRTOS::Queue::Receive(HTTPClient::Queue, &HashID, (TickType_t) Settings.HTTPClient.BlockTicks);

		if (!IsItemReceived) {
			if (ThreadNumber == 1)
				continue;
			else
				break;
		}

		HTTPClientData_t ClientData = QueryData[HashID];

		esp_http_client_config_t Config;
		::memset(&Config, 0, sizeof(Config));

		Config.auth_type 				= HTTP_AUTH_TYPE_NONE;

		Config.path						= "/";
		Config.host						= NULL;
		Config.query					= NULL;
		Config.username					= NULL;
		Config.password					= NULL;

		Config.cert_pem					= NULL;
		Config.client_cert_pem			= NULL;
		Config.client_key_pem			= NULL;
		Config.use_global_ca_store		= false;
		Config.crt_bundle_attach		= NULL;

		Config.transport_type 			= HTTP_TRANSPORT_UNKNOWN;

		Config.timeout_ms 				= 7000;
		Config.buffer_size 				= ClientData.BufferSize;
		Config.buffer_size_tx 			= ClientData.BufferSize;

		Config.disable_auto_redirect	= false;
		Config.max_redirection_count 	= 10;

		switch (ClientData.Method) {
			case POST   : Config.method = esp_http_client_method_t::HTTP_METHOD_POST; 	break;
			case DELETE : Config.method = esp_http_client_method_t::HTTP_METHOD_DELETE; break;
			case GET    :
			default     : Config.method = esp_http_client_method_t::HTTP_METHOD_GET; 	break;
		}

		Config.user_data 				= (void*)HashID;
		Config.event_handler			= QueryHandler;

		Config.url 						= ClientData.URL.c_str();
		Config.user_agent 				= UserAgent.c_str();

		Config.is_async					= false;

	    Config.if_name 					= NULL;

		esp_http_client_handle_t Handle = esp_http_client_init(&Config);

		if (ClientData.POSTData != "" && ClientData.Method == POST)
			::esp_http_client_set_post_field(Handle, ClientData.POSTData.c_str(), ClientData.POSTData.size());

		esp_err_t err = esp_http_client_perform(Handle);

		if (err == ESP_OK) {
			ESP_LOGI(tag, "Performing HTTP request success");
		}
		if (err != ESP_OK) {
			ESP_LOGE(tag, "Connect to http server failed: %s", esp_err_to_name(err));
			HTTPClient::Failed(ClientData);
		}

		if (Handle != NULL) {
			esp_http_client_close(Handle);
			esp_http_client_cleanup(Handle);
		}
		else
		    esp_http_client_cleanup(Handle);

		if (QueryData.count(HashID) > 0)
			QueryData.erase(HashID);

		if (ThreadNumber == 1)
			FreeRTOS::Sleep(100);
	}
	while (IsItemReceived || ThreadNumber == 1);

	ESP_LOGD(tag, "Task %lu removed", (uint32_t)TaskData);
    HTTPClient::ThreadsCounter--;
    FreeRTOS::DeleteTask();
}

/**
 * @brief Critical HTTP-query error handling
 *
 * @param [in] &ClientData HTTPClientData_t struct with query info.
 */
void HTTPClient::Failed(HTTPClientData_t &ClientData) {
	ESP_LOGE(tag, "Exiting HTTPClient task due to fatal error...");

	if (ClientData.AbortedCallback != NULL)
		ClientData.AbortedCallback(ClientData.URL.c_str());
}

void HTTPClient::CheckUserAgent() {
	if (UserAgent == "" && Settings.eFuse.DeviceID > 0)
		UserAgent = "LOOKin\\" + Settings.Firmware.ToString() + " " + Converter::ToHexString(Settings.eFuse.DeviceID,8);
}
