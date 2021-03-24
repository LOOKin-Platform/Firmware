#ifndef HOMEKIT_CUSTOM_SDK
#define HOMEKIT_CUSTOM_SDK

#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include "Converter.h"
#include "Device.h"

#include <string>

using namespace std;

#define CHAR_ACTIVE_IDENTIFIER_UUID 		"E7"
#define CHAR_IDENTIFIER_UUID 				"E6"
#define CHAR_CONFIGUREDNAME_UUID 			"E3"
#define CHAR_REMOTEKEY_UUID					"E1"
#define CHAR_SLEEP_DISCOVERY_UUID			"E8"
#define CHAR_MUTE_UUID						"11A"
#define CHAR_VOLUME_CONTROL_TYPE_UUID		"E9"
#define CHAR_VOLUME_UUID 					"119"
#define CHAR_CONFIGURED_NAME_UUID			"E3"
#define CHAR_INPUT_SOURCE_TYPE_UUID			"DB"
#define CHAR_CURRENT_VISIBILITY_STATE_UUID	"135"
#define CHAR_TARGET_VISIBILITY_STATE_UUID	"134"
#define CHAR_POWER_MODE_SELECTION_UUID		"DF"
#define CHAR_VOLUME_SELECTOR_UUID 			"EA"

#define SERVICE_TV_UUID 					"D8"
#define SERVICE_TELEVISION_SPEAKER_UUID		"113"
#define SERVICE_INPUT_SOURCE_UUID			"D9"

hap_char_t *hap_char_active_identifier_create(uint32_t ActiveID = 0);
hap_char_t *hap_char_identifier_create(uint32_t ActiveID = 0);
hap_char_t *hap_char_configured_name_create(char *name);
hap_char_t *hap_char_remotekey_create(uint8_t Key);
hap_char_t *hap_char_sleep_discovery_mode_create(uint8_t Key);
hap_char_t *hap_char_mute_create(bool IsMuted);
hap_char_t *hap_char_volume_control_type_create(uint8_t VolumeControlType);
hap_char_t *hap_char_volume_selector_create(uint8_t VolumeSelector);
hap_char_t *hap_char_volume_create(uint8_t Volume);
hap_char_t *hap_char_input_source_type_create(uint8_t Type);
hap_char_t *hap_char_current_visibility_state_create(uint8_t CurrentVisibilityState);
hap_char_t *hap_char_target_visibility_state_create(uint8_t CurrentVisibilityState);
hap_char_t *hap_char_power_mode_selection_create(uint8_t PowerModeSelectionValue);
hap_char_t *hap_char_rotation_speed_create_ac(float rotation_speed);
hap_char_t *hap_char_current_temperature_create_ac(float curr_temp);
hap_char_t *hap_char_custom_cooling_threshold_temperature_create(float cooling_threshold_temp);
hap_char_t *hap_char_custom_target_heater_cooler_state_create();
hap_char_t *hap_char_custom_heating_threshold_temperature_create(float heating_threshold_temp);
hap_char_t *hap_char_ac_fan_rotation_create(float rotation_speed);

hap_serv_t *hap_serv_tv_create(uint8_t active, uint8_t ActiveIdentifier = 1, char *ConfiguredName = NULL);
hap_serv_t *hap_serv_tv_speaker_create(bool IsMuted);
hap_serv_t *hap_serv_input_source_create(char *Name, uint8_t ID);
hap_serv_t *hap_serv_ac_create(uint8_t curr_heater_cooler_state, float curr_temp, float targ_temp, uint8_t temp_disp_units);
hap_serv_t *hap_serv_ac_fan_create(bool IsActive, uint8_t TargetFanState, uint8_t SwingMode, uint8_t FanSpeed);

void HomeKitUpdateCharValue(string AIDString, const char *ServiceUUID, const char *CharUUID, hap_val_t Value);
void HomeKitUpdateCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID, hap_val_t Value);

const hap_val_t* HomeKitGetCharValue(string StringAID, const char *ServiceUUID, const char *CharUUID);
const hap_val_t* HomeKitGetCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID);

#endif
