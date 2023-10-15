#include <stdio.h>
#include <stdint.h>


void printBin(uint64_t n) {
  for (int i = 0; i < 64; i++) {
    printf("%c", ((n >> 63 - i) & 1) ? '1' : '0');
  }
  printf("\n");
}

void printBint(uint32_t n) {
  for (int i = 0; i < 32; i++) {
    printf("%c", ((n >> 63 - i) & 1) ? '1' : '0');
  }
  printf("\n");
}