/***************************************************************
pmilja01,akoth02
ppmdiff.c
Things left to do:
- FIX MEMORY LEAK associated with methods main
- Fix "bogus EOF" error.
- Ensure matches spec
- Test different files to make sure the calcualte function is working
- Try to make more modular if possible
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pnm.h>
#include <assert.h>
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include <math.h>

float calculate_E(Pnm_ppm image1, Pnm_ppm image2);
int find_smaller_dim(Pnm_ppm image1,Pnm_ppm image2, int width);

int main (int argc, char* argv[]){
    assert(argc == 3);
    char *dash = "-";
    assert(argv[1] != dash && argv[2] != dash);
    FILE *image1;
    FILE *image2;
    if(argv[1] == dash){
        image1 = stdin;
        image2 = fopen(argv[2],"r");
    }
    else if(argv[2] == dash){
        image2 = stdin;
        image1 = fopen(argv[1],"r");
    }
    else{
        image1 = fopen(argv[1],"r");
        image2 = fopen(argv[2],"r");
    }

    /* default to UArray2 methods */
    A2Methods_T methods = uarray2_methods_plain;
    assert(methods);

    /* default to best map */
    A2Methods_mapfun *map = methods->map_default;
    assert(map);


    Pnm_ppm image1_ppm = Pnm_ppmread(image1,methods);
    Pnm_ppm image2_ppm = Pnm_ppmread(image2,methods);

    float E = calculate_E(image1_ppm,image2_ppm);
    printf("%.4f \n", E);
    Pnm_ppmfree(&image1_ppm);
    Pnm_ppmfree(&image2_ppm);
    fclose(image1);
    fclose(image2);
}

float calculate_E(Pnm_ppm image1, Pnm_ppm image2){
    float sum = 0;
    int width = find_smaller_dim(image1,image2,1);
    int height = find_smaller_dim(image1,image2,0);
    for (int i = 0; i < width; i++){
        for (int j = 0; j < height; j++){
            Pnm_rgb rgb1 = image1->methods->at(image1->pixels,i,j);
            Pnm_rgb rgb2 = image2->methods->at(image2->pixels,i,j);
            /* Write a function for the below */
            int red1 = (rgb1)->red;
            int red2 = (rgb2)->red;
            int blue1 = (rgb1)->blue;
            int blue2 = (rgb2)->blue;
            int green1 = (rgb1)->green;
            int green2 = (rgb2)->green;
            float val = pow(red2-red1,2) + pow(green2-green1,2) + pow(blue2-blue1,2);
            sum += val;
        }
    }
    float denominator = 3*width*height;
    float E = sqrt(sum/denominator); 

    return E;
}

int find_smaller_dim(Pnm_ppm image1,Pnm_ppm image2, int width){
    int i;
    int i1;
    int i2;
    if(width){
        i1 = image1->width;
        i2 = image2->width;
    }
    else{
        i1 = image1->height;
        i2 = image2->height;
    }
    if(i1 < i2){
        i = i1;
    }
    else{
        i = i2;
    }
    return i;
}
