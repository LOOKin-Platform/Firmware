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
	TickType_t Delay = ms/portTICK_PERIOD_MS;
	::vTaskDelay(Delay);
} // sleep


/**
 * Start a new task.
 * @param[in] task The function pointer to the function to be run in the task.
 * @param[in] taskName A string identifier for the task.
 * @param[in] param An optional parameter to be passed to the started task.
 * @param[in] stackSize An optional paremeter supplying the size of the stack in which to run the task.
 * @param[out] Handle to the created Task;
 */
TaskHandle_t FreeRTOS::StartTask(void task(void*), string taskName, void *param, int stackSize, uint8_t Priority) {
	TaskHandle_t TaskHandle = NULL;
	::xTaskCreate(task, taskName.data(), stackSize, param, tskIDLE_PRIORITY + Priority, &TaskHandle);
	return TaskHandle;
} // startTask

TaskHandle_t FreeRTOS::StartTaskPinnedToCore(void task(void*), string taskName, void *param, int stackSize, uint8_t Core) {
	TaskHandle_t TaskHandle = NULL;
	::xTaskCreatePinnedToCore(task, taskName.data(), stackSize, param, 5, &TaskHandle, Core);
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


/**
 * Get the current core number on which the task is running
 * @return 0 or 1 - current core number
 */
uint8_t FreeRTOS::GetCurrentCoreID() {
	return xPortGetCoreID();
}

/*
 * 	public:
		Semaphore(string = "<Unknown>");
		~Semaphore();
		void Give();
		void Take(string owner="<Unknown>");
		void Take(uint32_t timeoutMs, string owner="<Unknown>");
		void SetName(string name);
		uint32_t Wait(string owner);
	private:
		SemaphoreHandle_t m_semaphore;
		string m_name;
		string m_owner;
	};
 *
 */


FreeRTOS::Semaphore::Semaphore(string name) {
	m_usePthreads = false;   	// Are we using pThreads or FreeRTOS?

	if (m_usePthreads) {
		pthread_mutex_init(&m_pthread_mutex, nullptr);
	}
	else {
		m_semaphore = xSemaphoreCreateMutex();
	}

	m_name      = name;
	m_owner     = string("<N/A>");
	m_value     = 0;
} // FreeRTOS::Semaphore::Semaphore

FreeRTOS::Semaphore::~Semaphore() {
	if (m_usePthreads) {
		pthread_mutex_destroy(&m_pthread_mutex);
	} else {
		vSemaphoreDelete(m_semaphore);
	}
}


/**
 * @brief Give a semaphore.
 * The Semaphore is given.
 */
void FreeRTOS::Semaphore::Give() {
	ESP_LOGV(TAG, "Semaphore giving: %s", toString().c_str());

	if (m_usePthreads) {
		pthread_mutex_unlock(&m_pthread_mutex);
	} else {
		::xSemaphoreGive(m_semaphore);
	}

	m_owner = string("<N/A>");
} // FreeRTOS::Semaphore::Give

/**
 * @brief Give a semaphore.
 * The Semaphore is given with an associated value.
 * @param [in] value The value to associate with the semaphore.
 */
void FreeRTOS::Semaphore::Give(uint32_t value) {
	m_value = value;
	Give();
} // FreeRTOS::Semaphore::Give


/**
 * @brief Take a semaphore.
 * Take a semaphore and wait indefinitely.
 */
bool FreeRTOS::Semaphore::Take(string owner)
{
	ESP_LOGD(TAG, "Semaphore taking: %s for %s", toString().c_str(), owner.c_str());
	bool rc = false;

	if (m_usePthreads) {
		pthread_mutex_lock(&m_pthread_mutex);
	} else {
		rc = ::xSemaphoreTake(m_semaphore, portMAX_DELAY);
	}

	m_owner = owner;

	if (rc) {
		ESP_LOGD(TAG, "Semaphore taken:  %s", toString().c_str());
	} else {
		ESP_LOGE(TAG, "Semaphore NOT taken:  %s", toString().c_str());
	}
	return rc;
} // FreeRTOS::Semaphore::Take


/**
 * @brief Take a semaphore.
 * Take a semaphore but return if we haven't obtained it in the given period of milliseconds.
 * @param [in] timeoutMs Timeout in milliseconds.
 */
bool FreeRTOS::Semaphore::Take(uint32_t timeoutMs, string owner) {
	ESP_LOGV(TAG, "Semaphore taking: %s for %s", toString().c_str(), owner.c_str());
	bool rc = false;

	if (m_usePthreads) {
		assert(false);  // We apparently don't have a timed wait for pthreads.
	} else {
		rc = ::xSemaphoreTake(m_semaphore, timeoutMs/portTICK_PERIOD_MS);
	}

	m_owner = owner;
	if (rc) {
		ESP_LOGV(TAG, "Semaphore taken:  %s", toString().c_str());
	}
	else {
		ESP_LOGE(TAG, "Semaphore NOT taken:  %s", toString().c_str());
	}

	return rc;

} // FreeRTOS::Semaphore::Take


bool FreeRTOS::Semaphore::TakeFromISR(SemaphoreHandle_t Semaphore, bool IsHighPriorityTask) {
	BaseType_t HigherPriorityTaskWoken = (IsHighPriorityTask) ? pdTRUE : pdFALSE;
	BaseType_t Result = xSemaphoreTakeFromISR(Semaphore, &HigherPriorityTaskWoken);

	return (Result == pdTRUE) ? true : false;
}

string FreeRTOS::Semaphore::toString() {
	stringstream stringStream;
	stringStream << "name: "<< m_name << " (0x" << std::hex << std::setfill('0') << (uint32_t)m_semaphore << "), owner: " << m_owner;
	return stringStream.str();
}

void FreeRTOS::Semaphore::SetName(string name) {
	m_name = name;
}

/**
 * @brief Wait for a semaphore to be released by trying to take it and
 * then releasing it again.
 * @param [in] owner A debug tag.
 * @return The value associated with the semaphore.
 */
uint32_t FreeRTOS::Semaphore::Wait(string owner) {
	ESP_LOGV(TAG, ">> Wait: Semaphore waiting: %s for %s", toString().c_str(), owner.c_str());

	if (m_usePthreads) {
		pthread_mutex_lock(&m_pthread_mutex);
	} else {
		xSemaphoreTake(m_semaphore, portMAX_DELAY);
	}

	m_owner = owner;

	if (m_usePthreads) {
		pthread_mutex_unlock(&m_pthread_mutex);
	} else {
		xSemaphoreGive(m_semaphore);
	}

	ESP_LOGV(TAG, "<< Wait: Semaphore released: %s", toString().c_str());
	m_owner = string("<N/A>");
	return m_value;
} // Wait

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
	string			Name,
	TickType_t		period,
	UBaseType_t		reload,
	void 			*data,
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
	TimersMap.insert(make_pair(timerHandle, this));
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

uint8_t FreeRTOS::Queue::CountFromISR(QueueHandle_t QueueHandle) {
	return ::uxQueueMessagesWaitingFromISR(QueueHandle);
}

uint8_t FreeRTOS::Queue::SpaceAvaliable(QueueHandle_t QueueHandle) {
	return ::uxQueueSpacesAvailable(QueueHandle);
}

void FreeRTOS::Queue::Reset(QueueHandle_t QueueHandle) {
	xQueueReset(QueueHandle);
}

BaseType_t FreeRTOS::Queue::SendToBackFromISR(QueueHandle_t QueueHandle, void *Item, bool IsHighPriorityTask) {
	BaseType_t HigherPriorityTaskWoken = (IsHighPriorityTask) ? pdTRUE : pdFALSE;
	return ::xQueueSendToBackFromISR(QueueHandle, Item, &HigherPriorityTaskWoken);
}

bool FreeRTOS::Queue::IsQueueFullFromISR(QueueHandle_t QueueHandle) {
	return (::xQueueIsQueueFullFromISR(QueueHandle) == pdTRUE) ? true : false;
}

RingbufHandle_t	FreeRTOS::RingBuffer::Create(uint16_t Length, RingbufferType_t Type) {
	return ::xRingbufferCreate(Length, Type);
}

RingbufHandle_t FreeRTOS::RingBuffer::CreateNoSplit (uint16_t Length, size_t ItemSize) {
	return ::xRingbufferCreateNoSplit(ItemSize, Length);
}

BaseType_t FreeRTOS::RingBuffer::Send(RingbufHandle_t Handle, void *Item, size_t ItemSize, TickType_t xTicksToWait) {
	return ::xRingbufferSend(Handle, Item, ItemSize, xTicksToWait);
}

BaseType_t FreeRTOS::RingBuffer::SendFromISR(RingbufHandle_t Handle, void *Item, size_t ItemSize, bool IsHighPriorityTask) {
	BaseType_t HigherPriorityTaskWoken = (IsHighPriorityTask) ? pdTRUE : pdFALSE;
	return ::xRingbufferSendFromISR(Handle, Item, ItemSize, &HigherPriorityTaskWoken);
}

bool FreeRTOS::RingBuffer::Receive(RingbufHandle_t Handle, void *Item, size_t *ItemSize, TickType_t xTicksToWait) {
	Item = ::xRingbufferReceive(Handle, ItemSize, xTicksToWait);

	if (Item == NULL)
		return false;

	return true;
}

bool FreeRTOS::RingBuffer::ReceiveFromISR(RingbufHandle_t Handle, void *Item, size_t ItemSize, bool IsHighPriorityTask) {
	Item = xRingbufferReceiveFromISR(Handle, &ItemSize);
	BaseType_t HigherPriorityTaskWoken = (IsHighPriorityTask) ? pdTRUE : pdFALSE;

	::vRingbufferReturnItemFromISR(Handle, Item, &HigherPriorityTaskWoken);

	return (Item == NULL) ? false : true;
}

void FreeRTOS::RingBuffer::ReturnItem(RingbufHandle_t Handle, void* Item) {
	if (Item != nullptr)
		::vRingbufferReturnItem(Handle, Item);
}


