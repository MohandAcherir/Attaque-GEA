#include <stdint.h>

#ifndef UTIL_H
#define UTIL_H

/* Structures d'entiers. */

//suppression du padding
#pragma pack(push)
#pragma pack(1)

typedef struct {
    uint16_t n1;
    uint8_t n2;
} U24;

#define UINT24_MAX ((1U << 24) - 1)

typedef struct {
    uint32_t n1;
    uint32_t n2;
    uint8_t n3;
} U72;

typedef struct {
    U72 out;
    U24 t;
} tuple;

#pragma pack(pop)

/* Fonctions d'affichage d'entiers en binaire. */

void printBin(uint64_t n);

void printBint(uint32_t n);

void printBin72(U72 n);

/* Fonctions utilisées pour gérer les paramètres du programme principal. */

int get_params_range(int argc, char** argv, int *arg_count, uint32_t *start, uint32_t *end);

int lecture_chaine(char* chaine, char baseT[24], char baseUb[32], char baseV[8]);

int mitm_params_get_IV(char* str, uint32_t *IV);

U72 gen_suite(uint64_t s, int display);

void compose_vecteur(uint64_t *vec, const uint64_t* base, int taille_base, uint32_t bits);

#endif
