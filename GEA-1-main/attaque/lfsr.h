#include "util.h"

#ifndef LFSR_H
#define LFSR_H

struct lfsr_t {
    uint8_t longueur;
    uint64_t registre;
    uint64_t inverse;
    uint64_t masque;
    const uint8_t *select;
    uint8_t permutation;
};

typedef struct lfsr_t Lfsr;

typedef uint64_t lfsr_init_matrice[33];

typedef struct {
    uint64_t matrice[72][7];
    lfsr_init_matrice init_matrice;
} lfsr_opti;

#define TAILLE_A 31
#define TAILLE_B 32
#define TAILLE_C 33

#define TABLE_SIZE 128
typedef uint8_t Table_f [TABLE_SIZE];

#define DECALAGE_A 0
#define DECALAGE_B 16
#define DECALAGE_C 32

uint8_t parite16(uint16_t v);

void init_table_parite();

uint8_t parite_bits_set(uint64_t v);

void init_lfsr(Lfsr *lfsr, int id);

void lfsr_init(Lfsr *lfsr, uint64_t s);

void tour_registre(uint64_t galois, uint8_t tour, uint64_t* registre, uint8_t taille);

void tour_registre_sans_s(uint64_t galois, uint64_t* registre, uint8_t taille);

void init_lfsr_matrice(lfsr_init_matrice m, Lfsr *lfsr, uint8_t decalage);

void init_lfsr_opti(lfsr_opti *lfsr_o, Lfsr *lfsr, uint8_t decalage);

void output_xor_1_bit(lfsr_opti *lfsr, uint64_t s, U72 *output, uint8_t i);

void output_24_bits_opti(lfsr_opti *lfsr, uint64_t s, uint32_t *output);

uint8_t outputF(uint64_t registre, const uint8_t select[7]);

void clock_lfsr_s(uint64_t *registre, uint8_t nb_tours, uint64_t entree);

void output_72_bits(Lfsr *lfsr, U72 *output);

uint64_t recuperation_cle(uint64_t S, uint32_t IV, uint8_t dir);

U72 gen_suite(uint64_t s, int display);

#endif
