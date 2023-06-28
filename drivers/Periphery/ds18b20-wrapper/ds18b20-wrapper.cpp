#include "ds18b20-wrapper.h"

const char Tag[] 				= "ds18b20-wrapper";

bool		DS18B20::IsInited = false;

static OneWireBus * owb;
static owb_rmt_driver_info rmt_driver_info;

void DS18B20::Init(gpio_num_t GPIO) {
	if (IsInited)
		return;

    owb = owb_rmt_initialize(&rmt_driver_info, GPIO, RMT_CHANNEL_0, RMT_CHANNEL_1);

    owb_use_crc(owb, true);  // enable CRC check for ROM code

    IsInited = true;
}

void DS18B20::Deinit() {
    owb_uninitialize(owb);
}

vector<float> DS18B20::ReadData() {
    // Find all connected devices
    ESP_LOGI(Tag, "Find devices:");
    OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
    int num_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(owb, &search_state, &found);

    while (found)
    {
    	char rom_code_s[17];
    	owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
    	ESP_LOGI(Tag, "  %d : %s\n", num_devices, rom_code_s);
    	device_rom_codes[num_devices] = search_state.rom_code;
    	++num_devices;
    	owb_search_next(owb, &search_state, &found);
    }

    ESP_LOGI(Tag, "Found %d device%s", num_devices, num_devices == 1 ? "" : "s");

    // In this example, if a single device is present, then the ROM code is probably
    // not very interesting, so just print it out. If there are multiple devices,
    // then it may be useful to check that a specific device is present.

    if (num_devices == 1)
    {
    	// For a single device only:
    	OneWireBus_ROMCode rom_code;
    	owb_status status = owb_read_rom(owb, &rom_code);
    	if (status == OWB_STATUS_OK)
    	{
    		char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
    		owb_string_from_rom_code(rom_code, rom_code_s, sizeof(rom_code_s));
    		ESP_LOGI(Tag, "Single device %s present", rom_code_s);
    	}
    	else
    		ESP_LOGE(Tag, "An error occurred reading ROM code: %d", status);
    }

    // Create DS18B20 devices on the 1-Wire bus
    DS18B20_Info * devices[MAX_DEVICES] = {0};
    for (int i = 0; i < num_devices; ++i)
    {
    	DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
    	devices[i] = ds18b20_info;

    	if (num_devices == 1)
    	{
    		ESP_LOGI(Tag, "Single device optimisations enabled");
    		ds18b20_init_solo(ds18b20_info, owb);          // only one device on bus
    	}
    	else
    	{
    		ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
    	}
    	ds18b20_use_crc(ds18b20_info, true);           // enable CRC check for temperature readings
    	ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
    }

    // Check for parasitic-powered devices
    bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power) {
    	ESP_LOGI(Tag, "Parasitic-powered devices detected");
    }

    // In parasitic-power mode, devices cannot indicate when conversions are complete,
    // so waiting for a temperature conversion must be done by waiting a prescribed duration
    owb_use_parasitic_power(owb, parasitic_power);

    // An external pull-up circuit is used to supply extra current to OneWireBus devices
    // during temperature conversions.
    //owb_use_strong_pullup_gpio(owb, GPIO_NUM_17);


    // Read temperatures more efficiently by starting conversions on all devices at the same time
    int errors_count[MAX_DEVICES] = {0};
    int sample_count = 0;

    vector<float> TempData = vector<float>();

    if (num_devices > 0)
    {
        ds18b20_convert_all(owb);

        // In this application all devices use the same resolution,
        // so use the first device to determine the delay
        ds18b20_wait_for_conversion(devices[0]);

        // Read the results immediately after conversion otherwise it may fail
        // (using printf before reading may take too long)
        float readings[MAX_DEVICES] = { 0 };
        DS18B20_ERROR errors[MAX_DEVICES] = {};

        for (int i = 0; i < num_devices; ++i)
        	errors[i] = ds18b20_read_temp(devices[i], &readings[i]);

        // Print results in a separate loop, after all have been read
        ESP_LOGI("DS18B20", "Temperature readings (degrees C): sample %d", ++sample_count);
        for (int i = 0; i < num_devices; ++i)
        {
        	if (errors[i] != DS18B20_OK)
        		++errors_count[i];

        	TempData.push_back(readings[i]);

        	ESP_LOGI("DS18B20", "  %d: %.1f    %d errors\n", i, readings[i], errors_count[i]);
        }
    }

    // clean up dynamically allocated data
    for (int i = 0; i < num_devices; ++i)
        ds18b20_free(&devices[i]);

    return TempData;
}
