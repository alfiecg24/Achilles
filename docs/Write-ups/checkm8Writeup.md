# A comprehensive write-up of the checkm8 BootROM exploit

- [A comprehensive write-up of the checkm8 BootROM exploit](#a-comprehensive-write-up-of-the-checkm8-bootrom-exploit)
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
    - [The overwrite](#the-overwrite)
    - [The main payload](#the-main-payload)
  - [Executing the payload](#executing-the-payload)
- [Conclusion](#conclusion)

# Analysis
## Introduction
This is my analysis and writeup of the vulnerabilities exploited in the checkm8 BootROM exploit. I wrote this in order to help me gain a better understanding of the vulnerability so that I could design my own strategy for exploitation and write my own implementation of the exploit. The checkm8 exploit relies on a couple of vulnerabilities:
* The main use-after-free (not patched until A14)
* The memory leak (patched in A12)

The memory leak is essential in order to exploit the use-after-free, and I will be going into further detail later on in this writeup. However, it is the patching of this leak that is the reason the use-after-free cannot be exploited on A12 and A13 SoCs.

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
  if ((request->bmRequestType & 0x80) == 0)
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

   // return ... //
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

The following sections will have code examples for each of these stages - which are taken from my currently-unreleased checkm8-based project.

## Heap feng shui
Heap feng shui is the technique of deliberately manipulating the heap and shaping it to benefit exploitation. Using the memory leak discussed earlier, we can trick the heap allocator into allocating the IO buffer in a different location on re-entry - allowing us to access the freed buffer from the previous iteration of DFU.

In order to craft the hole for the next IO buffer, we should do the following:
* Stall the device-to-host endpoint.
* Send a large number requests to create a build-up of request allocations.
* Have the first and last of these requests meet the requirements for sending an additional zero-length packet.
* Trigger a USB reset so that `usb_quiesce()` is called and these requests are leaked.
* Be left with a hole that can be used to control allocation of the next IO buffer.

Accounting for all heap allocations being rounded up to the nearest multiple of `0x40`, and the `0x40`-sized header for each packet, we can safely assume that each `io_request` object will occupy `0x80` bytes on the heap. So, one strategy for heap feng shui would be to send `0x10` non-leaking packets to the device, which would create a hole of size `0x800` - which is exactly the size of the IO buffer. Testing this strategy proved it to be successful in exploitation.

However, a quicker and more simple strategy (which is utilised in most implementations) is to send enough packets that a hole will be created that is smaller than `0x800`, but big enough that allocations will end up being shuffled around enough so that the IO buffer is allocated elsewhere upon re-entry. This is the strategy used in the function shown below, and it makes the exploit quicker.

Here's the heap spray function from my project, which is adapted for the T8011 BootROM:
```c
bool checkm8HeapSpray(device_t *device)
{
    checkm8Stall(device)
    for (int i = 1; i <= config.hole; i++)
    {
        checkm8NoLeak(device)
    }
    checkm8USBRequestLeak(device)
    checkm8NoLeak(device)
    return true;
}
```
I'll walk through the function step-by-step.

```c
checkm8Stall(device)
```

This stalls the device-to-host endpoint, which will allow for a large number of `io_request` structures to be allocated as we can send requests but they will not be processed while the device is in the stalled state. Additionally, this request will leak a zero-length packet, as it matches the requirements in the callback function in order for an additional zero-length packet to be sent.

```c
for (int i = 1; i <= config.hole; i++)
{
    checkm8NoLeak(device)
}
```
This sends `config.hole` requests to the device, which will each have an `io_request` structure allocated for them. Such requests will not leak zero-length packets, as they do not match the requirements for the callback function to send an additional zero-length packet. This will create a 'hole' as such that they will all be correctly de-allocated when the USB stack is quiesced.

```c
checkm8USBRequestLeak(device)
```
This will leak an additional zero-length packet and give us the hole that we need. This is because we send a zero-length packet at the beginning of the function, so the allocations will currently look something like this:
```
[  Leaked packet  ]
[  Normal packet  ]
[  Normal packet  ]
[  Normal packet  ]
[  Normal packet  ]
[  Normal packet  ]
[  Normal packet  ]
[  Leaked packet  ]
```
After resetting the USB stack, it will look something like this:
```
[ Allocated space ]
[   Empty space   ]
[   Empty space   ]
[   Empty space   ]
[   Empty space   ]
[   Empty space   ]
[   Empty space   ]
[ Allocated space ]
```
The heap allocator will then allocate objects inside this hole enough to shuffle around other allocations, which will result in the IO buffer being allocated elsewhere on re-entry.

```c
checkm8NoLeak(device)
```
This will send a request that does not leak a zero-length packet, which will be de-allocated when the USB stack is quiesced. As it stands, I am not entirely sure why this is necessary, but it is - without this, the exploit will fail.

At this point, we will have the heap in such a state that the next IO buffer will be allocated in a location other than the standard address, which is occupied by the freed buffer. If the new IO buffer were to be allocated in the same place, we would not be able to exploit the use-after-free vulnerability as the freed buffer would be overwritten.

## Triggering the use-after-free
With the new IO buffer hopefully allocated elsewhere within the heap, thanks to our heap feng shui, we can now trigger the main use-after-free vulnerability.

* Send a setup packet with a request type of `0x80`, a `DFU_DNLOAD` request and a wLength that is less than or equal to `0x800` to the device. This will set all the global variables to their necessary values.
* Begin the data phase but leave it as incomplete in order to evade the clearing of the global state.
* Send a `DFU_ABORT` request in order to cause the IO buffer to be freed and the re-entry of DFU. This will trigger the use-after-free vulnerability.

```c
bool checkm8TriggerUaF(device_t *device)
{
  unsigned usbAbortTimeout = 10;
	transfer_ret_t transferRet;

	while(sendUSBControlRequestAsyncNoData(&device->handle, 0x21, DFU_DNLOAD, 0, 0, DFU_MAX_TRANSFER_SIZE, usbAbortTimeout, &transferRet)) {
		if(transferRet.sz < config.overwritePadding 
        && sendUSBControlRequestNoData(&device->handle, 0, 0, 0, 0, config.overwritePadding - transferRet.sz, &transferRet) 
        && transferRet.ret == USB_TRANSFER_STALL) {
			sendUSBControlRequestNoData(&device->handle, 0x21, DFU_CLRSTATUS, 0, 0, 0, NULL);
			return true;
		}
		if(!sendUSBControlRequestNoData(&device->handle, 0x21, DFU_DNLOAD, 0, 0, EP0_MAX_PACKET_SIZE, NULL)) {
			break;
		}
		usbAbortTimeout = (usbAbortTimeout + 1) % 10;
	}
	return false;
}
```

After this, the IO buffer from the first iteration has been freed while the global variables still retain their values - including the variable that points to the old IO buffer. The new IO buffer should have been allocated in the hole created during the heap feng shui phase. Hence, by sending data to the device, it will be written into the address in the global variable that points to the old IO buffer.

Next, we need to send our overwrite and payload in order to give us full arbitrary code execution.

## The payload
The payload is the machine code that we send to the device to be executed as part of the exploitation process. It is sent in two parts:
* The overwrite
* The actual payload
The overwrite is the data that we send to the device in order to overwrite the `callback` and `next` fields of the `io_request` structure. This will then direct the execution flow to the main payload.
### The overwrite
For the overwrite, the `callback` and `next` fields in the `io_request` structure at the beginning of the freed buffer need to be overwritten. Both fields are pointers to areas in memory - `callback` being a pointer to the callback function and `next` being a pointer to the next `io_request` structure in the linked list of pending requests.

When exploiting the checkm8 exploit, the overwriting of the `callback` function is an opportunity to restore the link and FP registers to prevent the current USB request from being freed. Because we have overwritten the data in the heap, trying to free the object will result in the invalid heap metadata causing issues and possibly a crash on the device.

For those who aren't sure what the link and FP registers are, here's a quick summary. The link register (LR) holds the address that the program should jump back to after returning from a function. The frame pointer (FP) is used to hold the address of the current **stack frame**, which looks something like this:
```
+-----------------+
|  Return Address |
+-----------------+
|  Arguments      |
|  and Parameters |
+-----------------+
| Local Variables |
+-----------------+
| Saved Registers |
+-----------------+
|  Frame Pointer  |
+-----------------+
```
The stack frame is the area of the stack that is currently being used by the program, and typically changes when a function is called or returns. As you can see, it holds local variables, the return address and other data important to the program at that time.

However, you may be wondering - what is the point of restoring these registers? Well, if you think back to what happens when the USB stack shuts down, it will process the list of pending requests and `usb_core_complete_endpoint_io()` will invoke the callback function for each of them. However, after doing so, this function will free the IO request object. If we can restore the link and FP registers, we can have execution jump back to the function that called `usb_core_complete_endpoint_io()`, instead of continuing on to free the IO request object in this function.

However, as `callback` is a pointer to an area in memory, we cannot simply just overwrite the field with machine code to do this job for us. This leads me to the `nop` gadget, which is used in popular checkm8 implementations - although the name is not particularly accurate. `nop` means "no operation", and is typically code that does nothing. However, in the case of checkm8, the `nop` gadget that is in the BootROM code looks like this:
```
ldp x29, x30, [sp, #0x10]
ldp x20, x19, [sp], #0x20
ret
```
For some context, the `x29` register is the frame pointer, and the `x30` is the link register. It's also important to know that for ARM64, the stack usually grows downwards, from a high address to a low address, and the stack pointer (SP) holds the address of the lowest address occupied by the stack.

So, with that, here is a breakdown of `ldp x29, x30, [sp, #0x10]`:
1. `ldp` is the load pair instruction, which loads a pair of registers from memory into the specified address.
2. `x29, x30` is the pair of registers to load from.
3. `[sp, #0x10]` is the address to load the registers from. `sp` is the stack pointer, and `#0x10` is the offset from the stack pointer to load the registers from.
Because the stack grows downwards, adding `0x10` to the stack pointer will point to the memory just above the stack pointer, which is where the link and FP registers are stored. `0x10` is the combined size of the pair of registers, AKA 16 bytes - as each register is 64 bits, or 8 bytes.

`ldp x20, x19, [sp], #0x20` does a similar job, except it loads the registers from the stack pointer without an offset, but then **increments** the stack pointer by `0x20` (32 bytes) - this is done for alignment purposes and to ensure that the stack pointer is pointing to the correct address for the next instruction that may access that memory.

Finally, `ret` is the return instruction, which will return to the address stored in the link register.

### The main payload

## Executing the payload
With the payload in place and an `io_request` having it's `next` field pointing to an address inside our payload, we can trigger a USB reset. As always, this will process the list of pending requests (which we just allocated while stalled) as failed, and will invoke the callback for each of these requests.

When it reaches our overflown `io_request` object, it will execute the callback (which is just a `nop` gadget to restore the link and FP registers) and then follow the `next` field to arrive in the middle of our payload. It will then try to execute the `callback` field of what it believes is an `io_request` object, but actually just begin executing our callback chain at the address we overflowed the `next` field with + the offset of the `callback` field in the `io_request` structure (`0x20`).

# Conclusion

If you spot any errors, please don't hesitate to contact me and let me know - I'd rather fix the errors ASAP than have anyone learn incorrect information! Furthermore, extra questions are welcome and I will try to answer all as quick as I can.

If you would like to contact me, drop me an email at alfie@alfiecg.uk. Thank you!