#ifndef MATTER_CUSTOM_SDK
#define MATTER_CUSTOM_SDK



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

void HomeKitUpdateCharValue(string AIDString, const char *ServiceUUID, const char *CharUUID, hap_val_t Value);
void HomeKitUpdateCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID, hap_val_t Value);

const hap_val_t* HomeKitGetCharValue(string StringAID, const char *ServiceUUID, const char *CharUUID);
const hap_val_t* HomeKitGetCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID);

#endif
