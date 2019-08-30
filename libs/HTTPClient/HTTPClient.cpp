/*
*    HTTPClient.h
*    Class to make HTTP Queries
*
*/

#include "HTTPClient.h"
#include "Globals.h"

static char tag[] = "HTTPClient";

QueueHandle_t HTTPClient::Queue = FreeRTOS::Queue::Create(Settings.HTTPClient.QueueSize, sizeof( struct HTTPClientData_t ));
uint8_t HTTPClient::ThreadsCounter = 0;
string HTTPClient::UserAgent = "";
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
 */
void HTTPClient::Query(string URL, uint16_t Port, QueryType Type, bool ToFront,
        ReadStarted ReadStartedCallback, ReadBody ReadBodyCallback, ReadFinished ReadFinishedCallback, Aborted AbortedCallback) {

	HTTPClientData_t QueryData;

	strncpy(QueryData.URL   , URL.c_str(), 64);

	QueryData.Port    = Port;
	QueryData.Method  = Type;

	QueryData.ReadStartedCallback   = ReadStartedCallback;
	QueryData.ReadBodyCallback      = ReadBodyCallback;
	QueryData.ReadFinishedCallback  = ReadFinishedCallback;
	QueryData.AbortedCallback       = AbortedCallback;

	Query(QueryData, ToFront);
}

/**
 * @brief Add query to the request queue.
 *
 * @param [in] Query HTTPClientData_t struct with query info
 * @param [in] ToFront If query is primary it will be better to send it to the front of queue
 */

void HTTPClient::Query(HTTPClientData_t Query, bool ToFront) {
	if( Queue == 0 ) {
		ESP_LOGE(tag, "Failed to create queue");
		return;
	}

	CheckUserAgent();

	if (ToFront)
		FreeRTOS::Queue::SendToFront(Queue, &Query, (TickType_t) Settings.HTTPClient.BlockTicks );
	else
		FreeRTOS::Queue::SendToBack(Queue, &Query, (TickType_t) Settings.HTTPClient.BlockTicks );

	if (ThreadsCounter <= 0) {
		ThreadsCounter = 1;
		FreeRTOS::StartTask(HTTPClientTask, "HTTPClientTask", (void *)(uint32_t)ThreadsCounter, Settings.HTTPClient.ThreadStackSize);
	}
	else if (FreeRTOS::Queue::Count(Queue) >= Settings.HTTPClient.NewThreadStep && ThreadsCounter < Settings.HTTPClient.ThreadsMax) {
		ThreadsCounter++;
		FreeRTOS::StartTask(HTTPClient::HTTPClientTask, "HTTPClientTask", (void *)(uint32_t)ThreadsCounter, Settings.HTTPClient.ThreadStackSize);
	}
}

esp_err_t HTTPClient::QueryHandler(esp_http_client_event_t *event)
{
	HTTPClientData_t ClientData = *(HTTPClientData_t*)event->user_data;

    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
        	Failed(ClientData);
            break;
        case HTTP_EVENT_ON_CONNECTED:
            //ESP_LOGI(tag, "HTTP_EVENT_ON_CONNECTED");
			if (ClientData.ReadStartedCallback != NULL)
				ClientData.ReadStartedCallback(ClientData.URL);
            break;
        case HTTP_EVENT_HEADER_SENT:
            //ESP_LOGI(tag, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            //ESP_LOGI(tag, "HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
			if (ClientData.ReadBodyCallback != NULL)
	            if (!esp_http_client_is_chunked_response(event->client))
					if (!ClientData.ReadBodyCallback((char*)event->data, event->data_len, ClientData.URL))
						HTTPClient::Failed(ClientData);
            break;
        case HTTP_EVENT_ON_FINISH:
			if (ClientData.ReadFinishedCallback != NULL)
				ClientData.ReadFinishedCallback(ClientData.URL);
			break;

        case HTTP_EVENT_DISCONNECTED:
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
	ESP_LOGD(tag, "Task %u created", (uint32_t)TaskData);

	HTTPClientData_t ClientData;

	if (Queue != 0)
		while (FreeRTOS::Queue::Receive(HTTPClient::Queue, &ClientData, (TickType_t) Settings.HTTPClient.BlockTicks)) {
			esp_http_client_config_t config;
			config.url 		= ClientData.URL;
			config.port 	= ClientData.Port;

			config.timeout_ms = 7000;
			config.buffer_size = ClientData.BufferSize;

			string URLString = ClientData.URL;

			switch (ClientData.Method) {
				case POST   : config.method = esp_http_client_method_t::HTTP_METHOD_POST; 	break;
				case DELETE : config.method = esp_http_client_method_t::HTTP_METHOD_DELETE; break;
				case GET    :
				default     : config.method = esp_http_client_method_t::HTTP_METHOD_GET; 	break;
			}

			config.user_data 	= (void*)&ClientData;
			config.event_handler= QueryHandler;

			esp_http_client_handle_t Handle = esp_http_client_init(&config);
			esp_http_client_set_header(Handle, "User-Agent", UserAgent.c_str());
			esp_err_t err = esp_http_client_perform(Handle);

			if (err == ESP_OK) {
				ESP_LOGI(tag, "Performing HTTP request success");
			}
			if (err != ESP_OK) {
				ESP_LOGE(tag, "Connect to http server failed: %s", esp_err_to_name(err));
				HTTPClient::Failed(ClientData);
				continue;
			}

			if (Handle != NULL) {
				esp_http_client_close(Handle);
				esp_http_client_cleanup(Handle);
			}
		}

	ESP_LOGD(tag, "Task %u removed", (uint32_t)TaskData);
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
		ClientData.AbortedCallback(ClientData.URL);
}

void HTTPClient::CheckUserAgent() {
	if (UserAgent == "" && Settings.eFuse.DeviceID > 0)
		UserAgent = "LOOK.in\\" + Settings.FirmwareVersion + " " + Converter::ToHexString(Settings.eFuse.DeviceID,8);
}
