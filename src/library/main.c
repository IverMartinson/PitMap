#include "../headers/pitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

uint8_t *file_buffer;
int current_byte;

// byte funcs

void skip(int bytes_to_skip){
    current_byte += bytes_to_skip;
}

int8_t get_1(){
    return file_buffer[current_byte++];
}

int16_t get_2(){
    return file_buffer[current_byte++] | file_buffer[current_byte++] << 8;
}

int32_t get_4(){
    return file_buffer[current_byte++] | file_buffer[current_byte++] << 8 | file_buffer[current_byte++] << 16 | file_buffer[current_byte++] << 24;
}


PM_image* PM_load_bitmap(){
    PM_image* image = malloc(sizeof(PM_image));

    current_byte = 0;

    // file header

    uint16_t header_field = get_2();

    if (header_field != 19778) {
        printf("file is not a bitmap file or bitmap is damaged\n");
        return NULL;
    }

    uint32_t file_size = get_4();
    skip(4); // reserved
    uint32_t pixel_buffer_offset = get_4();

    // bitmap information header / DIB header

    // BITMAPINFOHEADER
    uint32_t header_size = get_4();
    uint32_t image_width = header_size == 12 ? (uint32_t)get_2() : (uint32_t)get_4();
    uint32_t image_height = header_size == 12 ? (uint32_t)get_2() : (uint32_t)get_4();
    skip(2); // color planes, always 1
    uint16_t bits_per_pixel = get_2();
    uint32_t compression_method = get_4();
    uint32_t image_size = get_4();
    int32_t horizontal_res = get_4(); // pixels per meter
    int32_t vertical_res = get_4();
    uint32_t number_of_colors_in_palette = get_4();
    skip(4); // number of important colors, ignored

    image->frame_buffer = malloc(sizeof(uint32_t) * image_width * image_height);
    image->width = image_width;
    image->height = image_height;

    // extra bit masks not implemented

    // color table not implemented
    
    // icc color profile not implemented

    // pixel array

    current_byte = pixel_buffer_offset;

    int row_size = ceil((float)(bits_per_pixel * image_width) / (32)) * 4;

    if (bits_per_pixel < 8) {
        printf("under 8 bits per pixel not implemented");
    
        return NULL;
    }

    for (uint32_t y = 0; y < image_height; y++){
        int current_byte_of_row = 0;

        for (int x = image_width - 1; x >= 0; x--){ // starting reversed becuase image data is backwards
            switch (bits_per_pixel){
                case (32): { // RGBA 1 byte each
                    image->frame_buffer[y * image_height + x] = (uint32_t)((uint8_t)get_1() << 24 | (uint8_t)get_1() << 16 | (uint8_t)get_1() << 8 | (uint8_t)get_1());

                    current_byte_of_row += 4;

                    break;
                }

                case (24): { // RGB -> RGBA 1 byte each
                    image->frame_buffer[y * image_height + x] = (uint32_t)((uint8_t)get_1() << 24 | (uint8_t)get_1() << 16 | (uint8_t)get_1() << 8 | 255);
                    
                    current_byte_of_row += 3;

                    break;
                }

                case (16): {

                        
                    break;
                }

                case (8): {

                        
                    break;
                }

                case (4): {

                        
                    break;
                }

                case (1): {

                        
                    break;
                }
            }
        }

        skip(row_size - current_byte_of_row);
    }

    return image;
}

PM_image* PM_load_image(char *filename){
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL){
        printf("file does not exist !!!\n");

        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    file_buffer = malloc(size);

    fread(file_buffer, 1, size, fp);
    fclose(fp);

    return PM_load_bitmap();
}   
