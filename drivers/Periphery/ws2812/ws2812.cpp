#include <esp_log.h>
#include <driver/rmt.h>
#include <driver/gpio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>

#include "HardwareIO.h"
#include "sdkconfig.h"
#include "ws2812.h"

static const char* TAG = "WS2812";

/*
 * Internal function not exposed.  Get the pixel channel color from the channel
 * type which should be one of 'R', 'G' or 'B'.
 */
uint8_t WS2812::getChannelValueByType(char type, pixel_t pixel) {
	switch (type) {
		case 'r':
		case 'R':
			return pixel.red;
		case 'b':
		case 'B':
			return pixel.blue;
		case 'g':
		case 'G':
			return pixel.green;
		default:
			ESP_LOGW(TAG, "Unknown color channel 0x%2x", type);
			return 0;
	}
} // getChannelValueByType

void WS2812::led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

bool WS2812::GetIsLighted()
{
	return IsLighted;
}


/**
 * @brief Construct a wrapper for the pixels.
 *
 * In order to drive the NeoPixels we need to supply some basic information.  This
 * includes the GPIO pin that is connected to the data-in (DIN) of the devices.
 * Since we also want to be able to drive a string of pixels, we need to tell the class
 * how many pixels are present in the string.
 *
 * @param [in] dinPin The GPIO pin used to drive the data.
 * @param [in] pixelCount The number of pixels in the strand.
 * @param [in] channel The RMT channel to use.  Defaults to RMT_CHANNEL_0.
 */
WS2812::WS2812(gpio_num_t dinPin, uint16_t pixelCount, rmt_channel_t channel) {
	/*
	if (pixelCount == 0) {
		throw std::range_error("Pixel count was 0");
	}
	*/
	assert(GPIO::IsInRange(dinPin));

	this->pixelCount = pixelCount;
	this->channel    = (rmt_channel_t) channel;

	// The number of items is number of pixels * 24 bits per pixel + the terminator.
	// Remember that an item is TWO RMT output bits ... for NeoPixels this is correct because
	// on Neopixel bit is TWO bits of output ... the high value and the low value

	this->pixels     = new pixel_t[pixelCount];
	this->colorOrder = (char*) "GRB";
	Clear();

	rmt_config_t config;
	config.rmt_mode = RMT_MODE_TX;
	config.channel = channel;
	config.gpio_num = dinPin;
	config.clk_div 	= 2; //80;
	config.mem_block_num = 1;
	config.flags = 0;
	config.tx_config.carrier_freq_hz = 38000;
	config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
	config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	config.tx_config.carrier_duty_percent = 33;
	config.tx_config.carrier_en = false;
	config.tx_config.loop_en = false;
	config.tx_config.idle_output_en = true;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(this->channel, 0, 0));

    led_strip_config_t strip_config;
    strip_config.max_leds = this->pixelCount;
    strip_config.dev = (led_strip_dev_t)channel;

    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    ESP_ERROR_CHECK(strip->clear(strip, 100));

} // WS2812

/**
 * @brief Show the current Neopixel data.
 *
 * Drive the LEDs with the values that were previously set.
 */
void WS2812::Show() {
	for (uint16_t i = 0; i < this->pixelCount; i++)
        ESP_ERROR_CHECK(strip->set_pixel(strip, i, this->pixels[i].red, this->pixels[i].green, this->pixels[i].blue));

	IsLighted = true;

    ESP_ERROR_CHECK(strip->refresh(strip, 100));
} // show


/**
 * @brief Set the color order of data sent to the LEDs.
 *
 * Data is sent to the WS2812s in a serial fashion.  There are 8 bits of data for each of the three
 * channel colors (red, green and blue).  The WS2812 LEDs typically expect the data to arrive in the
 * order of "green" then "red" then "blue".  However, this has been found to vary between some
 * models and manufacturers.  What this means is that some want "red", "green", "blue" and still others
 * have their own orders.  This function can be called to override the default ordering of "GRB".
 * We can specify
 * an alternate order by supply an alternate three character string made up of 'R', 'G' and 'B'
 * for example "RGB".
 */
void WS2812::setColorOrder(char* colorOrder) {
	if (colorOrder != nullptr && strlen(colorOrder) == 3) {
		this->colorOrder = colorOrder;
	}
} // setColorOrder


/**
 * @brief Set the given pixel to the specified color.
 *
 * The LEDs are not actually updated until a call to show().
 *
 * @param [in] index The pixel that is to have its color set.
 * @param [in] red The amount of red in the pixel.
 * @param [in] green The amount of green in the pixel.
 * @param [in] blue The amount of blue in the pixel.
 */
void WS2812::setPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue) {
	assert(index < pixelCount);
	this->pixels[index].red   = red;
	this->pixels[index].green = green;
	this->pixels[index].blue  = blue;
} // setPixel


void WS2812::setAllPixels(uint8_t red, uint8_t green, uint8_t blue) {
	for (int i=0; i<this->pixelCount; i++)
		this->setPixel(i, red, green, blue);
} // setPixel

/**
 * @brief Set the given pixel to the specified HSB color.
 *
 * The LEDs are not actually updated until a call to show().
 *
 * @param [in] index The pixel that is to have its color set.
 * @param [in] hue The amount of hue in the pixel (0-360).
 * @param [in] saturation The amount of saturation in the pixel (0-255).
 * @param [in] brightness The amount of brightness in the pixel (0-255).
 */
void WS2812::setHSBPixel(uint16_t index, uint16_t hue, uint8_t saturation, uint8_t brightness) {
	double sat_red;
	double sat_green;
	double sat_blue;
	double ctmp_red;
	double ctmp_green;
	double ctmp_blue;
	double new_red;
	double new_green;
	double new_blue;
	double dSaturation = (double) saturation / 255;
	double dBrightness = (double) brightness / 255;

	assert(index < pixelCount);

	if (hue < 120) {
		sat_red = (120 - hue) / 60.0;
		sat_green = hue / 60.0;
		sat_blue = 0;
	} else if (hue < 240) {
		sat_red = 0;
		sat_green = (240 - hue) / 60.0;
		sat_blue = (hue - 120) / 60.0;
	} else {
		sat_red = (hue - 240) / 60.0;
		sat_green = 0;
		sat_blue = (360 - hue) / 60.0;
	}

	if (sat_red > 1.0) {
		sat_red = 1.0;
	}
	if (sat_green > 1.0) {
		sat_green = 1.0;
	}
	if (sat_blue > 1.0) {
		sat_blue = 1.0;
	}

	ctmp_red = 2 * dSaturation * sat_red + (1 - dSaturation);
	ctmp_green = 2 * dSaturation * sat_green + (1 - dSaturation);
	ctmp_blue = 2 * dSaturation * sat_blue + (1 - dSaturation);

	if (dBrightness < 0.5) {
		new_red = dBrightness * ctmp_red;
		new_green = dBrightness * ctmp_green;
		new_blue = dBrightness * ctmp_blue;
	} else {
		new_red = (1 - dBrightness) * ctmp_red + 2 * dBrightness - 1;
		new_green = (1 - dBrightness) * ctmp_green + 2 * dBrightness - 1;
		new_blue = (1 - dBrightness) * ctmp_blue + 2 * dBrightness - 1;
	}

	this->pixels[index].red   = (uint8_t)(new_red * 255);
	this->pixels[index].green = (uint8_t)(new_green * 255);
	this->pixels[index].blue  = (uint8_t)(new_blue * 255);
} // setHSBPixel


/**
 * @brief Clear all the pixel colors.
 *
 * This sets all the pixels to off which is no brightness for all of the color channels.
 * The LEDs are not actually updated until a call to show().
 */
void WS2812::Clear() {
	for (uint16_t i = 0; i < this->pixelCount; i++)
	{
		this->pixels[i].red   = 0;
		this->pixels[i].green = 0;
		this->pixels[i].blue  = 0;
	}

	IsLighted = false;

	if (strip != NULL)
		strip->clear(strip, 50);
} // clear


/**
 * @brief Class instance destructor.
 */
WS2812::~WS2812() {
	delete this->pixels;
} // ~WS2812()


void WS2812::Rainbow() {
	int EXAMPLE_CHASE_SPEED_MS  = 100;
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;

    led_strip_config_t strip_config;
    strip_config.max_leds = this->pixelCount;
    strip_config.dev = (led_strip_dev_t)channel;

    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    ESP_ERROR_CHECK(strip->clear(strip, 100));


    // Clear LED strip (turn off all LEDs)
    // Show simple rainbow chasing pattern
    ESP_LOGI(TAG, "LED Rainbow Chase Start");
    while (true) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < this->pixelCount; j += 3) {
                // Build RGB values
                hue = j * 360 / this->pixelCount + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                // Write RGB values to strip driver
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(strip->refresh(strip, 100));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            strip->clear(strip, 50);
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }
}
