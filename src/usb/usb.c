#include <usb/usb.h>

// ******************************************************
// Function: getDeviceSerialNumber()
//
// Purpose: Get the serial number of the device from the USB handle
//
// Parameters:
//      usb_handle_t *handle: the USB handle
//
// Returns:
//      char *: the serial number of the device
// ******************************************************
char *getDeviceSerialNumber(usb_handle_t *handle)
{
    CFTypeRef serialNumber = IORegistryEntryCreateCFProperty(handle->service, CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
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

// ******************************************************
// Function: sleep_ms()
//
// Purpose: Sleep for a given number of milliseconds
//
// Parameters:
//      unsigned ms: Length of time to sleep in milliseconds
//
// Taken from gaster
// ******************************************************
void sleep_ms(unsigned ms) {
    struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

// ******************************************************
// Function: cfDictionarySetInt16()
//
// Purpose: Set a 16-bit integer in a CFDictionary
//
// Parameters:
//      CFMutableDictionaryRef dict: the dictionary to set the value in
//      const void *key: the key to set the value for
//      uint16_t val: the value to set
// ******************************************************
void cfDictionarySetInt16(CFMutableDictionaryRef dict, const void *key, uint16_t val) {
	CFNumberRef cf_val = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &val);

	if(cf_val != NULL) {
		CFDictionarySetValue(dict, key, cf_val);
		CFRelease(cf_val);
	}
}

// ******************************************************
// Function: queryUSBInterface()
//
// Purpose: Query a USB interface
//
// Parameters:
//      io_service_t service: the service to query
//      CFUUIDRef plugin_type: the plugin type
//      CFUUIDRef interface_type: the interface type
//      LPVOID *interface: the interface to return
//
// Returns:
//      bool: true if the interface was queried successfully, false otherwise
// ******************************************************
bool queryUSBInterface(io_service_t service, CFUUIDRef plugin_type, CFUUIDRef interface_type, LPVOID *interface) {
	IOCFPlugInInterface **plugin_interface;
	bool ret = false;
	SInt32 score;

	if(IOCreatePlugInInterfaceForService(service, plugin_type, kIOCFPlugInInterfaceID, &plugin_interface, &score) == kIOReturnSuccess) {
		ret = (*plugin_interface)->QueryInterface(plugin_interface, CFUUIDGetUUIDBytes(interface_type), interface) == kIOReturnSuccess;
		IODestroyPlugInInterface(plugin_interface);
	}
	IOObjectRelease(service);
	return ret;
}

// ******************************************************
// Function: closeUSBDevice()
//
// Purpose: Close a USB device
//
// Parameters:
//      usb_handle_t *handle: the handle to close
// ******************************************************
void closeUSBDevice(usb_handle_t *handle) {
	CFRunLoopRemoveSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
	CFRelease(handle->async_event_source);
	(*handle->device)->USBDeviceClose(handle->device);
	(*handle->device)->Release(handle->device);
}

// ******************************************************
// Function: closeUSBHandle()
//
// Purpose: Close a USB handle
//
// Parameters:
//      usb_handle_t *handle: the handle to close
// ******************************************************
void closeUSBHandle(usb_handle_t *handle) {
	closeUSBDevice(handle);
}

// ******************************************************
// Function: openUSBDevice()
//
// Purpose: Open a USB device
//
// Parameters:
//      io_service_t service: the service to open
//      usb_handle_t *handle: the handle to return
//
// Returns:
//      bool: true if the device was opened successfully, false otherwise
// ******************************************************
bool openUSBDevice(io_service_t service, usb_handle_t *handle) {
	bool ret = false;

	if(queryUSBInterface(service, kIOUSBDeviceUserClientTypeID, kIOUSBDeviceInterfaceID320, (LPVOID *)&handle->device)) {
		if((*handle->device)->USBDeviceOpen(handle->device) == kIOReturnSuccess) {
			if((*handle->device)->SetConfiguration(handle->device, 1) == kIOReturnSuccess && (*handle->device)->CreateDeviceAsyncEventSource(handle->device, &handle->async_event_source) == kIOReturnSuccess) {
				CFRunLoopAddSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
				ret = true;
			} else {
				(*handle->device)->USBDeviceClose(handle->device);
			}
		}
		if(!ret) {
			(*handle->device)->Release(handle->device);
		}
	}
	return ret;
}

// ******************************************************
// Function: waitUSBHandle()
//
// Purpose: Wait for a USB handle to become available
//
// Parameters:
//      usb_handle_t *handle: the handle to wait for
//      usb_check_cb_t usb_check_cb: the callback to call when the handle is available
//      void *arg: the argument to pass to the callback
//
// Returns:
//      bool: true if the handle is available, false otherwise
// ******************************************************
bool waitUSBHandle(usb_handle_t *handle, usb_check_cb_t usb_check_cb, void *arg) {
	CFMutableDictionaryRef matching_dict;
	const char *darwin_device_class;
	io_iterator_t iter;
	io_service_t serv;
	bool ret = false;
	while((matching_dict = IOServiceMatching(kIOUSBDeviceClassName)) != NULL) {
		cfDictionarySetInt16(matching_dict, CFSTR(kUSBVendorID), handle->vid);
		cfDictionarySetInt16(matching_dict, CFSTR(kUSBProductID), handle->pid);
		if(IOServiceGetMatchingServices(0, matching_dict, &iter) == kIOReturnSuccess) {
			while((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
				if(openUSBDevice(serv, handle)) {
					if(usb_check_cb == NULL || usb_check_cb(handle, arg)) {
						ret = true;
						break;
					}
					closeUSBDevice(handle);
				}
			}
			IOObjectRelease(iter);
			if(ret) {
				break;
			}
			sleep_ms(USB_TIMEOUT);
		}
	}
	return ret;
}

// ******************************************************
// Function: resetUSBHandle()
//
// Purpose: Reset a USB handle
//
// Parameters:
//      usb_handle_t *handle: the handle to reset
// ******************************************************
void resetUSBHandle(usb_handle_t *handle) {
	(*handle->device)->ResetDevice(handle->device);
	(*handle->device)->USBDeviceReEnumerate(handle->device, 0);
}

// ******************************************************
// Function: USBAsyncCallback()
//
// Purpose: Callback for USB asynchronous transfers
//
// Parameters:
//      void *refcon: the reference context
//      IOReturn ret: the return code
//      void *arg: the argument
// ******************************************************
void USBAsyncCallback(void *refcon, IOReturn ret, void *arg) {
	transfer_ret_t *transfer_ret = refcon;

	if(transfer_ret != NULL) {
		memcpy(&transfer_ret->sz, &arg, sizeof(transfer_ret->sz));
		if(ret == kIOReturnSuccess) {
			transfer_ret->ret = USB_TRANSFER_OK;
		} else if(ret == kIOUSBPipeStalled) {
			transfer_ret->ret = USB_TRANSFER_STALL;
		} else {
			transfer_ret->ret = USB_TRANSFER_ERROR;
		}
	}
	CFRunLoopStop(CFRunLoopGetCurrent());
}

// ******************************************************
// Function: sendUSBControlRequest()
//
// Purpose: Send a USB control request
//
// Parameters:
//      const usb_handle_t *handle: the handle to use
//      uint8_t bmRequestType: the request type
//      uint8_t bRequest: the request
//      uint16_t wValue: the value
//      uint16_t wIndex: the index
//      void *pData: the data
//      size_t wLength: the length
//      transfer_ret_t *transferRet: the transfer return
//
// Returns:
//      bool: true if the request was sent successfully, false otherwise
// ******************************************************
bool sendUSBControlRequest(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet) {
	IOUSBDevRequestTO req;
	IOReturn ret;

	LOG(LOG_DEBUG, "sendUSBControlRequest: bmRequestType = 0x%02x, bRequest = 0x%02x, wValue = 0x%04x, wIndex = 0x%04x, wLength = %d, pData = %p", bmRequestType, bRequest, wValue, wIndex, wLength, pData);

	req.wLenDone = 0;
	req.pData = pData;
	req.bRequest = bRequest;
	req.bmRequestType = bmRequestType;
	req.wLength = OSSwapLittleToHostInt16(wLength);
	req.wValue = OSSwapLittleToHostInt16(wValue);
	req.wIndex = OSSwapLittleToHostInt16(wIndex);
	req.completionTimeout = req.noDataTimeout = USB_TIMEOUT;
	ret = (*handle->device)->DeviceRequestTO(handle->device, &req);
	if(transferRet != NULL) {
		if(ret == kIOReturnSuccess) {
			transferRet->sz = req.wLenDone;
			transferRet->ret = USB_TRANSFER_OK;
		} else if(ret == kIOUSBPipeStalled) {
			transferRet->ret = USB_TRANSFER_STALL;
		} else {
			transferRet->ret = USB_TRANSFER_ERROR;
		}
	}
	return true;
}

// ******************************************************
// Function: sendUSBControlRequestAsync()
//
// Purpose: Send a USB control request asynchronously
//
// Parameters:
//      const usb_handle_t *handle: the handle to use
//      uint8_t bmRequestType: the request type
//      uint8_t bRequest: the request
//      uint16_t wValue: the value
//      uint16_t wIndex: the index
//      void *pData: the data
//      size_t wLength: the length
//
// Returns:
//      bool: TODO
// ******************************************************
bool sendUSBControlRequestAsync(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet) {
	IOUSBDevRequestTO req;

	req.wLenDone = 0;
	req.pData = pData;
	req.bRequest = bRequest;
	req.bmRequestType = bmRequestType;
	req.wLength = OSSwapLittleToHostInt16(wLength);
	req.wValue = OSSwapLittleToHostInt16(wValue);
	req.wIndex = OSSwapLittleToHostInt16(wIndex);
	req.completionTimeout = req.noDataTimeout = USB_TIMEOUT;
	if((*handle->device)->DeviceRequestAsyncTO(handle->device, &req, USBAsyncCallback, transferRet) == kIOReturnSuccess) {
		sleep_ms(usbAbortTimeout);
		if((*handle->device)->USBDeviceAbortPipeZero(handle->device) == kIOReturnSuccess) {
			CFRunLoopRun();
			return true;
		}
	}
	return false;
}

// ******************************************************
// Function: initUSBHandle()
//
// Purpose: Initialize a USB handle
//
// Parameters:
//      usb_handle_t *handle: the handle to initialize
//      uint16_t vid: the vendor ID
//      uint16_t pid: the product ID
// ******************************************************
void initUSBHandle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
}

// ******************************************************
// Function: getCPIDFromSerialNumber()
//
// Purpose: Get the CPID from a serial number
//
// Parameters:
//      char *serial: the serial number
//
// Returns:
//      char *: the CPID
// ******************************************************
char *getCPIDFromSerialNumber(char *serial) {
	if (strstr(serial, "CPID:")) {
		char *cpid = strdup(strstr(serial, "CPID:") + 5);
		cpid[4] = '\0';
		return cpid;
	}
	return NULL;
}

uint16_t cpid;
size_t config_hole, ttbr0_vrom_off, ttbr0_sram_off, config_large_leak, config_overwrite_pad;
uint64_t tlbi, nop_gadget, ret_gadget, patch_addr, ttbr0_addr, func_gadget, write_ttbr0, memcpy_addr, aes_crypto_cmd, boot_tramp_end, gUSBSerialNumber, dfu_handle_request, usb_core_do_transfer, dfu_handle_bus_reset, insecure_memory_base, handle_interface_request, usb_create_string_descriptor, usb_serial_number_string_descriptor;

// ******************************************************
// Function: checkm8CheckUSBDevice()
//
// Purpose: Check if a USB device is vulnerable to checkm8 and update global variables accordingly
//
// Parameters:
//      usb_handle_t *handle: the handle to use
//      bool *pwned: whether the device is vulnerable
//
// Returns:
//      bool: true if the device is vulnerable, false otherwise
// ******************************************************
bool checkm8CheckUSBDevice(usb_handle_t *handle, bool *pwned) {
	char *usbSerialNumber = getDeviceSerialNumber(handle);
	bool ret = false;

	if(usbSerialNumber != NULL) {
		char *stringCPID = getCPIDFromSerialNumber(usbSerialNumber);
		if (stringCPID == NULL) {
			LOG(LOG_ERROR, "Failed to get CPID from serial number");
			return false;
		}
		int cpidNum = (int)strtol(stringCPID, NULL, 16);
		cpid = (uint16_t)cpidNum;

		if(strstr(usbSerialNumber, " SRTG:[iBoot-3135.0.0.2.3]") != NULL) {
			cpid = 0x8011;
			config_hole = 6;
			config_overwrite_pad = 0x540;
			tlbi = 0x100000444;
			nop_gadget = 0x10000CD0C;
			ret_gadget = 0x100000148;
			patch_addr = 0x100007630;
			ttbr0_addr = 0x1800A0000;
			func_gadget = 0x10000CCEC;
			write_ttbr0 = 0x1000003F4;
			memcpy_addr = 0x100010950;
			aes_crypto_cmd = 0x10000C994;
			boot_tramp_end = 0x1800B0000;
			ttbr0_vrom_off = 0x400;
			ttbr0_sram_off = 0x600;
			gUSBSerialNumber = 0x180083D28;
			dfu_handle_request = 0x180088A58;
			usb_core_do_transfer = 0x10000DD64;
			dfu_handle_bus_reset = 0x180088A88;
			insecure_memory_base = 0x1800B0000;
			handle_interface_request = 0x10000E08C;
			usb_create_string_descriptor = 0x10000D234;
			usb_serial_number_string_descriptor = 0x18008062A;
		} else {
            LOG(LOG_FATAL, "ERROR: AlfieLoader does not support CPID 0x%X at this time", cpid);
            return false;
        }
		
		if(cpid != 0) {
			*pwned = strstr(usbSerialNumber, "PWND") != NULL;
			ret = true;
		}
	}
	return ret;
}

// ******************************************************
// Function: sendUSBControlRequestNoData()
//
// Purpose: Send a USB control request with no data
//
// Parameters:
//      const usb_handle_t *handle: the handle to use
//      uint8_t bmRequestType: the request type
//      uint8_t bRequest: the request
//      uint16_t wValue: the value
//      uint16_t wIndex: the index
//      size_t wLength: the length
//      transfer_ret_t *transferRet: the transfer return
//
// Returns:
//      bool: true if the request was sent successfully, false otherwise
// ******************************************************
bool sendUSBControlRequestNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, transfer_ret_t *transferRet) {
	bool ret = false;
	void *pData;

	if(wLength == 0) {
		ret = sendUSBControlRequest(handle, bmRequestType, bRequest, wValue, wIndex, NULL, 0, transferRet);
	} else if((pData = malloc(wLength)) != NULL) {
		memset(pData, '\0', wLength);
		ret = sendUSBControlRequest(handle, bmRequestType, bRequest, wValue, wIndex, pData, wLength, transferRet);
		free(pData);
	}
	return ret;
}

// ******************************************************
// Function: sendUSBControlRequestAsyncNoData()
//
// Purpose: Send a USB control request with no data asynchronously
//
// Parameters:
//      const usb_handle_t *handle: the handle to use
//      uint8_t bmRequestType: the request type
//      uint8_t bRequest: the request
//      uint16_t wValue: the value
//      uint16_t wIndex: the index
//      size_t wLength: the length
//      unsigned USBAbortTimeout: the timeout
//      transfer_ret_t *transferRet: the transfer return
//
// Returns:
//      bool: true if the request was sent successfully, false otherwise
// ******************************************************
bool sendUSBControlRequestAsyncNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, unsigned USBAbortTimeout, transfer_ret_t *transferRet) {
	bool ret = false;
	void *pData;

	if(wLength == 0) {
		ret = sendUSBControlRequestAsync(handle, bmRequestType, bRequest, wValue, wIndex, NULL, 0, USBAbortTimeout, transferRet);
	} else if((pData = malloc(wLength)) != NULL) {
		memset(pData, '\0', wLength);
		ret = sendUSBControlRequestAsync(handle, bmRequestType, bRequest, wValue, wIndex, pData, wLength, USBAbortTimeout, transferRet);
		free(pData);
	}
	return ret;
}

// ******************************************************
// Function: resetUSBDevice()
//
// Purpose: Reset a USB device
//
// Parameters:
//      usb_handle_t *handle: the handle to use
// ******************************************************
void resetUSBDevice(usb_handle_t *handle)
{
    (*handle->device)->ResetDevice(handle->device);
    (*handle->device)->USBDeviceReEnumerate(handle->device, 0);
}

int createRequestType(transfer_direction direction, transfer_type type, transfer_recipient recipient) {
	// Returns bmRequestType using parameters for a control request
	return (direction << 7) | (type << 5) | recipient;
}

void reverseControlRequest(int bmRequestType) {
	transfer_direction direction = (bmRequestType & 0x80) >> 7;
	transfer_type type = (bmRequestType & 0x60) >> 5;
	transfer_recipient recipient = bmRequestType & 0x1F;
	// Log direction, type, and recipient of a control request
	LOG(LOG_DEBUG, "Direction: %s", direction == IN ? "IN" : "OUT");
	LOG(LOG_DEBUG, "Type: %s", type == STANDARD ? "Standard" : type == CLASS ? "Class" : "Vendor");
	LOG(LOG_DEBUG, "Recipient: %s", recipient == DEVICE ? "Device" : recipient == INTERFACE ? "Interface" : recipient == ENDPOINT ? "Endpoint" : "Other");
}