/* compress40.c 
 * Authors: Aryan Pandey and Arnav Kothari
 * Date: 10/23/2019
 * 
 * Purpose: Takes in a FILE pointer and compresses or decompresses according 
 *          to 40image.c. Contains the compress and decompress functions that
 *          are used in 40image.c and uses several other functions to create 
 *          the compress and decompress functions. 
 */

#include "compress40.h"
#include "assert.h"
#include <stdlib.h>
#include <stdio.h>
#include "pnm.h"
#include "a2methods.h"
#include "bitpack.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "arith40.h"
#include <math.h>

const int DENOM = 30000;
// const unsigned SCALED_UINT = 9;
// const int SCALED_INT = 5;
// const unsigned INDEX_UINT = 4;

typedef struct Pixel_float {
    float red, green, blue;
} *Pixel_float;

typedef struct Component_vid {
    float y, pb, pr;
} *Component_vid;
    
typedef struct dct_elem {
    float average_pb, average_pr, a, b, c, d;
} *dct_elem;
typedef struct A2_with_methods {
    A2Methods_UArray2 a2;
    A2Methods_T methods;
} *A2_with_methods;

/* Compression functions */
void RGB_to_CV();
void CV_to_DCT();
void pack_and_print();
void Write_to_disk();

/* Helper function for compression */
void RGB_to_float(Pnm_rgb rgb, Pixel_float pix, float denom);

unsigned int quantize_Y(float y);
int quantize_coef(float x);

/* Decompression functions */
void CV_to_RGB();
void DCT_to_CV(int i, int j, A2Methods_UArray2 cv_array,
               A2Methods_Object *elem, void *cl);

/* Helper functions for decompression */
void float_to_RGB(Pnm_rgb rgb, Pixel_float pix);

void print_float(int i, int j, A2Methods_UArray2 image, 
                  A2Methods_Object *elem, void *cl);
void Read_from_disk(int i, int j, A2Methods_UArray2 image, 
                  A2Methods_Object *elem, void *cl);
float get_Y(unsigned y);
float get_coef(int x);

/*
 * Function: compress40
 * Purpose: Takes in a FILE pointer and compresses the data using bitpacking
 *          inside that file.
 * Parameters: Takes a FILE pointer for input
 * Returns: Void
 * Expectations: Asserts that the map and methods pointers are not NULL.
 *               After a new cv_array and dct is created and dct is malloced, 
 *               asserts that the pointers to those objects are not NULL.
 */
void compress40(FILE *input) {
    A2Methods_T methods = uarray2_methods_plain;
    assert(methods != NULL);

    A2Methods_mapfun *map = methods->map_row_major;
    assert(map != NULL);

    Pnm_ppm image;
    image = Pnm_ppmread(input, methods);
    //

    A2Methods_UArray2 cv_array = methods->new(image->width, image->height, 
                                   sizeof(struct Component_vid));
    assert(cv_array != NULL);
    
    map(cv_array, RGB_to_CV, image);

    A2Methods_UArray2 dct_array = methods->new(image->width / 2, image->height / 2, sizeof(struct dct_elem));
    assert(dct_array != NULL);
    A2_with_methods dct = malloc(sizeof(*dct));
    assert(dct != NULL);
    dct->a2 = dct_array;
    dct->methods = methods;
    
    printf("COMP40 Compressed image format 2\n%u %u", 816, 458);
    printf("\n");
    map(cv_array, CV_to_DCT, dct);
    //map(dct_array, print_float, NULL);
    map(dct_array, pack_and_print, methods);
    // Pnm_ppmwrite(stdout, image);
    Pnm_ppmfree(&image);
    methods->free(&cv_array);
    methods->free(&dct_array);
    free(dct);
}
/* RGB_to_CV
 * Input: Two ints to represent col and row
 *        A pointer to a A2Methouds_Uarray2
 *        A pointer to element in the current index
 *        A pointer to a Pnm_ppm object as closure
 * Does:  It is an apply function that changes the pixels
 *        currently stored as RGB values to component video 
 *        representation and stores it in cv_array for 
 *        every pixel index
 * Returns: Nothing
 */
void RGB_to_CV(int i, int j, A2Methods_UArray2 cv_array,
               A2Methods_Object *elem, void *cl) 
{
    (void) cv_array;
    assert(elem != NULL && cl != NULL);
    
    /* Expects a pointer to a Pnm_ppm as closure */
    Pnm_ppm image = cl;

    /* Assert correct pointer assigned */
    assert(sizeof(*image) == sizeof(struct  Pnm_ppm));
    
    /* Assign rgb_pix to contain pixel values at location i, j */
    Pnm_rgb rgb_pix = image->methods->at(image->pixels, i, j);

    /* Declare new struct that stores floats */
    Pixel_float float_pix = malloc(sizeof(*float_pix));
    assert(float_pix != NULL);

    RGB_to_float(rgb_pix, float_pix, image->denominator);
    
    /* Set new struct pointer to current elem in array */
    Component_vid pixels = elem;
    pixels->y = 0.299 * float_pix->red + 0.587 * float_pix->green + 0.114 * float_pix->blue;
    pixels->pb = -0.168736 * float_pix->red - 0.331264 * float_pix->green + 0.5 * float_pix->blue;
    pixels->pr = 0.5 * float_pix->red - 0.418688 * float_pix->green - 0.081312 * float_pix->blue;

    free(float_pix);
}

void RGB_to_float(Pnm_rgb rgb, Pixel_float pix, float denom) 
{
    float r, g, b;

    r = rgb->red;
    g = rgb->green;
    b = rgb->blue;
    
    r = r / denom;
    g = g / denom;
    b = b / denom;

    pix->red = r;
    pix->green = g;
    pix->blue = b;
}
void CV_to_DCT(int i, int j, A2Methods_UArray2 cv_array,
               A2Methods_Object *elem, void *cl)
{
    (void) elem;
    A2_with_methods DCT = cl;
    A2Methods_UArray2 dct_array = DCT->a2;
    A2Methods_T methods = DCT->methods;

    if (i % 2 == 0 && j % 2 == 0){
        float pb_sum = 0, pr_sum = 0;
        float y1, y2, y3, y4;

        Component_vid cv_elem;
        
        cv_elem = methods->at(cv_array, i, j);
        pb_sum += cv_elem->pb;
        pr_sum += cv_elem->pr;
        y1 = cv_elem->y;

        cv_elem = methods->at(cv_array, i+1, j);
        pb_sum += cv_elem->pb;
        pr_sum += cv_elem->pr;
        y2 = cv_elem->y;

        cv_elem = methods->at(cv_array, i, j+1);
        pb_sum += cv_elem->pb;
        pr_sum += cv_elem->pr;
        y3 = cv_elem->y;

        cv_elem = methods->at(cv_array, i+1, j+1);
        pb_sum += cv_elem->pb;
        pr_sum += cv_elem->pr;
        y4 = cv_elem->y;

        dct_elem element = methods->at(dct_array, i / 2, j / 2);
        element->average_pb = pb_sum / 4.0;
        element->average_pr = pr_sum / 4.0;
        element->a = (y4 + y3 + y2 + y1) / 4.0;
        element->b = (y4 + y3 - y2 - y1)/ 4.0;
        element->c = (y4 - y3 + y2 - y1) / 4.0;
        element->d = (y4 - y3 - y2 + y1) / 4.0;
    }
}
void pack_and_print(int i, int j, A2Methods_UArray2 dct_array, A2Methods_Object *elem, void *cl)
{
    (void) dct_array;
    (void) cl;
    (void) i;
    (void) j;
    assert(dct_array != NULL);
    assert(elem != NULL);
    assert(cl != NULL);
    dct_elem element = elem;
    unsigned a = quantize_Y(element->a);
    int b = quantize_coef(element->b);
    int c = quantize_coef(element->c);
    int d = quantize_coef(element->d);

    unsigned pb = Arith40_index_of_chroma(element->average_pb);
    unsigned pr = Arith40_index_of_chroma(element->average_pr);

    int32_t word = 0;
    word = Bitpack_newu(word, 4, 0, pr);
    word = Bitpack_newu(word, 4, 4, pb);
    word = Bitpack_news(word, 5, 8, d);
    word = Bitpack_news(word, 5, 13, c);
    word = Bitpack_news(word, 5, 18, b);
    word = Bitpack_newu(word, 9, 23, a);

    char c0, c1, c2, c3;
    c0 = Bitpack_getu(word, 8, 0);
    c1 = Bitpack_getu(word, 8, 8);
    c2 = Bitpack_getu(word, 8, 16);
    c3 = Bitpack_getu(word, 8, 24);
    putchar(c3);
    putchar(c2);
    putchar(c1);
    putchar(c0);
    //printf("\n");
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CV_to_RGB (int i, int j, A2Methods_UArray2 cv_array,
               A2Methods_Object *elem, void *cl) 
{
    (void) cv_array;
    Pnm_ppm image = cl;
    Component_vid component_pixels = elem;

    Pixel_float float_pixels = malloc(sizeof(*float_pixels));
    assert(float_pixels != NULL);

    float_pixels->red = (float)1.0 * component_pixels->y + (float)0.0 * component_pixels->pb + (float)1.402 * component_pixels->pr;
    float_pixels->green = (float)1.0 * component_pixels->y - (float)0.344136 * component_pixels->pb - (float)0.714136 * component_pixels->pr;
    float_pixels->blue = (float)1.0 * component_pixels->y + (float)1.772 * component_pixels->pb + (float)0.0 * component_pixels->pr;

    if (float_pixels->red > 1.0) {
        float_pixels->red = 1.0;
    }
    if (float_pixels->red < 0){
        float_pixels->red = 0.0;
    }
    if (float_pixels->blue > 1.0) {
        float_pixels->blue = 1.0;
    }
    if (float_pixels->blue < 0){
        float_pixels->blue = 0.0;
    }
    if (float_pixels->green > 1.0) {
        float_pixels->green = 1.0;
    }
    if (float_pixels->green < 0){
        float_pixels->green = 0.0;
    }

    Pnm_rgb rgb_pixels = image->methods->at(image->pixels, i, j);
    float_to_RGB(rgb_pixels, float_pixels);

    free(float_pixels);
}
void float_to_RGB(Pnm_rgb rgb, Pixel_float pix) 
{

    unsigned int r, g, b;
    r = (pix->red * DENOM);
    g = (pix->green * DENOM);
    b = (pix->blue * DENOM);

    
    if (pix->red * DENOM - (float)r >= 0.5) {
        r++;
    }
    if (pix->green * DENOM - (float)g >= 0.5) {
        g++;
    }
    if (pix->blue * DENOM - (float)b >= 0.5) {
        b++;
    } 
    /*
    FILE *fp = fopen("test.txt", "a");
    assert(fp);
    fprintf(fp, "%u %u %u\n", r,g,b);
    fclose(fp); */

    rgb->red = r;
    rgb->green = g;
    rgb->blue = b;
}
/*
 * Function: decompress40
 * Purpose: Takes in a FILE pointer and decompresses the data by reversing
 *          and counteracting the operations used to compress the file. 
 * Parameters: Takes a FILE pointer for input
 * Returns: Void
 * Expectations: Asserts that the map and methods pointers are not NULL.
 *               Also, after a new cv_array, dct_array, and rgb_array are made,
 *               and dct and pixmap is malloced, asserts that the pointers to 
 *               those objects are not NULL.
 */
void decompress40(FILE *input) {

    A2Methods_T methods = uarray2_methods_plain;
    assert(methods != NULL);

    A2Methods_mapfun *map = methods->map_row_major;
    assert(map != NULL);

    unsigned height, width;
    int read = fscanf(input, "COMP40 Compressed image format 2\n%u %u", &width, &height);
    assert(read == 2);
    int c = getc(input);
    assert(c = '\n');

    A2Methods_UArray2 rgb_array = methods->new(width, height, sizeof(struct Pnm_rgb));
    assert(rgb_array != NULL);
    Pnm_ppm pixmap = malloc(sizeof(*pixmap));
    assert(pixmap != NULL);
    pixmap->width = width;
    pixmap->height = height;
    pixmap->denominator = DENOM;
    pixmap->methods = methods;
    pixmap->pixels = rgb_array;

    A2Methods_UArray2 dct_array = methods->new(width / 2, height / 2, sizeof(struct dct_elem));
    assert(dct_array != NULL);
    A2Methods_UArray2 cv_array = methods->new(width, height, sizeof(struct Component_vid));
    assert(cv_array != NULL);

    map(dct_array, Read_from_disk, input);
    //map(dct_array, print_float, NULL);

    A2_with_methods dct = malloc(sizeof(*dct));
    assert(dct != NULL);
    dct->a2 = dct_array;
    dct->methods = methods;

    map(cv_array, DCT_to_CV, dct);

    map(cv_array, CV_to_RGB, pixmap);

    Pnm_ppmwrite(stdout, pixmap);

    Pnm_ppmfree(&pixmap);

    methods->free(&cv_array);
    methods->free(&dct_array);
}
/*
 * Function: Read_from_disk
 * Purpose: apply function that is use to map through the disk and read data 
 * Parameters: integer value for index i and j, pointer to A2Methods_UArray2 
 *             image object, pointer to the element, and a void pointer for the
 *             closure
 *             
 * Returns: Void
 * Expectations: Asserts that the read is of size 6.
 */
void Read_from_disk(int i, int j, A2Methods_UArray2 image, 
                  A2Methods_Object *elem, void *cl) 
{
    (void) i;
    (void) j;
    (void) image;
    
    FILE *input = cl;
    dct_elem element = elem;
 //   uint32_t word = 0;
    char c0 = fgetc(input);
    char c1 = fgetc(input);
    char c2 = fgetc(input);
    char c3 = fgetc(input);


    uint32_t lit_word = 0;
    lit_word = Bitpack_news(lit_word, 8, 0, c3);
    lit_word = Bitpack_news(lit_word, 8, 8, c2);
    lit_word = Bitpack_news(lit_word, 8, 16, c1);
    lit_word = Bitpack_news(lit_word, 8, 24, c0);

    //printf("%d\n", lit_word);

    float a = get_Y(Bitpack_getu(lit_word, 9, 23));
    //printf("%lu\n", Bitpack_getu(lit_word, 9, 23));
    float b = get_coef(Bitpack_gets(lit_word, 5, 18));
    float c = get_coef(Bitpack_gets(lit_word, 5, 13));
    float d = get_coef(Bitpack_gets(lit_word, 5, 8));

    float pb = Arith40_chroma_of_index(Bitpack_getu(lit_word, 4, 4));
    float pr = Arith40_chroma_of_index(Bitpack_getu(lit_word, 4, 0));

    element->a = a;
    element->b = b;
    element->c = c;
    element->d = d;
    element->average_pb = pb;
    element->average_pr = pr;

}

void DCT_to_CV(int i, int j, A2Methods_UArray2 cv_array,
               A2Methods_Object *elem, void *cl)
{
    (void) elem;
    A2_with_methods DCT = cl;
    A2Methods_UArray2 dct_array = DCT->a2;
    A2Methods_T methods = DCT->methods;

    if (i % 2 == 0 && j % 2 == 0){
    
        Component_vid cv_elem;
        dct_elem dct_element = methods->at(dct_array, i / 2, j / 2);

        float a = dct_element->a;
        float b = dct_element->b;
        float c = dct_element->c;
        float d = dct_element->d;
        
        cv_elem = methods->at(cv_array, i, j);
        cv_elem->pb = dct_element->average_pb;
        cv_elem->pr = dct_element->average_pr;
        cv_elem->y = a - b - c + d;

        cv_elem = methods->at(cv_array, i+1, j);
        cv_elem->pb = dct_element->average_pb;
        cv_elem->pr = dct_element->average_pr;
        cv_elem->y = a - b + c - d;

        cv_elem = methods->at(cv_array, i, j+1);
        cv_elem->pb = dct_element->average_pb;
        cv_elem->pr = dct_element->average_pr;
        cv_elem->y = a + b - c - d;

        cv_elem = methods->at(cv_array, i+1, j+1);
        cv_elem->pb = dct_element->average_pb;
        cv_elem->pr = dct_element->average_pr;
        cv_elem->y = a + b + c + d;
    }
}



void print_float(int i, int j, A2Methods_UArray2 image, 
                  A2Methods_Object *elem, void *cl) {
    (void) i;
    (void) j;
    (void) image;
    (void) cl;
    
    dct_elem pixel = elem;
    if(i == 0 && j == 0) {
    printf("COMP40 Compressed image format 2\n%u %u", 816, 458);
    printf("\n");
    }
    printf("%f  %f  %f  %f  %f  %f\n", pixel->a, pixel->b, pixel->c, pixel->d, pixel->average_pb, pixel->average_pr);
}
/*void print_float(int i, int j, A2Methods_UArray2 image, 
                  A2Methods_Object *elem, void *cl) {
    (void) i;
    (void) j;
    (void) image;
    (void) cl;
    
    Component_vid pixel = elem;
    if(i == 0 && j == 0) {
    printf("COMP40 Compressed image format 2\n%u %u", 816, 458);
    printf("\n");
    }
    printf("%f  %f  %f \n", pixel->y, pixel->pb, pixel->pr);
}
*/

unsigned int quantize_Y(float y)
{
    return round(y * 511);
}

int quantize_coef(float x)
{
    if (x < -0.3) {
        x = -0.3;
    }
    if (x > 0.3) {
        x = 0.3;
    }
    return round(x * 50);
}

float get_Y(unsigned y)
{
    return (float)y /(float) 511;
}

float get_coef(int x)
{
    return (float)x / (float)50;
}