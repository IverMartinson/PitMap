#ifndef PM_STRUCTS_H
#define PM_STRUCTS_H

typedef struct {
    uint32_t* frame_buffer; // the array that stores the pixel data
    int width; // width of the image
    int height; // height of the image INCLUDING all stacked frames
    int frame_height; // height of each FRAME
    int frame_count; // number of frames
    uint16_t* frame_delays; // centisecond delay for each frame
} PM_image;

#endif
