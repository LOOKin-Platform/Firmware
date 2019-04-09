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
string WebServer_t::SetupPage					= "<html><head><meta charset='UTF-8'><meta name='apple-mobile-web-app-capable' content='yes'><style> body { margin : 0px; } h1 { font-size : 56px; font-weight : bold; margin : 30px 0px; } .content { align-items : center; width : 100%; margin : auto; } .element { width : 90%; padding : 0px 20px; min-height : 100px !important; line-height : 30px; } p.element { font-size : 40px; line-height : 40px; margin-bottom : 30px; } select { width : 90%; margin : auto; line-height : 40px; background-color : white; padding-left : 12px; } .lang { margin : 10px; padding : 10px; font-size : 25px; cursor : pointer; } .lang.active { border : 1px solid black; } #Success { margin-top : 20px; } .hide { display : none; } .logo { text-align : center; font-size : 96px; font-family : 'Arial'; height : 100px; margin-top : 100px; } input, button, select { height : 100px; font-size : 40px; line-height : 40px; border : 1px solid black; margin-bottom : 50px; background-color : white; } @media (min-width: 1025px) { .content { width: 600px; } .logo { font-size: 72px; margin-top: 50px; } h1 { font-size: 32px ; margin: 0px 0px; } .element { min-height: 20px !important; } p.element{ font-size: 25px ; line-height: 25px; } input, button, select { margin-bottom: 20px; line-height: 20px; font-size: 20px; height: 40px;} } </style><script type='text/javascript'> var Language = 'ru'; var SSIDs = [{SSID_LIST}]; var CurrentDevice = '{CURRENT_DEVICE}'; var Title = new Map([ ['ru', 'Подключение устройства'], ['en', 'Wi-fi connection'] ]); var Description = new Map([ ['ru', 'Для подключения устройства к вашей сети Wi-Fi необходимо выбрать её в списке и задать для нее пароль'], ['en', 'To connect your device to your Wi-Fi network, select it in the list and set a password for it'] ]); var Device = new Map([ ['ru', 'Устройство'], ['en', 'Device'] ]); var Connect = new Map([ ['ru', 'Подключить'], ['en', 'Connect'] ]); var PasswordHint = new Map([ ['ru', 'Пароль от Wi-Fi'], ['en', 'Wi-Fi password'] ]); var WiFiHint = new Map([ ['ru', 'Выберите Wi-Fi сеть'], ['en', 'Choose Wi-Fi network'] ]); var PasswordEmpty = new Map([ ['ru', 'Пароль не может быть пустым'], ['en', 'Password cannot be empty'] ]); var SaveError = new Map([ ['ru', 'При настройки устройства произошла непредвиденная ошибка'], ['en', 'Unexpected error occured'] ]); var SetupDone = new Map([ ['ru', 'Настройка устройства прошла успешно.<br/><br/>Подключитесь к указанной ранее Wi-Fi сети для и откройте приложение LOOK.in Hub для продолжения работы с устройством.<br/><br/>Данную страницу можно закрыть.'], ['en', 'The device setup was successful.<br/><br/>Connect to the previously specified Wi-Fi network and open the LOOK.in Hub mobile app to continue working with the device.<br/><br/>This page can be closed.'] ]); function OnSave() { var SSIDElement = document.getElementById('ssid'); var SSID = SSIDElement.options[SSIDElement.selectedIndex].text; var Password = document.getElementById('password').value; if (Password == '') { alert(PasswordEmpty.get(Language)); return; } var xhr = new XMLHttpRequest(); var body = '{' + '\"WiFiSSID\":\"' + encodeURIComponent(SSID) + '\", ' + '\"WiFiPassword\": \"' + encodeURIComponent(Password) + '\"}'; xhr.open('POST', '/network', true); xhr.setRequestHeader('Content-Type', 'text-plain'); xhr.onreadystatechange = function() { if (this.readyState != 4) return; alert(this.status);  if(this.status === 200) { var xhrConnect = new XMLHttpRequest(); xhrConnect.open('GET', '/network/connect', true); xhrConnect.onreadystatechange = function() { document.getElementById('StartScreen').classList.add('hide'); document.getElementById('Success').classList.remove('hide'); }; xhrConnect.send(null); } else alert(SaveError.get(Language) + ':\\n\\n' +this.responseText); }; xhr.send(body); } function LoadData(lang = Language) { Language = lang; document.getElementById('title') .innerHTML = Title.get(lang); document.getElementById('description') .innerHTML = Description.get(lang); document.getElementById('device') .innerHTML = Device.get(lang); document.getElementById('connect') .innerHTML = Connect.get(lang); document.getElementById('password') .placeholder= PasswordHint.get(lang); document.getElementById('Success') .innerHTML = SetupDone.get(lang); document.getElementById('deviceTypeAndID').innerHTML = CurrentDevice; var WiFiSelect = document.getElementById('ssid'); document.getElementById('ssid').innerHTML = ''; var FirstOption = document.createElement('option'); FirstOption.innerHTML = WiFiHint.get(lang); FirstOption.disabled = true; WiFiSelect.appendChild(FirstOption); for(var i = 0; i < SSIDs.length; i++) { var Option = document.createElement('option'); Option.innerHTML = SSIDs[i]; Option.value = SSIDs[i]; WiFiSelect.appendChild(Option); } var LangRE = /^lang-/; var Elements = document.getElementsByTagName('*'); for (var i = Elements.length; i-- ;) if (LangRE.test(Elements[i].id)) Elements[i].classList.remove('active'); document.getElementById('lang-' + lang).classList.add('active'); } </script></head><body onLoad='LoadData()'><div class='content' align='center'><a class='lang' id='lang-en' onclick='LoadData(&quot;en&quot;)'>English</a><a class='lang' id='lang-ru' onclick='LoadData(&quot;ru&quot;)'>Русский</a><div class='logo'>LOOK.in</div><h1 id='title'></h1><div align='center' id='StartScreen'><p class='element' id='description'></p><p class='element'><span id='device'></span>: <span id='deviceTypeAndID'></span></p><select id='ssid' placeholder='Выберите точку доступа'></select><input id = 'password' class='element' placeholder='Пароль от WiFi' /><button id = 'connect' class='element' onclick='OnSave();' ></button></div><div align='center' id='Success' class='hide'></div></div></body></html>";

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

static bool IsSetupPageTemplated = false;
string WebServer_t::GetSetupPage() {
	if (!IsSetupPageTemplated)
	{
		// Prepare index page
	    size_t index = SetupPage.find("{SSID_LIST}");
	    if (index != std::string::npos) {
	    	vector<string> SSIDList;

	    	for (WiFiAPRecord Record : Network.WiFiScannedList)
	    		SSIDList.push_back("'" + Record.getSSID() + "'");

	    	string SSIDListString = Converter::VectorToString(SSIDList, ",");
	    	SetupPage = SetupPage.substr(0, index) + SSIDListString + SetupPage.substr(index + 11);
	    }

	    index = SetupPage.find("{CURRENT_DEVICE}");
	    if (index != std::string::npos)
	    	SetupPage = SetupPage.substr(0, index) + Device.TypeToString() + " " + Device.IDToString() + SetupPage.substr(index + 16);

	    IsSetupPageTemplated = true;
	}

	return SetupPage;
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
