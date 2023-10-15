#include <stdint.h>
/**
 * Structure qui représente un registre à décalage à rétroaction linéaire en mode galois.
*/
struct lfsr_t {
    uint8_t longueur; // Nombre de bits dans le registre (entre 1 et 64).
    uint64_t registre;
    uint64_t inverse; // Les bits du registres qui sont inversés apres un shift si la sortie est 1 (comprend la remise du bit de sortie dans la 1ere cellule).
    const uint8_t *select; // Les indices des 7 bits qui sont sélectionnés pour être injectés dans f.
    uint8_t permutation; // Indique de combien de bits est shifté la suite s avant d'être donnée en entrée pour l'initialisation 
};

typedef struct lfsr_t Lfsr;

// Taille en bits des registres
#define TAILLE_A 31
#define TAILLE_B 32
#define TAILLE_C 33
#define TAILLE_D 29

// La table est représentée par un tableau d'octet.
// Les 7 bits d'entrée représentent l'indice du tableau où se trouve le résultat de la fontion,
// x0 étant le bit de poids faible et x6 le bit de poids fort (x6x5x4x3x2x1x0).
#define TABLE_SIZE 128
typedef uint8_t Table_f [TABLE_SIZE];

/**
 * Initialisation de la structure du lfsr en fontion de son id (peut être 1 (A), 2 (B), 3 (C) ou 4 (D)).
 * (Ne fait pas l'initialisation du registre, seulement de la structure)
*/
void init_lfsr(Lfsr *lfsr, int id);

/**
 * Effectue un tour du lfsr (un tour normal).
 * Le bit issu de la fonction f est renvoyé dans un char.
*/
uint8_t clock_lfsr(Lfsr *lfsr);

/**
 * Tour particulier pour initialiser le registre à partir de s.
 * Prend en argument le bit de s qui est injecté pour ce tour.
*/
void clock_lfsr_init(Lfsr *lfsr, uint8_t bit_s);

/**
 * Utilise la fontion f pour remplir les cases de la table avec les valeurs qui vont bien.
*/
//void init_table(Table_f t);

void lfsr_init(Lfsr *lfsr, uint64_t s);

void output_65_bits(Lfsr *lfsr, uint64_t *output1, uint8_t *output2);

/**
 *                       ----~~~oo f oo~~~----
 * La fonction prend en entrée les 7 bits sous la forme d'un octet.
 * Les bits de l'octet sont lus de cette façon : [ - x6 x5 x4 x3 x2 x1 x0 ].
*/
uint8_t f(uint8_t e);

uint64_t generateS(uint64_t K, uint8_t dir, unsigned int IV);

uint8_t outputF(uint64_t registre, const uint8_t select[7]);
