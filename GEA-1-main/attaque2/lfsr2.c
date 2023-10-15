#include <stdio.h>
#include <stdlib.h>
#include "lfsr2.h"

// Bits qui sont envoyés en entrée de f.
const int select_A[7] = {22, 0, 13, 21, 25, 2, 7};
const int select_D[7] = {12, 23, 3, 0, 10, 27, 17};

// Bits qui sont xorés avec la sortie (différent que pour l'implémentation de GEA-1 car le dernier bit n'est pas à 1).
const uint64_t inverse_A = 0b0000000000000000000000000000000000011101110110001001101110001101UL;
const uint64_t inverse_D = 0b0000000000000000000000000000000000001010010110011010101111111001UL;

// Tableau représentant les monomes de la fonction f.
// Pour chaque monome le i-ème bit est à 1 si le i-ème bit d'entrée de f en fait partie.
const uint8_t fonction_f[NB_MONOMES_F] = {
    0b01100101, 0b01101001, 0b01100011, 0b01100110, 0b01001101, 0b01011010, 0b01101010, 0b00010101,
    0b00001101, 0b00001011, 0b01000101, 0b00010011, 0b01000011, 0b01000110, 0b01100100, 0b00101001,
    0b01010010, 0b00100110, 0b00001001, 0b00100001, 0b00001010, 0b00100010, 0b01000010, 0b00000101,
    0b00000010, 0b00001100, 0b00100100, 0b01000100, 0b00110000, 0b01100000, 0b00000100, 0b00001000,
    0b00100000
};

// Initialise un lfsr pour GEA-2, avec les bonnes valeurs initiales pour les bits du registres. 
void init_lfsr2(Lfsr2 *l, int indice) {
    for (int i = 0; i < 31; i++)
            l->bits[i] = 0ull;
    switch (indice) {   
        case 1:
        default:
            l->longueur = TAILLE_A;
            l->select = select_A;
            l->inverse = inverse_A;
            for (int i = TAILLE_D; i < TAILLE_D + TAILLE_A; i++)
                l->bits[i] ^= (1ull << i);
            break;
        case 2:
            l->longueur = TAILLE_D;
            l->select = select_D;
            l->inverse = inverse_D;
            for (int i = 0; i < TAILLE_D; i++)
                l->bits[i] ^= (1ull << i);
            break;
    }
}

// Tour du lfsr avec en sortie des entiers de 64 bits représentant des combinaisons linéaires de bits de l'état initial.
// Donne en sortie des produits de combinaisons linéaires (à développer donc).
void tout_lfsr2(Lfsr2 *l, Monome_f monomes[NB_MONOMES_F]) {
    // 1. Sélection des bits
    uint64_t select[7];
    for (int i = 0; i < 7; i++)
        select[i] = l->bits[l->select[i]];

    // 2. Tour du lfsr
    uint64_t sortie = l->bits[0];
    for (int i = 0; i < l->longueur - 1; i++) {
        l->bits[i] = l->bits[i + 1];
        if (((l->inverse >> i) & 1) == 1)
            l->bits[i] ^= sortie;
    }
    l->bits[l->longueur - 1] = sortie;

    // 3. Fonction f
    for (int i = 0; i < NB_MONOMES_F; i++)
        monomes[i].taille = 0;
    for (int i = 0; i < NB_MONOMES_F; i++) {
        uint8_t m = fonction_f[i];
        for (int j = 0; j < 7; j++) {
            if (((m >> j) & 1) == 1) {
                monomes[i].bits[monomes[i].taille] = select[j];
                monomes[i].taille++;
            }
        }
    }
}


// Algo d'énumération récursif
void enum_combinaisons_rec(Tab_renommage tab, int *indice_tab, uint64_t monome, int restants, int k, int taille_nb, int decalage) {
    if (restants == 0) {
        tab[*indice_tab] = (monome << decalage);
        (*indice_tab)++;
        return;
    }
    uint64_t m;
    for (int i = k; i < taille_nb - (restants - 1); i++) {
        m = monome;
        m ^= (1ull << i);
        enum_combinaisons_rec(tab, indice_tab, m, restants - 1, i + 1, taille_nb, decalage);
    }
}

void init_tab_renommage(Tab_renommage tab) {
    int indice = 0;
    for (int i = 1; i <= 4; i++)
        enum_combinaisons_rec(tab, &indice, 0ull, i, 0, TAILLE_D - NB_GUESS_D, NB_GUESS_D);
    for (int i = 1; i <= 4; i++)
        enum_combinaisons_rec(tab, &indice, 0ull, i, 0, TAILLE_A - NB_GUESS_A, NB_GUESS_A + TAILLE_D);
}
