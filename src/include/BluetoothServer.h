/*
*    BluetoothServer.h
*    Class for handling Bluetooth connections
*
*/
#ifndef BLUETOOTHBSERVER_H
#define BLUETOOTHBSERVER_H

using namespace std;

#include <string>

class BluetoothServer_t {
  public:
	BluetoothServer_t();
    void Start();
    void Stop();
  private:
	TaskHandle_t BluetoothServerTaskHandle;
	TaskHandle_t BluetoothClientTaskHandle;

	static void BluetoothServerTask(void *);
	static void BluetoothClientTask(void *);
};

#endif
