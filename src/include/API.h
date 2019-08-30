/*
*    API.h
*    HTTP Api entry point
*
*/

#ifndef API_H
#define API_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <map>

#include <esp_log.h>

#include "Globals.h"
#include "Query.h"
#include "WebServer.h"

using namespace std;

#define SetupPage1 "<html><head><meta charset='UTF-8'><meta name='apple-mobile-web-app-capable' content='yes'><style> body { margin : 0px; } h1 { font-size : 56px; font-weight : bold; margin : 30px 0px; } .content { align-items : center; width : 100%; margin : auto; } .element { width : 90%; padding : 0px 20px; min-height : 100px !important; line-height : 30px; } p.element { font-size : 40px; line-height : 40px; margin-bottom : 30px; } select { width : 90%; margin : auto; line-height : 40px; background-color : white; padding-left : 12px; } .lang { margin : 10px; padding : 10px; font-size : 25px; cursor : pointer; } .lang.active { border : 1px solid black; } #Success { margin-top : 20px; } .hide { display : none; } .logo { text-align : center; font-size : 96px; font-family : 'Arial'; height : 100px; margin-top : 100px; } input, button, select { height : 100px; font-size : 40px; line-height : 40px; border : 1px solid black; margin-bottom : 50px; background-color : white; } @media (min-width: 1025px) { .content { width: 600px; } .logo { font-size: 72px; margin-top: 50px; } h1 { font-size: 32px ; margin: 0px 0px; } .element { min-height: 20px !important; } p.element{ font-size: 25px ; line-height: 25px; } input, button, select { margin-bottom: 20px; line-height: 20px; font-size: 20px; height: 40px;} } </style><script type='text/javascript'> var Language = 'ru'; var SSIDs = ["
#define SetupPage2 "]; var CurrentDevice = '"
#define SetupPage3 "'; var Title = new Map([ ['ru', 'Подключение устройства'], ['en', 'Wi-fi connection'] ]); var Description = new Map([ ['ru', 'Для подключения устройства к вашей сети Wi-Fi необходимо выбрать её в списке и задать для нее пароль'], ['en', 'To connect your device to your Wi-Fi network, select it in the list and set a password for it'] ]); var Device = new Map([ ['ru', 'Устройство'], ['en', 'Device'] ]); var Connect = new Map([ ['ru', 'Подключить'], ['en', 'Connect'] ]); var ConnectProcess = new Map([ ['ru', 'Идет подключение...'], ['en', 'Connecting...'] ]); var PasswordHint = new Map([ ['ru', 'Пароль от Wi-Fi'], ['en', 'Wi-Fi password'] ]); var WiFiHint = new Map([ ['ru', 'Выберите Wi-Fi сеть'], ['en', 'Choose Wi-Fi network'] ]); var PasswordEmpty = new Map([ ['ru', 'Пароль не может быть пустым'], ['en', 'Password cannot be empty'] ]); var SaveError = new Map([ ['ru', 'При настройки устройства произошла непредвиденная ошибка'], ['en', 'Unexpected error occured'] ]); var SetupDone = new Map([ ['ru', 'Настройка устройства прошла успешно.<br/><br/>Подключитесь к указанной ранее Wi-Fi сети для и откройте приложение LOOK.in Hub для продолжения работы с устройством.<br/><br/>Данную страницу можно закрыть.'], ['en', 'The device setup was successful.<br/><br/>Connect to the previously specified Wi-Fi network and open the LOOK.in Hub mobile app to continue working with the device.<br/><br/>This page can be closed.'] ]); function OnSave() { var SSIDElement = document.getElementById('ssid'); var SSID = SSIDElement.options[SSIDElement.selectedIndex].text; var Password = document.getElementById('password').value; if (Password == '') { alert(PasswordEmpty.get(Language)); return; } var xhr = new XMLHttpRequest(); var body = '{' + '\"WiFiSSID\":\"' + encodeURIComponent(SSID) + '\", ' + '\"WiFiPassword\": \"' + encodeURIComponent(Password) + '\"}'; document.getElementById('connect').innerHTML = ConnectProcess.get(Language); document.getElementById('connect').disabled = true; xhr.open('POST', '/network', true); xhr.setRequestHeader('Content-Type', 'text-plain'); xhr.onreadystatechange = function() { if (this.readyState != 4) return;  if(this.status === 200) { document.getElementById('StartScreen').classList.add('hide'); document.getElementById('Success').classList.remove('hide'); var xhrConnect = new XMLHttpRequest(); xhrConnect.open('GET', '/network/connect', true); xhrConnect.send(null); } else alert(SaveError.get(Language) + ':\\n\\n' +this.responseText); }; xhr.send(body); } function LoadData(lang = Language) { Language = lang; document.getElementById('title') .innerHTML = Title.get(lang); document.getElementById('description') .innerHTML = Description.get(lang); document.getElementById('device') .innerHTML = Device.get(lang); document.getElementById('connect') .innerHTML = Connect.get(lang); document.getElementById('password') .placeholder= PasswordHint.get(lang); document.getElementById('Success') .innerHTML = SetupDone.get(lang); document.getElementById('deviceTypeAndID').innerHTML = CurrentDevice; var WiFiSelect = document.getElementById('ssid'); document.getElementById('ssid').innerHTML = ''; var FirstOption = document.createElement('option'); FirstOption.innerHTML = WiFiHint.get(lang); FirstOption.disabled = true; WiFiSelect.appendChild(FirstOption); for(var i = 0; i < SSIDs.length; i++) { var Option = document.createElement('option'); Option.innerHTML = SSIDs[i]; Option.value = SSIDs[i]; WiFiSelect.appendChild(Option); } var LangRE = /^lang-/; var Elements = document.getElementsByTagName('*'); for (var i = Elements.length; i-- ;) if (LangRE.test(Elements[i].id)) Elements[i].classList.remove('active'); document.getElementById('lang-' + lang).classList.add('active'); } </script></head><body onLoad='LoadData()'><div class='content' align='center'><a class='lang' id='lang-en' onclick='LoadData(&quot;en&quot;)'>English</a><a class='lang' id='lang-ru' onclick='LoadData(&quot;ru&quot;)'>Русский</a><div class='logo'>LOOK.in</div><h1 id='title'></h1><div align='center' id='StartScreen'><p class='element' id='description'></p><p class='element'><span id='device'></span>: <span id='deviceTypeAndID'></span></p><select id='ssid' placeholder='Выберите точку доступа'></select><input id = 'password' class='element' placeholder='Пароль от WiFi' /><button id = 'connect' class='element' onclick='OnSave();' ></button></div><div align='center' id='Success' class='hide'></div></div></body></html>";

class API {
	public:
		static void Handle(WebServer_t::Response &, Query_t Query, httpd_req_t *Request = NULL);
		static void Handle(WebServer_t::Response &, QueryType Type, vector<string> URLParts, map<string,string> Params, string RequestBody = "", httpd_req_t *Request = NULL);

	private:
		static string GetSetupPage();
};

#endif
