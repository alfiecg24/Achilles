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

// struct {
// 	uint8_t b_len, b_descriptor_type;
// 	uint16_t bcd_usb;
// 	uint8_t b_device_class, b_device_sub_class, b_device_protocol, b_max_packet_sz;
// 	uint16_t id_vendor, id_product, bcd_device;
// 	uint8_t i_manufacturer, i_product, i_serial_number, b_num_configurations;
// } device_descriptor;

device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode, int vid, int pid);
int findDevice(device_t *device, bool waiting);
int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout);

#endif // DEVICE_H