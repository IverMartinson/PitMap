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

typedef enum {
    PM_ARGB,
    PM_ABRG,
    PM_RGBA,
    PM_BGRA,
} PM_color_type;

#define pmcodeargb a << 24 | r << 16 | g << 8 | b << 0
#define pmcodeabgr a << 24 | b << 16 | g << 8 | r << 0
#define pmcodergba r << 24 | g << 16 | b << 8 | a << 0
#define pmcodebgra b << 24 | g << 16 | r << 8 | a << 0

#endif
