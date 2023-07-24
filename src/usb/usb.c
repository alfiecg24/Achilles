#include <usb/usb.h>

char *getDeviceSerialNumber(usb_handle_t *handle) {
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

#ifdef ALFIELOADER_LIBUSB

void closeUSBHandle(usb_handle_t *handle) {
	libusb_release_interface(handle->device, 0);
	libusb_close(handle->device);
	libusb_exit(handle->context);
}

void resetUSBHandle(usb_handle_t *handle) {
	libusb_reset_device(handle->device);
}

bool waitUSBHandle(usb_handle_t *handle, usb_check_cb_t usb_check_cb, void *arg) {
	if (libusb_init(NULL) == LIBUSB_SUCCESS) {
		for (;;) {
			if ((handle->device = libusb_open_device_with_vid_pid(NULL, handle->vid, handle->pid)) != NULL) {
				LOG(LOG_DEBUG, "Opened device 0x%X, 0x%X", handle->vid, handle->pid);
				if (libusb_set_configuration(handle->device, 1) == LIBUSB_SUCCESS && (usb_check_cb == NULL || usb_check_cb(handle, arg))) {
					return true;
				}
				libusb_close(handle->device);
			}
			sleep_ms(USB_TIMEOUT);
		}
	}
	return false;
}

void USBAsyncCallback(struct libusb_transfer *transfer) {
	*(int *)transfer->user_data = 1;
}

bool sendUSBControlRequest(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet) {
	int ret = libusb_control_transfer(handle->device, bmRequestType, bRequest, wValue, wIndex, pData, (uint16_t)wLength, USB_TIMEOUT);

	if(transferRet != NULL) {
		if(ret >= 0) {
			transferRet->sz = (uint32_t)ret;
			transferRet->ret = USB_TRANSFER_OK;
		} else if(ret == LIBUSB_ERROR_PIPE) {
			transferRet->ret = USB_TRANSFER_STALL;
		} else {
			transferRet->ret = USB_TRANSFER_ERROR;
		}
	}
	return true;
}

bool sendUSBControlRequestAsync(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet) {
	struct libusb_transfer *transfer = libusb_alloc_transfer(0);
	struct timeval tv;
	int completed = 0;
	uint8_t *buf;

	if(transfer != NULL) {
		if((buf = malloc(LIBUSB_CONTROL_SETUP_SIZE + wLength)) != NULL) {
			if((bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
				memcpy(buf + LIBUSB_CONTROL_SETUP_SIZE, pData, wLength);
			}
			libusb_fill_control_setup(buf, bmRequestType, bRequest, wValue, wIndex, (uint16_t)wLength);
			libusb_fill_control_transfer(transfer, handle->device, buf, USBAsyncCallback, &completed, USB_TIMEOUT);
			if(libusb_submit_transfer(transfer) == LIBUSB_SUCCESS) {
				tv.tv_sec = usbAbortTimeout / 1000;
				tv.tv_usec = (usbAbortTimeout % 1000) * 1000;
				while(completed == 0 && libusb_handle_events_timeout_completed(NULL, &tv, &completed) == LIBUSB_SUCCESS) {
					libusb_cancel_transfer(transfer);
				}
				if(completed != 0) {
					if((bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
						memcpy(pData, libusb_control_transfer_get_data(transfer), transfer->actual_length);
					}
					if(transferRet != NULL) {
						transferRet->sz = (uint32_t)transfer->actual_length;
						if(transfer->status == LIBUSB_TRANSFER_COMPLETED) {
							transferRet->ret = USB_TRANSFER_OK;
						} else if(transfer->status == LIBUSB_TRANSFER_STALL) {
							transferRet->ret = USB_TRANSFER_STALL;
						} else {
							transferRet->ret = USB_TRANSFER_ERROR;
						}
					}
				}
			}
			free(buf);
		}
		libusb_free_transfer(transfer);
	}
	return completed != 0;
}

void initUSBHandle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
	handle->context = NULL;
}

int sendUSBBulkUpload(usb_handle_t *handle, void *buffer, size_t length) {
	int transferred;
	int interfaceRet = libusb_claim_interface(handle->device, 0);
	if (interfaceRet != LIBUSB_SUCCESS) {
		LOG(LOG_ERROR, "Failed to claim interface");
		return -1;
	}
	int ret = libusb_bulk_transfer(handle->device, 0x2, buffer, length, &transferred, 100);
	if (ret == LIBUSB_ERROR_PIPE) {
		LOG(LOG_ERROR, "USB pipe error sending bulk upload");
	} else if (ret == LIBUSB_ERROR_TIMEOUT) {
		LOG(LOG_ERROR, "USB timeout sending bulk upload");
	}
	libusb_release_interface(handle->device, 0);
	return transferred;
}

#else

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

bool waitUSBHandle(usb_handle_t *handle, usb_check_cb_t usb_check_cb, void *arg) {
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

#endif


char *getCPIDFromSerialNumber(char *serial) {
	if (strstr(serial, "CPID:") != NULL) {
		char *cpid = strdup(strstr(serial, "CPID:") + 5);
		cpid[4] = '\0';
		return cpid;
	}
	return NULL;
}

char *getBDIDFromSerialNumer(char *serial) {
	if (strstr(serial, "BDID:") != NULL) {
		char *bdid = strdup(strstr(serial, "BDID:") + 3);
		bdid[4] = '\0';
		return bdid;
	}
	return NULL;
}

uint16_t cpid, bdid;
size_t config_hole, ttbr0_vrom_off, ttbr0_sram_off, config_large_leak, config_overwrite_pad;
uint64_t tlbi, nop_gadget, ret_gadget, patch_addr, ttbr0_addr, func_gadget, write_ttbr0, memcpy_addr, aes_crypto_cmd, boot_tramp_end, gUSBSerialNumber, dfu_handle_request, usb_core_do_transfer, dfu_handle_bus_reset, insecure_memory_base, handle_interface_request, usb_create_string_descriptor, usb_serial_number_string_descriptor;

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
		char *stringBDID = getBDIDFromSerialNumer(usbSerialNumber);
		if (stringBDID == NULL) {
			LOG(LOG_ERROR, "Failed to get BDID from serial number");
			return false;
		}
		int bdidNum = (int)strtol(stringBDID, NULL, 16);
		bdid = (uint16_t)bdidNum;
		if (strstr(usbSerialNumber, " SRTG:[iBoot-1704.10]") != NULL) { // A7
			cpid = 0x8960;
			config_large_leak = 7936;
			config_overwrite_pad = 0x5C0;
			patch_addr = 0x100005CE0;
			memcpy_addr = 0x10000ED50;
			aes_crypto_cmd = 0x10000B9A8;
			boot_tramp_end = 0x1800E1000;
			gUSBSerialNumber = 0x180086CDC;
			dfu_handle_request = 0x180086C70;
			usb_core_do_transfer = 0x10000CC78;
			dfu_handle_bus_reset = 0x180086CA0;
			insecure_memory_base = 0x180380000;
			handle_interface_request = 0x10000CFB4;
			usb_create_string_descriptor = 0x10000BFEC;
			usb_serial_number_string_descriptor = 0x180080562;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-1992.0.0.1.19]") != NULL) { // A8
			cpid = 0x7000;
			config_overwrite_pad = 0x500;
			patch_addr = 0x100007E98;
			memcpy_addr = 0x100010E70;
			aes_crypto_cmd = 0x10000DA90;
			boot_tramp_end = 0x1800E1000;
			gUSBSerialNumber = 0x1800888C8;
			dfu_handle_request = 0x180088878;
			usb_core_do_transfer = 0x10000EBB4;
			dfu_handle_bus_reset = 0x180088898;
			insecure_memory_base = 0x180380000;
			handle_interface_request = 0x10000EEE4;
			usb_create_string_descriptor = 0x10000E074;
			usb_serial_number_string_descriptor = 0x18008062A;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-1991.0.0.2.16]") != NULL) { // A8X
			cpid = 0x7001;
			config_overwrite_pad = 0x500;
			patch_addr = 0x10000AD04;
			memcpy_addr = 0x100013F10;
			aes_crypto_cmd = 0x100010A90;
			boot_tramp_end = 0x1800E1000;
			gUSBSerialNumber = 0x180088E48;
			dfu_handle_request = 0x180088DF8;
			usb_core_do_transfer = 0x100011BB4;
			dfu_handle_bus_reset = 0x180088E18;
			insecure_memory_base = 0x180380000;
			handle_interface_request = 0x100011EE4;
			usb_create_string_descriptor = 0x100011074;
			usb_serial_number_string_descriptor = 0x180080C2A;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-2234.0.0.2.22]") != NULL
				|| strstr(usbSerialNumber, " SRTG:[iBoot-2234.0.0.3.3]") != NULL) { // A9
			cpid = strstr(usbSerialNumber, "2.22") != NULL ? 0x8003 : 0x8000;
			config_overwrite_pad = 0x500;
			patch_addr = 0x10000812C;
			ttbr0_addr = 0x1800C8000;
			memcpy_addr = 0x100011030;
			aes_crypto_cmd = 0x10000DAA0;
			ttbr0_vrom_off = 0x400;
			boot_tramp_end = 0x1800E1000;
			gUSBSerialNumber = 0x180087958;
			dfu_handle_request = 0x1800878F8;
			usb_core_do_transfer = 0x10000EE78;
			dfu_handle_bus_reset = 0x180087928;
			insecure_memory_base = 0x180380000;
			handle_interface_request = 0x10000F1B0;
			usb_create_string_descriptor = 0x10000E354;
			usb_serial_number_string_descriptor = 0x1800807DA;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-2481.0.0.2.1]") != NULL) { // A9X
			cpid = 0x8001;
			config_hole = 6;
			config_overwrite_pad = 0x5C0;
			tlbi = 0x100000404;
			nop_gadget = 0x10000CD60;
			ret_gadget = 0x100000118;
			patch_addr = 0x100007668;
			ttbr0_addr = 0x180050000;
			func_gadget = 0x10000CD40;
			write_ttbr0 = 0x1000003B4;
			memcpy_addr = 0x1000106F0;
			aes_crypto_cmd = 0x10000C9D4;
			boot_tramp_end = 0x180044000;
			ttbr0_vrom_off = 0x400;
			ttbr0_sram_off = 0x600;
			gUSBSerialNumber = 0x180047578;
			dfu_handle_request = 0x18004C378;
			usb_core_do_transfer = 0x10000DDA4;
			dfu_handle_bus_reset = 0x18004C3A8;
			insecure_memory_base = 0x180000000;
			handle_interface_request = 0x10000E0B4;
			usb_create_string_descriptor = 0x10000D280;
			usb_serial_number_string_descriptor = 0x18004486A;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-2696.0.0.1.33]") != NULL) { // A10
			cpid = 0x8010;
			config_hole = 5;
			config_overwrite_pad = 0x5C0;
			tlbi = 0x100000434;
			nop_gadget = 0x10000CC6C;
			ret_gadget = 0x10000015C;
			patch_addr = 0x1000074AC;
			ttbr0_addr = 0x1800A0000;
			func_gadget = 0x10000CC4C;
			write_ttbr0 = 0x1000003E4;
			memcpy_addr = 0x100010730;
			aes_crypto_cmd = 0x10000C8F4;
			boot_tramp_end = 0x1800B0000;
			ttbr0_vrom_off = 0x400;
			ttbr0_sram_off = 0x600;
			gUSBSerialNumber = 0x180083CF8;
			dfu_handle_request = 0x180088B48;
			usb_core_do_transfer = 0x10000DC98;
			dfu_handle_bus_reset = 0x180088B78;
			insecure_memory_base = 0x1800B0000;
			handle_interface_request = 0x10000DFB8;
			usb_create_string_descriptor = 0x10000D150;
			usb_serial_number_string_descriptor = 0x1800805DA;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-3135.0.0.2.3]") != NULL) { // A10X
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
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-3332.0.0.1.23]") != NULL) { // A11
			cpid = 0x8015;
			config_hole = 6;
			config_overwrite_pad = 0x540;
			tlbi = 0x1000004AC;
			nop_gadget = 0x10000A9C4;
			ret_gadget = 0x100000148;
			patch_addr = 0x10000624C;
			ttbr0_addr = 0x18000C000;
			func_gadget = 0x10000A9AC;
			write_ttbr0 = 0x10000045C;
			memcpy_addr = 0x10000E9D0;
			aes_crypto_cmd = 0x100009E9C;
			boot_tramp_end = 0x18001C000;
			ttbr0_vrom_off = 0x400;
			ttbr0_sram_off = 0x600;
			gUSBSerialNumber = 0x180003A78;
			dfu_handle_request = 0x180008638;
			usb_core_do_transfer = 0x10000B9A8;
			dfu_handle_bus_reset = 0x180008668;
			insecure_memory_base = 0x18001C000;
			handle_interface_request = 0x10000BCCC;
			usb_create_string_descriptor = 0x10000AE80;
			usb_serial_number_string_descriptor = 0x1800008FA;
		} else if (strstr(usbSerialNumber, " SRTG:[iBoot-3401.0.0.1.16]") != NULL) { // T2
			cpid = 0x8012;
			config_hole = 6;
			config_overwrite_pad = 0x540;
			tlbi = 0x100000494;
			nop_gadget = 0x100008DB8;
			ret_gadget = 0x10000012C;
			patch_addr = 0x100004854;
			ttbr0_addr = 0x18000C000;
			func_gadget = 0x100008DA0;
			write_ttbr0 = 0x100000444;
			memcpy_addr = 0x10000EA30;
			aes_crypto_cmd = 0x1000082AC;
			boot_tramp_end = 0x18001C000;
			ttbr0_vrom_off = 0x400;
			ttbr0_sram_off = 0x600;
			gUSBSerialNumber = 0x180003AF8;
			dfu_handle_request = 0x180008B08;
			usb_core_do_transfer = 0x10000BD20;
			dfu_handle_bus_reset = 0x180008B38;
			insecure_memory_base = 0x18001C000;
			handle_interface_request = 0x10000BFFC;
			usb_create_string_descriptor = 0x10000B1CC;
			usb_serial_number_string_descriptor = 0x18000082A;

		// These two are only implemented to stop the unsupported message
		} else if (strstr(usbSerialNumber, "YOLO:") != NULL) {
			// YoloDFU
		} else if ((handle->vid == 0x5ac && handle->pid == 0x4141) || (strstr(usbSerialNumber, "SRTG:[PongoOS") != NULL)) {
			// Pongo USB Device
		} 
		
		else {
            LOG(LOG_ERROR, "AlfieLoader does not support CPID 0x%X at this time", cpid);
            return false;
        }
		
		if(usbSerialNumber != 0) {
			extern bool bootingPongoOS;
			extern char *pwndString;
			*pwned = strstr(usbSerialNumber, "PWND") || strstr(usbSerialNumber, pwndString);
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