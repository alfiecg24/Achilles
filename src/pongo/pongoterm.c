#include <pongo/pongoterm.h>
#include <wordexp.h>

size_t pongo_fetch_output(usb_handle_t *handle, char *buffer) {
    uint32_t outPosition = 0;
    uint32_t outLength = 0;
    uint8_t inProgress = 1;
    while (inProgress) {
        transfer_ret_t transferRet;
        int ret = send_usb_control_request(handle, 0xA1, 2, 0, 0, &inProgress, (uint32_t)sizeof(inProgress), NULL);
        if (ret) {
            ret = send_usb_control_request(handle, 0xA1, 1, 0, 0, buffer + outPosition, 0x1000, &transferRet);
            outLength = transferRet.sz;
            if (transferRet.ret == USB_TRANSFER_OK) {
                outPosition += outLength;
                if (outPosition > 0x1000) {
                    memmove(buffer, buffer + outPosition - 0x1000, 0x1000);
                    outPosition = 0x1000;
                }
            }
        }
        if (transferRet.ret != USB_TRANSFER_OK) {
            return 0;
        }
    }
    return outPosition;
}

void write_to_stdout(char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        putchar(buffer[i]);
    }
}

char *get_file_name_from_command(char *str) {
    str += 6;

    // If dragged from a terminal, it will be enclosed in quotes
    if (str[0] == '"' || str[0] == '\'') {
        str++;
        str[strlen(str) - 1] = 0;
    }
    return str;
}

void pongoterm(void) {
    transfer_ret_t transferRet;
    usb_handle_t *handle = malloc(sizeof(usb_handle_t));
    init_usb_handle(handle, 0x5AC, 0x4141);
    wait_usb_handle(handle);

    while (1) {

        // Fetch existing output
        char buffer[0x2000];
        size_t outPosition = pongo_fetch_output(handle, buffer);
        write_to_stdout(buffer, outPosition);
        memset(buffer, 0, 0x2000);

        send_usb_control_request_no_data(handle, 0x21, 4, 0xffff, 0, 0, &transferRet);
        if (transferRet.ret != USB_TRANSFER_OK) {
            printf("Failed to clear buffer.\n");
        }

        // Get the next command
        char *command = malloc(0x200);
        fgets(command, 0x200, stdin);

        // Check if it's sending a file
        if (strncmp(command, "/send", 5) == 0) {
            // NULL the last character (newline)
            command[strlen(command) - 1] = 0;
            if (strlen(command) < 7) {
                printf("Usage: /send FILE\n");
                continue;
            }
            char *path = get_file_name_from_command(command);
            struct stat st;
            if (stat(path, &st) != 0) {
                printf("Failed to stat %s.\n", path);
                continue;
            }
            if (upload_file_to_pongo(handle, path)) {
                printf("%sUploaded 0x%llx bytes to PongoOS.\n", BCYN, st.st_size);
            } else {
                printf("%sFailed to upload file.\n", BRED);
            }
            printf(CRESET);
            printf("pongoOS> ");
        }
        
        // Check if it's a command to boot (we shouldn't fetch output after this)
        else if (strncmp(command, "boot", 4) == 0) {
            issue_pongo_command(handle, command, buffer);
            return;
        }

        else if (strncmp(command, "exit", 4) == 0) {
            return;
        }
        
        // Just a regular command
        else {
            issue_pongo_command(handle, command, buffer);
            write_to_stdout(buffer, strlen(buffer));
        }
    }
}