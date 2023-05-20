#include <usb/device.h>

device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode)
{
    device_t dev;
    dev.service = device;
    dev.serialNumber = serialNumber;
    dev.mode = mode;
    return dev;
}

int findDevice(device_t *device, bool isWaitingForDevice)
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
        if (vendorIDInt == 0x5ac && productIDInt == 0x1227)
        {
            *device = initDevice(service, getDeviceSerialNumber(service), MODE_DFU);
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x1281)
        {
            *device = initDevice(service, getDeviceSerialNumber(service), MODE_RECOVERY);
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x12ab)
        {
            *device = initDevice(service, getDeviceSerialNumber(service), MODE_NORMAL);
            return 0;
        }
    }
    if (isWaitingForDevice == false) {
        LOG(LOG_FATAL, "ERROR: Failed to find device in DFU mode!");
    }
    return -1;
}

int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout) {
    int i = 0;
    while (1) {
        if (findDevice(device, true) == 0 && device->mode == mode) {
            return 0;
        }
        if (i >= timeout) {
            return -1;
        }
        i++;
        sleep(1);
    }
}