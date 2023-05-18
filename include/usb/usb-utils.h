#ifndef USB_UTILS_H
#define USB_UTILS_H

#include <AlfieLoader.h>
#include <exploit/dfu.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

#define LOG(logLevel, ...) loaderLog(logLevel, verbosity, __FILE__, __LINE__, __func__, __VA_ARGS__)

char *getDeviceSerialNumber(io_service_t device);
io_service_t findDevice();
dfu_device_t initDevice(io_service_t device);
pwndfu_device_t initPwnedDevice(io_service_t device);
int isPwned(char *serialNumber);
int isSupported(int cpid);

#endif // USB_UTILS_H