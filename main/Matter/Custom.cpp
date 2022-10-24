#include "Custom.h"
#include "Matter.h"

extern Device_t	Device;

void HomeKitUpdateCharValue(string StringAID, const char *ServiceUUID, const char *CharUUID, uint8_t Value) {
	/*
    
    HomeKitUpdateCharValue((uint32_t)Converter::UintFromHexString<uint16_t>(StringAID), ServiceUUID, CharUUID, Value);
    
    */
}

void HomeKitUpdateCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID, uint8_t Value)
{
    /*
	hap_acc_t* Accessory = HomeKit::IsExperimentalMode() ? hap_acc_get_by_aid(AID) : hap_get_first_acc();

	if (Accessory == NULL) 	return;

	hap_serv_t *Service  	= hap_acc_get_serv_by_uuid(Accessory, ServiceUUID);
	if (Service == NULL) 	return;

	hap_char_t *Char 		= hap_serv_get_char_by_uuid(Service, CharUUID);
	if (Char == NULL) 		return;

	hap_char_update_val(Char, &Value);
    */
}

const uint8_t* HomeKitGetCharValue(string StringAID, const char *ServiceUUID, const char *CharUUID) {
	return {0};
    /*
    	return HomeKitGetCharValue((uint32_t)Converter::UintFromHexString<uint16_t>(StringAID), ServiceUUID, CharUUID);
    */
}

const uint8_t* HomeKitGetCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID) {
	return {0};
    /*
	hap_acc_t* Accessory = HomeKit::IsExperimentalMode() ? hap_acc_get_by_aid(AID) : hap_get_first_acc();

	if (Accessory == NULL) 	return NULL;

	hap_serv_t *Service  	= hap_acc_get_serv_by_uuid(Accessory, ServiceUUID);
	if (Service == NULL) 	return NULL;

	hap_char_t *Char 		= hap_serv_get_char_by_uuid(Service, CharUUID);
	if (Char == NULL) 		return NULL;

	return hap_char_get_val(Char);
    */
}


