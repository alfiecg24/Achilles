#ifndef USB_UTILS_H
#define USB_UTILS_H

#include <AlfieLoader.h>
#include <exploit/dfu.h>
#include <usb/device.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>

char *getDeviceSerialNumber(io_service_t device);

#endif // USB_UTILS_H