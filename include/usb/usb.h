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
	IOUSBInterfaceInterface300 **interface;
	pthread_t thread;
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

static struct {
	uint8_t b_len, b_descriptor_type;
	uint16_t bcd_usb;
	uint8_t b_device_class, b_device_sub_class, b_device_protocol, b_max_packet_sz;
	uint16_t id_vendor, id_product, bcd_device;
	uint8_t i_manufacturer, i_product, i_serial_number, b_num_configurations;
} device_descriptor;

enum transfer_direction {
	OUT = 0,
	IN = 1
};

enum transfer_type {
	STANDARD = 0,
	CLASS = 1,
	VENDOR = 2,
	RESERVED = 3
};

enum transfer_recipient {
	DEVICE = 0,
	INTERFACE = 1,
	ENDPOINT = 2,
	OTHER = 3
};

typedef enum transfer_direction transfer_direction;
typedef enum transfer_type transfer_type;
typedef enum transfer_recipient transfer_recipient;


typedef bool (*usb_check_cb_t)(usb_handle_t *, bool *);

// ******************************************************
// Function: getDeviceSerialNumberIOKit()
//
// Purpose: Get the serial number of the device from the USB handle using IOKit
//
// Parameters:
//      usb_handle_t *handle: the USB handle
//
// Returns:
//      char *: the serial number of the device
// ******************************************************
char *getDeviceSerialNumberIOKit(usb_handle_t *handle);

// ******************************************************
// Function: getDeviceSerialNumberTransfer()
//
// Purpose: Get the serial number of the device directly from the USB handle using USB control requests
//
// Parameters:
//      usb_handle_t *handle: the USB handle
//
// Returns:
//      char *: the serial number of the device
// ******************************************************
char *getDeviceSerialNumberWithTransfer(usb_handle_t *handle);

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
bool checkm8CheckUSBDevice(usb_handle_t *handle, bool *pwned);

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
bool sendUSBControlRequest(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, void *pData, size_t wLength, transfer_ret_t *transferRet);

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
bool sendUSBControlRequestNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, transfer_ret_t *transferRet);

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
bool sendUSBControlRequestAsyncNoData(const usb_handle_t *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, size_t wLength, unsigned USBAbortTimeout, transfer_ret_t *transferRet);

// ******************************************************
// Function: sendUSBBulkUpload()
//
// Purpose: Send a USB bulk upload
//
// Parameters:
//      usb_handle_t *handle: the handle to use
//      void *buffer: the buffer to send
//      size_t length: the length of the buffer
//
// Returns:
//      int: the IOReturn code
// ******************************************************
int sendUSBBulkUpload(usb_handle_t *handle, void *buffer, size_t length);

// ******************************************************
// Function: closeUSBHandle()
//
// Purpose: Close a USB handle
//
// Parameters:
//      usb_handle_t *handle: the handle to close
// ******************************************************
void closeUSBHandle(usb_handle_t *handle);

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
void initUSBHandle(usb_handle_t *handle, uint16_t vid, uint16_t pid);

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
bool waitUSBHandle(usb_handle_t *handle, uint8_t usb_interface, uint8_t usb_alt_interface, usb_check_cb_t usb_check_cb, void *arg);

// ******************************************************
// Function: resetUSBHandle()
//
// Purpose: Reset a USB handle
//
// Parameters:
//      usb_handle_t *handle: the handle to reset
// ******************************************************
void resetUSBHandle(usb_handle_t *handle);

// ******************************************************
// Function: sleep_ms()
//
// Purpose: Sleep for a given number of milliseconds
//
// Parameters:
//      unsigned ms: the number of milliseconds to sleep
// ******************************************************
void sleep_ms(unsigned ms);

#endif // USB_UTILS_H