/*
*    FreeRTOSWrapper.cpp
*    C++ wrapper for FreeRTOS functions
*
*/

#include "FreeRTOSWrapper.h"

static char TAG[] = "FreeRTOS";

FreeRTOS::FreeRTOS() {}
FreeRTOS::~FreeRTOS() {}

/**
 * Sleep for the specified number of milliseconds.
 * @param[in] ms The period in milliseconds for which to sleep.
 */
void FreeRTOS::Sleep(uint32_t ms) {
	::vTaskDelay(ms/portTICK_PERIOD_MS);
} // sleep


/**
 * Start a new task.
 * @param[in] task The function pointer to the function to be run in the task.
 * @param[in] taskName A string identifier for the task.
 * @param[in] param An optional parameter to be passed to the started task.
 * @param[in] stackSize An optional paremeter supplying the size of the stack in which to run the task.
 * @param[out] Handle to the created Task;
 */
TaskHandle_t FreeRTOS::StartTask(void task(void*), std::string taskName, void *param, int stackSize) {
	TaskHandle_t TaskHandle = NULL;
	::xTaskCreate(task, taskName.data(), stackSize, param, 5, &TaskHandle);
	return TaskHandle;
} // startTask

TaskHandle_t FreeRTOS::StartTaskPinnedToCore(void task(void*), std::string taskName, void *param, int stackSize) {
	TaskHandle_t TaskHandle = NULL;
	::xTaskCreatePinnedToCore(task, taskName.data(), stackSize, param, 5, &TaskHandle, 0);
	return TaskHandle;
} // startTask

/**
 * Delete the task.
 * @param[in] pTask An optional handle to the task to be deleted.  If not supplied the calling task will be deleted.
 */
void FreeRTOS::DeleteTask(TaskHandle_t pTask) {
	::vTaskDelete(pTask);
} // deleteTask


/**
 * Get the time in milliseconds since the %FreeRTOS_t scheduler started.
 * @return The time in milliseconds since the %FreeRTOS_t scheduler started.
 */
uint32_t FreeRTOS::GetTimeSinceStart() {
	return (uint32_t)(xTaskGetTickCount()*portTICK_PERIOD_MS);
} // getTimeSinceStart

/*
 * 	public:
		Semaphore(std::string = "<Unknown>");
		~Semaphore();
		void give();
		void take(std::string owner="<Unknown>");
		void take(uint32_t timeoutMs, std::string owner="<Unknown>");
	private:
		SemaphoreHandle_t m_semaphore;
		std::string m_name;
		std::string m_owner;
	};
 *
 */


FreeRTOS::Semaphore::Semaphore(std::string name) {
	m_semaphore = xSemaphoreCreateMutex();
	m_name      = name;
	m_owner     = "<N/A>";
}

FreeRTOS::Semaphore::~Semaphore() {
	vSemaphoreDelete(m_semaphore);
}


/**
 * @brief Give a semaphore.
 * The Semaphore is given.
 */
void FreeRTOS::Semaphore::give() {
	xSemaphoreGive(m_semaphore);
	ESP_LOGD(TAG, "Semaphore giving: %s", toString().c_str());
	m_owner = "<N/A>";
} // Semaphore::give


/**
 * @brief Take a semaphore.
 * Take a semaphore and wait indefinitely.
 */
void FreeRTOS::Semaphore::take(std::string owner)
{

	ESP_LOGD(TAG, "Semaphore taking: %s for %s", toString().c_str(), owner.c_str());
	xSemaphoreTake(m_semaphore, portMAX_DELAY);
	m_owner = owner;
	ESP_LOGD(TAG, "Semaphore taken:  %s", toString().c_str());
} // Semaphore::take


/**
 * @brief Take a semaphore.
 * Take a semaphore but return if we haven't obtained it in the given period of milliseconds.
 * @param [in] timeoutMs Timeout in milliseconds.
 */
void FreeRTOS::Semaphore::take(uint32_t timeoutMs, std::string owner) {
	m_owner = owner;
	xSemaphoreTake(m_semaphore, timeoutMs/portTICK_PERIOD_MS);
} // Semaphore::take

std::string FreeRTOS::Semaphore::toString() {
	std::stringstream stringStream;
	stringStream << "name: "<< m_name << " (0x" << std::hex << std::setfill('0') << (uint32_t)m_semaphore << "), owner: " << m_owner;
	return stringStream.str();
}

void FreeRTOS::Semaphore::setName(std::string name) {
	m_name = name;
}

map<void *, FreeRTOS::Timer *> FreeRTOS::Timer::TimersMap;

void FreeRTOS::Timer::internalCallback(TimerHandle_t xTimer) {
	FreeRTOS::Timer *timer = TimersMap.at(xTimer);
	timer->callback(timer);
}

/**
 * @brief Construct a timer.
 *
 * We construct a timer that will fire after the given period has elapsed.  The period is measured
 * in ticks.  When the timer fires, the callback function is invoked within the scope/context of
 * the timer demon thread.  As such it must **not** block.  Once the timer has fired, if the reload
 * flag is true, then the timer will be automatically restarted.
 *
 * Note that the timer does *not* start immediately.  It starts ticking after the start() method
 * has been called.
 *
 * The signature of the callback function is:
 *
 * @code{.cpp}
 * void callback(FreeRTOSTimer *pTimer) {
 *    // Callback code here ...
 * }
 * @endcode
 *
 * @param [in] name The name of the timer.
 * @param [in] period The period of the timer in ticks.
 * @param [in] reload True if the timer is to restart once fired.
 * @param [in] data Data to be passed to the callback.
 * @param [in] callback Callback function to be fired when the timer expires.
 */
FreeRTOS::Timer::Timer(
	string				Name,
	TickType_t		period,
	UBaseType_t		reload,
	void 					*data,
	void         	(*callback)(FreeRTOS::Timer *pTimer)) {
	/*
	 * The callback function to actually be called is saved as member data in the object and
	 * a static callback function is called.  This will be passed the FreeRTOS timer handle
	 * which is used as a key in a map to lookup the saved user supplied callback which is then
	 * actually called.
	 */

	assert(callback != nullptr);
	this->period = period;
	this->callback = callback;
	timerHandle = ::xTimerCreate(Name.c_str(), period, reload, data, internalCallback);

	// Add the association between the timer handle and this class instance into the map.
	TimersMap.insert(std::make_pair(timerHandle, this));
} // FreeRTOSTimer

/**
 * @brief Destroy a class instance.
 *
 * The timer is deleted.
 */
FreeRTOS::Timer::~Timer() {
	::xTimerDelete(timerHandle, portMAX_DELAY);
	TimersMap.erase(timerHandle);
}

/**
 * @brief Start the timer ticking.
 */
void FreeRTOS::Timer::Start(TickType_t blockTime) {
	::xTimerStart(timerHandle, blockTime);
} // start

/**
 * @brief Stop the timer from ticking.
 */
void FreeRTOS::Timer::Stop(TickType_t blockTime) {
	::xTimerStop(timerHandle, blockTime);
} // stop

/**
 * @brief Reset the timer to the period and start it ticking.
 */
void FreeRTOS::Timer::Reset(TickType_t blockTime) {
	::xTimerReset(timerHandle, blockTime);
} // reset


/**
 * @brief Return the period of the timer.
 *
 * @return The period of the timer.
 */
TickType_t FreeRTOS::Timer::GetPeriod() {
	return period;
} // getPeriod


/**
 * @brief Change the period of the timer.
 *
 * @param [in] newPeriod The new period of the timer in ticks.
 */
void FreeRTOS::Timer::ChangePeriod(TickType_t newPeriod, TickType_t blockTime) {
	if (::xTimerChangePeriod(timerHandle, newPeriod, blockTime) == pdPASS) {
		period = newPeriod;
	}
} // changePeriod


/**
 * @brief Get the name of the timer.
 *
 * @return The name of the timer.
 */
const char *FreeRTOS::Timer::GetName() {
	return ::pcTimerGetTimerName(timerHandle);
} // getName


/**
 * @brief Get the user supplied data associated with the timer.
 *
 * @return The user supplied data associated with the timer.
 */
void *FreeRTOS::Timer::GetData() {
	return ::pvTimerGetTimerID(timerHandle);
} // getData




QueueHandle_t FreeRTOS::Queue::Create(uint16_t Items, uint16_t ItemSize ) {
	return ::xQueueCreate(Items, ItemSize);
}

BaseType_t FreeRTOS::Queue::SendToBack(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait) {
	return ::xQueueSendToBack(QueueHandle, Item, xTicksToWait);
}

BaseType_t FreeRTOS::Queue::SendToFront(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait) {
	return ::xQueueSendToFront(QueueHandle, Item, xTicksToWait);
}

BaseType_t FreeRTOS::Queue::Receive(QueueHandle_t QueueHandle, void *Item, TickType_t xTicksToWait) {
	return ::xQueueReceive(QueueHandle, Item, xTicksToWait);
}

uint8_t FreeRTOS::Queue::Count(QueueHandle_t QueueHandle) {
	return ::uxQueueMessagesWaiting(QueueHandle);
}
