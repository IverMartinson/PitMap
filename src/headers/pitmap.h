#ifndef PITMAP_H
#define PITMAP_H

#include <stdint.h>

typedef struct {
    uint32_t* frame_buffer;
    int width, height;
} PM_image;

PM_image* PM_load_image(char* filename, unsigned char debug_mode);

#endif