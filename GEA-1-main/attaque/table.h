#include "lfsr.h"

#define NB_THREADS_MAX 32

#define NB_TABLE_TOTAL 256

struct shared_arg {
    char* dir;
    unsigned size;
    int* progres;
};

typedef struct param {
    uint64_t start;
    uint64_t end;
    struct shared_arg* shargs;
} param;

typedef struct {
    int *progres;
    uint32_t nb_tables;
} table_aff_args;

int compare(const void *t1, const void *t2);

void table_build_B(uint32_t v, char* dir, unsigned size);

void *buildTablesRange(void *arguments);

void affiche_barre_de_chargement(uint16_t progres, uint16_t nb_total);

void *thread_affichage(void* arg);

void table_build(int nb_threads, char* dir, unsigned size, uint32_t start, uint32_t end);

int generation_table(int argc, char** argv);
