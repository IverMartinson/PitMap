#include "../headers/pitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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


PM_image* PM_load_bitmap(unsigned char debug_mode){
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

    if (debug_mode) printf("file size is %d bytes\nimage res is %dx%d\ncompression method is #%d\npixel buffer offset is %d\n", file_size, image_width, image_height, compression_method, pixel_buffer_offset);
    
    uint32_t* color_palette;

    if (number_of_colors_in_palette > 0) { // we've got a color palette!
        if (debug_mode) printf("color palette exists and has %d colors\n", number_of_colors_in_palette);
        
        // skip to color palette
        current_byte = header_size + 14 + // skip the file header (14 bytes) and the DIB
            12 * (compression_method == 3) + 16 * (compression_method == 6); // skip the bit fields if they exist 
    
        color_palette = malloc(sizeof(uint32_t) * number_of_colors_in_palette);
        
        for (uint32_t i = 0; i < number_of_colors_in_palette; i++){
            // BGR0 -> RGB255
            
            unsigned char r = get_1();
            unsigned char g = get_1();
            unsigned char b = get_1();
            
            color_palette[i] = r << 24 | g << 16 | b << 8 | 255 << 0;
            skip(1);
        }
    }
    
    image->frame_buffer = malloc(sizeof(uint32_t) * image_width * image_height);
    image->width = image_width;
    image->height = image_height;

    // pixel array

    current_byte = pixel_buffer_offset;

    int row_size = ceil((float)(bits_per_pixel * image_width) / (32)) * 4;

    if (debug_mode) printf("%d bits per pixel\n", bits_per_pixel);

    switch (bits_per_pixel){
        case (32): { // RGBA 1 byte each
            for (int y = image_height - 1; y >= 0; y--){
                int current_byte_of_row = 0;

                for (int x = image_width - 1; x >= 0; x--){ // starting reversed becuase image data is backwards
                    image->frame_buffer[y * image_width + x] = (uint32_t)((uint8_t)get_1() << 24 | (uint8_t)get_1() << 16 | (uint8_t)get_1() << 8 | (uint8_t)get_1());

                    current_byte_of_row += 4;
                }
            
                skip(row_size - current_byte_of_row);
            }

            break;
        }

        case (24): { // RGB -> RGBA 1 byte each
            for (int y = image_height - 1; y >= 0; y--){
                int current_byte_of_row = 0;

                for (int x = image_width - 1; x >= 0; x--){ // starting reversed becuase image data is backwards
                    image->frame_buffer[y * image_width + x] = (uint32_t)((uint8_t)get_1() << 24 | (uint8_t)get_1() << 16 | (uint8_t)get_1() << 8 | 255);
                    
                    current_byte_of_row += 3;
                }
            
                skip(row_size - current_byte_of_row);        
            }    
            
            break;
        }

        case (16): {
            printf("16 BPP supporet not implemented");

            break;
        }

        case (8): { // paletted
            if (compression_method == 0){ // no compression
                for (int y = image_height - 1; y >= 0; y--){

                    for (int x = image_width - 1; x >= 0; x--){ // starting reversed becuase image data is backwards
                    uint8_t palette_index = get_1();

                        // if (debug_mode) printf("current pixel is %dx%d\npalette index is %d\n", x, y, palette_index);

                    
                        image->frame_buffer[y * image_width + x] = color_palette[palette_index];

                }
            }    
            }
            else { // assume RLE
                uint16_t x = image_width - 1;
            uint16_t y = image_height - 1;

            unsigned char reading_file = 1;
            
            while (reading_file){
                uint8_t number_of_repetitions = get_1();
                uint8_t palette_index = get_1();

                if (number_of_repetitions > 0){
                    for (uint8_t pixel = 0; pixel < number_of_repetitions; pixel++){
                        // if (debug_mode) printf("current pixel is %dx%d\npalette index is %d\n", x, y, palette_index);
                        image->frame_buffer[y * image_width + x--] = color_palette[palette_index];
                    }
                } else {
                    if (palette_index == 0) { // end of a line
                        x = image_width - 1;
                        y--;
                    } else if (palette_index == 1){ // end of image
                        reading_file = 0;
                    } else { // delta   
                        x -= get_1();
                        y -= get_1();
                    }
                }
            }}
                
            break;
        }

        case (4): { // paletted
            printf("4 BPP supporet not implemented");

                
            break;
        }

        case (1): {
            printf("1 BPP supporet not implemented");

                
            break;
        }
    }

    return image;
}

PM_image* PM_load_image(char *filename, unsigned char debug_mode){
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

    // see what the file type is

    char *strtok_string = strtok(filename, ".");
    while(strtok_string != NULL) {
        strtok_string = strtok(NULL, ".");
        printf("%s\n", strtok_string);
    }

    return NULL;

    return PM_load_bitmap(debug_mode);
}   
