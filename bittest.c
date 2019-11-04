#include "bitpack.h"

#include <stdio.h>
int main ()
{
  uint64_t word = 1;
  unsigned w = 4;
  unsigned lsb = 4;
  uint64_t val = 12;
  unsigned w2 = 5;
  unsigned lsb2 = 10;
  if(Bitpack_getu(Bitpack_newu(word,0,lsb,val),w,lsb) == val){
    printf("we out here baby");
  }
  if (Bitpack_getu(Bitpack_newu(word,w,lsb,val), w2, lsb2) == Bitpack_getu(word,w2,lsb2)) {
    printf("We on the floor\n");
  }

    return 0;
}