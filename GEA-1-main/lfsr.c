#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "util.h"
#include "lfsr.h"

/**
 * Précalcul de f(x) pour x = {0,...,127} = (0b0000000,...,0b1111111)
*/
Table_f tab_f = {0,0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,0,0,1,0,1,1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,0,0,0,1,1,1,1,0,1,1,0,1,0,0,0,1,1,1,1,0,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,0,1,1,0,1,0,1,1,0,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,0,1,0,1,0,0,1,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1};

// Les bits qui sont sélectionnés pour être injectés dans f
const uint8_t select_A[7] = {22, 0, 13, 21, 25, 2, 7};
const uint8_t select_B[7] = {12, 27, 0, 1, 29, 21, 5};
const uint8_t select_C[7] = {10, 30, 32, 3, 19, 0, 4};
const uint8_t select_D[7] = {12, 23, 3, 0, 10, 27, 17};
const uint8_t select_S[7] = {3, 12, 22, 38, 42, 55, 63};

//const char select_init[7] = {3, 12, 22, 38, 42, 55, 63};
const uint64_t inverse_A = 0b0000000000000000000000000000000001011101110110001001101110001101UL;
const uint64_t inverse_B = 0b0000000000000000000000000000000011110001110000001111000001000101UL;
const uint64_t inverse_C = 0b0000000000000000000000000000000101010000111001101111101000100100UL;
const uint64_t inverse_D = 0b0000000000000000000000000000000000011010010110011010101111111001UL;



/**
 * Initialisation de la structure du lfsr en fontion de son id (peut être 1 (A), 2 (B), 3 (C) ou 4 (D)).
*/
void init_lfsr(Lfsr *lfsr, int id) {
    lfsr->registre = 0UL;
    switch (id) {
        case 1 :
            lfsr->longueur = TAILLE_A;
            lfsr->select = select_A;
            lfsr->inverse = inverse_A;
            lfsr->permutation = 0;
            break;
        case 2 :
            lfsr->longueur = TAILLE_B;
            lfsr->select = select_B;
            lfsr->inverse = inverse_B;
            lfsr->permutation = 48;
            break;
        case 3 :
            lfsr->longueur = TAILLE_C;
            lfsr->select = select_C;
            lfsr->inverse = inverse_C;
            lfsr->permutation = 32;
            break;
        case 4 :
        default :
            lfsr->longueur = TAILLE_D;
            lfsr->select = select_D;
            lfsr->inverse = inverse_D;
            lfsr->permutation = 0; // ???
    }
}

/**
 * Prend en entrée K,dir,IV et renvoie le vecteur d'initialisation s sur 64 bits
*/
uint64_t generateS(uint64_t K, uint8_t dir, unsigned int IV) {
    uint64_t registre = 0UL;
    uint64_t sortie;
    // insertion IV
    for (int i = 0; i < 32; i++) {
        sortie = (registre & 1) ^ outputF(registre, select_S) ^ (IV & 1);
        registre >>= 1;
        registre ^= sortie << 63;
        IV >>= 1;
    }
    // insertion dir
    sortie = ((registre & 1) ^ outputF(registre, select_S) ^ (dir & 1));
    registre >>= 1;
    registre ^= sortie << 63;
    for (int i = 0; i < 64; i++) {
        sortie = ((registre & 1) ^ outputF(registre, select_S) ^ (K & 1));
        registre >>= 1;
        registre ^= sortie << 63;
        K >>= 1;
    }
    // clock 128 fois sans input
    for (int i = 0; i < 128; i++) {
        sortie = ((registre & 1) ^ outputF(registre, select_S));
        registre >>= 1;
        registre ^= sortie << 63;
    }
    return registre;
}

/**
 * Renvoie le resultat par la fonction f en fonction des bits selectionnés du registre
*/
uint8_t outputF(uint64_t registre, const uint8_t select[7]) {
    // select[i] selectionne le bit xi
    uint8_t output = 0;
    for (int i = 6; i >= 0; i--) {
        output ^= (registre >> select[i]) & 1;
        output <<= 1;
    }
    output >>= 1; 
    //Il faut que le bit le plus à gauche soit le bit non utilisé.
    // output = 0-x0-x1-x2..x6
    return tab_f[output];
}

uint8_t f(uint8_t e) {
    return (((e >> 0) & 1) * ((e >> 2) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 3) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 1) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 1) & 1) * ((e >> 2) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 2) & 1) * ((e >> 3) & 1) * ((e >> 6) & 1))
    ^ (((e >> 1) & 1) * ((e >> 3) & 1) * ((e >> 4) & 1) * ((e >> 6) & 1))
    ^ (((e >> 1) & 1) * ((e >> 3) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 2) & 1) * ((e >> 4) & 1))
    ^ (((e >> 0) & 1) * ((e >> 2) & 1) * ((e >> 3) & 1))
    ^ (((e >> 0) & 1) * ((e >> 1) & 1) * ((e >> 3) & 1))
    ^ (((e >> 0) & 1) * ((e >> 2) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 1) & 1) * ((e >> 4) & 1))
    ^ (((e >> 0) & 1) * ((e >> 1) & 1) * ((e >> 6) & 1))
    ^ (((e >> 1) & 1) * ((e >> 2) & 1) * ((e >> 6) & 1))
    ^ (((e >> 2) & 1) * ((e >> 5) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 3) & 1) * ((e >> 5) & 1))
    ^ (((e >> 1) & 1) * ((e >> 4) & 1) * ((e >> 6) & 1))
    ^ (((e >> 1) & 1) * ((e >> 2) & 1) * ((e >> 5) & 1))
    ^ (((e >> 0) & 1) * ((e >> 3) & 1))
    ^ (((e >> 0) & 1) * ((e >> 5) & 1))
    ^ (((e >> 1) & 1) * ((e >> 3) & 1))
    ^ (((e >> 1) & 1) * ((e >> 5) & 1))
    ^ (((e >> 1) & 1) * ((e >> 6) & 1))
    ^ (((e >> 0) & 1) * ((e >> 2) & 1))
    ^ ((e >> 1) & 1)
    ^ (((e >> 2) & 1) * ((e >> 3) & 1))
    ^ (((e >> 2) & 1) * ((e >> 5) & 1))
    ^ (((e >> 2) & 1) * ((e >> 6) & 1))
    ^ (((e >> 4) & 1) * ((e >> 5) & 1))
    ^ (((e >> 5) & 1) * ((e >> 6) & 1))
    ^ ((e >> 2) & 1)
    ^ ((e >> 3) & 1)
    ^ ((e >> 5) & 1);
}

/**
 * Effectue un tour du lfsr (un tour normal).
 * Le bit issu de la fonction f est renvoyé dans un char.
*/
uint8_t clock_lfsr(Lfsr *lfsr) {
    // Récupération des 7 bits.
    uint8_t output = outputF(lfsr->registre, lfsr->select);
    uint8_t sortie = lfsr->registre & 1;
    lfsr->registre >>= 1;
    if (sortie) {
        lfsr->registre ^= lfsr->inverse;
    }
    return output;
}

/**
 * Tour particulier pour initialiser le registre à partir de s.
 * Prend en argument le bit de s qui est injecté pour ce tour.
*/
void clock_lfsr_init(Lfsr *lfsr, uint8_t bit_s) {
    uint8_t sortie = (lfsr->registre & 1) ^ bit_s;
    lfsr->registre >>= 1;
    if (sortie) {
        lfsr->registre ^= lfsr->inverse;
    }
}

/**
 * Prend en entrée la chaine secrete s = s63 ... s0 et initialise le lfsr avec
*/
void lfsr_init(Lfsr *lfsr, uint64_t s) {
    lfsr->registre = 0UL;
    uint8_t sortie;
    s = (s << lfsr->permutation) ^ (s >> (64 - lfsr->permutation));
    for (int i = 0; i < 64; i++) {
        sortie = (lfsr->registre & 1) ^ (s & 1);
        s >>= 1;
        lfsr->registre >>= 1;
        if (sortie) {
            lfsr->registre ^= lfsr->inverse; // inverse contient la remise du bit de sortie en premiere position
        }
    }
    if (lfsr->registre == 0UL) {
        lfsr->registre = 1UL;
    }
}


/**
 * Renvoie les bits de sortie composés par la fonction f
 * output1 = b0-b1-...-b63 output2 = 0-0..-0-b64
*/
void output_65_bits(Lfsr *lfsr, uint64_t *output1, uint8_t *output2) {
    uint64_t sortie;
    *output1 = 0UL;
    *output2 = 0UL;
    for (int k = 0; k < 64; k++) {
        *output1 ^= (uint64_t) outputF(lfsr->registre, lfsr->select) << (63 - k);
        sortie = lfsr->registre & 1;
        lfsr->registre >>= 1;
        if (sortie) {
            lfsr->registre ^= lfsr->inverse;
        }
    }
    // output1 contient les premiers 64 bits d'output
    *output2 = outputF(lfsr->registre, lfsr->select);
    // output2 contient le dernier bit d'output
    // la partie qui suit remet le lfsr dans un etat coherent, on peut l'enlever
    sortie = lfsr->registre & 1;
    lfsr->registre >>= 1;
    if (sortie) {
        lfsr->registre ^= lfsr->inverse;
    }
}

