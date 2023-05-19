#ifndef ALFIELOADER_H
#define ALFIELOADER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define NAME "AlfieLoader"
#define VERSION "0.1.0"
#define CREDITS "Alfie"
#define RELEASE_TYPE "Development"

#define LOG(logLevel, ...) loaderLog(logLevel, __FILE__, __LINE__, __func__, __VA_ARGS__)

typedef enum {
    FLAG_BOOL,
    FLAG_INT
} flag_type_t;
typedef struct {
    char *name;
    char *shortOpt;
    char *longOpt;
    char *description;
    char *examples;
    flag_type_t type;
    union {
        bool boolVal;
        int intVal;
    };
} arg_t;

extern arg_t args[];

#endif // ALFIELOADER_H