/*      uarray2b.c
 *
 *      Implementation file for Blocked UArray2
 *      Authors: Aryan Pandey and Sam Berman 
 *      Date: 10/9/2019
 *      COMP40: locality
 */

#include <uarray2b.h>
#include <uarray2.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>

#define T UArray2b_T

/* 
 * Struct to hold information about UArray2b
 * Contains - row/col size of array
 *            size of elements
 *            height and width of blocked array
 *            blocksize
 *            Pointer to a UArray2_T where data is stored
 */
struct UArray2b_T {
        int row;
        int col;
        int size;
        int blocksize;
        int b_width;
        int b_height;
        UArray2_T uarray2;
}; 

/* 
 * Function: UArray2b_new()
 * Purpose: Allocate enough memory for a 2D Blocked Uarray with the given
 *          dimensions and sizes. Return a pointer to the 
 *          incomplete struct so that the
 *          client cannot directly manipulate the structure, but can pass
 *          it back to other member functions to manipulate the created 
 *          structure.
 * Parameters: int width, int height, int size, int blocksize
 * Returns: Pointer to an incomplete UArray2b struct
 * Expectations: Height, width, blocksize and size are all 
 *               greater than zero, memory is allocated correctly.
 */
T UArray2b_new (int width, int height, int size, int blocksize) 
{
        assert((width > 0) && (height > 0) && 
               (size > 0) && (blocksize > 0));
        int block_w, block_h;
        
        /* Calculate height and width of the blocked abstraction */
        block_w = (width / blocksize);
        block_h = (height / blocksize);

        if (width % blocksize != 0) {
                block_w++;
        }
        if (height % blocksize != 0) {
                block_h++;
        }
        
        /* 
         * Elements of the block are going to be kept in the columns of the rows
         * Each row is going to represent a block.
         * Total elements in a block would therefore be blocksize * blocksize
         * and total blocks would be block_w * block_h
         * This abstraction works off the architecture of UArray2 and ensures
         * that blocks are in adjacent memory spaces
         */
        UArray2_T uarray2 = UArray2_new(blocksize * blocksize, 
                                        block_w * block_h, size);
        assert(uarray2 != NULL);

        /* Allocate space for UArray2b struct */
        T uarray2b = malloc(sizeof((*uarray2b)));
        assert(uarray2b != NULL);

        /* Assign data members */
        uarray2b->col = width;
        uarray2b->row = height;
        uarray2b->blocksize = blocksize;
        uarray2b->size = size;
        uarray2b->uarray2 = uarray2;
        uarray2b->b_width = block_w;
        uarray2b->b_height = block_h;

        return uarray2b;
}

/* 
 * Function: UArray2b_new_64K_block()
 * Purpose:  Allocates blocksize parameter to ensure that blocks are 
 *           as close to 64KB as possible and calls the new function to
 *           allocate enough memory for a 2D Blocked Uarray with the given 
 *           dimensions and sizes.
 * Parameters: int width, int height, int size
 * Returns: Pointer to an incomplete UArray2b struct
 * Expectations: Height, width and size are all greater than zero, memory is 
 *               allocated correctly.
 */
T UArray2b_new_64K_block(int width, int height, int size)
{      
        assert((width > 0) && (height > 0) && 
               (size > 0));
        
        int blocksize;

        if (size >= (64 * 1024)) {
                blocksize = 1;
        } else 
        {
                blocksize = sqrt ( (64 * 1024) / size );
        }

        return UArray2b_new(width, height, size, blocksize);
}

/* 
 * Function: UArray2b_free()
 * Purpose:  Frees all dynamically allocated memory from the UArray2b
 *           struct as well as the Stored UArray2 within it
 * Parameters: Double pointer to a struct UArray2b_T
 * Returns: Nothing
 * Expectations: None of the pointers pointing to NULL
 */
void UArray2b_free(T *array2b)
{
        assert(array2b != NULL && *array2b != NULL);

        UArray2_free(&((*array2b)->uarray2));

        free(*array2b);
}

/* 
 * Function: UArray2b_width()
 * Purpose:  Getter function for Array width
 * Parameters:  Pointer to a struct UArray2b_T
 * Returns: Array width
 * Expectations: Pointer not being NULL
 */
int UArray2b_width(T array2b)
{
        assert(array2b != NULL);

        return array2b->col;
}

/* 
 * Function: UArray2b_height()
 * Purpose:  Getter function for Array height
 * Parameters:  Pointer to a struct UArray2b_T
 * Returns: Array height
 * Expectations: Pointer not being NULL
 */
int UArray2b_height(T array2b)
{
        assert(array2b != NULL);

        return array2b->row;
}

/* 
 * Function: UArray2b_size()
 * Purpose:  Getter function for Array size
 * Parameters:  Pointer to a struct UArray2b_T
 * Returns: Size of individual element within the array
 * Expectations: Pointer not being NULL
 */
int UArray2b_size(T array2b)
{
        assert(array2b != NULL);

        return array2b->size;
}

/* 
 * Function: UArray2b_blocksize()
 * Purpose:  Getter function for blocksize
 * Parameters:  Pointer to a struct UArray2b_T
 * Returns: The blocksize being used as int
 * Expectations: Pointer not being NULL
 */
int UArray2b_blocksize(T array2b)
{
        assert(array2b != NULL);

        return array2b->blocksize;
}

/* 
 * Function: UArray2b_at()
 * Purpose:  Returns a void pointer pointing to the element
 *           being stored at the array dimensions passed 
 *           through to the function
 * Parameters:  Pointer to a struct UArray2b_T, int column and row
 * Returns: Void pointer to the object stored at given location
 * Expectations: Pointer not being NULL and columns and rows 
 *               being within bounds
 */
void *UArray2b_at(T array2b, int column, int row)
{
        assert(array2b != NULL);
        assert(column >= 0 && column < array2b->col);
        assert(row >= 0 && row < array2b->row);

        int bsize = array2b->blocksize;
        int ua2_col, ua2_row;

        /* get which block to visit */
        ua2_row = ((row / bsize) * array2b->b_width + (column / bsize)); 

        /* get which index of the chosen block to visit */
        ua2_col = (bsize * (row % bsize)) + (column % bsize);
        
        return UArray2_at(array2b->uarray2, ua2_col, ua2_row );
}

/* 
 * Function: UArray2b_map()
 * Purpose:  Visits every index of the stored array within the passed 
 *           UArray2b and performs the passed apply function within it.
 *           If index is supposed to be empty, it is skipped.
 * Parameters:  Pointer to a struct UArray2b_T, 
 *              Pointer to an apply function,
 *              void pointer to closure argument
 * Returns: Nothing
 * Expectations: Pointers not being NULL (closure can be NULL)
 */
void  UArray2b_map(T array2b, void apply(int col, int row, T uarray2b,
                                         void *elem, void *cl), void *cl)
{
        assert(array2b != NULL && apply != NULL);
        int block_h = array2b->b_height;
        int block_w = array2b->b_width;
        int bsize = array2b->blocksize;

        /* 
         * Iterates through each block stored in the UArray2 through the
         * first loop and through each index within the block through
         * the second loop.
         */
        for (int height = 0; height < (block_h * block_w); height++){
                for (int width = 0; width < (bsize * bsize); width++){
                        
                        /* Obtain 2D block index from 1D index */
                        int b_col = height % block_w;
                        int b_row = height / block_w;
                        
                        /* Obtain 2D array index from block indices */
                        int a_col = (b_col * bsize) + (width % bsize); 
                        int a_row = (b_row * bsize) + (width / bsize);
                        
                        /* If array indices are supposed to be empty, 
                         * do nothing
                         */
                        if (a_row >= array2b->row || a_col >= array2b->col) {
                                continue;
                        }      
                        apply(a_col, a_row, array2b, 
                              UArray2_at(array2b->uarray2,  width , height),
                                         cl);
                }
        }
}