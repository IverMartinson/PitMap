#include "../headers/pitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

uint8_t *file_buffer;
int current_byte;

int (*current_printf_function)(const char *__restrict __format, ...) = printf;

// lsb, the least sig bit is first
// 1 = 10000000
char* lsb_byte_to_binary(uint32_t byte, uint8_t bits){
    char* string = malloc(sizeof(char) * (bits + 1));

    for (int i = 0; i < bits; i++){
        string[i] = ((byte >> i) & 1) + 0x30;
    }

    string[bits] = '\0';

    return string; 
}

// MSB, most sig bit is first
// 1 = 00000001
char* msb_byte_to_binary(uint32_t byte, uint8_t bits){
    char* string = malloc(sizeof(char) * (bits + 1));

    for (int i = 0; i < bits; i++){
        string[bits - 1 - i] = ((byte >> i) & 1) + 0x30;
    }

    string[bits] = '\0';

    return string;
}

// MSB, most sig bit is first
// 1 = 00000001
uint16_t binary_to_int(char* binary, uint8_t number_of_bits){
    uint16_t final_value = 0;

    for (int i = 0; i < number_of_bits; i++){
        final_value += (1 << i) * (binary[number_of_bits - i - 1] - 0x30);
    }

    return final_value;
}

// return individial bit's value. Zero indexed
// msb_get_bit(00100, 2) == 1
uint8_t msb_get_bit(uint32_t data, uint8_t bit){
    return (data >> bit) & 1;
}

// byte reading funcs

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
        (*current_printf_function)("file is not a bitmap file or bitmap is damaged\n");
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

    (*current_printf_function)("file size is %d bytes\nimage res is %dx%d\ncompression method is #%d\npixel buffer offset is %d\n", file_size, image_width, image_height, compression_method, pixel_buffer_offset);
    
    uint32_t* color_palette;

    if (number_of_colors_in_palette > 0) { // we've got a color palette!
        (*current_printf_function)("color palette exists and has %d colors\n", number_of_colors_in_palette);
        
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

    (*current_printf_function)("%d bits per pixel\n", bits_per_pixel);

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
            (*current_printf_function)("16 BPP supporet not implemented");

            break;
        }

        case (8): { // paletted
            if (compression_method == 0){ // no compression
                for (int y = image_height - 1; y >= 0; y--){

                    for (int x = image_width - 1; x >= 0; x--){ // starting reversed becuase image data is backwards
                    uint8_t palette_index = get_1();

                        // (*current_printf_function)("current pixel is %dx%d\npalette index is %d\n", x, y, palette_index);

                    
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
                        // (*current_printf_function)("current pixel is %dx%d\npalette index is %d\n", x, y, palette_index);
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
            (*current_printf_function)("4 BPP supporet not implemented");

                
            break;
        }

        case (1): {
            (*current_printf_function)("1 BPP supporet not implemented");

                
            break;
        }
    }

    return image;
}


PM_image* PM_load_gif(unsigned char debug_mode){
    PM_image* image = malloc(sizeof(PM_image));

    current_byte = 0;

    // magic number

    uint8_t is_87a = 0;

    if (get_1() != 0x47 || get_1() != 0x49 || get_1() != 0x46 || get_1() != 0x38 || get_1() != 0x39 || get_1() != 0x61){
        current_byte = 4;

        if (get_1() != 0x37){
            (*current_printf_function)("file is not a gif file, or gif is damaged\n");

            return NULL;
        } else {
            (*current_printf_function)("gif is verision 87a\n");
            is_87a = 1;
        }
        
        skip(1);
    } else {
        (*current_printf_function)("gif is verision 89a\n");
    }
    
    // image info

    uint16_t frame_count = 0;
    
    uint16_t image_width = get_2();
    uint16_t image_height = get_2();
    
    image->frame_buffer = NULL;

    image->width = image_width;

    if (is_87a){ // 87a doesnt have GCE so just do everything that it would do
        frame_count++;

        image->height = image_height * frame_count;
        image->frame_buffer = realloc(image->frame_buffer, sizeof(uint32_t) * image_width * image_height);
    }

    uint16_t total_image_height = image_height;

    uint8_t color_table_info = get_1();
    uint8_t background_color = get_1();
    uint8_t pixel_aspect_ratio = get_1();

    (*current_printf_function)("color table info: %x\n", color_table_info);

    // value used for encoding actual size of color table
    uint8_t size_of_gct = (color_table_info & 0x07); // bit mask 0000 0111
    // 0 = unsorted, 1 = sorted by importance
    uint8_t color_table_sort_flag = (color_table_info & 0x08) >> 3; // bit mask 0000 1000
    // original color depth of the image when it was created, bits per color
    // doesn't matter (?)
    uint8_t color_resolution = ((color_table_info & 0x70) >> 4) + 1; // bit mask 0111 0000
    // 1 = gct exists
    uint8_t gct_flag = (color_table_info & 0x80) >> 7; // bit mask 1000 0000
    // total number of colors in gct
    uint16_t number_of_colors_in_gct = 1 << (size_of_gct + 1);

    (*current_printf_function)("size_of_gct: %d color_table_sort_flag: %d color_resolution: %d gct_flag: %d number_of_colors_in_gct: %d\n", size_of_gct
,color_table_sort_flag
,color_resolution
,gct_flag
,number_of_colors_in_gct);

    uint32_t* global_color_table = malloc(sizeof(uint32_t) * number_of_colors_in_gct);

    // global color table (if present)

    if (gct_flag){
        (*current_printf_function)("started reading color table block at byte #%d\n", current_byte);
            
        for (int i = 0; i < number_of_colors_in_gct; i++){
            // just gonna assume 8bit
            unsigned char r = get_1(); 
            unsigned char g = get_1(); 
            unsigned char b = get_1();
            
            global_color_table[i] = b << 24 | g << 16 | r << 8 | 255 << 0;
        }

        // image->frame_buffer = global_color_table;
        // image->width = number_of_colors_in_gct + 1;
        // image->frame_buffer = realloc(image->frame_buffer, sizeof(uint32_t) * (number_of_colors_in_gct + 1));
        // image->frame_buffer[number_of_colors_in_gct] = 0xFF00FFFF;
        // image->height = 1;
        // return image;
    }

    unsigned char still_reading_file = 1;

    while (still_reading_file){        
        switch(get_1()){
            case 0x21: { // "!" graphics control extention
                if ((uint8_t)get_1() != (uint8_t)0xF9) break; // a different block denoted by "!", ignore
                
                (*current_printf_function)("started reading graphics control extention block at byte #%d\n", current_byte);

                frame_count++;

                image->height = image_height * frame_count;
                image->frame_buffer = realloc(image->frame_buffer, sizeof(uint32_t) * image_width * image_height * frame_count);

                uint8_t gce_size = get_1();
                uint8_t packed_field = get_1(); // bit field
                skip(2);
                uint8_t transparent_color_gct_index = get_1();
                skip(1);

                // "disposal method" is the first variable in the packed field
                // we can ignore this because I think it's mostly for UI programs

                // next is "input method" and it doesn't rly make sense to me to 
                // exist in the first place
                
                // transparency flag; transparency index is given

                (*current_printf_function)("transparency index: %d\n", transparent_color_gct_index);

                // set the transparent color to 0% opacity
                if (packed_field & 1) global_color_table[transparent_color_gct_index] &= 0xFFFFFF00; // bit mask for alpha channel to be 0

                break;
            }

            case 0x2C: { // "," image descriptor
                (*current_printf_function)("started reading image descriptor block at byte #%d\n", current_byte - 1);

                uint16_t image_left_pos = get_2();
                uint16_t image_top_pos = get_2();
                uint16_t image_descriptor_width = get_2();
                uint16_t image_descriptor_height = get_2();

                (*current_printf_function)("width height %dx%d\n", image_descriptor_width, image_descriptor_height);

                uint8_t packed_field = get_1();

                uint8_t local_color_table_flag = (packed_field & 0x80) >> 7;
                uint8_t interlace_flag = (packed_field & 0x40) >> 6;
                uint8_t local_color_table_is_sorted_flag = (packed_field & 0x20) >> 5;
                uint16_t entries_of_local_color_table = 1 << ((packed_field & 7) + 1);

                uint32_t* local_color_table = NULL;

                if (local_color_table_flag){
                    (*current_printf_function)("theres a local color table\n");

                    local_color_table = malloc(sizeof(uint32_t) * entries_of_local_color_table);

                    for (int i = 0; i < entries_of_local_color_table; i++){
                        // just gonna assume 8bit again
                        unsigned char r = get_1(); 
                        unsigned char g = get_1(); 
                        unsigned char b = get_1();
                        
                        local_color_table[i] = b << 24 | g << 16 | r << 8 | 255 << 0;
                    }

                    // image->frame_buffer = local_color_table;
                    // image->width = sqrt(entries_of_local_color_table);
                    // image->height = sqrt(entries_of_local_color_table);
                    // return image;
                }

                (*current_printf_function)("started reading image data block at byte #%d\n", current_byte);

                uint8_t number_of_initial_lzw_bits = get_1(); // how many possible colors and also LZW size + 1 = how many bits you need
                
                uint8_t image_data_byte_length = get_1(); 

                uint16_t* decoded_color_codes = malloc(sizeof(uint16_t) * image_descriptor_width * image_descriptor_height);
                uint32_t decoded_color_code_count = 0;

                uint32_t* code_table = malloc(sizeof(uint32_t) * 4096 * 2);

                (*current_printf_function)("LZW initial bit size: %d\n", number_of_initial_lzw_bits);
                
                uint8_t* image_data_byte_array = NULL;
                uint32_t total_image_data_byte_length = 0;
                
                while (image_data_byte_length != 0){
                    total_image_data_byte_length += image_data_byte_length;

                    image_data_byte_array = realloc(image_data_byte_array, sizeof(uint8_t) * total_image_data_byte_length);
                    
                    for (int i = 0; i < image_data_byte_length; i++){
                        image_data_byte_array[total_image_data_byte_length - image_data_byte_length + i] = get_1();
                        (*current_printf_function)("%s ", lsb_byte_to_binary(image_data_byte_array[total_image_data_byte_length - image_data_byte_length + i], 8));
                    }
                    (*current_printf_function)("\n");

                    image_data_byte_length = get_1();
                } 

                uint8_t lzw_bit_count = number_of_initial_lzw_bits + 1;
                uint32_t current_bit = 0;

                uint8_t is_parsing = 1;

                uint32_t clear_code = 1 << number_of_initial_lzw_bits;
                uint32_t stop_code = clear_code + 1;

                uint32_t previous_code = stop_code; 

                uint32_t current_highest_defined_code = stop_code;

                (*current_printf_function)("clear code: %d stop code: %d\n", clear_code, stop_code);

                while (is_parsing){
                    uint32_t code = 0;
                                                                        
                    if (current_bit < total_image_data_byte_length * 8){
                        for (uint32_t i = current_bit; i < lzw_bit_count + current_bit; i++){
                            uint32_t current_byte = floor(i / 8);
                            uint32_t current_bit_in_byte = i % 8;
                        
                            code += msb_get_bit(image_data_byte_array[current_byte], current_bit_in_byte) * (1 << (i - current_bit));
                        }

                        current_bit += lzw_bit_count;
                    } else {
                        (*current_printf_function)("error: read past data length\n");
                        exit(1);
                    }

                    (*current_printf_function)("parsing code %s (%d)\n", msb_byte_to_binary(code, lzw_bit_count), (int)code);

                    if (code == clear_code){
                        (*current_printf_function)("clear code!\n");

                        for (int i = 0; i < 4096 * 2; i++){
                            code_table[i] = (uint32_t)(0) - 1;
                        }

                        current_highest_defined_code = stop_code;

                        lzw_bit_count = number_of_initial_lzw_bits + 1;
                    }
                    else if (code == stop_code){
                        (*current_printf_function)("stop code!\n");
                        is_parsing = 0;
                    }
                    else {                        
                        uint32_t deconstructed_code = code;

                        uint8_t can_standardly_add_code = 1;

                        // add decoded values
                        if (code < clear_code){
                            (*current_printf_function)("literal!\n");

                            decoded_color_codes[decoded_color_code_count++] = deconstructed_code;
                        }
                        else if (code_table[deconstructed_code * 2] == (uint32_t)(0) - 1){ // code is undefined                    
                            (*current_printf_function)("undefined representative!\n");

                            can_standardly_add_code = 0;

                            uint32_t first_code = previous_code;

                            while (first_code > clear_code){
                                if (code_table[first_code * 2] == ((uint32_t)-1)){
                                    printf("error: code points to undefined code\n");
                                    exit(1);
                                }

                                first_code = code_table[first_code * 2];
                            }

                            code_table[++current_highest_defined_code * 2] = previous_code;
                            code_table[current_highest_defined_code * 2 + 1] = first_code;

                            if (current_highest_defined_code == (uint64_t)((1 << lzw_bit_count) - 1)){
                                lzw_bit_count++;
                            }

                            uint16_t temp_decoded_color_count = 0;
                            uint16_t cur_allocated_temps = 1000;
                            uint32_t* temp_decoded_colors = malloc(sizeof(uint32_t) * cur_allocated_temps);

                            uint32_t deconstructed_prev_code = previous_code;
                            
                            if (deconstructed_prev_code < clear_code) decoded_color_codes[decoded_color_code_count++] = previous_code;

                            while (deconstructed_prev_code > clear_code){
                                if (code_table[deconstructed_prev_code * 2] == (uint32_t)(0) - 1){
                                    printf("error: code points to undefined code\n");
                                    exit(1);
                                }

                                if (temp_decoded_color_count >= cur_allocated_temps){
                                    cur_allocated_temps += 1000;

                                    temp_decoded_colors = realloc(temp_decoded_colors, sizeof(uint32_t) * cur_allocated_temps);
                                }

                                temp_decoded_colors[temp_decoded_color_count++] = code_table[deconstructed_prev_code * 2 + 1]; // add the literal
                            
                                if (code_table[deconstructed_prev_code * 2] < clear_code)
                                    temp_decoded_colors[temp_decoded_color_count++] = code_table[deconstructed_prev_code * 2];

                                deconstructed_prev_code = code_table[deconstructed_prev_code * 2];
                            }

                            for (int i = temp_decoded_color_count - 1; i >= 0; i--){
                                decoded_color_codes[decoded_color_code_count++] = temp_decoded_colors[i];
                            }

                            free(temp_decoded_colors);

                            decoded_color_codes[decoded_color_code_count++] = first_code;

                            (*current_printf_function)("added new code (from undefined code branch): code_table[%d * 2] == %d, %d\n", current_highest_defined_code, previous_code, first_code);
                        } 
                        else { // code is defined
                            (*current_printf_function)("defined representative! code_table[%d * 2] == %d, %d\n", code, code_table[deconstructed_code * 2], code_table[deconstructed_code * 2 + 1]);

                            uint16_t temp_decoded_color_count = 0;
                            uint16_t cur_allocated_temps = 1000;
                            uint32_t* temp_decoded_colors = malloc(sizeof(uint32_t) * cur_allocated_temps);

                            while (deconstructed_code > clear_code){
                                if (code_table[deconstructed_code * 2] == (uint32_t)(0) - 1){
                                    printf("error: code points to undefined code\n");
                                    exit(1);
                                }

                                if (temp_decoded_color_count >= cur_allocated_temps){
                                    cur_allocated_temps += 1000;

                                    temp_decoded_colors = realloc(temp_decoded_colors, sizeof(uint32_t) * cur_allocated_temps);
                                }

                                temp_decoded_colors[temp_decoded_color_count++] = code_table[deconstructed_code * 2 + 1]; // add the literal
                            
                                if (code_table[deconstructed_code * 2] < clear_code)
                                    temp_decoded_colors[temp_decoded_color_count++] = code_table[deconstructed_code * 2];

                                deconstructed_code = code_table[deconstructed_code * 2];
                            }

                            for (int i = temp_decoded_color_count - 1; i >= 0; i--){
                                decoded_color_codes[decoded_color_code_count++] = temp_decoded_colors[i];
                            }

                            free(temp_decoded_colors);
                        }

                        // create new code table entry
                        if (can_standardly_add_code && previous_code != clear_code){
                            uint32_t first_code = code;

                            while (first_code > clear_code){
                                if (code_table[first_code * 2] == (uint32_t)(0) - 1){
                                    printf("error: code points to undefined code\n");
                                    exit(1);
                                }

                                first_code = code_table[first_code * 2];
                            }

                            code_table[++current_highest_defined_code * 2] = previous_code;
                            code_table[current_highest_defined_code * 2 + 1] = first_code;

                            if (current_highest_defined_code == (uint64_t)((1 << lzw_bit_count) - 1)){
                                lzw_bit_count++;
                            }

                            (*current_printf_function)("added new code (from standard branch): code_table[%d * 2] == %d, %d\n", current_highest_defined_code, previous_code, first_code);
                        }
                    }

                    // for (uint32_t i = 0; i < decoded_color_code_count; i++){
                    //     if (!decoded_color_codes[i])(*current_printf_function)("██ ");
                    //     else (*current_printf_function)("░░ ");
                    
                    //     if ((i + 1) % image_descriptor_width == 0) (*current_printf_function)("\n");
                    // }

                    (*current_printf_function)("\n");

                    (*current_printf_function)("%d/%d colors decoded\n", decoded_color_code_count, image_descriptor_width * image_descriptor_height);
                        
                    previous_code = code;
                }

                if (decoded_color_code_count != image_descriptor_width * image_descriptor_height){
                    printf("error: number of decoded pixels is not equal to number of total pixels\n");
                }

                for (int y = 0; y < image_descriptor_height; y++){
                    for (int x = 0; x < image_descriptor_width; x++){
                        image->frame_buffer[(y + image_top_pos + image_height * (frame_count - 1)) * image_width + (x + image_left_pos)] = local_color_table_flag ? local_color_table[decoded_color_codes[y * image_width + x]] : global_color_table[decoded_color_codes[y * image_width + x]];
                    }
                }
                
                break;
            } 

            case 0x3B: { // ";" EOF 
                (*current_printf_function)("EOF at byte #%d\n", current_byte - 1);

                still_reading_file = 0;

                break;
            }
        }
    }

    return image;
}

int printf_override(const char *__restrict __format, ...){
    return 0;
}

PM_image* PM_load_image(const char *filename, unsigned char debug_mode){
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL){
        (*current_printf_function)("file does not exist !!!\n");

        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    file_buffer = malloc(size);

    fread(file_buffer, 1, size, fp);
    fclose(fp);

    // see what the file type is

    char* mutable_filename = malloc(sizeof(char) * (strlen(filename) + 1));
    strcpy(mutable_filename, filename);

    char *strtok_string = strtok(mutable_filename, ".");
    char *filetype_string;
    
    while(strtok_string != NULL) {
        filetype_string = strtok_string;
        strtok_string = strtok(NULL, ".");
    }

    // override (*current_printf_function) so there is no debug output
    if (!debug_mode){
        current_printf_function = printf_override;
    }

    if (!strcmp(filetype_string, "gif") || !strcmp(filetype_string, "GIF")){ // GIF file
        return PM_load_gif(debug_mode);
    } else 
    if (!strcmp(filetype_string, "bmp") || !strcmp(filetype_string, "BMP")){ // BMP file
        return PM_load_bitmap(debug_mode);
    }

    (*current_printf_function)("file is unreadable by PitMap\n");

    return NULL;
}   
