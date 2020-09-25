#ifndef HOMEKIT_SDK_CUSTOM
#define HOMEKIT_SDK_CUSTOM

#include <HomeKit.h>

#define CHAR_ACTIVE_IDENTIFIER_UUID 		"000000E7-0000-1000-8000-0026BB765291"
#define CHAR_IDENTIFIER_UUID 				"000000E6-0000-1000-8000-0026BB765291"
#define CHAR_CONFIGUREDNAME_UUID 			"000000E3-0000-1000-8000-0026BB765291"
#define CHAR_REMOTEKEY_UUID					"000000E1-0000-1000-8000-0026BB765291"
#define CHAR_SLEEP_DISCOVERY_UUID			"000000E8-0000-1000-8000-0026BB765291"
#define CHAR_MUTE_UUID						"0000011A-0000-1000-8000-0026BB765291"
#define CHAR_VOLUME_CONTROL_TYPE_UUID		"000000E9-0000-1000-8000-0026BB765291"
#define CHAR_VOLUME_UUID 					"00000119-0000-1000-8000-0026BB765291"
#define CHAR_CONFIGURED_NAME_UUID			"000000E3-0000-1000-8000-0026BB765291"
#define CHAR_INPUT_SOURCE_TYPE_UUID			"000000DB-0000-1000-8000-0026BB765291"
#define CHAR_CURRENT_VISIBILITY_STATE_UUID	"00000135-0000-1000-8000-0026BB765291"
#define CHAR_TARGET_VISIBILITY_STATE_UUID	"00000134-0000-1000-8000-0026BB765291"
#define CHAR_POWER_MODE_SELECTION_UUID		"000000DF-0000-1000-8000-0026BB765291"
#define CHAR_VOLUME_SELECTOR_UUID 			"000000EA-0000-1000-8000-0026BB765291"

#define SERVICE_TV_UUID 					"000000D8-0000-1000-8000-0026BB765291"
#define SERVICE_TELEVISION_SPEAKER_UUID		"00000113-0000-1000-8000-0026BB765291"
#define SERVICE_INPUT_SOURCE_UUID			"000000D9-0000-1000-8000-0026BB765291"


/* Char: Active identifier */
hap_char_t *hap_char_active_identifier_create(uint32_t ActiveID = 0)
{
    hap_char_t *hc = hap_char_uint32_create(CHAR_ACTIVE_IDENTIFIER_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, ActiveID);
    if (!hc) {
        return NULL;
    }

    return hc;
}

/* Char: Identifier */
hap_char_t *hap_char_identifier_create(uint32_t ActiveID = 0)
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
    hap_char_t *hc = hap_char_string_create(CHAR_CONFIGUREDNAME_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, name);
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

/* Char: Target Temperature for ac */
hap_char_t *hap_char_target_temperature_create_ac(float curr_temp)
{
    hap_char_t *hc = hap_char_float_create(HAP_CHAR_UUID_TARGET_TEMPERATURE, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, curr_temp);
    if (!hc) {
        return NULL;
    }

    hap_char_float_set_constraints(hc, 16, 30, 1);
    hap_char_add_unit(hc, HAP_CHAR_UNIT_CELSIUS);

    return hc;
}


/* Char: Status Active (always ON) */
hap_char_t *hap_char_always_active_create(uint8_t active)
{
    hap_char_t *hc = hap_char_uint8_create(HAP_CHAR_UUID_ACTIVE, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV, active);

    hap_char_int_set_constraints(hc, 0, 1, 1);

    if (!hc) {
        return NULL;
    }

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
hap_serv_t *hap_serv_tv_create(uint8_t active)
{
    hap_serv_t *hs = hap_serv_create(SERVICE_TV_UUID);

    if (!hs)
        return NULL;

    if (hap_serv_add_char(hs, hap_char_active_create(active)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_active_identifier_create(1)) != HAP_SUCCESS)	{
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_configured_name_create(NULL)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_remotekey_create(0)) != HAP_SUCCESS) {
    	goto err;
    }

    if (hap_serv_add_char(hs, hap_char_sleep_discovery_mode_create(1)) != HAP_SUCCESS) {
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

    if (hap_serv_add_char(hs, hap_char_volume_create(10)) != HAP_SUCCESS) {
    	goto err;
    }

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

hap_serv_t *hap_serv_ac_tempmode_create(uint8_t curr_heating_cooling_state, uint8_t targ_heating_cooling_state,
										float curr_temp, float targ_temp, uint8_t temp_disp_units)
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_THERMOSTAT);
    if (!hs) {
        return NULL;
    }

    if (curr_heating_cooling_state > 2)
    	curr_heating_cooling_state = 2;

    if (hap_serv_add_char(hs, hap_char_current_heating_cooling_state_create(curr_heating_cooling_state)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_target_heating_cooling_state_create(targ_heating_cooling_state)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_current_temperature_create_ac(curr_temp)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_target_temperature_create_ac(targ_temp)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_temperature_display_units_create(temp_disp_units)) != HAP_SUCCESS) {
        goto err;
    }
    return hs;
err:
    hap_serv_delete(hs);
    return NULL;
}

hap_serv_t *hap_serv_ac_fanswing_create(uint8_t TargetFanState, uint8_t SwingMode, uint8_t FanSpeed)
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_FAN_V2);
    if (!hs) {
        return NULL;
    }
    if (hap_serv_add_char(hs, hap_char_always_active_create(1)) != HAP_SUCCESS) {
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

#endif
