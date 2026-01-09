#ifndef PITMAP_H
#define PITMAP_H

#include <stdint.h>

typedef struct {
    uint32_t* frame_buffer; // the array that stores the pixel data
    int width; // width of the image
    int height; // height of the image INCLUDING all stacked frames
    int frame_height; // height of each FRAME
    int frame_count; // number of frames
    uint16_t* frame_delays; // centisecond delay for each frame
} PM_image;

// loads an image file and returns a PM_image
PM_image* PM_load_image(const char* filename, unsigned char debug_mode);

// unallocated a PM_image*
void PM_free_image(PM_image* image);

#endif