#ifndef MY_WIFI_HANDLER
#define MY_WIFI_HANDLER

#include "Globals.h"

#include <esp_ping.h>
#include <ping/ping_sock.h>

class MyWiFiEventHandler: public WiFiEventHandler {
	public:
		MyWiFiEventHandler();

        static inline bool IsConnectedBefore 	= false;

	private:
		void Generic(void * arg, esp_event_base_t eventBase, int32_t eventId, void * eventData);

		esp_err_t apStart();
		esp_err_t apStop();
		esp_err_t staStart();
		esp_err_t staConnected();
		esp_err_t ConnectionTimeout();
		esp_err_t staDisconnected(system_event_sta_disconnected_t DisconnectedInfo);
        esp_err_t staGotIPv4(ip_event_got_ip_t GotIPv4Info);
		esp_err_t staGotIPv6(ip_event_got_ip6_t GotIPv6Info);

    	void MDNSSetServiceText();

        static FreeRTOS::Timer *IPDidntGetTimer;

        static void IPDidntGetCallback(FreeRTOS::Timer *pTimer);
        static void GatewayPingSuccess(esp_ping_handle_t hdl, void *args);
        static void GatewayPingEnd(esp_ping_handle_t hdl, void *args);
        static void RemoteControlStartTimerCallback(void *Param);

        static inline uint8_t   ConnectionTries		= 0;
        static inline bool      IsIPCheckSuccess 	= false;
        static inline FreeRTOS::Semaphore IsCorrectIPData 	= FreeRTOS::Semaphore("CorrectTCPIPData");
        static inline esp_timer_handle_t RemoteControlStartTimer = NULL;


};

#endif