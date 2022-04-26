#ifndef PERIPHERY_WS2812_H_
#define PERIPHERY_WS2812_H_
#include <stdint.h>
#include <driver/rmt.h>
#include <driver/gpio.h>

#include "led_strip.h"

/**
 * @brief A data type representing the color of a pixel.
 */
typedef struct {
	/**
	 * @brief The red component of the pixel.
	 */
	uint8_t red;
	/**
	 * @brief The green component of the pixel.
	 */
	uint8_t green;
	/**
	 * @brief The blue component of the pixel.
	 */
	uint8_t blue;
} pixel_t;

/**
 * @brief Driver for WS2812/NeoPixel data.
 *
 * NeoPixels or WS2812s are LED devices that can illuminate in arbitrary colors with
 * 8 bits of data for each of the red, green and blue channels.  These devices can be
 * daisy chained together to produce a string of LEDs.  If we call each LED instance
 * a pixel, then when we want to set the value of a string of pixels, we need to supply
 * the data for all the pixels.  This class encapsulates setting the color value for
 * an individual pixel within the string and, once you have set up all the desired colors,
 * you can then set all the pixels in a show() operation.  The class hides from you
 * the underlying details needed to drive the devices.
 *
 * @code{.cpp}
 * WS2812 ws2812 = WS2812(
 *   16, // Pin
 *   8   // Pixel count
 * );
 * ws2812.setPixel(0, 128, 0, 0);
 * ws2812.show();
 * @endcode
 */
class WS2812 {
public:

	void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

	WS2812(gpio_num_t gpioNum, uint16_t pixelCount, rmt_channel_t channel = RMT_CHANNEL_0);
	void Show();
	void setColorOrder(char* order);
	void setPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
	void setAllPixels(uint8_t red, uint8_t green, uint8_t blue);
	void setHSBPixel(uint16_t index, uint16_t hue, uint8_t saturation, uint8_t brightness);
	void Clear();

	bool GetIsLighted();

	void Rainbow();

	virtual ~WS2812();

private:
	char*          	colorOrder;
	uint16_t       	pixelCount;
	rmt_channel_t  	channel;
	pixel_t*       	pixels;
	bool 			IsLighted = false;

	led_strip_t		*strip = NULL;

	uint8_t 		getChannelValueByType(char type, pixel_t pixel);
};

#endif /* PERIPHERY_WS2812_H_ */
