#include "Custom.h"

/* Char: Active identifier */
hap_char_t *hap_char_active_identifier_create(uint32_t ActiveID)
{
    hap_char_t *hc = hap_char_uint32_create(CHAR_ACTIVE_IDENTIFIER_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, ActiveID);
    if (!hc) {
        return NULL;
    }

    return hc;
}

/* Char: Identifier */
hap_char_t *hap_char_identifier_create(uint32_t ActiveID)
{
    hap_char_t *hc = hap_char_uint32_create(CHAR_IDENTIFIER_UUID, HAP_CHAR_PERM_PR, ActiveID);
    if (!hc) {
        return NULL;
    }

    return hc;
}

/* Char: Configured Name */
hap_char_t *hap_char_configured_name_create(char *name)
{
    hap_char_t *hc = hap_char_string_create(CHAR_CONFIGUREDNAME_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, name);
    if (!hc) {
        return NULL;
    }
    return hc;
}

/* Char: Remote key */
hap_char_t *hap_char_remotekey_create(uint8_t Key)
{
    hap_char_t *hc = hap_char_uint8_create(CHAR_REMOTEKEY_UUID, HAP_CHAR_PERM_PW, Key);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 16, 1);

    return hc;
}

/* Char: Sleep discovery mode */
hap_char_t *hap_char_sleep_discovery_mode_create(uint8_t Key)
{
	/*
		static readonly NOT_DISCOVERABLE = 0;
		static readonly ALWAYS_DISCOVERABLE = 1;
	 */
    hap_char_t *hc = hap_char_uint8_create(CHAR_SLEEP_DISCOVERY_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, Key);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 1, 1);

    return hc;
}

/* Char: Mute  */
hap_char_t *hap_char_mute_create(bool IsMuted)
{
    hap_char_t *hc = hap_char_bool_create(CHAR_MUTE_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, IsMuted);
    if (!hc) {
        return NULL;
    }

    return hc;
}


/* Char: Volume control type  */
hap_char_t *hap_char_volume_control_type_create(uint8_t VolumeControlType)
{
	/*
	static readonly NONE = 0;
	static readonly RELATIVE = 1;
	static readonly RELATIVE_WITH_CURRENT = 2;
	static readonly ABSOLUTE = 3;
	*/

    hap_char_t *hc = hap_char_uint8_create(CHAR_VOLUME_CONTROL_TYPE_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, VolumeControlType);
    if (!hc) {
        return NULL;
    }

    return hc;
}

/* Char: volume selector  */
hap_char_t *hap_char_volume_selector_create(uint8_t VolumeSelector)
{
	/*
  	  static readonly INCREMENT = 0;
	  static readonly DECREMENT = 1;
	 */

    hap_char_t *hc = hap_char_uint8_create(CHAR_VOLUME_SELECTOR_UUID, HAP_CHAR_PERM_PW, VolumeSelector);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 1, 1);

    return hc;
}

/* Char: Volume  */
hap_char_t *hap_char_volume_create(uint8_t Volume)
{
    hap_char_t *hc = hap_char_uint8_create(CHAR_VOLUME_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, Volume);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 100, 1);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_PERCENTAGE);

    return hc;
}

/* Char: Input Source type */
hap_char_t *hap_char_input_source_type_create(uint8_t Type)
{
	/*
	static readonly OTHER = 0;
	static readonly HOME_SCREEN = 1;
	static readonly TUNER = 2;
	static readonly HDMI = 3;
	static readonly COMPOSITE_VIDEO = 4;
	static readonly S_VIDEO = 5;
	static readonly COMPONENT_VIDEO = 6;
	static readonly DVI = 7;
	static readonly AIRPLAY = 8;
	static readonly USB = 9;
	static readonly APPLICATION = 10;
	 */

    hap_char_t *hc = hap_char_uint8_create(CHAR_INPUT_SOURCE_TYPE_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, Type);
    if (!hc) {
        return NULL;
    }

    return hc;
}

/* Char: Current Visibility state */
hap_char_t *hap_char_current_visibility_state_create(uint8_t CurrentVisibilityState)
{
	/*
	  static readonly SHOWN = 0;
	  static readonly HIDDEN = 1;
	 */

    hap_char_t *hc = hap_char_uint8_create(CHAR_CURRENT_VISIBILITY_STATE_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, CurrentVisibilityState);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 1, 1);

    return hc;
}

/* Char: Current Visibility state */
hap_char_t *hap_char_target_visibility_state_create(uint8_t CurrentVisibilityState)
{
	/*
	  static readonly SHOWN = 0;
	  static readonly HIDDEN = 1;
	 */

    hap_char_t *hc = hap_char_uint8_create(CHAR_TARGET_VISIBILITY_STATE_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, CurrentVisibilityState);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 1, 1);

    return hc;
}

/* Char: Power mode selection */
hap_char_t *hap_char_power_mode_selection_create(uint8_t PowerModeSelectionValue)
{
	/*
	  static readonly SHOWN = 0;
	  static readonly HIDDEN = 1;
	 */

    hap_char_t *hc = hap_char_uint8_create(CHAR_POWER_MODE_SELECTION_UUID, HAP_CHAR_PERM_PW, PowerModeSelectionValue);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 1, 1);

    return hc;
}

/* Char: Rotation Speed for 4 positions */
hap_char_t *hap_char_rotation_speed_create_ac(float rotation_speed)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_ROTATION_SPEED, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, rotation_speed);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 0.0, 100.0, 25);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_PERCENTAGE);

    return hc;
}

/* Char: Current Temperature for ac */
hap_char_t *hap_char_current_temperature_create_ac(float curr_temp)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_CURRENT_TEMPERATURE, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, curr_temp);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 16, 30, 1);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_CELSIUS);

    return hc;
}

/* Char: Cooling temperature threshold for ac */
hap_char_t *hap_char_custom_cooling_threshold_temperature_create(float cooling_threshold_temp)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_COOLING_THRESHOLD_TEMPERATURE,
                                           HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, cooling_threshold_temp);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 16.0, 30.0, 1);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_CELSIUS);

    return hc;
}

/* Char: Target Heater Cooler State */
hap_char_t *hap_char_custom_target_heater_cooler_state_create(uint8_t targ_heater_cooler_state)
{
    hap_char_t *hc = hap_char_uint8_create(HAP_CHAR_UUID_TARGET_HEATER_COOLER_STATE,
                                           HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, targ_heater_cooler_state);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 2, 1);

    uint8_t ValidVals[] = { 2 };
    hap_char_add_valid_vals(hc, ValidVals, 1);

    return hc;
}


/* Char: Heating temperature threshold for ac */
hap_char_t *hap_char_custom_heating_threshold_temperature_create(float heating_threshold_temp)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_HEATING_THRESHOLD_TEMPERATURE,
                                           HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, heating_threshold_temp);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 16.0, 30.0, 1);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_CELSIUS);

    return hc;
}


/* Char: Vent Speed for AC */
hap_char_t *hap_char_ac_fan_rotation_create(float rotation_speed)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_ROTATION_SPEED, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, rotation_speed);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 0.0, 3.0, 1.0);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_PERCENTAGE);

    return hc;
}

/* Service: TV */
hap_serv_t *hap_serv_tv_create(uint8_t active, uint8_t ActiveIdentifier, char *ConfiguredName)
{
    hap_serv_t *hs = hap_serv_create(SERVICE_TV_UUID);

    if (!hs)
        return NULL;

    if (hap_serv_add_char(hs, hap_char_active_create(active)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_active_identifier_create(ActiveIdentifier)) != HAP_SUCCESS)	{
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_configured_name_create(ConfiguredName)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_remotekey_create(0)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_sleep_discovery_mode_create(active)) != HAP_SUCCESS) {
    	goto err;
    }

    /*
    if (hap_serv_add_char(hs, hap_char_power_mode_selection_create(0)) != HAP_SUCCESS) {
    	goto err;
    }
    */

    return hs;

err:
    hap_serv_delete(hs);
    return NULL;
}

/* Service: Speaker */
hap_serv_t *hap_serv_tv_speaker_create(bool IsMuted)
{
    hap_serv_t *hs = hap_serv_create(SERVICE_TELEVISION_SPEAKER_UUID);

    if (!hs) return NULL;

    if (hap_serv_add_char(hs, hap_char_mute_create(IsMuted)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_active_create(1)) != HAP_SUCCESS) {
    	goto err;
    }

    /*
    if (hap_serv_add_char(hs, hap_char_volume_create(10)) != HAP_SUCCESS) {
    	goto err;
    }
    */

    if (hap_serv_add_char(hs, hap_char_volume_control_type_create(1)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_volume_selector_create(1)) != HAP_SUCCESS) {
    	goto err;
    }

    return hs;

err:
    hap_serv_delete(hs);
    return NULL;
}

/* Service: Input Source */
hap_serv_t *hap_serv_input_source_create(char *Name, uint8_t ID)
{
    hap_serv_t *hs = hap_serv_create(SERVICE_INPUT_SOURCE_UUID);

    if (!hs) return NULL;

    if (hap_serv_add_char(hs, hap_char_identifier_create(ID)) != HAP_SUCCESS)	{
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_name_create(Name)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_configured_name_create(Name)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_input_source_type_create(3)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_is_configured_create(1)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_current_visibility_state_create(0)) != HAP_SUCCESS) {
    	goto err;
    }

    /*
    if (hap_serv_add_char(hs, hap_char_target_visibility_state_create(0)) != HAP_SUCCESS) {
    	goto err;
    }
    */

    return hs;

err:
    hap_serv_delete(hs);
    return NULL;
}

hap_serv_t *hap_serv_ac_create(uint8_t curr_heater_cooler_state, float curr_temp, float targ_temp, uint8_t temp_disp_units)
{

    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_HEATER_COOLER);
    if (!hs) {
        return NULL;
    }

    if (hap_serv_add_char(hs, hap_char_active_create(curr_heater_cooler_state > 0)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_current_temperature_create(curr_temp)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_current_heater_cooler_state_create(curr_heater_cooler_state)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_custom_target_heater_cooler_state_create(2)) != HAP_SUCCESS) {
        goto err;
    }
    /*
    if (hap_serv_add_char(hs, hap_char_temperature_display_units_create(temp_disp_units)) != HAP_SUCCESS) {
        goto err;
    }
    */
    if (hap_serv_add_char(hs, hap_char_custom_cooling_threshold_temperature_create(20.0)) != HAP_SUCCESS) {
    	goto err;
    }
    /*
    if (hap_serv_add_char(hs, hap_char_custom_heating_threshold_temperature_create(20.0)) != HAP_SUCCESS) {
    	goto err;
    }
    */
    return hs;
err:
    hap_serv_delete(hs);
    return NULL;
}

hap_serv_t *hap_serv_ac_fan_create(bool IsActive, uint8_t TargetFanState, uint8_t SwingMode, uint8_t FanSpeed)
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_FAN_V2);
    if (!hs) {
        return NULL;
    }
    if (hap_serv_add_char(hs, hap_char_active_create(IsActive ? 1 : 0)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_target_fan_state_create(TargetFanState)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_ac_fan_rotation_create(FanSpeed)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_swing_mode_create(SwingMode)) != HAP_SUCCESS) {
    	goto err;
    }

    return hs;
err:
    hap_serv_delete(hs);
    return NULL;
}

void HomeKitUpdateCharValue(string StringAID, const char *ServiceUUID, const char *CharUUID, hap_val_t Value) {
	HomeKitUpdateCharValue((uint32_t)Converter::UintFromHexString<uint16_t>(StringAID), ServiceUUID, CharUUID, Value);
}

extern Device_t	Device;

void HomeKitUpdateCharValue(uint32_t AID, const char *ServiceUUID, const char *CharUUID, hap_val_t Value)
{
	hap_acc_t* Accessory = (Device.HomeKitBridge) ? hap_acc_get_by_aid(AID) : hap_get_first_acc();

	if (Accessory == NULL) 	return;

	hap_serv_t *Service  	= hap_acc_get_serv_by_uuid(Accessory, ServiceUUID);
	if (Service == NULL) 	return;

	hap_char_t *Char 		= hap_serv_get_char_by_uuid(Service, CharUUID);
	if (Char == NULL) 		return;

	hap_char_update_val(Char, &Value);
}


