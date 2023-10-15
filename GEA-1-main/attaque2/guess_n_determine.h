#include <stdint.h>

typedef struct monomes {
  uint64_t mono;
  struct monomes *suivant;
} monomes;