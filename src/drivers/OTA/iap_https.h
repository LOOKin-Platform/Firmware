//  This module is responsible to trigger and coordinate firmware updates.

#ifndef __IAP_HTTPS__
#define __IAP_HTTPS__ 1

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct iap_https_config_ {

    // Version number of the running firmware image.
    int current_software_version;

    // Name of the host that provides the firmware images, e.g. "www.classycode.io".
    const char *server_host_name;

    // TCP port for TLS communication, e.g. "443".
    const char *server_port;

    // Public key of the server's root CA certificate.
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *server_root_ca_public_key_pem;

    // Public key of the server's peer certificate (for certificate pinning).
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *peer_public_key_pem;

    // Path to the metadata file which contains information on the firmware image,
    // e.g. /ota/meta.txt. We perform an HTTP/1.1 GET request on this file.
    char server_metadata_path[256];

    // Path to the firmware image file.
    char server_firmware_path[256];

    // Automatic re-boot after upgrade.
    // If the application can't handle arbitrary re-boots, set this to 'false'
    // and manually trigger the reboot.
    int auto_reboot;

} iap_https_config_t;


// Module initialisation, call once at application startup.
int iap_https_init(iap_https_config_t *config);

// Manually trigger a firmware update check.
// Queries the server for a firmware update and, if one is available, installs it.
// If automatic checks are enabled, calling this function causes the timer to be re-set.
int iap_https_check_now();

// Returns 1 if an update is currently in progress, 0 otherwise.
int iap_https_update_in_progress();

// Returns 1 if a new firmware has been installed but not yet booted, 0 otherwise.
int iap_https_new_firmware_installed();


#ifdef __cplusplus
}
#endif

#endif // __IAP_HTTPS__
