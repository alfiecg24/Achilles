#include <usb/device.h>

#ifdef ALFIELOADER_LIBUSB
device_t initDevice(char *serialNumber, DeviceMode mode, int vid, int pid)
{
    device_t dev;
    usb_handle_t handle;
    handle.vid = vid;
    handle.pid = pid;
    handle.device = NULL;
    dev.handle = handle;
    dev.serialNumber = serialNumber;
    dev.mode = mode;
    return dev;
}
#else
device_t initDevice(io_service_t device, char *serialNumber, DeviceMode mode, int vid, int pid)
{
    device_t dev;
    usb_handle_t handle;
    handle.service = device;
    handle.async_event_source = NULL;
    handle.vid = vid;
    handle.pid = pid;
    handle.device = NULL;
    dev.handle = handle;
    dev.serialNumber = serialNumber;
    dev.mode = mode;
    return dev;
}
#endif

#ifdef ALFIELOADER_LIBUSB



int findDevice(device_t *device, bool waiting) {
    // get all USB devices
    libusb_device **list;
    libusb_context *context = NULL;
    libusb_init(&context);
    ssize_t count = libusb_get_device_list(context, &list);
    if (count < 0) {
        LOG(LOG_ERROR, "Failed to get USB device list!");
        return -1;
    }

    for (int i = 0; i < count; i++) {
        libusb_device_handle *libusbHandle;
        struct libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(list[i], &desc);
        if (ret < 0) {
            LOG(LOG_ERROR, "Failed to get USB device descriptor!");
            return -1;
        }
        libusb_open(list[i], &libusbHandle);
        if (libusbHandle != NULL) {
            unsigned char serialNumber[256];
            libusb_get_string_descriptor_ascii(libusbHandle, desc.iSerialNumber, serialNumber, 256);

            int productID = desc.idProduct;
            int vendorID = desc.idVendor;
            usb_handle_t handle;
            initUSBHandle(&handle, vendorID, productID);
            handle.device = libusbHandle;
            
            if (vendorID == 0x5ac && productID == 0x1227)
            {
                *device = initDevice(getDeviceSerialNumber(&handle), MODE_DFU, vendorID, productID);
                if (!waiting) {
                    if (isInDownloadMode(device->serialNumber)) {
                        LOG(LOG_DEBUG, "Initialised device in YoloDFU/download mode"); 
                    } else {
                        LOG(LOG_DEBUG, "Initialised device in DFU mode"); 
                    }
                }
                return 0;
            }
            if (vendorID == 0x5ac && productID == 0x1281)
            {
                *device = initDevice(getDeviceSerialNumber(&handle), MODE_RECOVERY, vendorID, productID);
                if (!waiting) { LOG(LOG_DEBUG, "Initialised device in recovery mode"); }
                return 0;
            }
            if (vendorID == 0x5ac && (productID == 0x12ab || productID == 0x12a8))
            {
                *device = initDevice(getDeviceSerialNumber(&handle), MODE_NORMAL, vendorID, productID);
                if (!waiting) { LOG(LOG_DEBUG, "Initialised device in normal mode"); }
                return 0;
            }
            if (vendorID == 0x5ac && productID == 0x4141)
            {
                *device = initDevice(getDeviceSerialNumber(&handle), MODE_PONGO, vendorID, productID);
                if (!waiting) { LOG(LOG_DEBUG, "Initialised Pongo USB device"); }
                return 0;   
            }
            libusb_close(libusbHandle);
        }
        else {
            LOG(LOG_ERROR, "Failed to open USB device");
            return -1;
        }
    }
    libusb_free_device_list(list, 1);
    return -1;
}

#else

int findDevice(device_t *device, bool waiting)
{
    // get all USB devices
    io_iterator_t iterator;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOUSBDevice"), &iterator);
    if (kr != KERN_SUCCESS)
    {
        LOG(LOG_ERROR, "Failed to get IOUSBDevice services!");
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
            LOG(LOG_ERROR, "Failed to get USB vendor ID!");
            return -1;
        }
        if (productID == NULL)
        {
            LOG(LOG_ERROR, "Failed to get USB product ID!");
            return -1;
        }
        if (CFGetTypeID(vendorID) != CFNumberGetTypeID())
        {
            LOG(LOG_ERROR, "Bad USB vendor ID, not a number!");
            return -1;
        }
        if (CFGetTypeID(productID) != CFNumberGetTypeID())
        {
            LOG(LOG_ERROR, "Bad USB product ID, not a number!");
            return -1;
        }
        int vendorIDInt = 0;
        int productIDInt = 0;
        // convert CFNumber to int
        CFNumberGetValue(vendorID, kCFNumberIntType, &vendorIDInt);
        CFNumberGetValue(productID, kCFNumberIntType, &productIDInt);
        usb_handle_t handle;
        handle.service = service;
        if (service == 0)
        {
            LOG(LOG_ERROR, "Failed to get IOUSBDevice service!");
            return -1;
        }

        if (vendorIDInt == 0x5ac && productIDInt == 0x1227)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_DFU, vendorIDInt, productIDInt);
            if (!waiting) {
                if (isInDownloadMode(device->serialNumber)) {
                    LOG(LOG_DEBUG, "Initialised device in YoloDFU/download mode"); 
                } else {
                    LOG(LOG_DEBUG, "Initialised device in DFU mode"); 
                }
            }
            closeUSBHandle(&handle);
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x1281)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_RECOVERY, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised device in recovery mode"); }
            closeUSBHandle(&handle);
            return 0;
        }
        if (vendorIDInt == 0x5ac && (productIDInt == 0x12ab || productIDInt == 0x12a8))
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_NORMAL, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised device in normal mode"); }
            closeUSBHandle(&handle);
            return 0;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x4141)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_PONGO, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised Pongo USB device"); }
            closeUSBHandle(&handle);
            return 0;   
        }
    }
    return -1;
}

#endif

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