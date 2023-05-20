#ifndef USB_UTILS_H
#define USB_UTILS_H

#include <AlfieLoader.h>
#include <exploit/dfu.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

char *getDeviceSerialNumber(io_service_t device);
io_service_t findDevice();

#endif // USB_UTILS_H