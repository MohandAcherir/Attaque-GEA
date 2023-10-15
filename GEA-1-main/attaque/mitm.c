#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mitm.h"

extern const uint64_t base_T_AC[24];
extern const uint64_t base_U_B[32];
extern const uint64_t base_V[8];

extern pthread_mutex_t mutex;

uint32_t tab_gen[33] = { 0x00000000u,0x80000000u,0xc0000000u,0xe0000000u,
    0xf0000000u,0xf8000000u,0xfc000000u,0xfe000000u,0xff000000u,
    0xff800000u,0xffc00000u,0xffe00000u,0xfff00000u,
    0xfff80000u,0xfffc0000u,0xfffe0000u,0xffff0000u,
    0xffff8000u,0xffffc000u,0xffffe000u,0xfffff000u,
    0xfffff800u,0xfffffc00u,0xfffffe00u,0xffffff00u,
    0xffffff80u,0xffffffc0u,0xffffffe0u,0xfffffff0u,
    0xfffffff8u,0xfffffffcu,0xfffffffeu,0xffffffffu};

uint8_t tab_gen_8[9] = { 0x00u,0x80u,0xc0u,0xe0u,
    0xf0u,0xf8u,0xfcu,0xfeu,0xffu };


int8_t output_and_compare(lfsr_opti *lfsra, lfsr_opti *lfsrc, uint64_t s, const U72 *outz, U72 *xor, uint8_t *deja_gen, const tuple *tpl)
{
    if ((xor->n1 & 0xffffff00u) != ((tpl->out.n1 ^ outz->n1) & 0xffffff00u))
        return 0;
    while (*deja_gen < 32) {
        if (xor->n1 != ((tpl->out.n1 ^ outz->n1) & tab_gen[*deja_gen]))
            return 1;
        output_xor_1_bit(lfsra, s, xor, *deja_gen);
        output_xor_1_bit(lfsrc, s, xor, *deja_gen);
        (*deja_gen)++;
    }
    if (xor->n1 != ((tpl->out.n1 ^ outz->n1) & tab_gen[32]))
            return 1;
    while (*deja_gen < 64) {
        if (xor->n2 != ((tpl->out.n2 ^ outz->n2) & tab_gen[*deja_gen - 32]))
            return 1;
        output_xor_1_bit(lfsra, s, xor, *deja_gen);
        output_xor_1_bit(lfsrc, s, xor, *deja_gen);
        (*deja_gen)++;
    }
    if (xor->n2 != ((tpl->out.n2 ^ outz->n2) & tab_gen[32]))
            return 1;
    while (*deja_gen < 72) {
        if (xor->n3 != ((tpl->out.n3 ^ outz->n3) & tab_gen_8[*deja_gen - 64]))
            return 1;
        output_xor_1_bit(lfsra, s, xor, *deja_gen);
        output_xor_1_bit(lfsrc, s, xor, *deja_gen);
        (*deja_gen)++;
    }
    if (xor->n3 != ((tpl->out.n3 ^ outz->n3) & tab_gen_8[8]))
            return 1;
    return 2; //match parfait
}

int find(const tuple *arr, U72 z, U24 *res) {
    tuple curTuple;
    int l = 0;
    int r = (1 << 24) - 1;
    int m;
    while (l <= r) {
      m = l + (r - l) / 2;
      curTuple = arr[m];
      if  (curTuple.out.n1 == z.n1 && curTuple.out.n2 == z.n2 && curTuple.out.n3 == z.n3) {
        *res = curTuple.t;
        return 1;
      }
      if (curTuple.out.n1 > z.n1 || (curTuple.out.n1 == z.n1 && curTuple.out.n2 > z.n2)
      || (curTuple.out.n1 == z.n1 && curTuple.out.n2 == z.n2 && curTuple.out.n3 > z.n3)) {
        r = m - 1;
      } else {
        l = m + 1;
      }
    }
    return -1;
}

void affiche_barre_de_chargement_mitm(uint32_t table, uint32_t start, uint32_t end, uint64_t progres, uint64_t nb_total) {
    float pourcentage = (float) progres / nb_total * 100;
    printf("Table n°%u/%u (%u - %u) : [", table + 1 - start, end - start, start, end - 1);
    int i;
    for (i = 0; i < pourcentage / 2; i++)
        printf("#");
    for ( ; i < 50; i++)
        printf("_");
    printf("] %3.1f%%  \r", pourcentage);
    fflush(stdout);
}

void *thread_affichage_mitm(void* arg) {
    args_aff_mitm* args = (args_aff_mitm*) arg;
    uint64_t nb_ttl = (1ULL << 32);
    affiche_barre_de_chargement_mitm(args->table, args->start, args->end, 0, nb_ttl);
    while(*args->progres < nb_ttl && *(args->termine) == 0) {
        affiche_barre_de_chargement_mitm(args->table, args->start, args->end, *args->progres, nb_ttl);
        sleep(1);
    }
    if (*(args->termine) == 0)
        affiche_barre_de_chargement_mitm(args->table, args->start, args->end, nb_ttl, nb_ttl);
    return NULL;
}

void set_results_mitm(infos_retour_mitm *inf, int *termine, uint32_t t, uint32_t u) {
    pthread_mutex_lock(&mutex);
    inf->t = t;
    inf->u = u;
    *termine = 1;
    pthread_mutex_unlock(&mutex);
}

// Le problème de générer les bits de suite chiffrante avec une matrice est que le cas précis
// où le registre est à 0 après l'initialisation n'est pas pris en compte.
// En effet le registre est alors forcé à 1 (tout à 0 sauf le bit d'indice 0).
// Cela a pour conséquence que les bits ne dépendent pas simplement de s de façon linaire.
// Ce cas particulier agit comme un boîte-s un peu triviale.
// Il faut donc vérifier ce cas en générant les bits du registre après initialisation.
// Dans tous les autres cas pas besoin de faire quoi que ce soit.
// On génère donc les bits un par un et au premier 1 on renvoie 0.
int check_registre_nul(lfsr_init_matrice m, uint8_t longueur, uint64_t s) {
    for (uint8_t i = 0; i < longueur; i++) {
        if (parite_bits_set(m[i] & s) == 1)
            return 0;
    }
    return 1;
}

void *recherce_table_range(void* arg) {
    args_mitm* args = (args_mitm*) arg;
    Lfsr A, C;
    init_lfsr(&A, 1);
    init_lfsr(&C, 3);
    lfsr_opti Ao, Co;
    init_lfsr_opti(&Ao, &A, DECALAGE_A);
    init_lfsr_opti(&Co, &C, DECALAGE_C);
    uint8_t res, deja_gen;
    uint32_t val, id_res;
    U72 xor;
    uint64_t pgr = 0, s;
    for (uint64_t u = args->start; u < args->end && *(args->termine) == 0; u++) {
        s = 0ull;
        compose_vecteur(&s, base_V, 8, args->v);
        compose_vecteur(&s, base_U_B, 32, u);
        /*for (int i = 0; i < 32; i++) {
            if (((u >> i) & 1) == 1) {
                s ^= base_U_B[i];
            }
        }*/
        // Vérification nécessaire d'un cas particulier (1 sur 2^31 ?).
        // Parce qu'ici on aime les choses bien faites.
        if (check_registre_nul(Ao.init_matrice, TAILLE_A, s)
        || check_registre_nul(Co.init_matrice, TAILLE_C, s)) {
            lfsr_init(&A, s);
            lfsr_init(&C, s);
            U72 outa, outc;
            output_72_bits(&A, &outa);
            output_72_bits(&C, &outc);
            U72 xor = {outa.n1 ^ outc.n1 ^ args->z.n1,
            outa.n2 ^ outc.n2 ^ args->z.n2,
            outa.n3 ^ outc.n3 ^ args->z.n3};
            U24 result = {0,0};
            if ((id_res = find(args->table, xor, &result)) != -1) {
                set_results_mitm(args->retours, args->termine, (((uint32_t)result.n1 << 8) & 0xffff00u) ^ result.n2, u);
                return NULL;
            }
            pgr++;
            continue;
        }

        xor = (U72) {0u,0u,0};
        output_24_bits_opti(&Ao, s, &xor.n1);
        output_24_bits_opti(&Co, s, &xor.n1);
        deja_gen = 24;
        val = (xor.n1 ^ (args->z.n1 & 0xffffff00u)) >> 8;
        id_res = ((((uint32_t)args->table_r[val].n1) << 8) & 0xffff00u) ^ args->table_r[val].n2;
        while ((res = output_and_compare(&Ao, &Co, s, &(args->z), &xor, &deja_gen, &(args->table[id_res]))) > 0 && (id_res <= UINT24_MAX)) {
            if (res == 2) {
                U24 t = args->table[id_res].t;
                set_results_mitm(args->retours, args->termine, (((uint32_t)t.n1 << 8) & 0xffff00u) ^ t.n2, u);
                return NULL;
            }
            id_res++;
        }
        pgr++;
        if (pgr > 100000) {
            pthread_mutex_lock(&mutex);
            *(args->progres) += pgr;
            pthread_mutex_unlock(&mutex);
            pgr = 0;
        }
    }
    pthread_mutex_lock(&mutex);
    *(args->progres) += pgr;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

infos_retour_mitm meet_in_the_middle(U72 z, int nb_threads, char *dir, uint32_t start, uint32_t end) {
    char *filename = malloc((strlen(dir) + 9) * sizeof(char));
    tuple *arr = malloc((1 << 24) * sizeof(tuple));
    U24 *tabr = malloc((1 << 24) * sizeof(U24));
    init_table_parite();
    int termine = 0;
    infos_retour_mitm inf_ret = {0u, 0u, 0u};
    printf("Début de la recherche.\n");
    for (uint64_t v = start; v < end; v++) {
        #ifdef _WIN32
            sprintf(filename, "%s/tab_%llu", dir, v);
        #else
            sprintf(filename, "%s/tab_%lu", dir, v);
        #endif
        FILE* f = fopen(filename, "rb");
        if (!f) {
            printf("Erreur d'ouverture du fichier %s\n", filename);
            return (infos_retour_mitm){UINT32_MAX, UINT32_MAX, UINT32_MAX};
        }
        if (fread(arr, sizeof(tuple) * (1 << 24), 1, f) != 1) {
            printf("Erreur de lecture du fichier %s\n", filename);
            return (infos_retour_mitm){UINT32_MAX, UINT32_MAX, UINT32_MAX};
        }
        if (fread(tabr, sizeof(U24) * (1 << 24), 1, f) != 1) {
            printf("Erreur de lecture du fichier %s\n", filename);
            return (infos_retour_mitm){UINT32_MAX, UINT32_MAX, UINT32_MAX};
        }
        fclose(f);
        uint64_t curStart = 0;
        uint64_t curEnd = 0;
        uint64_t progres = 0;
        args_mitm **p = malloc(sizeof(args_mitm*) * nb_threads);
        pthread_t *threads = malloc(sizeof(pthread_t) * nb_threads);
        pthread_t thread_aff;
        for (int t = 0; t < nb_threads; t++) {
            curStart = curEnd;
            #ifdef _WIN32
                curEnd += ((1ULL << 32) / nb_threads);
                curEnd += (t < ((1ULL << 32) % nb_threads)) ? 1 : 0;
            #else
                curEnd += ((1UL << 32) / nb_threads);
                curEnd += (t < ((1UL << 32) % nb_threads)) ? 1 : 0;
            #endif
            p[t] = malloc(sizeof(args_mitm));
            p[t]->start = curStart;
            p[t]->end = curEnd;
            p[t]->v = v;
            p[t]->table = arr;
            p[t]->table_r = tabr;
            p[t]->z = z;
            p[t]->progres = &progres;
            p[t]->retours = &inf_ret;
            p[t]->termine = &termine;
            int ret = pthread_create(&threads[t], NULL, recherce_table_range, (void *) p[t]);
            if (ret) {
                printf("ERREUR CREATION THREAD : ERR#%d\n", ret);
                exit(-1);
            }
        }
        args_aff_mitm argaff = {&progres, v, start, end, &termine};
        pthread_create(&thread_aff, NULL, thread_affichage_mitm, (void *) &argaff);
        for (int t = 0; t < nb_threads; t++) {
            pthread_join(threads[t], NULL);
            free(p[t]);
        }
        pthread_join(thread_aff, NULL);
        free(p);
        free(threads);
        if (termine == 1) {
            inf_ret.v = v;
            return inf_ret;
        }
    }
    free(arr);
    free(filename);
    return (infos_retour_mitm){UINT32_MAX, UINT32_MAX, UINT32_MAX};
}

int get_params_ivdir(int argc, char** argv, int *arg_count, uint32_t *IV, uint8_t *dir) {
    (*arg_count)++;
    if (argc < *arg_count + 1) {
        printf("key : IV:dir ou -f <fichier> doivent être précisés.\n"
                "Formats pour IV : 0b|0B[0-1]+ = binaire, 0[0-7]+ = octal,"
                " [0-9]+ = decimal, 0x|0X[0-9a-fA-F]+ = hexa\n");
        return -1;
    }
    char str[42];
    if (strcmp(argv[*arg_count], "-f") == 0) {
        if (argc < *arg_count + 2) {
            printf("-f : Un nom de fichier doit être précisé.\n");
            return -1;
        }
        (*arg_count)++;
        FILE *f = NULL;
        if (!(f = fopen(argv[*arg_count], "rb"))) {
            printf("Erreur d'ouverture du fichier %s.\n", argv[*arg_count]);
            return -1;
        }
        int res = fread(str, sizeof(char), 42, f);
        if (res == 0) {
            printf("Erreur de lecture du fichier %s.\n", argv[*arg_count]);
            return -1;
        }
        if (res == 42) {
            printf("Erreur de lecture du fichier %s : chaine trop longue.\n", argv[*arg_count]);
            return -1;
        }
        fclose(f);
        (*arg_count)++;
    } else {
        if (strlen(argv[*arg_count]) >= 42) {
            printf("Erreur de lecture du fichier %s : chaine trop longue.\n", argv[*arg_count]);
            return -1;
        }
        strncpy(str, argv[*arg_count], 42);
        (*arg_count)++;
    }
    
    char* strcp = str;
    while (*strcp != ':') {
        if (*strcp == '\0') {
            printf("Erreur, format attendu : <IV>:<dir>.\n");
            return -1;
        }
        strcp++;
    }
    *strcp = '\0';
    strcp++;
    if (*strcp != '1' && *strcp != '0') {
        printf("Erreur, dir peut être soit 0 soit 1.\n");
        return -1;
    }
    *dir = *strcp - '0';
    if (mitm_params_get_IV(str, IV) < 0)
        return -1;
    return 0;
}

int attaque_mitm(int argc, char** argv) {
    if (argc < 5) {
        printf("%s meet <nb threads> <dossier tables> (-f <fichier> | <chaine>) [range <deb-fin>] [key (<IV:dir> | -f <fichier>)]\n"
              "Si l'option -f est utilisée, la suite chiffrante à attaquer est récuperée dans le ficher indiqué.\n"
              "Sinon, la suite chiffrante est calculée à partir de s.\n"
              "La chaine de caractères indique alors les vecteurs de chaque base qui composent s, avec le format suivant :\n"
              "((u|t|v)([0-9]+))*\n"
              "exemple : u4u22v7v0 -> les vecteurs 4 et 22 de la base de Ub et les vecteurs 0 et 7 de base de v\n"
              "\"range\" permet de préciser un intervalle de tables à tester sous la forme borne1-borne2 avec des bornes entre 0 et 255.\n"
              "\"key\" indique que la clé doit être donnée en sortie de l'attaque. IV et dir doivent être précisés avec le format IV:dir.\n"
              "IV peut avoir les préfixes 0b,0B,0,0x,0X pour utiliser respectivement la base 2, 8, ou 16.\n"
              "Par défaut IV est lu en base 10.\n"
              "IV et dir peuvent être lus dans un fichier précisé avec -f.\n"
              "Le format des chaînes dans le fichier est le même que directement en argument (pas écrits en binaire).\n", argv[0]);
        return 1;
    }
    U72 z = {0u, 0u, 0};
    uint64_t s;
    int nb_threads = atoi(argv[2]);
    int arg_count;
    if (strcmp(argv[4], "-f") == 0) {
        if (argc < 6) {
            printf("%s meet <nb threads> -f <fichier> [range <deb-fin>]\n", argv[0]);
            return 1;
        }
        FILE *f = fopen(argv[5], "rb");
        if (!f) {
            printf("Erreur d'ouverture du fichier %s\n", argv[5]);
            return 1;
        }
        if (fread(&z.n1, sizeof(uint32_t), 1, f) != 1) {
            printf("Erreur de lecture dans le fichier %s\n", argv[5]);
            fclose(f);
            return 1;
        }
        if (fread(&z.n2, sizeof(uint32_t), 1, f) != 1) {
            printf("Erreur de lecture dans le fichier %s\n", argv[5]);
            fclose(f);
            return 1;
        }
        if (fread(&z.n3, sizeof(uint8_t), 1, f) != 1) {
            printf("Erreur de lecture dans le fichier %s\n", argv[5]);
            fclose(f);
            return 1;
        }
        fclose(f);
        arg_count = 6;
    }
    
    else {
        if (mitm_params_chaine(argv[4], &z, &s, 1))
            return 1;
        printf("suite chiffrante : ");
        printBin72(z);
        arg_count = 5;
    }

    uint32_t start = 0;
    uint32_t end = 255;
    int range = 0;
    int recup_cle = 0;
    uint32_t IV = 0;
    uint8_t dir = 0;

    while (arg_count < argc) {
        if (strcmp(argv[arg_count], "range") == 0 && !range) {
            if (get_params_range(argc, argv, &arg_count, &start, &end) < 0)
                return 1;
            range = 1;
            continue;
        }
        if (strcmp(argv[arg_count], "key") == 0 && !recup_cle) {
            if (get_params_ivdir(argc, argv, &arg_count, &IV, &dir) < 0)
                return 1;
            printf("IV : ");
            printBint(IV);
            printf("dir : %d\n", dir);
            recup_cle = 1;
            continue;
        }
        printf("Option %s invalide.\n", argv[arg_count]);
        return 1;
    }
    infos_retour_mitm irm = meet_in_the_middle(z, nb_threads, argv[3], start, end + 1);
    printf("\n");
    if (irm.v != UINT32_MAX) {
        if (recup_cle) {
            s = 0;
            compose_vecteur(&s, base_T_AC, 24, irm.t);
            compose_vecteur(&s, base_U_B, 32, irm.u);
            compose_vecteur(&s, base_V, 8, irm.v);
            /*for (int i = 0; i < 24; i++)
                if (((irm.t >> i) & 1) == 1)
                    s ^= base_T_AC[i];
            for (int i = 0; i < 32; i++)
                if (((irm.u >> i) & 1) == 1)
                    s ^= base_U_B[i];
            for (int i = 0; i < 8; i++)
                if (((irm.v >> i) & 1) == 1)
                    s ^= 1ULL << i;*/
            uint64_t k = recuperation_cle(s, IV, dir);
            printf("Candidat trouvé :\n");
            printf("k : ");
            printBin(k);
        } else {
            printf("Candidat trouvé :\n");
            printf("t : ");
            printBint(irm.t);
            printf("u : ");
            printBint(irm.u);
            printf("v : ");
            printBint(irm.v);
        }
    }
    return 0;
}

int mitm_params_chaine(char* chaine, U72 *z, uint64_t *s, int display) {
    char baseT[24] = {0};
    char baseUb[32] = {0};
    char baseV[8] = {0};
    if (lecture_chaine(chaine, baseT, baseUb, baseV))
        return 1;
    *s = 0UL;
    for (int i = 0; i < 24; i++)
        if (baseT[i] == 1)
            *s ^= base_T_AC[i];
    for (int i = 0; i < 32; i++)
        if (baseUb[i] == 1)
            *s ^= base_U_B[i];
    for (int i = 0; i < 8; i++)
        if (baseV[i] == 1)
            *s ^= base_V[i];
    *z = gen_suite(*s, display);
    return 0;
}
