#ifndef USB_UTILS_H
#define USB_UTILS_H

#include <AlfieLoader.h>
#include <exploit/dfu.h>
#include <usb/device.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

char *getDeviceSerialNumber(io_service_t device);
 // building throws a unknown type name 'device_t' error
/*
this is because device_t is defined in device.h, but device.h is not included in usb_utils.h
*/

#endif // USB_UTILS_H