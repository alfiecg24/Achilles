#include <usb/device.h>

// ******************************************************
// Function: initDevice()
//
// Purpose: Initializes a device_t struct
//
// Parameters:
//      io_service_t device: the device to initialize
//      char *serialNumber: the serial number of the device
//      DeviceMode mode: the mode of the device
//      int vid: the vendor ID of the device
//      int pid: the product ID of the device
//
// Returns:
//      device_t: the initialized device
// ******************************************************
device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode, int vid, int pid)
{
    device_t dev;
    usb_handle_t handle;
    handle.service = device;
    handle.vid = vid;
    handle.pid = pid;
    handle.device = NULL;
    handle.async_event_source = NULL;
    dev.handle = handle;
    dev.serialNumber = serialNumber;
    dev.mode = mode;
    return dev;
}

// ******************************************************
// Function: findDevice()
//
// Purpose: Find a device
//
// Parameters:
//      device_t *device: the device struct to return
//
// Returns:
//      int: 0 if the device was found, 1 otherwise
// ******************************************************
int findDevice(device_t *device)
{
    // get all USB devices
    io_iterator_t iterator;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOUSBDevice"), &iterator);
    if (kr != KERN_SUCCESS)
    {
        LOG(LOG_FATAL, "ERROR: Failed to get IOUSBDevice services!");
        return 0;
    }
    // iterate over all devices
    io_service_t service = 0;
    while ((service = IOIteratorNext(iterator)))
    {
        CFTypeRef vendorID = IORegistryEntryCreateCFProperty(service, CFSTR("idVendor"), kCFAllocatorDefault, 0);
        CFTypeRef productID = IORegistryEntryCreateCFProperty(service, CFSTR("idProduct"), kCFAllocatorDefault, 0);
        if (vendorID == NULL)
        {
            LOG(LOG_FATAL, "ERROR: Failed to get USB vendor ID!");
            return -1;
        }
        if (productID == NULL)
        {
            LOG(LOG_FATAL, "ERROR: Failed to get USB product ID!");
            return -1;
        }
        if (CFGetTypeID(vendorID) != CFNumberGetTypeID())
        {
            LOG(LOG_FATAL, "ERROR: Bad USB vendor ID, not a number!");
            return -1;
        }
        if (CFGetTypeID(productID) != CFNumberGetTypeID())
        {
            LOG(LOG_FATAL, "ERROR: Bad USB product ID, not a number!");
            return -1;
        }
        int vendorIDInt = 0;
        int productIDInt = 0;
        // convert CFNumber to int
        CFNumberGetValue(vendorID, kCFNumberIntType, &vendorIDInt);
        CFNumberGetValue(productID, kCFNumberIntType, &productIDInt);
        usb_handle_t handle;
        handle.service = service;
        if (vendorIDInt == 0x5ac && productIDInt == 0x1227)
        {
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_DFU, vendorIDInt, productIDInt);
            LOG(LOG_DEBUG, "Initialized device in DFU mode");
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x1281)
        {
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_RECOVERY, vendorIDInt, productIDInt);
            LOG(LOG_DEBUG, "Initialized device in recovery mode");
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x12ab)
        {
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_NORMAL, vendorIDInt, productIDInt);
            LOG(LOG_DEBUG, "Initialized device in normal mode");
            return 0;
        }
    }
    return -1;
}

// ******************************************************
// Function: waitForDeviceInMode()
//
// Purpose: Wait for a device to be in a certain mode
//
// Parameters:
//      device_t *device: the device to wait for
//      DeviceMode mode: the mode to wait for
//      int timeout: the timeout in seconds
//
// Returns:
//      int: 0 if the device was found, 1 otherwise
// ******************************************************
int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout) {
    int i = 0;
    while (1) {
        if (findDevice(device) == 0 && device->mode == mode) {
            return 0;
        }
        if (i >= timeout) {
            return -1;
        }
        i++;
        sleep(1);
    }
}