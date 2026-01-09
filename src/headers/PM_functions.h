#ifndef PM_FUNCTIONS_H
#define PM_FUNCTIONS_H

// loads an image file and returns a PM_image
PM_image* PM_load_image(const char* filename, unsigned char debug_mode);

// unallocated a PM_image*
void PM_free_image(PM_image* image);

#endif
