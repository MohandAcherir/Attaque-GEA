#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "table.h"
#include "util.h"

extern pthread_mutex_t mutex;

extern const uint64_t base_T_AC[24];
extern const uint64_t base_V[8];

int compare(const void *t1, const void *t2) {
    tuple *tuple1 = (tuple *) t1;
    tuple *tuple2 = (tuple *) t2;
    if (tuple1->out.n1 == tuple2->out.n1) {
        if (tuple1->out.n2 == tuple2->out.n2) {
            return (tuple1->out.n3 > tuple2->out.n3) ? 1 : -1;
        } else {
            return (tuple1->out.n2 > tuple2->out.n2) ? 1 : -1;
        }
    } else {
        return (tuple1->out.n1 > tuple2->out.n1) ? 1 : -1;
    }
}

void table_build_B(uint32_t v, char* dir, unsigned size) {
    tuple *buffer = malloc(sizeof(tuple) * (1 << 24));
    uint64_t curVector;
    Lfsr B;
    init_lfsr(&B, 2);
    U72 output;
    // On enumere toutes les valeurs possibles de v + t, v fixe et t appartient base_T_AC
    for (uint32_t t = 0; t < 1U << 24; t++) {
        curVector = 0ull;
        compose_vecteur(&curVector, base_V, 8, v);
        compose_vecteur(&curVector, base_T_AC, 24, t);
        /*for (int i = 0; i < 24; i++) {
            if (((t >> i) & 1) == 1) {
                curVector ^= base_T_AC[i];
            }
        }*/
        lfsr_init(&B, curVector);
        output_72_bits(&B, &output);
        buffer[t].out = output;
        buffer[t].t.n1 = (t & 0xffff00u) >> 8;
        buffer[t].t.n2 = t & 0xff;
    }
    qsort(buffer, 1 << 24, sizeof(tuple), compare);

    
    U24 *buffer2 = malloc(sizeof(U24) * (1 << 24));
    uint32_t pivot = 0;
    for (uint32_t t = 0; t < 1U << 24; t++) {
        while(((buffer[pivot].out.n1 & 0xffffff00) >> 8) < t) {
            pivot++;
            if (pivot >= (1U << 24)) {
                pivot = (1U << 24) - 1;
                break;
            }
        }
        if (((buffer[pivot].out.n1 & 0xffffff00) >> 8) > t) {
            //pas de match dans la table
            buffer2[t].n1 = UINT16_MAX;
            buffer2[t].n2 = UINT8_MAX;
        }
        else {
            //match
            buffer2[t].n1 = (pivot & 0xffff00) >> 8;
            buffer2[t].n2 = pivot & 0xff;
        }

    }

    // Ecriture de la table
    char *filename = malloc((size + 9) * sizeof(char));
    sprintf(filename, "%s/tab_%u", dir, v);

    FILE *f = fopen(filename, "wb");
    if (fwrite(buffer, sizeof(tuple), (1 << 24), f) != (1 << 24)) {
        printf("Erreur d'écriture\n");
        exit(-1);
    }
    free(buffer);
    if (fwrite(buffer2, sizeof(U24), (1 << 24), f) != (1 << 24)) {
        printf("Erreur d'écriture\n");
        exit(-1);
    }
    fclose(f);
    free(buffer2);
    free(filename);
}

void *buildTablesRange(void *arguments) {
    param *args = (param *)arguments;
    for (uint32_t v = args->start; v < args->end; v++) {
        table_build_B(v, args->shargs->dir, args->shargs->size);
        pthread_mutex_lock(&mutex);
        *(args->shargs->progres) += 1;
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
    return NULL;
}

void affiche_barre_de_chargement(uint16_t progres, uint16_t nb_total) {
    float pourcentage = (float) progres / nb_total * 100;
    printf("%u / %u [", progres, nb_total);
    int i;
    for (i = 0; i < pourcentage / 2; i++)
        printf("#");
    for ( ; i < 50; i++)
        printf("_");
    printf("] %3.1f%%\r", pourcentage);
    fflush(stdout);
}

void *thread_affichage(void* arg) {
    table_aff_args* args = (table_aff_args*) arg;
    int previous = *(args->progres);
    printf("Début de la génération des tables.\n");
    affiche_barre_de_chargement(0, args->nb_tables);
    while(*(args->progres) < args->nb_tables) {
        if (*(args->progres) != previous)
            affiche_barre_de_chargement(*(args->progres), args->nb_tables);
        previous = *(args->progres);
        sleep(1);
    }
    affiche_barre_de_chargement(args->nb_tables, args->nb_tables);
    printf("\nTables générées.\n");
    return NULL;
}

void table_build(int nb_threads, char* dir, unsigned size, uint32_t start, uint32_t end) {
    pthread_t *threads = malloc(sizeof(pthread_t) * nb_threads);
    pthread_t thread_aff;
    param **p = malloc(sizeof(param*) * nb_threads); // liste des parametres pour les threads
    int progres_calcul = 0;
    struct shared_arg args = {dir, size, &progres_calcul};
    int ret;
    uint64_t curStart = start;
    uint64_t curEnd = start;
    uint32_t nb_table_total = end - start + 1;
    for (int t = 0; t < nb_threads; t++) {
        curStart = curEnd;
        curEnd += (nb_table_total / nb_threads);
        curEnd += (t < (nb_table_total % nb_threads)) ? 1 : 0; // les premiers threads s'occupent du reste de la division
        p[t] = malloc(sizeof(param));
        p[t]->start = curStart;
        p[t]->end = curEnd;
        p[t]->shargs = &args;
        ret = pthread_create(&threads[t], NULL, buildTablesRange, (void *) p[t]);
        if (ret) {
              printf("ERREUR CREATION THREAD ERR#%d\n", ret);
              exit(-1);
        }
    }
    table_aff_args aff_args = {&progres_calcul, nb_table_total};
    pthread_create(&thread_aff, NULL, thread_affichage, (void *) &aff_args);
    for (int t = 0; t < nb_threads; t++) {
        pthread_join(threads[t], NULL);
        free(p[t]);
    }
    pthread_join(thread_aff, NULL);
    free(p);
    free(threads);
}

int generation_table(int argc, char** argv) {
    if (argc < 4) {
        printf("%s table <nb threads> <dossier de sortie> [range <bornes>]\n"
                "Les bornes varient entre 0 et 255, sous la forme borne1-borne2\n", argv[0]);
        return 1;
    }
    int nb_threads = atoi(argv[2]);
    if (nb_threads <= 0 || nb_threads > NB_THREADS_MAX) {
        printf("Nombre de threads invalide\n");
            return 1;
    }

    struct stat sb;
    if (stat(argv[3], &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        #ifdef _WIN32
            int result = mkdir(argv[3]);
        #else
            int result = mkdir(argv[3], 0777);
        #endif
        if (result < 0) {
            printf("Erreur de création du dossier %s\n", argv[3]);
            return 1;
        }
    }
    
    uint32_t start = 0;
    uint32_t end = 255;
    int arg_count = 4;
    if (arg_count < argc) {
        if (strcmp(argv[arg_count], "range") == 0) {
            if (get_params_range(argc, argv, &arg_count, &start, &end) < 0)
                return 1; 
        } else {
            printf("Erreur : argument non reconnu : %s\n", argv[arg_count]);
            return 1;
        }
    }
    table_build(nb_threads, argv[3], strlen(argv[3]), start, end);
    return 0;
}
