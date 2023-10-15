#ifndef LFSR_H
#define LFSR_H

#include <stdint.h>

// Taille des registres
#define TAILLE_A 31
#define TAILLE_D 29

// Nombre total de monomes possibles après distribution...
#define NB_MONOMES 12390

// Nombres de nomomes dans la définition de la fonction f.
#define NB_MONOMES_F 33

// Nombre max de bits de sortie pour une session de GEA...
#define NB_BITS_SORTIE 12800

// Pour chaque bit du lfsr, on a un entier de 64 bits.
// Ces entiers de 64 bits représentent une combinaison linéaires de bits de l'état initial.
// Si le i-ème bit est à 1, alors le i-ème bit de l'état initial est dans la combinaison.
// Pour le registre D, les 29 premiers bits (à droite), et pour le registre A, les 31 suivants.
// Au tout début, pour D par exemple : bits = {000...001, 000...010, ..., 000...010...000}
//                                                                               ^ bit d'indice 28.
// Avant le premier tour, donc à l'état initial, le i-ème bit dépend du i-ème bit...
typedef struct lfsr_2 {
    uint64_t bits[31]; // tableau des bits :^)
    int longueur; // 29 ou 31 ^^
    
    uint64_t inverse; // les bits qui sont xorés avec la sortie :3
    const int *select; // les bits qui partent en entrée de f, dans l'ordre UwU
} Lfsr2;

// Représentation des monomes de la fonction f, avant la distribution. 
typedef struct {
    uint64_t bits[4];
    int taille; // 1 - 4
} Monome_f;

// Le tableau qui contient tous les monomes possibles (après guess) pour faire le renommage.
// Les 29 premiers bits (poids faible) contiennent les monomes possibles pour D (en prenant les 9 premiers comme guessés et donc nuls),
// et les 31 suivants pour A...
typedef uint64_t Tab_renommage[NB_MONOMES];

#define NB_GUESS_A 11
#define NB_GUESS_D 9

// La ligne de la matrice contient autant de bits que de monomes possibles (donc autant de bits que d'entrées dans le tableau de renommage).
// On a donc un tableau d'entiers de 64 bits. Le dernier entier n'est utilisé que sur ses 38 premiers bits...
typedef uint64_t Ligne_matrice[NB_MONOMES / 64 + 1];

// Initialise un lfsr pour GEA-2, avec les bonnes valeurs initiales pour les bits du registres. 
void init_lfsr2(Lfsr2 *l, int indice);

// Tour du lfsr avec en sortie des entiers de 64 bits représentant des combinaisons linéaires de bits de l'état initial.
// Donne en sortie des produits de combinaisons linéaires (à développer donc).
void tout_lfsr2(Lfsr2 *l, Monome_f monomes[NB_MONOMES_F]);

// Changement des monomes suite au guess de certains bits.
// Le contenu et la taille de 'result' peuvent changer.
// Les 20 premiers bits de 'guesses' contient les bits guessés.
void modif_guess_bits(const uint32_t guesses, uint64_t *result, int *nb_monomes);

// Remplis le tableau avec à chaque indice les bits qui composent le monomes à 1 et tous les autres à 0.
void init_tab_renommage(Tab_renommage tab);

// Renommage des variables, pour remplir la ligne de la matrice qui correspond au bit de sortie.
void renommage(const Tab_renommage tab, const uint64_t *result, const int *nb_monomes, Ligne_matrice);

#endif
