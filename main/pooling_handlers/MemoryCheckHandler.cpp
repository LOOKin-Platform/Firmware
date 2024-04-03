#include "HandlersPooling.h"

void HandlersPooling_t::MemoryCheckHandler::Pool() {
    ESP_LOGI("Pooling","RAM left %d bytes", xPortGetFreeHeapSize());//esp_get_free_heap_size());
}
