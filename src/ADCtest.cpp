#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_adc_cal.h"

#include "FreeRTOSWrapper.h"
#include "ISR.h"
#include "Converter.h"
#include "DateTime.h"
#include "WebServer.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static const adc1_channel_t 	channel1 	= ADC1_CHANNEL_5;     // GPIO33 for ADC1
static const adc1_channel_t 	channel2 	= ADC1_CHANNEL_6;     // GPIO34 for ADC1
static const adc1_channel_t 	channel3 	= ADC1_CHANNEL_7;     // GPIO35 for ADC1
static const adc_atten_t 	atten 		= ADC_ATTEN_11db;

static void check_efuse() {
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    }
    else {
        printf("eFuse Vref: NOT supported\n");
    }
}

struct ValueStruct {
	uint32_t Uptime = 0;
    uint16_t  Value1 = 0;
    uint16_t  Value2 = 0;
    uint16_t  Value3 = 0;
};

static vector<ValueStruct> ADCData = {};

static uint32_t TimerCounter = 0;

static void ADCCallback(void *) {
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    if (TimerCounter < 1000) // считать 100 милисекунд c момента старта
    {
    		ValueStruct ValueItem;

    	    ValueItem.Uptime	= Time::UptimeU(); //?
    	    ValueItem.Value1	= (uint16_t)adc1_get_raw(channel1);
    	    ValueItem.Value2	= (uint16_t)adc1_get_raw(channel2);
    	    ValueItem.Value3	= (uint16_t)adc1_get_raw(channel3);

    	    TimerCounter++;

    	    ADCData.push_back(ValueItem);
    }

    //dac_out_voltage( DACChannel, tmp );
}

void ADCHandleHTTPRequest(WebServer_t::Response &Result, QueryType Type, vector<string> URLParts, map<string,string> Params) {
  if (Type == QueryType::GET) {
    if (URLParts.size() == 0) {
        for (ValueStruct& Item : ADCData) {
			Result.Body += "{" + Converter::ToString((uint32_t)Item.Uptime) + ";" + Converter::ToString((uint32_t)Item.Value1) + ";" + Converter::ToString((uint32_t)Item.Value2) + ";" + Converter::ToString((uint32_t)Item.Value3) + "}";
        }
    }
  }
}


void ADCTest() {
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel1, atten);
    adc1_config_channel_atten(channel2, atten);
    adc1_config_channel_atten(channel3, atten);

    //dac_output_enable(DACChannel);

    //FreeRTOS::StartTask(UDPPacketSend, "UDPPacketSend", nullptr, 4096);

    ISR::HardwareTimer Timer1(TIMER_GROUP_0, TIMER_0, 100, &ADCCallback);
    Timer1.Start();
}
