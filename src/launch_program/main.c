#include "../headers/pitmap.h"
#include <stdio.h>


int write_tga(const char* path, const PM_image* img) {
    if (!img || !img->frame_buffer || img->width <= 0 || img->height <= 0)
        return -1;

    FILE* f = fopen(path, "wb");
    if (!f)
        return -2;

    unsigned char header[18] = {0};

    header[2]  = 2;                           // Uncompressed true-color image
    header[12] = img->width & 0xFF;
    header[13] = (img->width >> 8) & 0xFF;
    header[14] = img->height & 0xFF;
    header[15] = (img->height >> 8) & 0xFF;
    header[16] = 32;                          // 32 bits per pixel (RGBA)
    header[17] = 0x20;                        // Image origin: top-left

    fwrite(header, 1, 18, f);

    int total = img->width * img->height;

    // TGA expects BGRA, so we must swizzle R and B.
    // If your buffer is already BGRA, tell me and I'll remove this.
    for (int i = 0; i < total; i++) {
        uint32_t px = img->frame_buffer[i];

        uint8_t r = (px >> 24) & 0xFF;
        uint8_t g = (px >> 16) & 0xFF;
        uint8_t b = (px >> 8)  & 0xFF;
        uint8_t a = (px >> 0)  & 0xFF;

        uint8_t out[4] = { r, g, b, a };      // TGA = BGRA
        fwrite(out, 1, 4, f);
    }

    fclose(f);
    return 0;
}

int main(){
    write_tga("./image.tga", PM_load_image("images/simple_gif.gif", 1));

    

    return 0;
}