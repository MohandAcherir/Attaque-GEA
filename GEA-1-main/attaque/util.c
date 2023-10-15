#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

void printBin(uint64_t n) {
    for (int i = 0; i < 64; i++) {
        printf("%c", ((n >> (63 - i)) & 1) ? '1' : '0');
    }
    printf("\n");
}

void printBint(uint32_t n) {
    for (int i = 0; i < 32; i++) {
        printf("%c", ((n >> (31 - i)) & 1) ? '1' : '0');
    }
    printf("\n");
}

void printBin72(U72 n) {
    for (int i = 0; i < 32; i++)
        printf("%c", ((n.n1 >> (31 - i)) & 1) ? '1' : '0');
    for (int i = 0; i < 32; i++)
        printf("%c", ((n.n2 >> (31 - i)) & 1) ? '1' : '0');
    for (int i = 0; i < 8; i++)
        printf("%c", ((n.n3 >> (7 - i)) & 1) ? '1' : '0');
    printf("\n");
}

int get_params_range(int argc, char** argv, int *arg_count, uint32_t *start, uint32_t *end) {
    if (argc < *arg_count + 2) {
        printf("Les bornes doivent être renseignées (format : ([0-9]+)-([0-9]+).\n");
        return -1;
    }
    (*arg_count)++;
    char *ptr = argv[*arg_count];
    char buf1[8] = {0}, buf2[8] = {0};
    int i = 0;
    while (*ptr != '-') {
        if (i>=8 || (*ptr < '0') || (*ptr > '9')) {
            printf("Erreur : borne invalide.\n");
            return -1;
        }
        buf1[i] = *ptr;
        ptr++; i++;
    }
    i = 0;
    ptr++;
    while (*ptr != '\0') {
        if (i>=8 || (*ptr < '0') || (*ptr > '9')) {
            printf("Erreur : borne invalide.\n");
            return -1;
        }
        buf2[i] = *ptr;
        ptr++; i++;
    }
    *start = atoi(buf1);
    *end = atoi(buf2);
    if (*start < 0 || *start > 255 || *end < 0 || *end > 255 || *start > *end) {
        printf("Erreur : bornes invalides.\n");
        return -1;
    }
    (*arg_count)++;
    return 0;
}

int check_indice_valide(int indice, int max, char base) {
    if (indice >= max) {
        printf("Erreur : indices des vecteurs pour la base de %s entre 0 et %d\n",
        base == 'u' ? "Ub" : (base == 't' ? "Tac" : "v"), max - 1);
        return 1;
    }
    return 0;
}

int lecture_chaine(char* chaine, char baseT[24], char baseUb[32], char baseV[8]) {
    char buf[8];
    char base;
    while(*chaine != '\0') {
        base = *chaine;
        chaine++;
        int i = 0;
        while (*chaine >= '0' && *chaine <= '9') {
            buf[i] = *chaine;
            i++;
            chaine++;
            if (i >= 16) {
                printf("Erreur : buffer overflow, nombre trop grand dans la chaine\n");
                return 1;
            }
        }
        if (i == 0) {
            printf("Erreur : mauvaise chaine de caractères.\nFormat attendu : ((u|t|v)([0-9]+))+\nex : u4u22v7v0\n");
            return 1;
        }
        buf[i] = '\0';
        int indice = atoi(buf);
        switch (base) {
            case 'u' :
                if (check_indice_valide(indice, 32, 'u'))
                    return 1;
                baseUb[indice] = 1;
                break;
            case 't' :
                if (check_indice_valide(indice, 24, 't'))
                    return 1;
                baseT[indice] = 1;
                break;
            case 'v' :
                if (check_indice_valide(indice, 8, 'v'))
                    return 1;
                baseV[indice] = 1;
                break;
        }
    }
    return 0;
}

int mitm_params_get_IV(char* str, uint32_t *IV) {
    char* ptr_err = NULL;
    if (str[0] == '0') {
        if (strlen(str) > 1) {
            if ((str[1] == 'b') | (str[1] == 'B')) {
                *IV = (uint32_t)strtoll(str + 2, &ptr_err, 2);
                if (ptr_err != NULL)  {
                    if (*ptr_err != '\0') {
                        printf("Erreur : chiffre invalide %c (%d)\n", *ptr_err, *ptr_err);
                        return -1;
                    }
                }
            } else if ((str[1] == 'x') | (str[1] == 'X')) {
                *IV = (uint32_t)strtoll(str + 2, &ptr_err, 16);
                if (ptr_err != NULL)  {
                    if (*ptr_err != '\0') {
                        printf("Erreur : chiffre invalide %c (%d)\n", *ptr_err, *ptr_err);
                        return -1;
                    }
                }
            } else {
                *IV = (uint32_t)strtoll(str + 1, &ptr_err, 8);
                if (ptr_err != NULL)  {
                    if (*ptr_err != '\0') {
                        printf("Erreur : chiffre invalide %c (%d)\n", *ptr_err, *ptr_err);
                        return -1;
                    }
                }
            }
        }
    } else {
        *IV = (uint32_t)strtoll(str, &ptr_err, 10);
        if (ptr_err != NULL)  {
            if (*ptr_err != '\0') {
                printf("Erreur : chiffre invalide %c (%d)\n", *ptr_err, *ptr_err);
                return -1;
            }
        }
    }
    return 0;
}

void compose_vecteur(uint64_t *vec, const uint64_t* base, int taille_base, uint32_t bits) {
    for (int i = 0; i < taille_base; i++) {
        if (((bits >> i) & 1) == 1) {
            *vec ^= base[i];
        }
    }
}
