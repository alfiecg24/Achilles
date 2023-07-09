# checkm8 write-up

## Table of contents
- [checkm8 write-up](#checkm8-write-up)
  - [Table of contents](#table-of-contents)
- [Analysis](#analysis)
  - [Introduction](#introduction)
    - [Resources](#resources)
    - [Disclaimer](#disclaimer)
  - [USB initialisation](#usb-initialisation)
  - [Handling of USB transfers](#handling-of-usb-transfers)
    - [Initial request handling](#initial-request-handling)
    - [Data phase](#data-phase)
  - [Use-after-free](#use-after-free)
    - [Lifecycle of image transfer](#lifecycle-of-image-transfer)
    - [USB stack shutdown](#usb-stack-shutdown)
  - [Memory leak](#memory-leak)
    - [Why is a leak needed?](#why-is-a-leak-needed)
    - [USB request structure](#usb-request-structure)
    - [The bug](#the-bug)
- [Exploitation](#exploitation)
  - [Heap feng shui](#heap-feng-shui)
  - [Triggering the use-after-free](#triggering-the-use-after-free)
  - [The payload](#the-payload)
    - [](#)
  - [Executing the payload](#executing-the-payload)
  - [Putting the plan into action](#putting-the-plan-into-action)
  - [Testing](#testing)
  - [Troubleshooting](#troubleshooting)
- [Conclusion](#conclusion)

# Analysis
## Introduction
This is my analysis and writeup of the vulnerabilities exploited in the checkm8 BootROM exploit. I wrote this in order to help me gain a better understanding of the vulnerability so that I could design my own strategy for exploitation and write my own implementation of the exploit. The checkm8 exploit relies on a couple of vulnerabilities:
* The main use-after-free (not patched until A14)
* The memory leak (patched in A12)

The memory leak is essential in order to exploit the use-after-free, and I will be going into further detail later on in this writeup. However, it is this leak that is the reason the use-after-free cannot be exploited on A12 and A13 SoCs.

### Resources
Before we start, there are some important resources that I used to help me understand the exploit:
* [This technical analysis of checkm8](https://habr.com/en/companies/dsec/articles/472762/) by [a1exdandy](https://twitter.com/a1exdandy)
* [This presentation about checkra1n's implementation](https://papers.put.as/papers/ios/2019/LucaPOC.pdf) by [Luca Todesco](https://twitter.com/qwertyoruiopz)
* [This vulnerability writeup](https://gist.github.com/littlelailo/42c6a11d31877f98531f6d30444f59c4) by [littlelailo](https://twitter.com/littlelailo)
* [This checkm8 "Q&A"](https://medium.com/@deepaknx/a-inquisitive-q-a-on-checkm8-bootrom-exploit-82da0d6f6c)
* [ipwndfu](https://github/Axi0mX/ipwndfu) by [axi0mX](https://twitter.com/axi0mX)
* [gaster](https://github.com/0x7FF/gaster) by [0x7FF](https://github.com/0x7FF)
* The leaked iBoot/BootROM source codes - not linked here for obvious reasons
* [securerom.fun](https://securerom.fun) for their collection of BootROM dumps that I reverse engineered

### Disclaimer
Throughout this writeup, any code examples will be taken from the pseudocode to show the flow of control, for legal reasons, but the corresponding functions can also be easily found within the leaked iBoot/BootROM source codes. Additionally, in order to simplify these examples, I have removed any unnecessary code and renamed variables to make them more readable. This includes various size checks and other safety checks that are not relevant. However, function names remain the same.

## USB initialisation
USB is initialised within the `usb_init()` function, which will result in `usb_dfu_init()` being called. The function initialises an interface for DFU which will handle all USB transfers and requests. Furthermore, it allocates and zeroes out the global input/output buffer that is used for data transfers.
```c
int usb_dfu_init()
{   
  // Initialise and zero out the global IO buffer
  // 0x800-sized buffer on a 0x40-byte alignment
  io_buffer = memalign(0x800, 0x40);
  bzero(io_buffer, 0x800);

  // Initialise the global variables
  completionStatus = -1;
  totalReceived = 0;
  dfuDone = false;

  // Initialise the usb interface instance ... //

  return 0;
}
```

Information to take away from this:
* The global IO buffer, which holds all data from USB transfers, is allocated
* `bzero()` is used to fill the entire buffer with zeroes (empty values)
* Global variable to keep track of data received is initialised
* The global USB interface instance is initialised

## Handling of USB transfers
### Initial request handling
When a USB control transfer is received by DFU, the `usb_core_handle_usb_control_receive()` is called. This function finds the registered interface for handling DFU requests and then calls the `handle_request()` function of that interface. In our case, this is the `handle_interface_request()` function, and the following code shows the control flow in the case of the host transferring data to the device. It checks whether the direction of the transfer is host-to-device or device-to-host, and then acts on the request in order to determine what to do next.

In the case of downloading data, which is what we will be using as part of the vulnerability, it will return one of three outcomes:
* **0** - the transfer is completed
* **-1** - the wLength exceeds the size of the IO buffer
* **wLength from the request** - the device is ready to receive the data and is expecting wLength bytes
```c
int handle_interface_request(struct usb_device_request *request, uint8_t **out_buffer)
{
  int ret = -1;

  // Host to device
  if ( (request->bmRequestType & 0x80) == 0)
  {
    switch(request->bRequest)
    {
      case 1: // DFU_DNLOAD
      {
        if(wLength > sizeof(*io_buffer)) {
          return -1;
        }

        *out_buffer = (uint8_t *)io_buffer; // Set out_buffer to point to IO buffer
        expecting = wLength;
        ret = wLength;
        break;
      }

      case 4: // DFU_CLR_STATUS
      case 6: // DFU_ABORT
      {
        totalReceived = 0;
        if(!dfuDone) {
          // Update global variables to abort DFU
          completionStatus = -1;
          dfuDone = true;
        }
        ret = 0;
        break;
      }
    }
    return ret;
  }
  return -1;
}
```
The important things to note from this are:
* The `out_buffer` pointer passed as an argument is updated to point to the global IO buffer
* It returns the wLength (provided it passes all the checks) as the length it is **expecting to receive** into the IO buffer

The result of this function, which was called from `usb_core_handle_usb_control_receive()`, is then used to indicate the status of the transfer, as shown below. 
```c
int ret = registeredInterfaces[interfaceNumber]->handleRequest(&setupRequest, &ep0DataPhaseBuffer);

// Host to device
if((setupRequest.bmRequestType & 0x80) == 0) {

  // Interface handler returned wLength of data, update global variables
  if (ret > 0) {
    ep0DataPhaseLength = ret;
    ep0DataPhaseInterfaceNumber = interfaceNumber;
    // Begin data phase
  }

  // Interface handler returned 0, transfer is complete
  else if (ret == 0) {
    usb_core_send_zlp();
    // Begin data phase
  }
}

// Device to host
else if((setupRequest.bmRequestType & 0x80) == 0x80) {
    // Begin data phase
}
```
As you can see, if the `handle_interface_request()` function returns a value that is greater than 0, the global variable for the size of the data expected to be transferred is then updated. It's also important to note that the `ep0DataPhaseBuffer` global variable will be updated to point to the global IO buffer if the device prepares for the data phase.

### Data phase
This function is followed by the beginning of the data phase. The important parts of the function for handling the data phase are shown below, and the control flow of this function is crucial for understanding the main vulnerability here. After copying the data into the global data phase buffer, the function checks if all the data has been transferred. If so, it will reset the global variables in order to prepare for the next image to be downloaded.
```c
void handle_ep0_data_phase(u_int8_t *rxBuffer, u_int32_t dataReceived, bool *dataPhase)
{
  // Copying received data into the data phase buffer
  // ...

  // All data has been received
  if(ep0DataPhaseReceived == ep0DataPhaseLength)
  { 
    // Call the interface data phase callback and 
    // send zero-length packet to signify end of transfer

    goto done; // Clear global state
  }
  return;
}
```
Once the data phase is complete, the data from the IO buffer is copied into the image buffer to be loaded and booted later on. After this, the following code is executed in order to clear the global variables as the data transfer is complete. This will then allow DFU to prepare to receive the next image over USB.
```c
done:
  ep0DataPhaseReceived = 0;
  ep0DataPhaseLength = 0; 
  ep0DataPhaseBuffer = NULL;
  ep0DataPhaseInterfaceNumber = -2;
```

This has been a lot to take in, so I will quickly summarise the process:
* In DFU initialisation, the IO buffer is allocated and zeroed out
* When transferring data, the global buffer for the data is set to point to the IO buffer
* Data transferred over USB is hence copied into the IO buffer
* When image transfer is complete, the contents of the IO buffer are copied into an image buffer
* This is followed by the resetting of the global state to prepare for a new image transfer

## Use-after-free
### Lifecycle of image transfer
Now, here's the fun part of the writeup - where I go into the actual vulnerability. When DFU mode is started, the main function that is called is the `getDFUImage()` function, the importants parts of which are shown below:
```c
int getDFUImage(void* buffer, int maxLength)
{
  // Update global variables with parameters
  imageBuffer = buffer;
  imageBufferSize = maxLength;

  // Waits until DFU is finished
  while (!dfuDone) {
    event_wait(&dfuEvent);
  }

  // Shut down all USB operations once done
  usb_quiesce();
  return completionStatus;
}
```
So, what the function does is essentially allow for image transfers to happen and for DFU to do it's thing, and then shuts down the USB stack once it is finished. Now, looking back at the `handle_ep0_data_phase()` function, the global variables are all reset once the data phase has completed. However, if the data is _never fully transferred_, what happens then? The function simply returns **without clearing the global state**. This is good for us, as the attacker, because it means that the global variable holding the pointer to the IO buffer will still be intact.

### USB stack shutdown
Although it wasn't touched on above, taking another look at the `handle_interface_request()` function above will reveal that sending a `DFU_ABORT` command to DFU will cause it to set the `dfuDone` global variable to `true`, and signal the end of DFU. This can also be done by triggering a USB reset, which calls `handle_bus_reset()`. Back in `getDFUImage()`, this will result in the calling of `usb_quiesce()` to shut down the USB stack. The function looks like this:
```c
void usb_quiesce()
{
  usb_core_stop();
  usb_free();
  usb_inited = false;
}
```
The `usb_free()` function calls `usb_dfu_exit()`, and the only important part of that function is the following:
```c
if (io_buffer) {
  free(io_buffer);
  io_buffer = NULL;
}
```

So, by following the paper trail, we can see that:
* Not completing the data phase results in the global variables not being cleared
* Sending a `DFU_ABORT` command results in the `dfuDone` global variable being set to true
* This causes `usb_quiesce()` to be called, leading to the IO buffer being freed
* `getDFUImage()` returns, and is called again upon re-entry
* The global variables are not re-initialised upon re-entry
* The global variable pointing to the IO buffer remains, but points to the now-freed buffer

As I'm sure you can now tell, this is a use-after-free vulnerability, and it is the one utilised by checkm8. Next, I will go into how this vulnerability can be exploited, in order to gain code execution on the device. However, before it can be exploited, a certain memory leak is required.

## Memory leak
### Why is a leak needed?
The SecureROM is highly deterministic and, for this reason, the IO buffer is allocated at roughly the same location on the heap each time the USB stack is initialised. However, as the use-after-free requires the re-entry of DFU, and `getDFUImage()` to be called again, it creates a problem for us - as the newly-allocated IO buffer will normally just be placed over the freed buffer - rendering the main vulnerability completely useless. This is where the memory leak comes in - allowing the attacker to trick the heap allocator into allocating the new IO buffer elsewhere on the heap. This is also why the A12 and A13 SecureROMs are not vulnerable to the checkm8 exploit. They are vulnerable to the use-after-free, and it can be triggered, but there is no way to prevent the re-allocation of the IO buffer over the freed one.

For context, a memory leak occurs when objects that are allocated in memory are incorrectly de-allocated or freed - resulting in the memory remaining allocated, but inaccessible.

### USB request structure 
Below is the `usb_device_io_request` structure, which will henceforth be known as simply `io_request`:
```c
struct usb_device_io_request
{
	u_int32_t                       endpoint;
	volatile u_int8_t               *io_buffer;
	int                             status;
	u_int32_t                       io_length;
	u_int32_t                       return_count;
	void (*callback) (struct usb_device_io_request *io_request);
	struct usb_device_io_request    *next;
};
```
There are two fields of this structure that are important in order to understand the memory leak. The `callback` field is a pointer to a function that is called once the request is completed. The `next` field is a pointer to the next `io_request` structure in the linked list of requests.

### The bug
If you stall the device-to-host pipe of DFU, where it will not process any requests, you can then cause a large number of allocations by sending a large number of requests during the stalled period. This will result in each request having it's `io_request` structure being allocated and becoming part of the linked list for the endpoint. When you unstall the pipe, you can cause all of these requests to be freed and de-allocated. So, with this, we have the ability to allocate and delay the de-allocation of objects on the heap.

Despite being able to do this, these allocations will not persist through a shut down of the USB stack. For this to be the case, we need a memory leak wherein certain requests are never properly de-allocated. 

Luckily, there is a leak vulnerability within the standard callback for an `io_request` object. The device will try to send a zero-length packet if, and only if, the request has a length that is more than zero **and** an exact multiple of the packet size (`0x40`) **and** the host has requested more bytes than this. If both of these conditions are true, the device has to send an additional zero-length packet.
```c
void standard_device_request_cb (struct usb_device_io_request *request)
{
  if ((request->io_length > 0)
  && ((request->io_length % 0x40) == 0)
  && (setup_request.wLength > request->io_length)) { 
    usb_core_send_zlp();
  }
}
```
When a USB reset or a DFU abort causes the USB stack to quiesce, the device initally aborts and disables all endpoints, before performing `bzero()` on the entire endpoint structure array. In the process of shutting down the USB stack, all pending requests are processed as failed, which triggers each of their callbacks. The problem is, these additional zero-length packets are never sent while the stack is shutting down, so are therefore leaked.

So, by stalling the pipe and sending a large number of requests, we can cause lots of request allocations to pile up. By then triggering a USB reset, we can invoke the callbacks of these requests, which will queue additional zero-length packets, which will be leaked.

In A12+ SoCs, when a USB reset occurs, the abort that is subsequently triggered also aborts `EP0_IN` for each setup packet - resulting in `abort()` being called twice. The first abort will queue an additional zero-length packet, but the second will successfully reap it and de-allocate it. It is only after this that the `bzero()` happens.

There is a second bug that factors into the memory leak - wherein the transfer length that is expected by the host device is checked from the setup packet, but said setup packet could have been replaced by the time of the check. When the host receives a last packet with a size of less than `0x40` (because transfers are split into `0x40`-sized packets), the transfer is complete. So, if the transfer length is an exact multiple of `0x40`, a zero-length packet must be sent to signal that the transfer has ended.

However, the callbacks invoked during the shut down of the USB stack could have queued new zero-length packet requests, which would then be leaked - and these can be used for heap shaping. Because of how to heap allocator logic works, if the IO buffer is `0x800` bytes and two allocations are leaked that are exactly `0x800` bytes apart, the space in between them will be preferred as the spot for the next `0x800`-sized allocation (A.K.A. the IO buffer upon DFU re-entry). This is due to the heap allocator choosing the smallest possible space for the allocation, of which the space between the two leaked allocations will be the perfect size.

# Exploitation
Unfortunately, to trigger the use-after-free with an incomplete data phase, you must go beyond the normal boundaries of USB transfers, as defined in the USB specification. There are two solutions for this that have been utilised in the open-source community: firstly, using micro-controllers (such as an Arduino + USB Host Controller) like [this](https://github.com/a1exdandy/checkm8-a5), to gain maximum control over the USB stack of the host device, allowing you to control exactly what is sent and when; secondly, forcing the cancellation of the transfer midway through, as is done in [ipwndfu](https://github.com/aXi0mX/ipwndfu) by using an extremely short timeout on an asynchronous transfer.

The stages of exploitation are as follows:
* Shaping the heap
* Trigger the use-after-free vulnerability
* Sending and executing the payload

## Heap feng shui
Heap feng shui is the technique of deliberately manipulating the heap and shaping it to benefit exploitation. Using the memory leak discussed earlier, we can trick the heap allocator into allocating the IO buffer in a different location on re-entry - allowing us to access the freed buffer from the previous iteration of DFU.

In order to craft the hole for the next IO buffer, we should do the following:
* Stall the device-to-host endpoint.
* Send a large number requests to create a `0x800`-sized group of request allocations.
* Have the first and last of these requests meet the requirements for sending an additional zero-length packet.
* Trigger a USB reset so that `usb_quiesce()` is called and these requests are leaked.
* Be left with a perfect hole that is of size `0x800` as DFU re-enters and allocates a new IO buffer.

We first need to figure out at which address the IO requests will be allocated from, before figuring out how many are needed to reach the base of the IO buffer.

At this point, we will have the hole for our next IO buffer to be allocated upon re-entry from DFU in the next stage. Without this stage, the IO buffer would be allocated in the same place as before and the use-after-free would be inexpoloitable.

## Triggering the use-after-free
With the new IO buffer hopefully allocated within the hole we created using heap feng shui, we can now trigger the main use-after-free vulnerability.

* Send a setup packet with a request type of `0x80`, a `DFU_DNLOAD` request and a wLength that is less than `0x800` to the device. This will set all the global variables to their necessary values.
* Begin the data phase but leave it as incomplete in order to evade the clearing of the global state.
* Send a `DFU_ABORT` request in order to cause the IO buffer to be freed and the re-entry of DFU. This will trigger the use-after-free vulnerability.

After this, the IO buffer from the first iteration has been freed while the global variables still retain their values - including the variable that points to the old IO buffer. The new IO buffer should have been allocated in the hole created during the heap feng shui phase.

## The payload

### 
As the use-after-free has now been triggered, we now need to send our overwrite to the device. Before doing so, however, we need to allocate some `io_request` objects - I will explain why in a second. At the moment, I am using the gaster payload in my exploit.

First of all, we need to send the payload, which will be done like a regular DFU image transfer. We send the payload in `0x800`-sized chunks (which matches the wLength of the request we used to set the global state originally). If you think back to how the data phase is handled, you will know that at the end of the data phase the data in the IO buffer is copied into the image buffer. The image buffer is allocated at the insecure memory, A.K.A. a predictable address for us.

Next, we will stall the device and queue requests so that they are added to the linked list of requests. If we queue enough so that they begin to be allocated in our freed buffer, we can then have an overwrite written directly on top of these requests. Using this overwrite, we can change the `callback` field to simply point to a `nop` gadget, and the `next` field to point to a location within the image buffer - where our payload now sits.

You may be wondering why we didn't just overwrite the `callback` field to point directly to our payload. The `nop` gadget is at offset `0x1800BCD0C` in the `T8011` SecureROM dump and actually looks like this:
```s
ldp x29, x30, [sp, 0x10]
ldp x20, x19, [sp], 0x20
ret
```
This gadget, in addition to performing it's operation as a `nop` gadget, also restores the previous `LR` register. The `LR` register (link register) is also known as the return address register, and is used to store the address to return to after a function call.  Within `usb_core_complete_endpoint_io()`, which is called during the shut down of the USB stack, the callback of the request is called. After this callback has finished, the request object is freed.

If we don't restore this register, the device will reach the `ret` instruction at the end of the payload and return to the `usb_core_complete_endpoint_io()` function and the request will then be freed. So, by restoring the register, the call to `free()` in `usb_core_complete_endpoint_io()` is skipped, so the request stays intact. Not doing so would also cause issues with `free()` as we have damaged the heap's metadata by overflowing.

So, as a result of this, the device will execute from the address at the `callback` field to restore the previous `LR` register, and then return to the previous `LR` register, which will point to the function that originally called `usb_core_complete_endpoint_io()` and hence bypass the call to `free()`.

After this, the device will follow the `next` field to the address of our payload, and move to the middle of the payload.

## Executing the payload
With the payload in place and an `io_request` having it's `next` field pointing to an address inside our payload, we can trigger a USB reset. As always, this will process the list of pending requests we just allocated while stalled as failed, and will invoke the callback for each of these.

When it reaches our overflown `io_request` object, it will execute the callback (which is just a `nop` gadget) and then follow the `next` field to arrive in the middle of our payload. It will then try to execute the `callback` field of what it believes is an `io_request` object, but actually just begin exexcuting our callback chain at the address we overflowed the `next` field with + the offset of the `callback` field in the `io_request` structure (`0x20`).

## Putting the plan into action

## Testing

## Troubleshooting

# Conclusion

If you spot any errors, please don't hesitate to contact me and let me know - I'd rather fix the errors ASAP than have anyone learn incorrect information! Furthermore, extra questions are welcome and I will try to answer all as quick as I can.

If you would like to contact me, drop me an email at alfie@alfiecg.uk. Thank you!