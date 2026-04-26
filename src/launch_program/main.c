#include "../headers/pitmap.h"
#include <stdio.h>

int main(){
    PM_image* image = PM_load_image("images/rgb.bmp", PM_BGRA, 0);

    printf("\n");

    PM_free_image(image);

    return 0;
}