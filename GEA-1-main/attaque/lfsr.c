#include <stdio.h>
#include <stdlib.h>
#include "lfsr.h"

Table_f tab_f = {0,0,1,1,1,0,0,1,1,0,1,1,1,0,1,1,0,0,1,0,1,1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,0,0,0,1,1,1,1,0,1,1,0,1,0,0,0,1,1,1,1,0,0,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,0,1,1,0,1,0,1,1,0,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,0,1,0,1,0,0,1,0,1,1,1,0,0,0,0,0,1,1,1,1,1,1};

const uint8_t select_A[7] = {22, 0, 13, 21, 25, 2, 7};
const uint8_t select_B[7] = {12, 27, 0, 1, 29, 21, 5};
const uint8_t select_C[7] = {10, 30, 32, 3, 19, 0, 4};
const uint8_t select_S[7] = {3, 12, 22, 38, 42, 55, 63};

const uint64_t inverse_A = 0b0000000000000000000000000000000001011101110110001001101110001101UL;
const uint64_t inverse_B = 0b0000000000000000000000000000000011110001110000001111000001000101UL;
const uint64_t inverse_C = 0b0000000000000000000000000000000101010000111001101111101000100100UL;

static uint8_t table_parite[1U << 16];

uint8_t parite16(uint16_t v) {
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
}

void init_table_parite() {
    for (uint64_t i = 0; i < 1U << 16; i++)
        table_parite[i] = parite16(i);
}

uint8_t parite_bits_set(uint64_t v) {
    v ^= v >> 32;
    return table_parite[(v ^ (v >> 16)) & 0xffff] & 1;
}

void init_lfsr(Lfsr *lfsr, int id) {
    //lfsr->registre = 0UL;
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
        default :
            lfsr->longueur = TAILLE_C;
            lfsr->select = select_C;
            lfsr->inverse = inverse_C;
            lfsr->permutation = 32;
            break;
    }
    lfsr->masque = (1UL << (lfsr->longueur - 1)) - 1UL;
}

void lfsr_init(Lfsr *lfsr, uint64_t s) {
    lfsr->registre = 0UL;
    s = (s << lfsr->permutation) ^ (s >> (64 - lfsr->permutation));
    for (int i = 0; i < 64; i++) {
        lfsr->registre = ((lfsr->registre ^ (s >> i)) & 1) ? (((lfsr->registre >> 1) & lfsr->masque) ^ lfsr->inverse) : ((lfsr->registre >> 1) & lfsr->masque);
    }
    if (lfsr->registre == 0) {
        lfsr->registre = 1UL;
    }
}

void tour_registre(uint64_t galois, uint8_t tour, uint64_t* registre, uint8_t taille) {
    // la sortie + le bit de s qui entre
    #ifdef _WIN32
        uint64_t entree = registre[0] ^ (1ULL << tour);
    #else
        uint64_t entree = registre[0] ^ (1UL << tour);
    #endif
    for (uint8_t i = 0; i < taille - 1; i++) {
        // décalage
        registre[i] = registre[i + 1];
        // xor pour les cellules concernées
        if (((galois >> i) & 1) == 1)
            registre[i] ^= entree;
    }
    registre[taille - 1] = entree;
}

void tour_registre_sans_s(uint64_t galois, uint64_t* registre, uint8_t taille) {
    uint64_t entree = registre[0];
    for (uint8_t i = 0; i < taille - 1; i++) {
        registre[i] = registre[i + 1];
        if (((galois >> i) & 1) == 1)
            registre[i] ^= entree;
    }
    registre[taille - 1] = entree;
}

void init_lfsr_matrice(lfsr_init_matrice m, Lfsr *lfsr, uint8_t decalage) {
    for (uint8_t i = 0; i < lfsr->longueur; i++)
        m[i] = 0UL;

    for (uint8_t i = 0; i < 64; i++)
        tour_registre(lfsr->inverse, (i + decalage) % 64, m, lfsr->longueur);
}

void init_lfsr_opti(lfsr_opti *lfsr_o, Lfsr *lfsr, uint8_t decalage) {
    uint64_t *registre = malloc(sizeof(uint64_t) * lfsr->longueur);
    for (uint8_t i = 0; i < lfsr->longueur; i++)
        registre[i] = 0;

    for (uint8_t i = 0; i < 64; i++)
        tour_registre(lfsr->inverse, (i + decalage) % 64, registre, lfsr->longueur);

    uint64_t *registre_cpy = malloc(sizeof(uint64_t) * lfsr->longueur);
    for (uint8_t i = 0; i < lfsr->longueur; i++)
        registre_cpy[i] = registre[i];

    for (uint8_t i = 0; i < 72; i++) {
        for (uint8_t j = 0; j < 7; j++) {
            lfsr_o->matrice[i][j] = registre[lfsr->select[j]];
        }
        tour_registre_sans_s(lfsr->inverse, registre, lfsr->longueur);
    }
    free(registre);
    init_lfsr_matrice(lfsr_o->init_matrice, lfsr, decalage);
}

void output_xor_1_bit(lfsr_opti *lfsr, uint64_t s, U72 *output, uint8_t i) {
    uint8_t entree_f = sizeof(tuple);
    uint64_t *m = lfsr->matrice[i];
    entree_f = 0;
    for (int8_t j = 6; j >= 0; j--)
        entree_f ^= (parite_bits_set(m[j] & s)) << j;
    
    if (i < 32)
        output->n1 ^= (uint32_t) tab_f[entree_f] << (31 - i);
    else {
        if (i < 64)
            output->n2 ^= (uint32_t) tab_f[entree_f] << (63 - i);
        else
            output->n3 ^= tab_f[entree_f] << (71 - i);
    }
}

void output_24_bits_opti(lfsr_opti *lfsr, uint64_t s, uint32_t *output) {
    //*output = 0U;
    uint8_t entree_f;
    uint64_t *m;
    for (uint8_t i = 0; i < 24; i++) {
        entree_f = 0;
        m = lfsr->matrice[i];
        for (int8_t j = 6; j >= 0; j--)
            entree_f ^= (parite_bits_set(m[j] & s)) << j;
        *output ^= (uint32_t) tab_f[entree_f] << (31 - i);
    }
}

uint8_t outputF(uint64_t registre, const uint8_t select[7]) {
    // select[i] selectionne le bit xi
    uint8_t output = 0;
    for (int i = 6; i >= 0; i--)
        output = (output << 1) ^ ((registre >> select[i]) & 1);
    //Il faut que le bit le plus à gauche soit le bit non utilise.
    // output = 0-x0-x1-x2..x6
    return tab_f[output];
}

void clock_lfsr_s(uint64_t *registre, uint8_t nb_tours, uint64_t entree) {
    uint64_t s_mask = (1ULL << 63) - 1ULL;
    uint64_t sortie;
    for (int i = 0; i < nb_tours; i++) {
        sortie = outputF(*registre, select_S) ^ (*registre & 1) ^ ((entree >> i) & 1);
        *registre = ((*registre >> 1) & s_mask) ^ (sortie << 63);
    }
}

void output_72_bits(Lfsr *lfsr, U72 *output) {
    uint64_t sortie;
    output->n1 = 0U;
    output->n2 = 0U;
    output->n3 = 0;
    for (int k = 0; k < 32; k++) {
        output->n1 ^= (uint32_t) outputF(lfsr->registre, lfsr->select) << (31 - k);
        sortie = lfsr->registre & 1;
        lfsr->registre = (lfsr->registre >> 1) & lfsr->masque;
        if (sortie) {
            lfsr->registre ^= lfsr->inverse;
        }
    }
    for (int k = 0; k < 32; k++) {
        output->n2 ^= (uint32_t) outputF(lfsr->registre, lfsr->select) << (31 - k);
        sortie = lfsr->registre & 1;
        lfsr->registre = (lfsr->registre >> 1) & lfsr->masque;
        if (sortie) {
            lfsr->registre ^= lfsr->inverse;
        }
    }
    for (int k = 0; k < 8; k++) {
        output->n3 ^= outputF(lfsr->registre, lfsr->select) << (7 - k);
        sortie = lfsr->registre & 1;
        lfsr->registre = (lfsr->registre >> 1) & lfsr->masque;
        if (sortie) {
            lfsr->registre ^= lfsr->inverse;
        }
    }
}

uint64_t recuperation_cle(uint64_t S, uint32_t IV, uint8_t dir) {
    //clock à l'envers
    uint8_t b0, a0;
	for(int i = 0 ; i < 128 ; i++) {
		b0 = (S >> 63) & 1;
		S = (S << 1) & (((1ULL << 63) - 1) << 1);
		a0 = outputF(S, select_S);
		S ^= a0 ^ b0;
	}
    //clock à l'endroit
    uint64_t S33 = 0;
    clock_lfsr_s(&S33, 32, IV);
	clock_lfsr_s(&S33, 1, dir);
	uint64_t K = 0;
    uint8_t f, in, out;
	for(int i = 0 ; i < 64 ; i++) {
		f = outputF(S33, select_S);
        in = (S >> i) & 1;
        out = S33 & 1;
		S33 = ((S33 >> 1) & ((1ULL << 63) - 1ULL)) ^ ((uint64_t)in << 63);
		K ^= (uint64_t) (out ^ f ^ in) << i;
	}
	return K;
}

U72 gen_suite(uint64_t s, int display) {
    U72 z = {0u, 0u, 0};
    Lfsr A,B,C;
    init_lfsr(&A, 1);
    init_lfsr(&B, 2);
    init_lfsr(&C, 3);

    lfsr_init(&A, s);
    lfsr_init(&B, s);
    lfsr_init(&C, s);

    U72 output;

    output_72_bits(&B, &output);
    z.n1 ^= output.n1;
    z.n2 ^= output.n2;
    z.n3 ^= output.n3;
    output_72_bits(&A, &output);
    z.n1 ^= output.n1;
    z.n2 ^= output.n2;
    z.n3 ^= output.n3;
    output_72_bits(&C, &output);
    z.n1 ^= output.n1;
    z.n2 ^= output.n2;
    z.n3 ^= output.n3;

    if (display) {
        printf("s : ");
        printBin(s);
    }
    return z;
}