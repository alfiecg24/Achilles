#ifndef USB_H
#define USB_H

#include <fcntl.h>

#include <Achilles.h>
#include <utils/log.h>
#include <libusb-1.0/libusb.h>

#define USB_TIMEOUT 10

typedef struct {
	uint16_t vid, pid;
	struct libusb_device_handle *device;
	int usb_interface;
	struct libusb_context *context;
} usb_handle_t;

enum usb_transfer_ret {
	USB_TRANSFER_OK,
	USB_TRANSFER_ERROR,
	USB_TRANSFER_STALL
};

typedef struct {
	enum usb_transfer_ret ret;
	uint32_t sz;
} transfer_ret_t;

void sleep_ms(unsigned ms);

bool send_usb_control_request(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet);
bool send_usb_control_request_no_timeout(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet);
bool send_usb_control_request_no_data(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, transfer_ret_t *transferRet);
bool send_usb_control_request_async(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet);
bool send_usb_control_request_async_no_data(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, unsigned usbAbortTimeout, transfer_ret_t *transferRet);
bool send_usb_bulk_upload(usb_handle_t *handle, void *buffer, size_t length);

void init_usb_handle(usb_handle_t *handle, uint16_t vid, uint16_t pid);
bool wait_usb_handle(usb_handle_t *handle);
bool wait_usb_handle_with_timeout(usb_handle_t *handle, unsigned timeout);
void reset_usb_handle(usb_handle_t *handle);
void close_usb_handle(usb_handle_t *handle);

char *get_usb_device_serial_number(usb_handle_t *handle);

#endif // USB_H