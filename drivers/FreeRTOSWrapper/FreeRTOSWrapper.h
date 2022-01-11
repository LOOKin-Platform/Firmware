/*
*    FreeRTOS.h
*    C++ wrapper for FreeRTOS functions
*
*/

#ifndef DRIVERS_FREERTOS_H_
#define DRIVERS_FREERTOS_H_

using namespace std;

#include <ctime>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/ringbuf.h>

#include <esp_log.h>

/**
 * @brief Interface to %FreeRTOS functions.
 */
class FreeRTOS {
	public:
		FreeRTOS();
		virtual ~FreeRTOS();

		static void 				Sleep(uint32_t ms);
		static TaskHandle_t 		StartTask(void task(void *), string taskName, void *param=nullptr, int stackSize = 2048, uint8_t Priority = 5);
		static TaskHandle_t 		StartTaskPinnedToCore(void task(void *), string taskName, void *param=nullptr, int stackSize = 2048, uint8_t Core = 0);
		static void 				DeleteTask(TaskHandle_t pTask = NULL);

		static uint32_t 			GetTimeSinceStart();
		static uint8_t				GetCurrentCoreID();

		static void					CleanUpMemory();

		static void					SetTaskPriority(TaskHandle_t pTask, uint8_t Priority);

		class Semaphore {
			public:
				Semaphore(string owner = "<Unknown>");
				~Semaphore();
				void 				Give();
				void        		Give(uint32_t value);
				void 				SetName(string name);
				bool 				Take(string owner="<Unknown>");
				bool 				Take(uint32_t timeoutMs, string owner="<Unknown>");
				string 				toString();
				uint32_t			Wait(std::string owner="<Unknown>");

				static bool			TakeFromISR(SemaphoreHandle_t Semaphore, bool IsHighPriorityTask = true);
			private:
				SemaphoreHandle_t 	m_semaphore;
				pthread_mutex_t   	m_pthread_mutex;
				string				m_name;
				string				m_owner;
				uint32_t			m_value;
				bool				m_usePthreads;
		};

		/**
		 * @brief Wrapper around the %FreeRTOS timer functions.
		 */
		class Timer {
			public:
				Timer		(string Name, TickType_t period, UBaseType_t reload, void *data, void (*callback)(FreeRTOS::Timer *pTimer));
				virtual 	~Timer();
				void 		ChangePeriod(TickType_t newPeriod, TickType_t blockTime=portMAX_DELAY);
				void 		*GetData();
				const char	*GetName();
				TickType_t 	GetPeriod();
				void 		Reset(TickType_t blockTime=portMAX_DELAY);
				void		Start(TickType_t blockTime=portMAX_DELAY);
				void		Stop(TickType_t blockTime=portMAX_DELAY);

			private:
				TimerHandle_t	timerHandle;
				TickType_t		period;
				void			(*callback)(Timer *pTimer);
				static void		internalCallback(TimerHandle_t xTimer);

				static map<void *, FreeRTOS::Timer *> TimersMap;
		};

		class Queue {
			public:
				static QueueHandle_t 		Create				(uint16_t Items, uint16_t ItemSize);
				static BaseType_t			Send				(QueueHandle_t QueueHandle, void *Item, bool IsFront = false, TickType_t xTicksToWait = 50);
				static BaseType_t			SendToBack			(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
				static BaseType_t			SendToFront			(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
				static BaseType_t			Receive				(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
				static BaseType_t			Peek				(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
				static uint8_t				Count				(QueueHandle_t QueueHandle);
				static uint8_t				SpaceAvaliable		(QueueHandle_t QueueHandle);
				static void					Reset				(QueueHandle_t QueueHandle);

				static BaseType_t			SendToBackFromISR	(QueueHandle_t QueueHandle, void *Item, bool IsHighPriorityTask = true);
				static bool					IsQueueFullFromISR	(QueueHandle_t QueueHandle);
				static uint8_t 				CountFromISR		(QueueHandle_t QueueHandle);

		};

		// experimental. need to test ringbuffer implementation
		class RingBuffer {
			public:
				static RingbufHandle_t		Create				(uint16_t Length, RingbufferType_t type);
				static RingbufHandle_t		CreateNoSplit		(uint16_t Length, size_t ItemSize);
				static BaseType_t			Send				(RingbufHandle_t Handle, void *Item, size_t ItemSize, TickType_t xTicksToWait = 50);
				static BaseType_t			SendFromISR			(RingbufHandle_t Handle, void *Item, size_t ItemSize, bool IsHighPriorityTask = true);
				static bool					Receive				(RingbufHandle_t Handle, void *Item, size_t *ItemSize, TickType_t xTicksToWait = 50);
				static bool					ReceiveFromISR		(RingbufHandle_t Handle, void *Item, size_t ItemSize, bool IsHighPriorityTask = true);
				static void					ReturnItem			(RingbufHandle_t Handle, void *Item);
		};

};

#endif /* DRIVERS_FREERTOS_H_ */
