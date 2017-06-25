//  More information is available on our blog:
//  https://blog.classycode.com/secure-over-the-air-updates-for-esp32-ec25ae00db43

#include "OTA.h"


#include <string.h>
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"

#include "iap_https.h"  // Coordinating firmware updates

#define TAG "main"

static const char *server_root_ca_public_key_pem  = OTA_SERVER_ROOT_CA_PEM;
static const char *peer_public_key_pem            = OTA_PEER_PEM;

// Static because the scope of this object is the usage of the iap_https module.
static iap_https_config_t ota_config;

    /*
    while (1) {

        // If the application could only re-boot at certain points, you could
        // manually query iap_https_new_firmware_installed and manually trigger
        // the re-boot. What we do in this example is to let the firmware updater
        // re-boot automatically after installing the update (see init_ota below).
        //
        // if (iap_https_new_firmware_installed()) {
        //     ESP_LOGI(TAG, "New firmware has been installed - rebooting...");
        //     esp_restart();
        // }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }*/

    // Should never arrive here.

void UpdateFirmware()
{
  iap_https_check_now();
}

void init_ota()
{
    ESP_LOGI(TAG, "Initialising OTA firmware updating.");

    ota_config.current_software_version = SOFTWARE_VERSION;
    ota_config.server_host_name = OTA_SERVER_HOST_NAME;
    ota_config.server_port = "443";
    strncpy(ota_config.server_metadata_path, OTA_SERVER_METADATA_PATH, sizeof(ota_config.server_metadata_path) / sizeof(char));
    bzero(ota_config.server_firmware_path, sizeof(ota_config.server_firmware_path) / sizeof(char));
    ota_config.server_root_ca_public_key_pem = server_root_ca_public_key_pem;
    ota_config.peer_public_key_pem = peer_public_key_pem;
    ota_config.auto_reboot = OTA_AUTO_REBOOT;

    iap_https_init(&ota_config);
}
