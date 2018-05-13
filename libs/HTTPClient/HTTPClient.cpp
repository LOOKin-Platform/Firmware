/*
*    HTTPClient.h
*    Class to make HTTP Queries
*
*/

#include "HTTPClient.h"

static char tag[] = "HTTPClient";

QueueHandle_t HTTPClient::Queue = FreeRTOS::Queue::Create(Settings.HTTPClient.QueueSize, sizeof( struct HTTPClientData_t ));
uint8_t HTTPClient::ThreadsCounter = 0;

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
void HTTPClient::Query(string Hostname, uint16_t Port, string ContentURI, QueryType Type, string IP, bool ToFront,
        ReadStarted ReadStartedCallback, ReadBody ReadBodyCallback, ReadFinished ReadFinishedCallback, Aborted AbortedCallback) {

  HTTPClientData_t QueryData;

  strncpy(QueryData.Hostname   , Hostname.c_str(), 32);
  strncpy(QueryData.ContentURI , ContentURI.c_str(), 32);

  QueryData.Port    = Port;
  QueryData.Method  = Type;

  if (!IP.empty()) strncpy(QueryData.IP , IP.c_str(), 15);

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
			if (HTTPClient::Connect(ClientData)) {
				ESP_LOGI(tag, "Connected to http server");
			}
			else {
				ESP_LOGE(tag, "Connect to http server failed!");
				HTTPClient::Failed(ClientData);
				continue;
			}

			int res = -1;

			// send request to http server
			string Method = "";
			switch (ClientData.Method) {
				case POST   : Method = "GET"    ; break;
				case DELETE : Method = "DELETE" ; break;
				case GET    :
				default     : Method = "GET"    ; break;
			}

			char Request[128];
			sprintf (Request, "%s %s HTTP/1.1\r\nHost:%s:%u \r\n\r\n", Method.c_str(), ClientData.ContentURI, ClientData.Hostname, ClientData.Port);

			res = send(ClientData.SocketID, Request, sizeof(Request), 0);
			if (res == -1) {
				ESP_LOGE(tag, "Send request to server failed");
				HTTPClient::Failed(ClientData);
				continue;
			}
			else
				ESP_LOGI(tag, "Send request to server succeeded");

			bool resp_body_start = false, flag = true;
			char ReadData[Settings.HTTPClient.NetbuffLen + 1] = { 0 };
			char Text[Settings.HTTPClient.NetbuffLen + 1]     = { 0 };

			if (ClientData.ReadStartedCallback != NULL)
				ClientData.ReadStartedCallback(ClientData.IP);

			while (flag) {
				memset(Text, 0, Settings.HTTPClient.NetbuffLen);
				memset(ReadData, 0, Settings.HTTPClient.NetbuffLen);

				int BuffLen = recv(ClientData.SocketID, Text, Settings.HTTPClient.NetbuffLen, 0);

				if (BuffLen < 0) {
					ESP_LOGE(tag, "Error: receive data error! errno=%d", errno);
					HTTPClient::Failed(ClientData);
					break;
				}
				else if (BuffLen > 0 && !resp_body_start) { // skip header
					memcpy(ReadData, Text, BuffLen);
					resp_body_start = HTTPClient::ReadPastHttpHeader(ClientData, Text, BuffLen);
				}
				else if (BuffLen > 0 && resp_body_start) {
					memcpy(ReadData, Text, BuffLen);

					if (ClientData.ReadBodyCallback != NULL)
						if (!ClientData.ReadBodyCallback(ReadData, BuffLen, ClientData.IP)) {
							HTTPClient::Failed(ClientData);
							break;
						}
				}
				else if (BuffLen == 0) {
					flag = false;
					ESP_LOGI(tag, "Connection closed, all packets received");
					close(ClientData.SocketID);
				}
				else
					ESP_LOGE(tag, "Unexpected recv result");
			}

			if (ClientData.ReadFinishedCallback != NULL)
				ClientData.ReadFinishedCallback(ClientData.IP);
		}

	ESP_LOGD(tag, "Task %u removed", (uint32_t)TaskData);
    HTTPClient::ThreadsCounter--;
    FreeRTOS::DeleteTask();
}

/**
 * @brief Attempt to connect to HTTP server.
 *
 * @param [in] ClientData HTTPClientData_t struct with query info.
 * @result Is connect to HTTP server succes or not
 */
bool HTTPClient::Connect(HTTPClientData_t &ClientData) {
	if (strlen(ClientData.IP) == 0) {
		char *IP = ResolveIP(ClientData.Hostname, ClientData.Port);
		strcpy(ClientData.IP, IP);

		if (strlen(ClientData.IP) == 0)
			return false;
	}

	ClientData.SocketID = socket(PF_INET, SOCK_STREAM, 0);

	if (ClientData.SocketID == -1) {
		ESP_LOGE(tag, "Create socket failed!");
		return false;
	}

	struct sockaddr_in sock_info;
	// set connect info
	memset(&sock_info, 0, sizeof(struct sockaddr_in));
	sock_info.sin_family = AF_INET;
	sock_info.sin_addr.s_addr = inet_addr(ClientData.IP);
	sock_info.sin_port = htons(ClientData.Port);

	ESP_LOGI("IP"	,"%s",ClientData.IP);
	ESP_LOGI("Port"	,"%i",ClientData.Port);

	// connect to http server
	if (connect(ClientData.SocketID, (struct sockaddr *)&sock_info, sizeof(sock_info)) == -1) {
		ESP_LOGE(tag, "Connect to server failed! errno=%d", errno);
		close(ClientData.SocketID);

		return false;
	}
	else
		return true;

	return false;
}

/**
 * @brief Resolving an IP by hostname
 *
 * @param [in] Hostname Hostname to IP resolving
 * @return Resolved IP in string representation, e. g. 192.168.1.10
 */
char* HTTPClient::ResolveIP(const char *Hostname, uint16_t Port) {
	ESP_LOGI(tag, "IP not specified. Trying to resolve IP address");

	struct addrinfo hints, *infoptr;

    memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses //AF_UNSPEC for all
	hints.ai_protocol = IPPROTO_TCP;

	int result = ::getaddrinfo(Hostname, Converter::ToString(Port).c_str(), &hints, &infoptr);

	if (result) {
		ESP_LOGE(tag, "Can't resolve IP adress. Error code %i", result);
	    return "";
	}

	struct sockaddr_in * p2 = (struct sockaddr_in *)(infoptr->ai_addr);
	return inet_ntoa(p2->sin_addr);
}

/**
 * @brief Read buffer by byte still delimeter
 *
 * @param [in] buffer Buffer
 * @param [in] delim Delimeter
 * @param [in] len Buffer lenght
 * @return Read bytes counts
 */
int HTTPClient::ReadUntil(char *buffer, char delim, int len) {
	int i = 0;
	while (buffer[i] != delim && i < len) {
      ++i;
	}

	return i + 1;
}

 /**
  * @brief Resolve a packet from http socket
  *
  * @param [in] ClientData HTTPClientData_t struct with query info.
  * @param [in] text Buffer
  * @param [in] total_len Buffer length
  * @return Return true if packet including \\r\\n\\r\\n that means http packet header finished, otherwise return false
  */
bool HTTPClient::ReadPastHttpHeader(HTTPClientData_t &ClientData, char text[], int total_len) {
	/* i means current position */
	int i = 0, i_read_len = 0;
	char ReadData[Settings.HTTPClient.NetbuffLen + 1] = { 0 };

	while (text[i] != 0 && i < total_len) {
		i_read_len = ReadUntil(&text[i], '\n', total_len);
		// if we resolve \r\n line,we think packet header is finished

		if (i_read_len == 2) {
			int i_write_len = total_len - (i + 2);

			memset(ReadData, 0, Settings.HTTPClient.NetbuffLen);
			memcpy(ReadData, &(text[i + 2]), i_write_len);

			if (ClientData.ReadBodyCallback != NULL) {
				if (!ClientData.ReadBodyCallback(ReadData,i_write_len, ClientData.IP))
					return false;
			}

			return true;
		}

		i += i_read_len;
	}

	return false;
}

/**
 * @brief Critical HTTP-query error handling
 *
 * @param [in] &ClientData HTTPClientData_t struct with query info.
 */
void HTTPClient::Failed(HTTPClientData_t &ClientData) {
	ESP_LOGE(tag, "Exiting HTTPClient task due to fatal error...");
	close(ClientData.SocketID);

	if (ClientData.AbortedCallback != NULL)
		ClientData.AbortedCallback(ClientData.IP);
}
