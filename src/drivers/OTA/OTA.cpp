#include "OTA.h"

#include "include/Globals.h"

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024
static const char *TAG = "ota";

char ota_write_data[BUFFSIZE + 1] = { 0 };  // OTA data write buffer ready to write to the flash
char text[BUFFSIZE + 1] = { 0 };            // packet receive buffer
int binary_file_length = 0;                 // an image total length
int socket_id = -1;                         // socket id
char http_request[64] = {0};

/* operate handle : uninitialized value is zero ,every ota begin would exponential growth*/
esp_ota_handle_t out_handle = 0;
esp_partition_t operate_partition;

OTA::OTA()
{
    bool isInitSucceed = false;

    esp_err_t err;
    const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();

    if (esp_current_partition->type != ESP_PARTITION_TYPE_APP)
        ESP_LOGE(TAG, "Error： esp_current_partition->type != ESP_PARTITION_TYPE_APP");

    esp_partition_t find_partition;
    memset(&operate_partition, 0, sizeof(esp_partition_t));
    /*choose which OTA image should we write to*/
    switch (esp_current_partition->subtype)
    {
      case ESP_PARTITION_SUBTYPE_APP_FACTORY:
          find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
          break;
      case  ESP_PARTITION_SUBTYPE_APP_OTA_0:
          find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
          break;
      case ESP_PARTITION_SUBTYPE_APP_OTA_1:
          find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
          break;
      default:
        break;
    }

    find_partition.type = ESP_PARTITION_TYPE_APP;

    const esp_partition_t *partition = esp_partition_find_first(find_partition.type, find_partition.subtype, NULL);
    assert(partition != NULL);
    memset(&operate_partition, 0, sizeof(esp_partition_t));

    err = esp_ota_begin( partition, OTA_SIZE_UNKNOWN, &out_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "esp_ota_begin failed err=0x%x!", err);
    }
    else
    {
        memcpy(&operate_partition, partition, sizeof(esp_partition_t));
        ESP_LOGI(TAG, "esp_ota_begin init OK");
        isInitSucceed = true;
    }

    if ( isInitSucceed )
    {
      ESP_LOGI(TAG, "OTA Init succeeded");
    }
    else
    {
        ESP_LOGE(TAG, "OTA Init failed");
        task_fatal_error();
    }
}


/*read buffer by byte still delim ,return read bytes counts*/
int OTA::ReadUntil(char *buffer, char delim, int len)
{
    /* TODO: delim check,buffer check,further: do an buffer length limited */
    int i = 0;
    while (buffer[i] != delim && i < len) {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
bool OTA::ReadPastHttpHeader(char text[], int total_len, esp_ota_handle_t out_handle)
{
    /* i means current position */
    int i = 0, i_read_len = 0;

    while (text[i] != 0 && i < total_len)
    {
        i_read_len = ReadUntil(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished

        if (i_read_len == 2)
        {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_ota_write( out_handle, (const void *)ota_write_data, i_write_len);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                return false;
            }
            else
            {
                ESP_LOGI(TAG, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }
            return true;
        }
        i += i_read_len;
    }

    return false;
}

bool OTA::HttpConnect()
{
    ESP_LOGI(TAG, "Server IP: %s Server Port:%s", OTA_SERVER_IP, OTA_SERVER_PORT);
    sprintf(http_request, "GET %s HTTP/1.1\r\nHost: %s:%s \r\n\r\n", OTA_FILENAME, OTA_SERVER_IP, OTA_SERVER_PORT);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1)
    {
        ESP_LOGE(TAG, "Create socket failed!");
        return false;
    }

    // set connect info
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(OTA_SERVER_IP);
    sock_info.sin_port = htons(atoi(OTA_SERVER_PORT));

    // connect to http server
    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1)
    {
        ESP_LOGE(TAG, "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return false;
    }
    else
    {
        ESP_LOGI(TAG, "Connected to server");
        return true;
    }
    return false;
}

void __attribute__((noreturn)) OTA::task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    close(socket_id);
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

void OTA::Update()
{
    esp_err_t err;
    ESP_LOGI(TAG, "Starting OTA...");

    ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

    /*connect to http server*/
    if (HttpConnect())
    {
        ESP_LOGI(TAG, "Connected to http server");
    }
    else
    {
        ESP_LOGE(TAG, "Connect to http server failed!");
        task_fatal_error();
    }

    int res = -1;
    /*send GET request to http server*/
    res = send(socket_id, http_request, strlen(http_request), 0);
    if (res == -1) {
        ESP_LOGE(TAG, "Send GET request to server failed");
        task_fatal_error();
    } else {
        ESP_LOGI(TAG, "Send GET request to server succeeded");
    }

    bool resp_body_start = false, flag = true;
    /*deal with all receive packet*/
    while (flag)
    {
        memset(text, 0, TEXT_BUFFSIZE);
        memset(ota_write_data, 0, BUFFSIZE);

        int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);

        if (buff_len < 0)
        {
            /* receive error */
            ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
            task_fatal_error();
        }
        else if (buff_len > 0 && !resp_body_start)
        {
            /* deal with response header*/
            memcpy(ota_write_data, text, buff_len);
            resp_body_start = ReadPastHttpHeader(text, buff_len, out_handle);
        }
        else if (buff_len > 0 && resp_body_start)
        {
            /* deal with response body */
            memcpy(ota_write_data, text, buff_len);
            err = esp_ota_write( out_handle, (const void *)ota_write_data, buff_len);

            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                task_fatal_error();
            }

            binary_file_length += buff_len;
            ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
        }
        else if (buff_len == 0)
        {
            /* packet over */
            flag = false;
            ESP_LOGI(TAG, "Connection closed, all packets received");
            close(socket_id);
        }
        else
        {
            ESP_LOGE(TAG, "Unexpected recv result");
        }
    }

    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(out_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(&operate_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
    }

    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}
