#include <usb/usb.h>

char *getDeviceSerialNumberIOKit(usb_handle_t *handle)
{
    CFTypeRef serialNumber = IORegistryEntryCreateCFProperty(handle->service, CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
    if (serialNumber == NULL)
    {
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

char *getDeviceSerialNumberWithTransfer(usb_handle_t *handle) {
	transfer_ret_t transfer_ret;
	uint8_t buf[UINT8_MAX];
	char *str = NULL;
	size_t i, sz;
	if(sendUSBControlRequest(handle, 0x80, 6, 1U << 8U, 0, &device_descriptor, sizeof(device_descriptor), &transfer_ret)
	&& transfer_ret.ret == USB_TRANSFER_OK && transfer_ret.sz == sizeof(device_descriptor)
	&& sendUSBControlRequest(handle, 0x80, 6, (3U << 8U) | device_descriptor.i_serial_number, 0x409, buf, sizeof(buf), &transfer_ret)
	&& transfer_ret.ret == USB_TRANSFER_OK && transfer_ret.sz == buf[0] && (sz = buf[0] / 2) != 0 && (str = malloc(sz)) != NULL) {
		for(i = 0; i < sz; ++i) {
			str[i] = (char)buf[2 * (i + 1)];
		}
		str[sz - 1] = '\0';
	}
	return str;
}

void sleep_ms(unsigned ms) {
    struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

void cfDictionarySetInt16(CFMutableDictionaryRef dict, const void *key, uint16_t val) {
	CFNumberRef cf_val = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &val);

	if(cf_val != NULL) {
		CFDictionarySetValue(dict, key, cf_val);
		CFRelease(cf_val);
	}
}

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

void closeUSBDevice(usb_handle_t *handle) {
	CFRunLoopRemoveSource(CFRunLoopGetCurrent(), handle->async_event_source, kCFRunLoopDefaultMode);
	CFRelease(handle->async_event_source);
	(*handle->device)->USBDeviceClose(handle->device);
	(*handle->device)->Release(handle->device);
}

void closeUSBHandle(usb_handle_t *handle) {
	closeUSBDevice(handle);
}

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

bool openUSBInterface(uint8_t usb_interface, uint8_t usb_alt_interface, usb_handle_t *handle) {
	IOUSBFindInterfaceRequest interface_request;
	io_iterator_t iter;
	io_service_t serv;
	bool ret = false;
	size_t i;

	interface_request.bInterfaceProtocol = interface_request.bInterfaceSubClass = interface_request.bAlternateSetting = interface_request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	if((*handle->device)->CreateInterfaceIterator(handle->device, &interface_request, &iter) == kIOReturnSuccess) {
		serv = IO_OBJECT_NULL;
		for(i = 0; i <= usb_interface; ++i) {
			if((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
				if(i == usb_interface) {
					break;
				}
				IOObjectRelease(serv);
			}
		}
		IOObjectRelease(iter);
		if(serv != IO_OBJECT_NULL) {
			if(queryUSBInterface(serv, kIOUSBInterfaceUserClientTypeID, kIOUSBInterfaceInterfaceID300, (LPVOID *)&handle->interface)) {
				if((*handle->interface)->USBInterfaceOpenSeize(handle->interface) == kIOReturnSuccess) {
					if(usb_alt_interface != 1 || (*handle->interface)->SetAlternateInterface(handle->interface, usb_alt_interface) == kIOReturnSuccess) {
						ret = true;
					} else {
						(*handle->interface)->USBInterfaceClose(handle->interface);
					}
				}
				if(!ret) {
					(*handle->interface)->Release(handle->interface);
				}
			}
			IOObjectRelease(serv);
		}
	}
	return ret;
}

bool waitUSBHandle(usb_handle_t *handle, uint8_t usb_interface, uint8_t usb_alt_interface, usb_check_cb_t usb_check_cb, void *arg) {
	CFMutableDictionaryRef matching_dict;
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

void resetUSBHandle(usb_handle_t *handle) {
	(*handle->device)->ResetDevice(handle->device);
	(*handle->device)->USBDeviceReEnumerate(handle->device, 0);
}

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

bool sendUSBControlRequest(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet) {
	IOUSBDevRequestTO req;
	IOReturn ret;

	// LOG(LOG_DEBUG, "bmRequestType = 0x%02x, bRequest = 0x%02x, wValue = 0x%04x, wIndex = 0x%04x, wLength = %d, pData = %p", bmRequestType, bRequest, wValue, wIndex, wLength, pData);

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

bool sendUSBControlRequestAsync(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet) {
	IOUSBDevRequestTO req;

	// LOG(LOG_DEBUG, "bmRequestType = 0x%02x, bRequest = 0x%02x, wValue = 0x%04x, wIndex = 0x%04x, wLength = %d, pData = %p", bmRequestType, bRequest, wValue, wIndex, wLength, pData);

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

int sendUSBBulkUpload(usb_handle_t *handle, void *buffer, size_t length) {
	openUSBInterface(0, 0, handle);
	return (*handle->interface)->WritePipe((handle->interface), 2, buffer, length);
}

void initUSBHandle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
}

char *getCPIDFromSerialNumber(char *serial) {
	if (strstr(serial, "CPID:")) {
		char *cpid = strdup(strstr(serial, "CPID:") + 5);
		cpid[4] = '\0';
		return cpid;
	}
	return NULL;
}

bool foundYoloDevice, foundPongoDevice;
uint16_t cpid;
size_t config_hole, ttbr0_vrom_off, ttbr0_sram_off, config_large_leak, config_overwrite_pad;
uint64_t tlbi, nop_gadget, ret_gadget, patch_addr, ttbr0_addr, func_gadget, write_ttbr0, memcpy_addr, aes_crypto_cmd, boot_tramp_end, gUSBSerialNumber, dfu_handle_request, usb_core_do_transfer, dfu_handle_bus_reset, insecure_memory_base, handle_interface_request, usb_create_string_descriptor, usb_serial_number_string_descriptor;

bool checkm8CheckUSBDevice(usb_handle_t *handle, bool *pwned) {
	char *usbSerialNumber = getDeviceSerialNumberWithTransfer(handle);
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
		} else if (strstr(usbSerialNumber, "YOLO:checkra1n") != NULL) {
			foundYoloDevice = true;
		} else {
            LOG(LOG_FATAL, "ERROR: AlfieLoader does not support CPID 0x%X at this time", cpid);
            return false;
        }
		
		if(usbSerialNumber != 0) {
			extern bool bootingPongoOS;
			*pwned = strstr(usbSerialNumber, "PWND");
			ret = true;
		}
	} else {
		LOG(LOG_ERROR, "Failed to get serial number");
	}
	return ret;
}

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