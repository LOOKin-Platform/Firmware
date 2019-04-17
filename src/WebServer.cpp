/*
*    WebServer.cpp
*    Class for handling TCP and UDP connections
*
*/

using namespace std;

#include <stdio.h>
#include <string.h>

#include "Globals.h"
#include "WebServer.h"
#include "Query.h"
#include "API.h"

#include <esp_log.h>

static char tag[] = "WebServer";

static vector<uint16_t> UDPPorts 				= { Settings.WiFi.UPDPort };
QueueHandle_t WebServer_t::UDPBroadcastQueue 	= FreeRTOS::Queue::Create(Settings.WiFi.UDPBroadcastQueue.Size, sizeof(UDPBroacastQueueItem));

WebServer_t::WebServer_t() {
	HTTPListenerTaskHandle  = NULL;
	UDPListenerTaskHandle   = NULL;
}

void WebServer_t::Start() {

	if (HTTPListenerTaskHandle == NULL)
		HTTPListenerTaskHandle  = FreeRTOS::StartTask(HTTPListenerTask, "HTTPListenerTask", NULL, 6144);

	if (UDPListenerTaskHandle == NULL)
		UDPListenerTaskHandle   = FreeRTOS::StartTask(UDPListenerTask , "UDPListenerTask" , NULL, 4096);
}

void WebServer_t::Stop() {
	FreeRTOS::DeleteTask(HTTPListenerTaskHandle);
	FreeRTOS::DeleteTask(UDPListenerTaskHandle);
}

void WebServer_t::UDPSendBroadcastAlive() {
	UDPSendBroadcast(UDPAliveBody());
}

void WebServer_t::UDPSendBroadcastDiscover() {
	UDPSendBroadcast(UDPDiscoverBody());
}

void WebServer_t::UDPSendBroadcastUpdated(uint8_t SensorID, string Value, uint8_t Repeat) {
	for (int i=0; i < Repeat; i++)
		UDPSendBroadcast(UDPUpdatedBody(SensorID, Value));
}

string WebServer_t::UDPAliveBody() {
	string Message = "Alive!"
			+ Device.IDToString()
			+ ":" + Device.Type.ToHexString()
			+ ":" + (Device.Type.IsBattery() ? "0" : "1")
			+ ":" + Network.IPToString()
			+ ":" + Converter::ToHexString(Automation.CurrentVersion(),8)
			+ ":" + Converter::ToHexString(Storage.CurrentVersion(),4);

	return Settings.WiFi.UDPPacketPrefix + Message;
}

string WebServer_t::UDPDiscoverBody(string ID) {
	string Message = (ID != "") ? ("Discover!" + ID) : "Discover!";
	return Settings.WiFi.UDPPacketPrefix + Message;
}

string WebServer_t::UDPUpdatedBody(uint8_t SensorID, string Value) {
	string Message = "Updated!" + Device.IDToString() + ":" + Converter::ToHexString(SensorID, 2) + ":" + Value;
	return Settings.WiFi.UDPPacketPrefix + Message;
}

void WebServer_t::UDPSendBroadcast(string Message) {
	if (!WiFi.IsRunning()) {
		UDPSendBroadcastQueueAdd(Message);
		ESP_LOGE(tag, "WiFi switched off - can't send UDP message");
		return;
	}

	struct netconn *Connection;
	struct netbuf *Buffer;
	char *Data;

	if (Message.length() > 128)
		Message = Message.substr(0, 128);

	for (uint16_t Port : UDPPorts) {
		Connection = netconn_new(NETCONN_UDP);
		netconn_connect(Connection, IP_ADDR_BROADCAST, Port);

		Buffer  = netbuf_new();
		Data    = (char *)netbuf_alloc(Buffer, Message.length());
		memcpy (Data, Message.c_str(), Message.length());
		netconn_send(Connection, Buffer);

		netconn_close(Connection);
		netconn_delete(Connection);

		netbuf_free(Buffer);
		netbuf_delete(Buffer); 		// De-allocate packet buffer

		ESP_LOGI(tag, "UDP broadcast \"%s\" sended to port %d", Message.c_str(), Port);
	}
}

void WebServer_t::UDPSendBroacastFromQueue() {
	UDPBroacastQueueItem ItemToSend;

	if (UDPBroadcastQueue != 0)
		while (FreeRTOS::Queue::Receive(UDPBroadcastQueue, &ItemToSend, (TickType_t) Settings.WiFi.UDPBroadcastQueue.BlockTicks)) {
			if (ItemToSend.Updated + Settings.WiFi.UDPBroadcastQueue.CheckGap >= Time::Uptime()) {
				for (int i=0; i<3; i++) {
					UDPSendBroadcast(string(ItemToSend.Message));
					FreeRTOS::Sleep(Settings.WiFi.UDPBroadcastQueue.Pause);
				}
			}
		}
}

void WebServer_t::UDPSendBroadcastQueueAdd(string Message) {
	if (Message.size() > Settings.WiFi.UDPBroadcastQueue.Size)
		Message = Message.substr(0, Settings.WiFi.UDPBroadcastQueue.MaxMessageSize);

	UDPBroacastQueueItem ItemToSend;
	ItemToSend.Updated = Time::Uptime();
	strcpy(ItemToSend.Message, Message.c_str());

	FreeRTOS::Queue::SendToBack(UDPBroadcastQueue, &ItemToSend, (TickType_t) Settings.HTTPClient.BlockTicks );
}

void WebServer_t::UDPListenerTask(void *data) {
	if (!WiFi.IsRunning()) {
		ESP_LOGE(tag, "WiFi switched off - can't start UDP listener task");
		WebServer.UDPListenerTaskHandle = NULL;
		FreeRTOS::DeleteTask();
		return;
	}
	ESP_LOGD(tag, "UDPListenerTask Run");

	struct netconn 	*UDPConnection;
	struct netbuf 	*UDPInBuffer, *UDPOutBuffer;
	char 			*UDPInData	, *UDPOutData;
	u16_t inDataLen;
	err_t err;

	UDPConnection = netconn_new(NETCONN_UDP);
	netconn_bind(UDPConnection, IP_ADDR_ANY, Settings.WiFi.UPDPort);

	do {
		err = netconn_recv(UDPConnection, &UDPInBuffer);

		if (err == ERR_OK) {
			netbuf_data(UDPInBuffer, (void * *)&UDPInData, &inDataLen);
			string Datagram = UDPInData;

			ESP_LOGI(tag, "UDP received \"%s\"", Datagram.c_str());

			if(find(UDPPorts.begin(), UDPPorts.end(), UDPInBuffer->port) == UDPPorts.end()) {
				if (UDPPorts.size() == Settings.WiFi.UDPHoldPortsMax) UDPPorts.erase(UDPPorts.begin() + 1);
				UDPPorts.push_back(UDPInBuffer->port);
			}

			UDPOutBuffer = netbuf_new();

			// Redirect UDP message if in access point mode
			//if (WiFi_t::GetMode() == WIFI_MODE_AP_STR)
			//	WebServer->UDPSendBroadcast(Datagram);

			// answer to the Discover query
			if (Datagram == WebServer_t::UDPDiscoverBody() || Datagram == WebServer_t::UDPDiscoverBody(Device.IDToString())) {
				string Answer = WebServer_t::UDPAliveBody();

				UDPOutData = (char *)netbuf_alloc(UDPOutBuffer, Answer.length());
				memcpy (UDPOutData, Answer.c_str(), Answer.length());

				netconn_sendto(UDPConnection, UDPOutBuffer, &UDPInBuffer->addr, UDPInBuffer->port);

				ESP_LOGD(tag, "UDP \"%s\" sended to %s:%u", Answer.c_str(), inet_ntoa(UDPInBuffer->addr), UDPInBuffer->port);
			}

			string AliveText = Settings.WiFi.UDPPacketPrefix + string("Alive!");
			size_t AliveFound = Datagram.find(AliveText);

			if (AliveFound != string::npos) {
				string Alive = Datagram;
				Alive.replace(Alive.find(AliveText),AliveText.length(),"");
				vector<string> Data = Converter::StringToVector(Alive, ":");

				while (Data.size() < 6)
					Data.push_back("");

				Network.DeviceInfoReceived(Data[0], Data[1], Data[2], Data[3], Data[4], Data[5]);
			}

			netbuf_delete(UDPOutBuffer);
			netbuf_delete(UDPInBuffer);
		}

	} while(err == ERR_OK);

	netbuf_delete(UDPInBuffer);
	netconn_close(UDPConnection);
	netconn_delete(UDPConnection);
}

void WebServer_t::HTTPListenerTask(void *data) {
	if (!WiFi.IsRunning()) {
		ESP_LOGE(tag, "WiFi switched off - can't start HTTP listener task");
		WebServer.HTTPListenerTaskHandle = NULL;
		FreeRTOS::DeleteTask();
		return;
	}
	ESP_LOGD(tag, "HTTPListenerTask Run");

	struct netconn	*HTTPConnection, *HTTPNewConnection;
	err_t err;

	HTTPConnection = netconn_new(NETCONN_TCP);
	netconn_bind(HTTPConnection, NULL, 80);
	netconn_listen(HTTPConnection);

	do {
		err = netconn_accept(HTTPConnection, &HTTPNewConnection);

		if (err == ERR_OK) {
			WebServer_t::HandleHTTP(HTTPNewConnection);
			netconn_close(HTTPNewConnection);
			netconn_delete(HTTPNewConnection);
		}
	} while(err == ERR_OK);

	netconn_close(HTTPConnection);
	netconn_delete(HTTPConnection);
}

void WebServer_t::HandleHTTP(struct netconn *conn) {
	ESP_LOGD(tag, "Handle HTTP Query");

	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;

	err_t err;
	string HTTPString;

	conn->recv_timeout = 50; // timeout on 50 msecs

	while((err = netconn_recv(conn,&inbuf)) == ERR_OK) {
		do {
			netbuf_data(inbuf, (void **)&buf, &buflen);

			if (HTTPString.size() + buflen > Settings.WiFi.HTTPMaxQueryLen) {
				HTTPString.clear();
				WebServer_t::Response Result;
				Result.SetInvalid();
				Result.Body = "{ \"success\" : \"false\", \"message\" : \"Query length exceed max. It is "+
						Converter::ToString(Settings.WiFi.HTTPMaxQueryLen) + "for UTF-8 queries and " +
						Converter::ToString((uint16_t)(Settings.WiFi.HTTPMaxQueryLen/2)) + "for UTF-16 queries\"}";
				Write(conn, Result.toString());
				netbuf_free(inbuf);
				netbuf_delete(inbuf);
				return;
			}

			buf[buflen] = '\0';
			HTTPString += string(buf);

			ESP_LOGI(tag,"RAM left %d", system_get_free_heap_size());
		} while(netbuf_next(inbuf) >= 0);

		netbuf_free(inbuf);
		netbuf_delete(inbuf);
	}

	netbuf_delete(inbuf);

	if (!HTTPString.empty()) {
		Query_t Query(HTTPString);
		WebServer_t::Response Result;
		HTTPString = "";

		API::Handle(Result, Query);
		Write(conn, Result.toString());

		Result.Clear();
	}
}

void WebServer_t::Write(struct netconn *conn, string Data) {
	uint8_t PartSize = 100;

	for (int i=0; i < Data.size(); i += PartSize)
		netconn_write(conn,
				Data.substr(i, ((i + PartSize <= Data.size()) ? PartSize : Data.size() - i)).c_str(),
				(i + PartSize <= Data.size()) ? PartSize : Data.size() - i,
				NETCONN_NOCOPY);
}

/*
*    WebServer_t::Response
*    class for building user-friendly browser answer
*
*/

WebServer_t::Response::Response() {
	ResponseCode = CODE::OK;
	ContentType  = TYPE::JSON;
}

string WebServer_t::Response::toString() {
	string Result = "";

	Result = "HTTP/1.1 " + ResponseCodeToString() + "\r\n";
	Result += "Content-type:" + ContentTypeToString() + "; charset=utf-8\r\n\r\n";
	Result += Body;

	return Result;
}

void WebServer_t::Response::SetSuccess() {
	ResponseCode  = CODE::OK;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"true\"}";
}

void WebServer_t::Response::SetFail() {
	ResponseCode  = CODE::ERROR;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"false\"}";
}

void WebServer_t::Response::SetInvalid() {
	ResponseCode  = CODE::INVALID;
	ContentType   = TYPE::JSON;
	Body          = "{\"success\" : \"false\"}";
}

void WebServer_t::Response::Clear() {
	Body = "";
	ResponseCode = OK;
	ContentType = JSON;
}

string WebServer_t::Response::ResponseCodeToString() {
	switch (ResponseCode) {
		case CODE::INVALID  : return "400"; break;
		case CODE::ERROR    : return "500"; break;
		default             : return "200"; break;
	}
}

string WebServer_t::Response::ContentTypeToString() {
	switch (ContentType) {
		case TYPE::PLAIN  : return "text/plain"; 		break;
		case TYPE::JSON   : return "application/json"; 	break;
		case TYPE::HTML   : return "text/html"; 		break;
		default           : return "";
	}
}
