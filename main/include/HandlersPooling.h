#ifndef HANDLERS_POOLING
#define HANDLERS_POOLING

#include "Globals.h"
#include "BootAndRestore.h"

#include "ping/ping_sock.h"

using PoolingHandler_fn = std::function<void()>;

class HandlersPooling_t {
	private:
		class HandlerItem_t {
			public:
				uint8_t				Index		= 0;
				PoolingHandler_fn 	Handler		= nullptr;
				uint32_t			PeriodU 	= 0;
				uint32_t			PeriodTimer = 0;	
				uint32_t			Priority 	= 5;
		};

		static inline vector<HandlerItem_t> Handlers = vector<HandlerItem_t>();
		static inline uint64_t LastHandlerFired = 0;

	public:
		static void BARCheck();

		static uint8_t RegisterHandler(PoolingHandler_fn Handler, uint32_t PeriodU, uint32_t Priority=5);

		static void CheckHandlers();

		static void Test1();
		static void Test2();

		static void Pool ();

		class PingPeriodicHandler {
			public:
				static void Pool();

				static void FirmwareCheckStarted(const char *IP);
				static bool FirmwareCheckReadBody(char Data[], int DataLen, const char *IP);
				static void FirmwareCheckFinished(const char *IP);
				static void FirmwareCheckFailed(const char *IP);

			private:
				static void PerformLocalPing();
				static void RouterPingFinished(esp_ping_handle_t hdl, void *args);
				static void CheckForPeriodicalyReboot();
	
				static inline uint32_t 	RemotePingRestartCounter 		= 0;
				static inline uint32_t 	RouterPingRestartCounter 		= 0;
				static inline string	FirmwareUpdateURL 				= "";
				static inline FreeRTOS::Semaphore IsRouterPingFinished 	= FreeRTOS::Semaphore("IsRouterPingFinished");
		};


		class WiFiUptimeHandler {
			public:
				static void Start();
				static void Pool();
				static void SetClientModeNextTime(uint32_t);
				static void SetLastAPQueryTime();

				static bool GetIsIPCheckSuccess();

				static void RemoteControlStartTimerCallback(void *Param);
			private:
				static inline uint64_t WiFiStartedTime 		= 0;
				static inline uint32_t BatteryUptime   		= 0;
				static inline uint32_t ClientModeNextTime 	= 0;
		};

	private:


		class NetworkMapHandler {
			public:
				static void 	Pool();
				static void 	SetLastInfoReceivedTimer();
		};

		class EnergyPeriodicHandler {
			public:
				static void Init();
				static void Pool();
				static void InnerHandler(bool SkipTimeCheck = false);

			private:
				static inline bool IsInited 		= false;
				static inline bool IsPowerTypeSet 	= false;
		};

		class OverheatHandler
		{
			public:
				static void Start();
				static void Stop();
				static void Pool();

			private:
				static inline bool 		IsActive 		= true;
				static inline uint16_t	OverheatTimer	= 0;
		};


		class RemoteControlPeriodicHandler {
			public:
				static void Pool();
			private:
				static inline uint32_t MQTTRestartCounter = 0;
		};

		class SensorPeriodicHandler {
			public:
				static void Pool();
		};


		class WirelessPriorityHandler {
			public:
				static void 	Pool();
		};
};

#endif
