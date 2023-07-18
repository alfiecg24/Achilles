#ifndef ALFIELOADER_H
#define ALFIELOADER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/utsname.h>

#define NAME "AlfieLoader"
#define VERSION "0.1.0"
#define CREDITS "Alfie"
#define RELEASE_TYPE "Development"

#define LOG(logLevel, ...) AlfieLoaderLog(logLevel, true, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NO_NEWLINE(logLevel, ...) AlfieLoaderLog(logLevel, false, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define MAX_ARG_LEN 0x100

// Taken from gaster
#ifndef MIN
#	define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
typedef enum
{
    FLAG_BOOL,
    FLAG_INT,
    FLAG_STRING
} flag_type_t;
typedef struct
{
    char *name;
    char *shortOpt;
    char *longOpt;
    char *description;
    char *examples;
    bool set;
    flag_type_t type;
    union
    {
        bool boolVal;
        int intVal;
        char *stringVal;
    };
} arg_t;

// ******************************************************
// Function: getArgumentByName()
//
// Purpose: Get the arg_t struct for an argument by its name
//
// Parameters:
//      char *name: the name of the argument
//
// Returns:
//      arg_t *: the arg_t struct for the argument, NULL if the argument doesn't exist
// ******************************************************
arg_t *getArgumentByName(char *name);

extern arg_t args[];

extern uint16_t cpid;
extern size_t config_hole, ttbr0_vrom_off, ttbr0_sram_off, config_large_leak, config_overwrite_pad;
extern uint64_t tlbi, nop_gadget, ret_gadget, patch_addr, ttbr0_addr, func_gadget, write_ttbr0, memcpy_addr, aes_crypto_cmd, boot_tramp_end, gUSBSerialNumber, dfu_handle_request, usb_core_do_transfer, dfu_handle_bus_reset, insecure_memory_base, handle_interface_request, usb_create_string_descriptor, usb_serial_number_string_descriptor;


#endif // ALFIELOADER_H