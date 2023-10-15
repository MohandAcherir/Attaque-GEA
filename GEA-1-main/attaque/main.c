#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "table.h"
#include "mitm.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

const uint64_t base_T_AC[24] = {
0b1000000000000000000000001001110011110011100110000010011111010101ULL,
0b0100000000000000000000001101000001111001110011000001001100001100ULL,
0b0010000000000000000000000110100000111100111001100000100110000110ULL,
0b0001000000000000000000000011010000011110011100110000010011000011ULL,
0b0000100000000000000000001000010000001111001110011000001010000111ULL,
0b0000010000000000000000001101110000000111100111001100000110100101ULL,
0b0000001000000000000000001111000000000011110011100110000000110100ULL,
0b0000000100000000000000000111100000000001111001110011000000011010ULL,
0b0000000010000000000000000011110000000000111100111001100000001101ULL,
0b0000000001000000000000001000000000000000011110011100110011100000ULL,
0b0000000000100000000000000100000000000000001111001110011001110000ULL,
0b0000000000010000000000000010000000000000000111100111001100111000ULL,
0b0000000000001000000000000001000000000000000011110011100110011100ULL,
0b0000000000000100000000000000100000000000000001111001110011001110ULL,
0b0000000000000010000000000000010000000000000000111100111001100111ULL,
0b0000000000000001000000001001110000000000000000011110011111010101ULL,
0b0000000000000000100000001101000000000000000000001111001100001100ULL,
0b0000000000000000010000000110100000000000000000000111100110000110ULL,
0b0000000000000000001000000011010000000000000000000011110011000011ULL,
0b0000000000000000000100001000010000000000000000000001111010000111ULL,
0b0000000000000000000010001101110000000000000000000000111110100101ULL,
0b0000000000000000000001001111000000000000000000000000011100110100ULL,
0b0000000000000000000000100111100000000000000000000000001110011010ULL,
0b0000000000000000000000010011110000000000000000000000000111001101ULL};

const uint64_t base_U_B[32] = {
0b1000000000000000000000000000000010000010100101111010011000111110ULL,
0b0100000000000000000000000000000001110110011111001001101010111011ULL,
0b0010000000000000000000000000000010111001101010011110101101100011ULL,
0b0001000000000000000000000000000011101001011101000001101000101011ULL,
0b0000100000000000000000000000000011110110001011011010101100101011ULL,
0b0000010000000000000000000000000011001110101101100011101000001111ULL,
0b0000001000000000000000000000000011100101110011001011101100111001ULL,
0b0000000100000000000000000000000011110000011100011111101110100010ULL,
0b0000000010000000000000000000000001001111000011111011010001110101ULL,
0b0000000001000000000000000000000010010010001001110011010110100000ULL,
0b0000000000100000000000000000000001111110001001001101001101110100ULL,
0b0000000000010000000000000000000000111111000100100110100110111010ULL,
0b0000000000001000000000000000000000011111100010010011010011011101ULL,
0b0000000000000100000000000000000010111010011001000111010111110100ULL,
0b0000000000000010000000000000000001011101001100100011101011111010ULL,
0b0000000000000001000000000000000000101110100110010001110101111101ULL,
0b0000000000000000100000000000000010100010111011000110000100100100ULL,
0b0000000000000000010000000000000001010001011101100011000010010010ULL,
0b0000000000000000001000000000000000101000101110110001100001001001ULL,
0b0000000000000000000100000000000010100001111111010110001110111110ULL,
0b0000000000000000000010000000000001100111110010011111100001111011ULL,
0b0000000000000000000001000000000010000110010001000001001110100111ULL,
0b0000000000000000000000100000000011000001101101011010111111101101ULL,
0b0000000000000000000000010000000011010101011110100011100001101100ULL,
0b0000000000000000000000001000000001101010101111010001110000110110ULL,
0b0000000000000000000000000100000000000010011010011100011110111111ULL,
0b0000000000000000000000000010000010110100100101000000110001000101ULL,
0b0000000000000000000000000001000011011000110111011010000000011100ULL,
0b0000000000000000000000000000100001011011010110011001100110101010ULL,
0b0000000000000000000000000000010000011010100110111000010101110001ULL,
0b0000000000000000000000000000001010111000111011010010110100100010ULL,
0b0000000000000000000000000000000101101011010000011101111100110101ULL};

const uint64_t base_V[8] = {
0b1000000000000000000000000000000000000000000000000000000000000000ULL,
0b0100000000000000000000000000000000000000000000000000000000000000ULL,
0b0010000000000000000000000000000000000000000000000000000000000000ULL,
0b0001000000000000000000000000000000000000000000000000000000000000ULL,
0b0000100000000000000000000000000000000000000000000000000000000000ULL,
0b0000010000000000000000000000000000000000000000000000000000000000ULL,
0b0000001000000000000000000000000000000000000000000000000000000000ULL,
0b0000000100000000000000000000000000000000000000000000000000000000ULL};

int digit_to_int(char digit, int base) {
    if (base <= 10) {
        if (digit < '0' && digit >= ('0' + base)) {
            printf("Erreur chiffre invalide %c\n", digit);
            return -1;
        }
        int chiffre = digit - '0';
        if (chiffre < base)
            return chiffre;
        else {
            printf("Erreur : chiffre invalide %c\n", digit);
            return -1;
        }
    } else { //base > 10
        if (digit >= '0' && digit <= '9')
            return digit - '0';
        else {
            if (digit >= 'a' && digit < ('a' + base - 10))
                return digit - 'a' + 10;
            if (digit >= 'A' && digit < ('A' + base - 10))
                return digit - 'A' + 10;
            printf("Erreur : chiffre invalide %c\n", digit);
            return -1;
        }
    }
    return 0;
}

int gen_s(int argc, char** argv) {
    if (argc < 4) {
        printf("%s gen <fichier> (<chaine> | key <[args]>)\n"
              "Utilise les paramètres donnés pour générer 72 bits de suite chiffrante qui sont stockés dans <fichier>.\n"
              "<key> permet de donner un clé ou d'en générer une aléatoirement, ainsi que IV et dir.\n"
              "Sinon, <chaine> indique les vecteurs de chaque base qui composent s, avec le format suivant :\n"
              "((u|t|v)([0-9]+))*\n"
              "exemple : u4u22v7v0 -> les vecteurs 4 et 22 de la base de Ub et les vecteurs 0 et 7 de base de v\n", argv[0]);
        return 1;
    }
    U72 z = {0u, 0u, 0};
    uint64_t s = 0;
    int next_arg = 5;
    if (strcmp(argv[3], "key") == 0) {
        if (argc < 5) {
            printf("%s gen <fichier> key (random | <cle> | -f <fichier>) [IV:dir]\n", argv[0]);
            return 1;
        }
        uint64_t k = 0lu;
        uint32_t IV = 0u;
        uint8_t dir = 0;
        srand(time(NULL));
        if (strcmp(argv[4], "random") == 0) {
            k ^= ((int64_t)rand()) << 32;
            k ^= rand();
        }
        else {
            char buf[70], *bufcp = buf;
            if (strcmp(argv[4], "-f") == 0) {
                if (argc < 6) {
                    printf("%s gen <fichier> key (random | <cle> | -f <fichier>) [IV:dir]\n"
                            "random : une clé est générée aléatoirement.\n"
                            "-f <fichier> : La clé est lue dans le fichier donné."
                            "Sinon la clé est lue avec le format suivant : préfixe 0, 0x, 0X, 0b, 0B (ou aucun), puis une suite de chiffres.\n"
                            "Le format est le même que si elle est lue dans un fichier."
                            "Si IV et dir ne sont pas donnés, ils sont générés aléatoirement et donnés.\n", argv[0]);
                }
                FILE* f = fopen(argv[5], "wb");
                if (!f) {
                    printf("Erreur d'ouverture du fichier %s\n", argv[5]);
                    return 1;
                }
                int res = fread(buf, sizeof(char), 70, f);
                if (res == 0) {
                    printf("Erreur de lecture du fichier %s.\n", argv[5]);
                    return 1;
                }
                if (res == 70) {
                    printf("Erreur de lecture du fichier %s : chaine trop longue.\n", argv[5]);
                    return 1;
                }
                next_arg = 6;
            } else {
                strncpy(buf, argv[4], 70);
            }
            buf[69] = '\0';
            int base = 10;
            if (buf[0] == '0') {
                if (buf[1] != '\0') {
                    if ((buf[1] == 'x') | (buf[1] == 'X')) {
                        base = 16;
                        bufcp += 2;
                    }
                    else if ((buf[1] == 'b') | (buf[1] == 'B')) {
                        base = 2;
                        bufcp += 2;
                    }
                    else {
                        base = 8;
                        bufcp++;
                    }
                }
            }
            int ret;
            while (*bufcp != '\0') {
                ret = digit_to_int(*bufcp, base);
                if (ret < 0)
                    return 1;
                k *= base;
                k += ret;
                bufcp++;
            }
        }
        if (argc > next_arg) {
            char* str = argv[next_arg];
            while (*str != ':') {
                if (*str == '\0') {
                    printf("Erreur, format attendu : <IV>:<dir>.\n");
                    return 1;
                }
                str++;
            }
            *str = '\0';
            str++;
            if (*str != '1' && *str != '0') {
                printf("Erreur, dir peut être 0 ou 1.\n");
                return 1;
            }
            dir = *str - '0';
            if (mitm_params_get_IV(argv[next_arg], &IV) < 0) {
                return 1;
            }
        } else {
            IV = rand();
            dir = rand() & 1;
        }
        printf("k : ");
        printBin(k);
        printf("IV : ");
        printBint(IV);
        printf("dir : %d\n", dir);
        
        uint64_t registre = 0;
        clock_lfsr_s(&registre, 32, IV);
        clock_lfsr_s(&registre, 1, dir);
        clock_lfsr_s(&registre, 64, k);
        clock_lfsr_s(&registre, 128, 0);
        printf("s : ");
        printBin(registre);
        z = gen_suite(registre, 0);
    } else {
        if (mitm_params_chaine(argv[3], &z, &s, 1))
            return 1;
    }

    FILE* f = fopen(argv[2], "wb");
    if (!f) {
        printf("Erreur d'ouverture du fichier %s\n", argv[2]);
        return 1;
    }
    if (fwrite(&z.n1, sizeof(uint32_t), 1, f) != 1) {
        printf("Erreur d'écriture dans le fichier %s\n", argv[2]);
        return 1;
    }
    if (fwrite(&z.n2, sizeof(uint32_t), 1, f) != 1) {
        printf("Erreur d'écriture dans le fichier %s\n", argv[2]);
        return 1;
    }
    if (fwrite(&z.n3, sizeof(uint8_t), 1, f) != 1) {
        printf("Erreur d'écriture dans le fichier %s\n", argv[2]);
        return 1;
    }
    fclose(f);
    printf("Suite chiffrante générée.\n");
    return 0;
}

void print_usage(char *str) {
    printf("%s <action> <[param]>\n"
            "actions :\n"
            " - table : génère les tables\n"
            " - meet : effectuer l'attaque (les tables doivent avoir été générées)\n"
            " - gen : génère une suite chiffrante (72 bits) et la stocke dans un fichier\n", str);
}


int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "table") == 0)
        return generation_table(argc, argv);
    if (strcmp(argv[1], "meet") == 0)
        return attaque_mitm(argc, argv);
    if (strcmp(argv[1], "gen") == 0)
        return gen_s(argc, argv);
    print_usage(argv[0]);
    return -1;
}
