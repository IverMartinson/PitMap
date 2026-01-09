#ifndef PITMAP_H
#define PITMAP_H

#include <stdint.h>

typedef struct {
    uint32_t* frame_buffer;
    int width, height;
    int frame_count;
    int frame_height;
} PM_image;

PM_image* PM_load_image(const char* filename, unsigned char debug_mode);

#endif