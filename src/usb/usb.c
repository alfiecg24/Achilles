#include <usb/usb.h>
//#include <usb/device.h>

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