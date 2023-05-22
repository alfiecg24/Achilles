#include <usb/usb.h>

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

uint32_t usb_control_transfer_without_timeout(io_service_t service, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
    IOReturn ret;
    IOUSBDevRequest request;
    request.bmRequestType = bmRequestType;
    request.bRequest = bRequest;
    request.wValue = OSSwapLittleToHostInt16(wValue);
    request.wIndex = OSSwapLittleToHostInt16(wIndex);
    request.wLength = OSSwapLittleToHostInt16(wLength);
    request.pData = data;

    // raw control transfer in IOKit using IOUSBDevRequest and the service parameter
    //ret = IOUSBDeviceRequest(service, &request);


    if (ret != kIOReturnSuccess)
    {
        LOG(LOG_FATAL, "ERROR: Failed to send USB control transfer!");
        return -1;
    }
    return request.wLenDone;
}

uint32_t usb_control_transfer(io_service_t service, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, uint32_t timeout)
{
    IOReturn ret;
    IOUSBDevRequestTO request;
    request.bmRequestType = bmRequestType;
    request.bRequest = bRequest;
    request.wValue = OSSwapLittleToHostInt16(wValue);
    request.wIndex = OSSwapLittleToHostInt16(wIndex);
    request.wLength = OSSwapLittleToHostInt16(wLength);
    request.pData = data;
    request.noDataTimeout = timeout;
    request.completionTimeout = timeout;
    //ret = IOUSBDeviceRequestTO(service, &request);
    if (ret != kIOReturnSuccess)
    {
        LOG(LOG_FATAL, "ERROR: Failed to send USB control transfer!");
        return -1;
    }
    return request.wLenDone;
}