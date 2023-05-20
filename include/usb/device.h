#ifndef DEVICE_H
#define DEVICE_H

// #include <AlfieLoader.h>
// #include <usb/usb.h>
// #include <utils/log.h>

#include <AlfieLoader.h>
#include <exploit/dfu.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>

enum DeviceMode {
    MODE_NORMAL,
    MODE_RECOVERY,
    MODE_DFU
};

typedef enum DeviceMode DeviceMode;

struct device_t
{
    io_service_t service;
    char *serialNumber;
    DeviceMode mode;
};

typedef struct device_t device_t;

device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode);
int findDevice(device_t *device, bool isWaiting);
int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout);

#endif // DEVICE_H