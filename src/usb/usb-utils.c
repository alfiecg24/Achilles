#include <usb/usb-utils.h>

extern int verbosity;

char *getDeviceSerialNumber(io_service_t device) {
    // get the serial number
    CFTypeRef serialNumber = IORegistryEntryCreateCFProperty(device, CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
    if (serialNumber == NULL) {
        LOG(LOG_FATAL, "ERROR: Failed to get USB serial number!");
        return NULL;
    }
    if (CFGetTypeID(serialNumber) != CFStringGetTypeID()) {
        LOG(LOG_FATAL, "ERROR: Bad USB serial number, not a string!");
        return NULL;
    }
    CFStringRef serialNumberString = (CFStringRef)serialNumber;
    char *serialNumberCString = (char *)CFStringGetCStringPtr(serialNumberString, kCFStringEncodingUTF8);
    return serialNumberCString;
}

io_service_t findDevice() {
    // get all USB devices
    io_iterator_t iterator;
    kern_return_t kr = IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching("IOUSBDevice"), &iterator);
    if (kr != KERN_SUCCESS) {
        LOG(LOG_FATAL, "ERROR: Failed to get IOUSBDevice services!");
        return 0;
    }
    // iterate over all devices
    io_service_t service = 0;
    while ((service = IOIteratorNext(iterator))) {
        CFTypeRef vendorID = IORegistryEntryCreateCFProperty(service, CFSTR("idVendor"), kCFAllocatorDefault, 0);
        CFTypeRef productID = IORegistryEntryCreateCFProperty(service, CFSTR("idProduct"), kCFAllocatorDefault, 0);
        if (vendorID == NULL) {
            LOG(LOG_FATAL, "ERROR: Failed to get USB vendor ID!");
            return 0;
        }
        if (productID == NULL) {
            LOG(LOG_FATAL, "ERROR: Failed to get USB product ID!");
            return 0;
        }
        if (CFGetTypeID(vendorID) != CFNumberGetTypeID()) {
            LOG(LOG_FATAL, "ERROR: Bad USB vendor ID, not a number!");
            return 0;
        }
        if (CFGetTypeID(productID) != CFNumberGetTypeID()) {
            LOG(LOG_FATAL, "ERROR: Bad USB product ID, not a number!");
            return 0;
        }
        int vendorIDInt = 0;
        int productIDInt = 0;
        // convert CFNumber to int
        CFNumberGetValue(vendorID, kCFNumberIntType, &vendorIDInt);
        CFNumberGetValue(productID, kCFNumberIntType, &productIDInt);
        if (vendorIDInt == 0x5ac && productIDInt == 0x1227) {
            LOG(LOG_SUCCESS, "Found device in pwned DFU mode");
            return service;
        }
    }
    LOG(LOG_FATAL, "ERROR: Failed to find device in DFU mode!");
    return -1;
}

struct dfu_device_t initDevice(io_service_t device)
{
    dfu_device_t dfuDevice;
    dfuDevice.service = device;
    dfuDevice.serial = parseSerialNumber(getDeviceSerialNumber(device));
    return dfuDevice;
}

struct pwndfu_device_t initPwnedDevice(io_service_t device)
{
    pwndfu_device_t pwnedDevice;
    pwnedDevice.service = device;
    pwnedDevice.serial = parseSerialNumberPwned(getDeviceSerialNumber(device));
    return pwnedDevice;
}

int isPwned(char *serialNumber) {
    if (strncmp(serialNumber, "PWND", 4) != 0) {
        LOG(LOG_DEBUG, "Serial number: %s", serialNumber);
        return 1;
    }

    return 0;
}

int isSupported(int cpid) {
    // check if CPID is supported
	if (
		cpid == 0x8960 || cpid == 0x7000 || cpid == 0x7001
		|| cpid == 0x8000 || cpid == 0x8001 || cpid == 0x8003
		|| cpid == 0x8010 || cpid == 0x8011 || cpid == 0x8012
		|| cpid == 0x8015
	
	) return 0;
	return 1;
}