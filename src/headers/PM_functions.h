#ifndef PM_FUNCTIONS_H
#define PM_FUNCTIONS_H

#include "PM_structs.h"

// loads an image file and returns a PM_image
PM_image* PM_load_image(const char* filename, PM_color_type color_type, unsigned char debug_mode);

// unallocated a PM_image*
void PM_free_image(PM_image* image);

#endif
