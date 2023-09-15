#include <usb/device.h>

#ifdef ACHILLES_LIBUSB
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

#ifdef ACHILLES_LIBUSB



int findUSBDevice(device_t *device, bool waiting) {
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

int findUSBDevice(device_t *device, bool waiting)
{
    // get all USB devices
    io_iterator_t iterator;
    int ret;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOUSBDevice"), &iterator);
    if (kr != KERN_SUCCESS)
    {
        LOG(LOG_ERROR, "Failed to get IOUSBDevice services!");
        ret = -1;
        goto done;
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
            ret = -1;
            goto done;
        }
        if (productID == NULL)
        {
            LOG(LOG_ERROR, "Failed to get USB product ID!");
            ret = -1;
            goto done;
        }
        if (CFGetTypeID(vendorID) != CFNumberGetTypeID())
        {
            LOG(LOG_ERROR, "Bad USB vendor ID, not a number!");
            ret = -1;
            goto done;
        }
        if (CFGetTypeID(productID) != CFNumberGetTypeID())
        {
            LOG(LOG_ERROR, "Bad USB product ID, not a number!");
            ret = -1;
            goto done;
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
            ret = -1;
            goto done;
        }

        if (vendorIDInt == 0x5ac && productIDInt == 0x1227)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            
            if (isInDownloadMode(device->serialNumber)) {
                if (!waiting) { LOG(LOG_DEBUG, "Initialised device in YoloDFU/download mode"); }
                *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_YOLO, vendorIDInt, productIDInt);
            } else {
                if (!waiting) { LOG(LOG_DEBUG, "Initialised device in DFU mode"); }
                *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_DFU, vendorIDInt, productIDInt);
            }
            
            closeUSBHandle(&handle);
            ret = 0;
            goto done;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x1281)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_RECOVERY, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised device in recovery mode"); }
            closeUSBHandle(&handle);
            ret = 0;
            goto done;
        }
        if (vendorIDInt == 0x5ac && (productIDInt == 0x12ab || productIDInt == 0x12a8))
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_NORMAL, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised device in normal mode"); }
            closeUSBHandle(&handle);
            ret = 0;
            goto done;
        }
        if (vendorIDInt == 0x5ac && productIDInt == 0x4141)
        {
            initUSBHandle(&handle, vendorIDInt, productIDInt);
            waitUSBHandle(&handle, NULL, NULL);
            *device = initDevice(service, getDeviceSerialNumber(&handle), MODE_PONGO, vendorIDInt, productIDInt);
            if (!waiting) { LOG(LOG_DEBUG, "Initialised Pongo USB device"); }
            closeUSBHandle(&handle);
            ret = 0;
            goto done;  
        }
    }
done:
    if (iterator) { IOServiceClose(iterator); }
    return ret;
}

#endif

bool getRecoveryDeviceIntoDFU(device_t *device) {
    waitUSBHandle(&device->handle, NULL, NULL);
    bool ret = sendRecoveryModeCommand(&device->handle, "setenv auto-boot true");
    if (!ret) {
        LOG(LOG_ERROR, "Failed to send auto-boot true");
    }
    ret = sendRecoveryModeCommand(&device->handle, "saveenv");
    if (!ret) {
        LOG(LOG_ERROR, "Failed to send saveenv");
    }
    LOG(LOG_DEBUG, "Sent auto-boot true and saveenv");
    LOG_NO_NEWLINE(LOG_INFO, "Press Enter when you are ready to enter DFU mode");
    getchar();
    DFUHelper();
    if (waitForDeviceInMode(device, MODE_DFU, 30) != 0) {
        LOG(LOG_ERROR, "Could not find device in DFU mode after 30 seconds", device->serialNumber);
        return false;
    }
    LOG(LOG_SUCCESS, "Device entered DFU mode successfully!");
    return true;
}

int findDevice(device_t *device, bool waiting) {
    idevice_t idevice = NULL;
    char **deviceIDs;
    int count;
    idevice_error_t listRet = idevice_get_device_list(&deviceIDs, &count);
    if (listRet != IDEVICE_E_SUCCESS)
    {
        LOG(LOG_ERROR, "Failed to get device list");
        return -1;
    }
    if (count == 0)
    {
        LOG(LOG_DEBUG, "No devices in normal mode found, looking for recovery/DFU devices");
        if (findUSBDevice(device, waiting) != 0) {
            return -1;
        }
        if (device->mode == MODE_RECOVERY) {
            if (!getRecoveryDeviceIntoDFU(device)) { return -1; }
        }
        return 0;
    }
    if (count > 1)
    {
        LOG(LOG_ERROR, "More than one device found, Achilles currently does not support multiple device connections at once.");
        return -1;
    }
    char *udid = deviceIDs[0];
	lockdownd_client_t lockdown = NULL;
	if (idevice_new(&idevice, udid) != IDEVICE_E_SUCCESS) {
		LOG(LOG_ERROR, "Could not connect to device");
		return -1;
	}
    LOG(LOG_SUCCESS, "Found device in normal mode, entering recovery mode");
    lockdownd_error_t ldret = lockdownd_client_new(idevice, &lockdown, "Achilles");
    if (ldret != LOCKDOWN_E_SUCCESS) {
        LOG(LOG_ERROR, "Could not connect to lockdownd: %s", lockdownd_strerror(ldret));
        return -1;
    }
    ldret = lockdownd_enter_recovery(lockdown);
    if (ldret == LOCKDOWN_E_SESSION_INACTIVE) {
        lockdownd_client_free(lockdown);
        lockdown = NULL;
        ldret = lockdownd_client_new_with_handshake(idevice, &lockdown, "Achilles");
        if (ldret != LOCKDOWN_E_SUCCESS) {
            LOG(LOG_ERROR, "Could not connect to lockdownd: %s", lockdownd_strerror(ldret));
            return -1;
        }
        ldret = lockdownd_enter_recovery(lockdown);
    }
    if (ldret != LOCKDOWN_E_SUCCESS) {
        LOG(LOG_ERROR, "Could not trigger entering recovery mode: %s", lockdownd_strerror(ldret));
        return -1;
    }
    lockdownd_client_free(lockdown);
    idevice_free(idevice);
    sleep(5);
    waitForDeviceInMode(device, MODE_RECOVERY, 10);
    
    if (!getRecoveryDeviceIntoDFU(device)) { return -1; }
    
	return 0;
}

int waitForDeviceInMode(device_t *device, DeviceMode mode, int timeout) {
    int i = 0;
    while (1) {
        if (findUSBDevice(device, true) == 0 && device->mode == mode) {
            return 0;
        }
        if (i >= timeout) {
            return -1;
        }
        i++;
        sleep(1);
    }
}