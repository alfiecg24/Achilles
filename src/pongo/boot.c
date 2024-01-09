#include <pongo/boot.h>
#include <pongo/pongo.h> // This can only be included in the boot.c file, not the boot.h file.

void wait_for_device_to_enter_yolo_dfu(usb_handle_t *handle) {
    char *serialNumber = NULL;
    wait_usb_handle(handle);
    serialNumber = get_usb_device_serial_number(handle);
    if (serialNumber == NULL) {
        LOG(LOG_ERROR, "Failed to get device serial number.");
        close_usb_handle(handle);
        return;
    }
    while (!dfu_serial_number_is_in_yolo_dfu(serialNumber)) {
        wait_usb_handle(handle);
        serialNumber = get_usb_device_serial_number(handle);
        close_usb_handle(handle);
        if (serialNumber == NULL) {
            LOG(LOG_ERROR, "Failed to get device serial number.");
            return;
        }
        sleep_ms(100);
    }
    close_usb_handle(handle);
    return;
}

bool send_pongo_to_yolo_dfu(usb_handle_t *handle) {
    unsigned char *pongoData = (unsigned char *)pongo_bin;
    size_t pongoSize = pongo_bin_len;
    bool ret = false;
    if (wait_usb_handle(handle)) {
        if (dfu_serial_number_is_in_yolo_dfu(get_usb_device_serial_number(handle))) {
            size_t lengthSent = 0, size;
            transfer_ret_t transferRet;
            while (lengthSent < pongoSize) {
                retry:
                    size = ((pongoSize - lengthSent) > 0x800) ? 0x800 : (pongoSize - lengthSent);
                    send_usb_control_request(handle, 0x21, DFU_DNLOAD, 0, 0, pongoData + lengthSent, size, &transferRet);
                    if (transferRet.sz != size || transferRet.ret != USB_TRANSFER_OK) {
                        LOG(LOG_VERBOSE, "Retrying at size 0x%lx...", size);
                        sleep_ms(100);
                        goto retry;
                    }
                    lengthSent += size;
            }
            send_usb_control_request_no_data(handle, 0x21, DFU_CLRSTATUS, 0, 0, 0, NULL);
            reset_usb_handle(handle);
            close_usb_handle(handle);
            init_usb_handle(handle, 0x5AC, 0x4141);
            wait_usb_handle(handle);
            LOG(LOG_SUCCESS, "Device is now in PongoOS!");
        }
        else {
            LOG(LOG_ERROR, "Device is not in Yolo DFU mode!");
        }
    }
    return ret;
}