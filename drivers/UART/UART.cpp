/*
*    UART.cpp
*    Class to using UART connections
*
*/

#include "UART.h"

map<uart_port_t, UART::UARTItem> UART::UARTItems = {};

void UART::Setup(int BaudRate, uart_port_t UARTPort, UARTReadCallback Callback, uint16_t BufSize) {

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate 				= BaudRate,
        .data_bits 				= UART_DATA_8_BITS,
        .parity    				= UART_PARITY_DISABLE,
        .stop_bits 				= UART_STOP_BITS_1,
        .flow_ctrl 				= UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh 	= 120,
		.use_ref_tick			= false
    };

    uart_param_config(UARTPort, &uart_config);
    uart_set_pin(UARTPort, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UARTPort, BufSize * 2, 0, 0, NULL, 0);

    UART::UARTItem Item;
    Item.BufferSize	= BufSize;
    Item.Callback	= Callback;

    UARTItems[UARTPort] = Item;
}

void UART::Start(uart_port_t UARTPortNum) {
	UARTItem Item = UARTGetItem(UARTPortNum);

	if (Item.Handle == 0) {
		Item.Handle = FreeRTOS::StartTask(ReadTask, "UARTReadTask" + UARTPort(UARTPortNum), (void *)(uint32_t)UARTPort(UARTPortNum), 2048);
		UARTItems[UARTPortNum] = Item;
	}
}

void UART::Stop(uart_port_t UARTPortNum) {
    if (UARTItems.count(UARTPortNum) != 0) {
    		UARTItem Item = UARTGetItem(UARTPortNum);

    		if (Item.Handle != nullptr) {
    			FreeRTOS::DeleteTask(Item.Handle);
    			Item.Handle = nullptr;
    			UARTItems[UARTPortNum] = Item;
    		}
    }
}

void UART::ReadTask(void *TaskData) {
	uart_port_t Port = UARTPort((uint32_t)TaskData);
	UARTItem Item = UARTGetItem(Port);

	uint8_t *data = (uint8_t *) malloc(Item.BufferSize);

    while (1) {
        int len = uart_read_bytes(Port, data, Item.BufferSize, 20 / portTICK_RATE_MS);

        if (len > 0) {
        		ESP_LOGI("tada","callback issued")
    			Item.Callback(Converter::ToString(*data));
        }
    }

    free(data);
}

uint8_t UART::UARTPort(uart_port_t UARTPort) {
	switch (UARTPort) {
		case UART_NUM_0 : return 0;
		case UART_NUM_1 : return 1;
		case UART_NUM_2 : return 2;
		default: return 3;
	}
}

uart_port_t 	UART::UARTPort(uint8_t UARTPortNum) {
	switch (UARTPortNum) {
		case 0 : return UART_NUM_0;
		case 1 : return UART_NUM_1;
		case 2 : return UART_NUM_2;
		default: return UART_NUM_MAX;
	}
}

UART::UARTItem UART::UARTGetItem(uart_port_t UARTPort) {
    if (UARTItems.count(UARTPort) != 0)
    		return UARTItems[UARTPort];
    else
    		return UART::UARTItem();
}

