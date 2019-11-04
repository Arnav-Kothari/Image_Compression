/* bitpack.c 
 *
 * Implementation  file for bitpacking functionality of bitpack.h
 * Authors: Aryan Pandey and Arnav Kothari 
 * Date: 12/22/2019
 * COMP40: arith
 */

#include "bitpack.h"
#include <stdio.h>
#include <math.h>
#include "except.h"
#include "assert.h"

Except_T Bitpack_Overflow = { "Overflow packing bits" };

/* 
 * Function: Bitpack_fitsu
 * Purpose: Checks if a certain unsigned integer value fits in the given width
 * Parameters: Takes an unsigned integer value and a unsigned width
 * Returns: A boolean value
 * Expectations: Makes sure that width is in between 0 and 64
 */
bool Bitpack_fitsu(uint64_t n, unsigned width) 
{
    assert(width <= 64);
    if (width == 0) {
        return false;
    }
    
    /* Range is the maximum value that the unsigned int
     * can hold within the bounds of the width bits
     */
    uint64_t range = pow(2, width) - 1;
    if (n <= range) {
        return true;
    }
    return false;
}
/* 
 * Function: Bitpack_fitss
 * Purpose: Checks if a certain signed integer value fits in the given width
 * Parameters: Takes an signed integer value and a unsigned width
 * Returns: A boolean value
 * Expectations: Makes sure that width is in between 0 and 64
 */
bool Bitpack_fitss(int64_t n, unsigned width) 
{
    assert(width <= 64);
    if (width == 0) {
        return false;
    }

    int range = pow(2, width - 1) - 1;
    if (n >= (-range - 1) && n <= range) {
        return true;
    }
    return false;
}
/* 
 * Function: Bitpack_getu
 * Purpose: Returns unsigned value stored at particular field in a word
 * Parameters: An unsigned integer word of 64 bits, a unsigned value for width and lsb
 * Returns: The value retrieved from the word, an unsigned integer value of size 64 bits 
 * Expectations: Makes sure that width + lsb is less than or equal to 64
 */
uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
    assert(width + lsb <= 64);
    if (width == 0) {
        return 0;
    }
 
    /* Create mask to extract desired field */
    uint64_t mask = ~0;
    mask = mask >> (64 - width) << lsb;

    /* Extract value */
    word = word & mask; 
    word = word >> lsb;

    return word;
}

/* 
 * Function: Bitpack_gets
 * Purpose: Returns signed value stored at particular field in a word
 * Parameters: An signed integer word of 64 bits, a unsigned value for width and lsb
 * Returns: The value retrieved from the word, an signed integer value of size 64 bits 
 * Expectations: Makes sure that width + lsb is less than or equal to 64
 */
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb) 
{
    assert(width + lsb <= 64);
    if (width == 0) {
        return 0;
    }

    /* Use getu to get the actual bits in the field */
    int64_t signed_word = Bitpack_getu(word, width, lsb);

    /* Get value of the signed bit */
    int64_t sig_bit = signed_word >> (width - 1);

    /* If positive, return as is */
    if (sig_bit == 0) {
        return signed_word;
    }
    
    /* If negative, flip significant bits to represent
       negative number */
    uint64_t mask = ~0;
    mask = mask << width;
    signed_word = signed_word | mask;

    return signed_word;
}
/* 
 * Function: Bitpack_newu
 * Purpose: Stores an unsigned value into a word in designted field
 * Parameters: An original unsigned integer word of 64 bits, an unsinged 
 *             integer word that is going to be stored in the original 
 *             word, and a unsigned value for width and lsb
 * Returns: The original word with the new contents of the value
 * Expectations: Makes sure that width + lsb is less than or equal 64
 */
uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, uint64_t value) 
{
    assert(width + lsb <= 64);
    
    /* Raise overflow exception if value does not fit
     * within given width
     */
    if  (!Bitpack_fitsu(value, width)) {
        RAISE(Bitpack_Overflow);
    }

    uint64_t mask = ~0;
    mask = mask >> (64 - width) << lsb;

    /* Shift value into appropriate lsb position */
    value = value << lsb;

    /* Overwrite respective field of the mask with appropriate value */
    value = value & mask;

    /* Clear out respective field in the word with a mask */
    mask = ~mask; 
    word = word & mask;

    /* Overwrite appropriate field of the word with respective value
     * and return
     */
    return word | value;
}

/* 
 * Function: Bitpack_news
 * Purpose: Stores a signed value into a word in designated field
 * Parameters: An original unsigned integer word of 64 bits, an singed 
 *             integer word that is going to be stored in the original 
 *             word, and a unsigned value for width and lsb
 * Returns: The original word with the new contents of the value
 * Expectations: Makes sure that width + lsb is less than or equal 64
 */
uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb, int64_t value)
{
    assert(width + lsb <= 64);

    /* Checks if given value fits in the width range */
    if (!Bitpack_fitss(value, width)) {
        RAISE(Bitpack_Overflow);
    }
        assert(width + lsb <= 64);
    
    uint64_t mask = ~0;
    mask = mask >> (64 - width) << lsb;

    /* Shift value into appropriate lsb position */
    value = value << lsb;

    /* Overwrite respective field of the mask with appropriate value */
    value = value & mask;

    /* Clear out respective field in the word with a mask */
    mask = ~mask; 
    word = word & mask;

    /* Overwrite appropriate field of the word with respective value
     * and return
     */
    return word | value;
}