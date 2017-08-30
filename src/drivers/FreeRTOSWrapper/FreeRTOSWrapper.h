/*
*    FreeRTOS.h
*    C++ wrapper for FreeRTOS functions
*
*/

#ifndef DRIVERS_FREERTOS_H_
#define DRIVERS_FREERTOS_H_

#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_log.h>

using namespace std;

/**
 * @brief Interface to %FreeRTOS functions.
 */
class FreeRTOS {
public:
	FreeRTOS();
	virtual ~FreeRTOS();

	static void 				Sleep(uint32_t ms);
	static TaskHandle_t StartTask(void task(void *), std::string taskName, void *param=nullptr, int stackSize = 2048);
	static void 				DeleteTask(TaskHandle_t pTask = nullptr);

	static uint32_t 		GetTimeSinceStart();

	class Semaphore {
	public:
		Semaphore(string owner = "<Unknown>");
		~Semaphore();
		void give();
		void setName(string name);
		void take(string owner="<Unknown>");
		void take(uint32_t timeoutMs, string owner="<Unknown>");
		std::string toString();
	private:
		SemaphoreHandle_t m_semaphore;
		std::string m_name;
		std::string m_owner;
	};

	class Queue {
		public:
			static QueueHandle_t 	Create			(uint16_t Items, uint16_t ItemSize);
			static BaseType_t			SendToBack	(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
			static BaseType_t			SendToFront	(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
			static BaseType_t			Receive			(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait = 50);
			static uint8_t				Count				(QueueHandle_t QueueHandle);
	};
};

#endif /* DRIVERS_FREERTOS_H_ */
