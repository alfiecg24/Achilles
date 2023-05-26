#ifndef DEVICE_H
#define DEVICE_H

#include <AlfieLoader.h>
#include <usb/usb.h>
#include <exploit/dfu.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>

enum DeviceMode {
    MODE_NORMAL,
    MODE_RECOVERY,
    MODE_DFU
};

typedef enum DeviceMode DeviceMode;

typedef struct
{
    usb_handle_t handle;
    char *serialNumber;
    DeviceMode mode;
} device_t;


device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode, int vid, int pid);
int findDevice(device_t *device);
int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout);

#endif // DEVICE_H