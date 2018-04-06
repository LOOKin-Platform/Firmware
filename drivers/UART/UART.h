/*
*    UART.h
*    Class to using UART connections, experimental/not tested
*
*/

#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include <map>

#include "driver/uart.h"

#include "FreeRTOSWrapper.h"
#include "Converter.h"

using namespace std;

typedef void (*UARTReadCallback)(string);

class UART {
  public:
	struct UARTItem {
		TaskHandle_t 		Handle 		= 0;
		UARTReadCallback		Callback 	= nullptr;
		uint16_t				BufferSize	= 1024;
	};

	static void Setup(int BaudRate, uart_port_t UARTPort, UARTReadCallback Callback=nullptr, uint16_t BufSize = 1024);

	static void Start(uart_port_t UARTPortNum);
	static void Stop(uart_port_t UARTPortNum);
  private:
	static map<uart_port_t, UARTItem> UARTItems;

	static void ReadTask(void *);

	static uint8_t 		UARTPort(uart_port_t);
	static uart_port_t 	UARTPort(uint8_t);
	static UARTItem 		UARTGetItem(uart_port_t UARTPort);
};
#endif
