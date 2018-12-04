/*
 * Task.h
 *
 *  Created on: Mar 4, 2017
 *      Author: kolban
 */

#ifndef DRIVERS_FREERTOS_TASK_H_
#define DRIVERS_FREERTOS_TASK_H_
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
/**
 * @brief Encapsulate a runnable task.
 *
 * This class is designed to be subclassed with the method:
 *
 * @code{.cpp}
 * void run(void *data) { ... }
 * @endcode
 *
 * For example:
 *
 * @code{.cpp}
 * class CurlTestTask : public Task {
 *    void run(void *data) {
 *       // Do something
 *    }
 * };
 * @endcode
 *
 * implemented.
 */
class Task {
	public:
		Task(std::string taskName="Task", uint16_t stackSize=10000, uint8_t priority=5);
		virtual ~Task();
		void SetStackSize(uint16_t stackSize);
		void SetPriority(uint8_t priority);
		void SetName(std::string name);
		void SetCore(BaseType_t coreId);
		void Start(void* taskData=nullptr);
		void Stop();
		/**
		 * @brief Body of the task to execute.
		 *
		 * This function must be implemented in the subclass that represents the actual task to run.
		 * When a task is started by calling start(), this is the code that is executed in the
		 * newly created task.
		 *
		 * @param [in] data The data passed in to the newly started task.
		 */
		virtual void Run(void *data) = 0; // Make run pure virtual
		void Delay(int ms);

	private:
		xTaskHandle m_handle;
		void*       m_taskData;
		static void RunTask(void *data);
		std::string m_taskName;
		uint16_t    m_stackSize;
		uint8_t     m_priority;
		BaseType_t  m_coreId;
};

#endif /* DRIVERS_FREERTOS_TASK_H_ */
