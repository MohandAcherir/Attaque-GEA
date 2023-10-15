#include <stdint.h>

typedef struct param {
  uint64_t start;
  uint64_t end;
} param;

/**
 * Représente une entrée du tableau Tab[v]
 * 64 bits (long) + 1 bit (char)=65bits (l) pour les bits d'output 
 * 64 bits pour stocker le vecteur t
*/
typedef struct tuple {
  uint64_t out1;
  uint8_t out2;
  unsigned int t;
} tuple;

int compare(const void *t1, const void *t2);

uint64_t vectorDict(uint64_t);

void buildTableB(uint64_t v);

void *buildTablesRange(void *arguments);

void buildTables();