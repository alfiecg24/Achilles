#ifndef USB_UTILS_H
#define USB_UTILS_H

#include <AlfieLoader.h>
#include <utils/log.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonCrypto.h>

#define USB_TIMEOUT 5

typedef struct {
	uint16_t vid, pid;
	io_service_t service;
	IOUSBDeviceInterface320 **device;
	CFRunLoopSourceRef async_event_source;
} usb_handle_t;

enum usb_transfer {
	USB_TRANSFER_OK,
	USB_TRANSFER_ERROR,
	USB_TRANSFER_STALL
};

typedef struct {
	enum usb_transfer ret;
	uint32_t sz;
} transfer_ret_t;

typedef bool (*usb_check_cb_t)(usb_handle_t *, bool *);

char *getDeviceSerialNumber(usb_handle_t *handle);

bool checkm8CheckUSBDevice(usb_handle_t *handle, bool *pwned);

bool sendUSBControlRequest(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet);

bool sendUSBControlRequestNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, transfer_ret_t *transferRet);
bool sendUSBControlRequestAsync(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet);
bool sendUSBControlRequestAsyncNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, unsigned USBAbortTimeout, transfer_ret_t *transferRet);

void closeUSBDevice(usb_handle_t *handle);
bool waitUSBHandle(usb_handle_t *handle, usb_check_cb_t usb_check_cb, void *arg);
void resetUSBDevice(usb_handle_t *handle);
#endif // USB_UTILS_H