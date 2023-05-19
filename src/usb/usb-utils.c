#include <usb/usb-utils.h>

char *getDeviceSerialNumber(io_service_t device)
{
    // get the serial number
    CFTypeRef serialNumber = IORegistryEntryCreateCFProperty(device, CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
    if (serialNumber == NULL)
    {
        LOG(LOG_FATAL, "ERROR: Failed to get USB serial number!");
        return NULL;
    }
    if (CFGetTypeID(serialNumber) != CFStringGetTypeID())
    {
        LOG(LOG_FATAL, "ERROR: Bad USB serial number, not a string!");
        return NULL;
    }
    CFStringRef serialNumberString = (CFStringRef)serialNumber;
    char *serialNumberCString = (char *)CFStringGetCStringPtr(serialNumberString, kCFStringEncodingUTF8);
    return serialNumberCString;
}

io_service_t findDevice()
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
            return 0;
        }
        if (productID == NULL)
        {
            LOG(LOG_FATAL, "ERROR: Failed to get USB product ID!");
            return 0;
        }
        if (CFGetTypeID(vendorID) != CFNumberGetTypeID())
        {
            LOG(LOG_FATAL, "ERROR: Bad USB vendor ID, not a number!");
            return 0;
        }
        if (CFGetTypeID(productID) != CFNumberGetTypeID())
        {
            LOG(LOG_FATAL, "ERROR: Bad USB product ID, not a number!");
            return 0;
        }
        int vendorIDInt = 0;
        int productIDInt = 0;
        // convert CFNumber to int
        CFNumberGetValue(vendorID, kCFNumberIntType, &vendorIDInt);
        CFNumberGetValue(productID, kCFNumberIntType, &productIDInt);
        if (vendorIDInt == 0x5ac && productIDInt == 0x1227)
        {
            LOG(LOG_SUCCESS, "Found device in DFU mode");
            return service;
        }
    }
    LOG(LOG_FATAL, "ERROR: Failed to find device in DFU mode!");
    return -1;
}