/*
 * BLExceptions.h
 *
 *  Created on: Nov 27, 2017
 *      Author: kolban
 */

#ifndef DRIVERS_BLEEXCEPTIONS_H_
#define DRIVERS_BLEEXCEPTIONS_H_
#include "sdkconfig.h"

#if CONFIG_CXX_EXCEPTIONS != 1
#error "C++ exception handling must be enabled within make menuconfig. See Compiler Options > Enable C++ Exceptions."
#endif

#include <exception>


class BLEDisconnectedException : public std::exception {
	const char* what() const throw () {
		return "BLE Disconnected";
	}
};

class BLEUuidNotFoundException : public std::exception {
	const char* what() const throw () {
		return "No such UUID";
	}
};

#endif /* DRIVERS_BLEEXCEPTIONS_H_ */
