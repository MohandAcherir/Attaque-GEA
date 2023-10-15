#include <stdlib.h>
#include <stdio.h>
#include "lfsr.h"

typedef struct {
    char* tab;
    unsigned h;
    unsigned l;
} Matrice;

#define DECALAGE_A 0
#define DECALAGE_B 16
#define DECALAGE_C 32

// Utilisée par calcul_matrice
void tour_registre(Matrice galois, unsigned tour, long* registre) {
    // la sortie + le bit de s qui entre
    long entree = registre[0] ^ (1L << tour);
    for (unsigned j = 0; j < galois.h - 1; j++) {
        // décalage
        registre[j] = registre[j + 1];
        // xor pour les cellules concernées
        if (galois.tab[j] == 1)
            registre[j] ^= entree;
    }
    // la dernière cellule ne peut évidemment pas être xorée
    registre[galois.h - 1] = entree;
}

/**
 * Prend en argument un vecteur (matrice h * 1, h <= 64) représentant la structure d'un lfsr, et une matrice.
 * Calcule la matrice M en fonction du vecteur (M * s = r), en fonction du nombre de tours demandés.
 * Les matrices doivent être allouées, les dimensions doivent être initialisés et avoir des valeurs cohérentes.
*/
void calcul_matrice(Matrice galois, Matrice M, unsigned tours, int decalage_s) {
    // 'registre' indique pour chaque cellule du registre quels sont les bits de s
    // qu'il faut xorer entre eux pour obtenir sa valeur après l'initialisation.
    // L'information est sous forme d'un long, avec un 1 pour les bits de s à prendre.
    long* registre = malloc(sizeof(long) * galois.h);
    for (unsigned i = 0; i < galois.h; i++)
        registre[i] = 0;
    
    // Calcul de 'registre'
    for (unsigned i = 0; i < tours; i++)
        // Un tour du registre est simulé :
        // Un nouveau bit de s entre en jeu, et xoré avec la sortie pour être réinjecté au début,
        // et xorés avec les cellules indiqués dans 'galois'.
        tour_registre(galois, (i + decalage_s) % tours, registre);

    // Génération de la matrice à partir du contenu de 'registre'.
    for (unsigned i = 0; i < M.h; i++) {
        for (unsigned j = 0; j < M.l; j++)
            M.tab[i * M.l + j] = (registre[i] >> j) & 1;
    }
}

void affiche_matrice(Matrice m) {
    for (int i = 0; i < m.h; i++) {
        for (int j = 0; j < m.l; j++)
            printf("%d", m.tab[i * m.l + j]);
        printf("\n");
    }
}

/**
 * Produit de deux matrice, ne contenant que des bits (0 ou 1).
 * Les dimensions doivent être cohérentes et les tableaux alloués.
*/
Matrice produit_matrice(Matrice a, Matrice b) {
    Matrice result;
    result.h = a.h;
    result.l = b.l;
    result.tab = malloc(result.h * result.l);
    for (unsigned i = 0; i < a.h; i++) {
        for (unsigned j = 0; j < b.l; j++) {
            char res = 0;
            for (unsigned k = 0; k < a.l; k++)
                res ^= a.tab[i * a.l + k] & b.tab[k * b.l + j];
            result.tab[i * result.l + j] = res;
        }
    }
    return result;
}

// int main(int argc, char **argv) {
//     //vérification du fonctionnement

//     Lfsr la;
//     init_lfsr(&la, 1);

//     Matrice galois = {malloc(TAILLE_A), TAILLE_A, 1};
//     for (int i = 0; i < TAILLE_A; i++)
//         galois.tab[i] = (la.inverse >> i) & 1;
//     affiche_matrice((Matrice){galois.tab, galois.l, galois.h});
//     printf("\n");

//     Matrice Ma = {malloc(galois.h * 64), galois.h, 64};

//     calcul_matrice(galois, Ma, 64, DECALAGE_A);
    
//     Matrice s = { (char[64]){0,1,0,1,1,0,1,0,1,0,1,0,1,1,0,0,1,1,0,
//     0,0,1,0,1,1,1,0,1,0,0,1,0,0,0,0,1,0,1,1,0,1,0,1,1,1,1,0,0,1,0,1,
//     0,1,1,1,1,1,0,0,1,0,1,0,1},
//     64, 1};

//     //calcul du registre en faisant les tours
//     for (unsigned i = 0; i < s.h; i++)
//         clock_lfsr_init(&la, s.tab[i]);

//     //calcul du registre en faisant le produit des matrices
//     Matrice registre = produit_matrice(Ma, s);

//     affiche_matrice(Ma);
//     printf("\n");

//     //affichage
//     for (unsigned i = 0; i < la.longueur; i++)
//         printf(((la.registre >> i) & 1) == 1 ? "1" : "0");
//     printf("\n");
//     for (unsigned i = 0; i < la.longueur; i++)
//         printf(registre.tab[i] == 1 ? "1" : "0");
//     printf("\n");

//     return 0;
// }