#ifndef BOOT_H
#define BOOT_H

#include <Achilles.h>
#include <usb/usb.h>
#include <exploit/dfu.h>
#include <pongo/pongo_helper.h>

bool wait_for_device_to_enter_yolo_dfu(usb_handle_t *handle);
bool send_pongo_to_yolo_dfu(usb_handle_t *handle);

#endif // BOOT_H