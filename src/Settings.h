#define FIRMWARE_VERSION            "0.83"

#define OTA_SERVER_HOST             "download.look-in.club"
#define OTA_SERVER_PORT             80
#define OTA_URL_PREFIX              "/firmwares/"

#define TIME_SERVER_HOST            "api.look-in.club"
#define TIME_API_URL                "/v1/time"

#define WIFI_AP_NAME                "LOOK.in_" + Device->TypeToString() + "_" + Device->IDToString()
#define WIFI_AP_PASSWORD            Device->IDToString()
#define WIFI_IP_COUNTDOWN           10000

#define UDP_SERVER_PORT             61201
#define UDP_PACKET_PREFIX           "LOOK.in:"

// Devices definitions
#define DEVICE_TYPE_PLUG_HEX        0x03
#define DEVICE_TYPE_PLUG_STRING     "Plug"

// Commands and Sensors Pin Map
#define SWITCH_PLUG_PIN_NUM         GPIO_NUM_2

// HTTPClient class Settings
#define HTTPCLIENT_QUEUE_SIZE       16
#define HTTPCLIENT_THREADS_MAX      4
#define HTTPCLIENT_BLOCK_TICKS      50
#define HTTPCLIENT_NETBUF_LEN       512
#define HTTPCLIENT_NEW_THREAD_STEP  2
#define HTTPCLIENT_TASK_STACKSIZE   4096

// Scenarios globals
#define SCENARIOS_QUEUE_SIZE        64
#define SCENARIOS_BLOCK_TICKS       50
#define SCENARIOS_TASK_STACKSIZE    8192
#define SCENARIOS_OPERAND_BIT_LEN   64
#define SCENARIOS_CACHE_BIT_LEN     32

#define SCENARIOS_TIME_INTERVAL     5

#define SCENARIOS_TYPE_EVENT_HEX    0x00
#define SCENARIOS_TYPE_TIMER_HEX    0x01
#define SCENARIOS_TYPE_CALENDAR_HEX 0x02
