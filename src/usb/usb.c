#include <usb/usb.h>

void sleep_ms(unsigned ms) {
    struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

void close_usb_handle(usb_handle_t *handle) {
	libusb_close(handle->device);
	libusb_exit(NULL);
}

void reset_usb_handle(usb_handle_t *handle) {
	libusb_reset_device(handle->device);
}

extern bool stopThreads;

bool wait_usb_handle(usb_handle_t *handle) {
	if (stopThreads) { 
		LOG(LOG_ERROR, "Thread stopped.");
		return false;
	}
	if(libusb_init(NULL) == LIBUSB_SUCCESS) {
		for(;;) {
			if((handle->device = libusb_open_device_with_vid_pid(NULL, handle->vid, handle->pid)) != NULL) {
				if(libusb_set_configuration(handle->device, 1) == LIBUSB_SUCCESS) {
					return true;
				}
				libusb_close(handle->device);
			}
			sleep_ms(USB_TIMEOUT);
		}
	}
	return false;
}

bool wait_usb_handle_with_timeout(usb_handle_t *handle, unsigned timeout) {
	unsigned totalTime = 0;
	if(libusb_init(NULL) == LIBUSB_SUCCESS) {
		for(;;) {
			if((handle->device = libusb_open_device_with_vid_pid(NULL, handle->vid, handle->pid)) != NULL) {
				if(libusb_set_configuration(handle->device, 1) == LIBUSB_SUCCESS) {
					return true;
				}
				libusb_close(handle->device);
			}
			sleep_ms(USB_TIMEOUT);
			totalTime += USB_TIMEOUT;
			if (totalTime >= timeout) {
				return false;
			}
		}
	}
	return false;
}

void usb_async_callback(struct libusb_transfer *transfer) {
	*(int *)transfer->user_data = 1;
}

bool send_usb_control_request_no_timeout(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet) {
	int ret = libusb_control_transfer(handle->device, bmRequestType, bRequest, wValue, wIndex, pData, (uint16_t)wLength, 0);

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

bool send_usb_control_request(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet) {
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

bool send_usb_control_request_async(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet) {
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
			libusb_fill_control_transfer(transfer, handle->device, buf, usb_async_callback, &completed, USB_TIMEOUT);
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

void init_usb_handle(usb_handle_t *handle, uint16_t vid, uint16_t pid) {
	handle->vid = vid;
	handle->pid = pid;
	handle->device = NULL;
}

bool send_usb_bulk_upload(usb_handle_t *handle, void *buffer, size_t length) {
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
	return transferred == length;
}


bool send_usb_control_request_no_data(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, transfer_ret_t *transferRet) {
	bool ret = false;
	void *pData;

	if (wLength == 0) {
		ret = send_usb_control_request(handle, bmRequestType, bRequest, wValue, wIndex, NULL, 0, transferRet);
	} else if ((pData = malloc(wLength)) != NULL) {
		memset(pData, '\0', wLength);
		ret = send_usb_control_request(handle, bmRequestType, bRequest, wValue, wIndex, pData, wLength, transferRet);
		free(pData);
	}
	return ret;
}

bool send_usb_control_request_async_no_data(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet) {
    bool ret = false;
    void *pData;

    if (wLength == 0) {
        ret = send_usb_control_request_async(handle, bmRequestType, bRequest, wValue, wIndex, NULL, 0, usbAbortTimeout, transferRet);
    } else if ((pData = malloc(wLength)) != NULL) {
        memset(pData, '\0', wLength);
        ret = send_usb_control_request_async(handle, bmRequestType, bRequest, wValue, wIndex, pData, wLength, usbAbortTimeout, transferRet);
        free(pData);
    }
    return ret;
}

static struct {
	uint8_t b_len, b_descriptor_type;
	uint16_t bcd_usb;
	uint8_t b_device_class, b_device_sub_class, b_device_protocol, b_max_packet_sz;
	uint16_t id_vendor, id_product, bcd_device;
	uint8_t i_manufacturer, i_product, i_serial_number, b_num_configurations;
} usb_device_descriptor;

char *get_usb_device_serial_number(usb_handle_t *handle) {
	transfer_ret_t transfer_ret;
	uint8_t buf[UINT8_MAX];
	char *str = NULL;
	size_t i, sz;
	if (send_usb_control_request(handle, 0x80, 6, 1U << 8U, 0, &usb_device_descriptor, sizeof(usb_device_descriptor), &transfer_ret)
	&& transfer_ret.ret == USB_TRANSFER_OK && transfer_ret.sz == sizeof(usb_device_descriptor)
	&& send_usb_control_request(handle, 0x80, 6, (3U << 8U) | usb_device_descriptor.i_serial_number, 0x409, buf, sizeof(buf), &transfer_ret)
	&& transfer_ret.ret == USB_TRANSFER_OK && transfer_ret.sz == buf[0] && (sz = buf[0] / 2) != 0 && (str = malloc(sz)) != NULL) {
		for (i = 0; i < sz; ++i) {
			str[i] = (char)buf[2 * (i + 1)];
		}
		str[sz - 1] = '\0';
	}
	return str;
}

