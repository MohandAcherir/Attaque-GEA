#include <stdint.h>
#include "lfsr.h"

#ifndef MITM_H
#define MITM_H

#define NB_THREADS_MAX 32

typedef struct {
    uint32_t t;
    uint32_t u;
    uint32_t v;
} infos_retour_mitm;

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t v;
    U72 z;
    const tuple* table;
    const U24* table_r;
    uint64_t *progres;
    infos_retour_mitm *retours;
    int *termine; //signal entre threads
} args_mitm;

typedef struct {
    uint64_t* progres;
    uint32_t table;
    uint32_t start;
    uint32_t end;
    int *termine;
} args_aff_mitm;

int8_t output_and_compare(lfsr_opti *lfsra, lfsr_opti *lfsrc, uint64_t s, const U72 *outz, U72 *xor, uint8_t *deja_gen, const tuple *tpl);

int find(const tuple *arr, U72 z, U24 *res);

void affiche_barre_de_chargement_mitm(uint32_t table, uint32_t start, uint32_t end, uint64_t progres, uint64_t nb_total);

void *thread_affichage_mitm(void* arg);

void set_results_mitm(infos_retour_mitm *inf, int *termine, uint32_t t, uint32_t u);

// Le problème de générer les bits de suite chiffrante avec une matrice est que le cas précis
// où le registre est à 0 après l'initialisation n'est pas pris en compte.
// En effet le registre est alors forcé à 1 (tout à 0 sauf le bit d'indice 0).
// Cela a pour conséquence que les bits ne dépendent pas simplement de s de façon linaire.
// Ce cas particulier agit comme un boîte-s un peu triviale.
// Il faut donc vérifier ce cas en générant les bits du registre après initialisation.
// Dans tous les autres cas pas besoin de faire quoi que ce soit.
// On génère donc les bits un par un et au premier 1 on renvoie 0.
int check_registre_nul(lfsr_init_matrice m, uint8_t longueur, uint64_t s);

void *recherce_table_range(void* arg);

infos_retour_mitm meet_in_the_middle(U72 z, int nb_threads, char *dir, uint32_t start, uint32_t end);

int get_params_ivdir(int argc, char** argv, int *arg_count, uint32_t *IV, uint8_t *dir);

int attaque_mitm(int argc, char** argv);

int mitm_params_chaine(char* chaine, U72 *z, uint64_t *s, int display);

#endif
